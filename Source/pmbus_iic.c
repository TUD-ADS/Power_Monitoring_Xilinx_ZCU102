#include "pmbus_iic.h"
#include "math.h"
#include "fcntl.h"
#include "unistd.h"

XGpioPs gpio;
XIicPs iic;

int setupIic(void) {
	XIicPs_Config *config;
	int status;
	u8 WriteBuffer[10] = { 0 };
	u16 SlaveAddr;

	/* Initialize the IIC controller */
	config = XIicPs_LookupConfig(XPAR_XIICPS_0_DEVICE_ID);
	if (config == NULL) {
		return XST_FAILURE;
	}

	status = XIicPs_CfgInitialize(&iic, config, config->BaseAddress);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	status = XIicPs_SetSClk(&iic, IIC_SCLK_RATE_I2CMUX);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	WriteBuffer[0] = CMD_CH_2_REG;
	SlaveAddr = PCA9544A_ADDR;

	status = XIicPs_MasterSendPolled(&iic, WriteBuffer, 1, SlaveAddr);
	if (status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	while (XIicPs_BusIsBusy(&iic) > 0) {
	}

	return XST_SUCCESS;
}

unsigned int pmBusWrite(unsigned char address, unsigned char command,
		unsigned char data) {
	unsigned char writeBuffer[2];
	unsigned int status;

	/* The register address is the first byte of data sent to the IIC device,
	 * followed by the data
	 */
	writeBuffer[0] = command;
	writeBuffer[1] = data;

	/* Wait until the bus is available */
	while (XIicPs_BusIsBusy(&iic)) {
		/* NOP */
	}

	/* Write the data at the specified address to the IIC device */
	status = XIicPs_MasterSendPolled(&iic, writeBuffer, 2, address);

	if (status != XST_SUCCESS) {
		xil_printf("SEND ERROR: 0x%08X\r\n", status);
		return status;
	}

	while (XIicPs_BusIsBusy(&iic)) {
		/* NOP */
	}

	usleep(250000);

	return XST_SUCCESS;
}

unsigned char pmBusWriteWord(unsigned char address, unsigned char command,
		unsigned char *data) {
	unsigned int status;
	unsigned char writeBuffer[3];

	/* The register address if the first byte of data to send to the IIC device,
	 * followed by the data
	 */
	writeBuffer[0] = command;
	writeBuffer[1] = data[0];
	writeBuffer[2] = data[1];

	/* Wait until the bus is available */
	while (XIicPs_BusIsBusy(&iic)) {
		/* NOP */
	}

	status = XIicPs_MasterSendPolled(&iic, writeBuffer, 3, address);

	if (status != XST_SUCCESS) {
		xil_printf("SEND ERROR: 0x%08X\r\n", status);
		return status;
	}

	while (XIicPs_BusIsBusy(&iic)) {
		/* NOP */
	}

	usleep(250000);

	return XST_SUCCESS;
}

unsigned int pmBusRead(unsigned char address, unsigned char command,
		unsigned char byteCount, unsigned char *buffer) {
	unsigned int status;

	status = XIicPs_SetOptions(&iic, XIICPS_REP_START_OPTION);
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: Unable to set repeated start option: 0x%08X\r\n",
				status);
		return status;
	}
	/* Send the command byte to the IIC device */

	status = XIicPs_MasterSendPolled(&iic, &command, 1, address);

	if (status != XST_SUCCESS) {
		xil_printf("ERROR: RX send error: 0x%08X\r\n", status);
		return status;
	}

	usleep(10000);

	status = XIicPs_MasterRecvPolled(&iic, buffer, byteCount, address);
	if (status != XST_SUCCESS) {
		status = XIicPs_ReadReg(iic.Config.BaseAddress, XIICPS_ISR_OFFSET);
		xil_printf("ERROR: RX error: 0x%08X\r\n", status);
		return status;
	}

	status = XIicPs_ClearOptions(&iic, XIICPS_REP_START_OPTION);
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: Unable to clear repeated start option: 0x%08X\r\n",
				status);
		return status;
	}

	return XST_SUCCESS;
}

unsigned char readVoltage(unsigned char deviceAddress,
		unsigned char *receiveBuf, unsigned char command) {
	unsigned int status;

	//status = pmBusRead(deviceAddress, CMD_READ_VOUT, 2, receiveBuf);
	status = pmBusRead(deviceAddress, command, 2, receiveBuf);
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: readVoltage()\r\n");
		return XST_SUCCESS;
	}

	return XST_SUCCESS;
}

unsigned char readCurrent(unsigned char deviceAddress,
		unsigned char *receiveBuf) {

	unsigned int status;

	status = pmBusRead(deviceAddress, CMD_READ_IOUT, 2, receiveBuf);
	if (status != XST_SUCCESS) {
		return 0;
	}

	return 2;
}

float readVoltage_real(unsigned char deviceAddress, unsigned char command) {
	u8 SendArray[2];
	int status;
	u16 data;
	float voltage;
	usleep(10000);
	status = readVoltage(deviceAddress, SendArray, command);
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: readVoltage_real()\r\n");
	}
	data = (u16) ((SendArray[0]) | (SendArray[1]) << 8);
	voltage = (float) data * 0.000244;
	return voltage;
}

double readCurrent_real(unsigned char deviceAddress) {
	double current;
	u8 SendArray[2];
	u16 Current_Data;
	u16 data;
	usleep(10000);
	Current_Data = readCurrent(deviceAddress, SendArray);
	data = (u16) ((SendArray[0]) | (SendArray[1]) << 8);

	current = linear11ToFloat((unsigned char) SendArray[1],
			(unsigned char) SendArray[0]);
	return current;
}

double linear11ToFloat(unsigned char highByte, unsigned char lowByte) {
	unsigned short combinedWord;
	signed char exponent;
	signed short mantissa;
	double current;

	combinedWord = highByte;
	combinedWord <<= 8;
	combinedWord += lowByte;

	exponent = combinedWord >> 11;
	mantissa = combinedWord & 0x7ff;

	/* Sign extend the exponent and the mantissa */
	/* Sign extend the exponent and the mantissa */

	if (exponent > 0x0f) {
		exponent |= 0xe0;
	}
	if (mantissa > 0x03ff) {
		mantissa |= 0xf800;
	}
	//xil_printf("%f--------%f",mantissa,exponent );
	current = mantissa * pow(2.0, exponent);
	return (float) current;
}

void power_mon(unsigned char device_adr, char *voltageName) {
	float voltage, current;

	int i = 0;

	for (i = 0; i < 25; i++) {
		voltage = voltage + readVoltage_real(device_adr, CMD_READ_VOUT);
		current = current + readCurrent_real(device_adr);
	}

	voltage = voltage / 25;
	current = current / 25;

	printf("|%s \t\t | %f V\t | %f mA\t|   %f mW\t|\n", voltageName, voltage,
			current*1000.0, (current * voltage)*1000.0);
}

int setupGpio(void) {
	int status;
	XGpioPs_Config *gpioConfig;

	gpioConfig = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);
	status = XGpioPs_CfgInitialize(&gpio, gpioConfig, gpioConfig->BaseAddr);
	if (status != XST_SUCCESS) {
		return status;
	}

	return XST_SUCCESS;
}

int setupHardwareSubsystem(void) {
	int status;
	/* Set up the hardware subsystems */
	status = setupGpio();
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: setupGpio()\r\n");
		return status;
	}
	status = setupIic();
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: setupIic()\r\n");
		return status;
	}
	return XST_SUCCESS;
}

int programVoltage(float targetVoltage, unsigned char deviceAddress,
		unsigned char command) {
	u16 dataArray[2];
	u32 bufferData;
	bufferData = (u32) (targetVoltage / 0.000244);
	dataArray[0] = (u16) bufferData;
	dataArray[1] = (u16) bufferData >> 8;

	int status = pmBusWriteWord(deviceAddress, command, dataArray);

	if (status != XST_SUCCESS) {
		xil_printf("ERROR: programVoltage()\r\n");
		return status;
	}

	return XST_SUCCESS;
}

void voltageScaling(float targetVoltage, unsigned char deviceAddress) {
	float defaultVoltageValue = 0.85;
	int status;

	if (targetVoltage < 1.0 && targetVoltage > 0.55)//voltage scaling threshold
			{
		if (targetVoltage <= defaultVoltageValue) {
			status = programVoltage((0.85 * targetVoltage), deviceAddress,
					POWER_GOOD_OFF);
			status = programVoltage((0.90 * targetVoltage), deviceAddress,
					POWER_GOOD_ON);
			status = programVoltage((0.85 * targetVoltage), deviceAddress,
					VOUT_UV_FAULT_LIMIT);
			status = programVoltage((0.95 * targetVoltage), deviceAddress,
					VOUT_MARGIN_LOW);

			status = programVoltage((targetVoltage), deviceAddress,
					CMD_VOUT_COMMAND);

			status = programVoltage((1.05 * targetVoltage), deviceAddress,
					VOUT_MARGIN_HIGH);
			status = programVoltage((1.10 * targetVoltage), deviceAddress,
					VOUT_MAX);
			status = programVoltage((1.15 * targetVoltage), deviceAddress,
					VOUT_OV_FAULT_LIMIT);
		}

		if (targetVoltage > defaultVoltageValue) {
			status = programVoltage((0.85 * targetVoltage), deviceAddress,
					POWER_GOOD_OFF);
			status = programVoltage((0.90 * targetVoltage), deviceAddress,
					POWER_GOOD_ON);
			status = programVoltage((1.05 * targetVoltage), deviceAddress,
					VOUT_MARGIN_HIGH);
			status = programVoltage((1.10 * targetVoltage), deviceAddress,
					VOUT_MAX);
			status = programVoltage((1.15 * targetVoltage), deviceAddress,
					VOUT_OV_FAULT_LIMIT);

			status = programVoltage((targetVoltage), deviceAddress,
					CMD_VOUT_COMMAND);

			status = programVoltage((0.85 * targetVoltage), deviceAddress,
					VOUT_UV_FAULT_LIMIT);
			status = programVoltage((0.95 * targetVoltage), deviceAddress,
					VOUT_MARGIN_LOW);
		}

		// ERROR PROTECTION //
		if (checkError(deviceAddress) != XST_SUCCESS) {
			resetVoltageLevel(deviceAddress, targetVoltage);
		}
	} else {
		status = programVoltage((0.85 * defaultVoltageValue), deviceAddress,
				POWER_GOOD_OFF);
		status = programVoltage((0.90 * defaultVoltageValue), deviceAddress,
				POWER_GOOD_ON);
		status = programVoltage((0.85 * defaultVoltageValue), deviceAddress,
				VOUT_UV_FAULT_LIMIT);
		status = programVoltage((0.95 * defaultVoltageValue), deviceAddress,
				VOUT_MARGIN_LOW);

		status = programVoltage((defaultVoltageValue), deviceAddress,
				CMD_VOUT_COMMAND);

		status = programVoltage((1.05 * defaultVoltageValue), deviceAddress,
				VOUT_MARGIN_HIGH);
		status = programVoltage((1.10 * defaultVoltageValue), deviceAddress,
				VOUT_MAX);
		status = programVoltage((1.15 * defaultVoltageValue), deviceAddress,
				VOUT_OV_FAULT_LIMIT);

		xil_printf(
				"ERROR: voltageScaling() : targetVoltage<1.0 && targetVoltage>0.55\r\n");
	}
}

void resetVoltageLevel(unsigned char deviceAddress, float targetVoltage) {
	int status;
	float defaultVoltageValue = 0.85;

	if (targetVoltage > defaultVoltageValue) {
		status = programVoltage((0.85 * defaultVoltageValue), deviceAddress,
				POWER_GOOD_OFF);
		status = programVoltage((0.90 * defaultVoltageValue), deviceAddress,
				POWER_GOOD_ON);
		status = programVoltage((0.85 * defaultVoltageValue), deviceAddress,
				VOUT_UV_FAULT_LIMIT);
		status = programVoltage((0.95 * defaultVoltageValue), deviceAddress,
				VOUT_MARGIN_LOW);

		status = programVoltage((defaultVoltageValue), deviceAddress,
				CMD_VOUT_COMMAND);

		status = programVoltage((1.05 * defaultVoltageValue), deviceAddress,
				VOUT_MARGIN_HIGH);
		status = programVoltage((1.10 * defaultVoltageValue), deviceAddress,
				VOUT_MAX);
		status = programVoltage((1.15 * defaultVoltageValue), deviceAddress,
				VOUT_OV_FAULT_LIMIT);
	}
	if (targetVoltage <= defaultVoltageValue) {
		status = programVoltage((0.85 * defaultVoltageValue), deviceAddress,
				POWER_GOOD_OFF);
		status = programVoltage((0.90 * defaultVoltageValue), deviceAddress,
				POWER_GOOD_ON);
		status = programVoltage((1.05 * defaultVoltageValue), deviceAddress,
				VOUT_MARGIN_HIGH);
		status = programVoltage((1.10 * defaultVoltageValue), deviceAddress,
				VOUT_MAX);
		status = programVoltage((1.15 * defaultVoltageValue), deviceAddress,
				VOUT_OV_FAULT_LIMIT);

		status = programVoltage((defaultVoltageValue), deviceAddress,
				CMD_VOUT_COMMAND);

		status = programVoltage((0.85 * defaultVoltageValue), deviceAddress,
				VOUT_UV_FAULT_LIMIT);
		status = programVoltage((0.95 * defaultVoltageValue), deviceAddress,
				VOUT_MARGIN_LOW);
	}

	power_mon(deviceAddress, "VCCINT");

	if (status != XST_SUCCESS) {
		xil_printf("ERROR: resetVoltageLevel()\r\n");
	}
}

int checkError(unsigned char deviceAddress) {
	u8 aStatusVOUT[1];
	u8 aStatusWORD[2];
	u16 StatusWORD;
	int status;
	usleep(10000);
	status = pmBusRead(deviceAddress, STATUS_VOUT, 1, aStatusVOUT);
	if (aStatusVOUT[0] != XST_SUCCESS) {
		/*
		 BIT 	NAME 				MEANING 						LATCHED
		 7 		VOUT_OV_FAULT 		VOUT overvoltage fault. 		Yes
		 6 		VOUT_OV_WARN 		VOUT overvoltage warning. 		Yes
		 5 		VOUT_UV_WARN 		VOUT undervoltage warning. 		Yes
		 4 		VOUT_UV_FAULT 		VOUT undervoltage fault. 		Yes
		 3 		0 					This bit always returns a 0. 	—
		 2 		TON_MAX_FAULT 		TON maximum fault. 				Yes
		 1 		0 					This bit always returns a 0. 	—
		 0 		0 					This bit always returns a 0. 	—
		 https://datasheets.maximintegrated.com/en/ds/MAX34460.pdf
		 */

		xil_printf("ERROR: StatusVOUT:%d see comment in checkError()\r\n",
				aStatusVOUT[0]);
		return aStatusVOUT[0];
	}
	usleep(10000);
	status = pmBusRead(deviceAddress, STATUS_WORD, 2, aStatusWORD);
	StatusWORD = (u16) ((aStatusWORD[0]) | (aStatusWORD[1]) << 8);
	if (StatusWORD != XST_SUCCESS) {
		/*
		 BIT 	NAME 				MEANING
		 15 		VOUT 				An output voltage fault or warning or TON_MAX_FAULT has occurred.
		 14 		0 					This bit always returns a 0.
		 13 		0 					This bit always returns a 0.
		 12 		MFR 				A bit in STATUS_MFR_SPECIFIC (PAGE = 255) has been set.
		 11 		POWER_GOOD# 		Any power-supply voltage has fallen from POWER_GOOD_ON to less than POWER_GOOD_
		 OFF (logical OR of all the POWER_GOOD# bits in STATUS_MFR_SPECIFC).
		 10 		0 					This bit always returns a 0.
		 9 		0 					This bit always returns a 0.
		 8 		MARGIN 				A margining fault has occurred.
		 7 		0 					This bit always returns a 0.
		 6 		SYS_OFF 			Set when any of the power supplies are sequenced off (logical OR of all the OFF bits in
		 STATUS_MFR_SPECIFC).
		 5 		VOUT_OV 			An overvoltage fault has occurred.
		 4 		0 					This bit always returns a 0.
		 3 		0 					This bit always returns a 0.
		 2 		TEMPERATURE 		A temperature fault or warning has occurred.
		 1 		CML 				A communication, memory, or logic fault has occurred.
		 0 		0 					This bit always returns a 0.
		 https://datasheets.maximintegrated.com/en/ds/MAX34460.pdf
		 */
		xil_printf("ERROR: StatusWORD:%d see comment in checkError()\r\n",
				StatusWORD);
		return StatusWORD;
	}
	return XST_SUCCESS;
}
