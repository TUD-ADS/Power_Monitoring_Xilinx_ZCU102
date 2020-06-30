
#ifndef PMBUS_IIC_H_
#define PMBUS_IIC_H_

#include "xiicps.h"
#include "math.h"
#include "xparameters.h"
#include "xil_printf.h"
#include "xgpiops.h"

/*
OM 										LP FP
VCCPSINTLP 								on on
VCCPSINTFP 								off on
VCCINT_IO / VCCBRAM 					off on
VCCINT 									off on
VCCPSAUX / VCCPSIO (0:2:3) / PSBATT 	on on
VCCPLL 									on on
VCCDDR / VCCO / DDR4 devices 			off on
VCCAUX / AUX_IO / VCCO / ENET PHY 		off on
VCCO 									off on
VCCO / ENET PHY 						off on
VTERM 									off on
ENET PHY 								off on
https://www.infineon.com/dgdl/Infineon-Infineon+Power+Cookbook+-+Zynq+UltraScale+-+March+2017rev0.1-ATI-v01_00-EN.pdf?fileId=5546d4625c54d85b015c9c8928e36684
*/


/* Device Addresses */
//Programmable Logic
//Internal supply voltage 								-0.5-1.0V
#define VCCINT						0x13				//U47 0X13 MAX15301 VCCINT
//Processing System
//PS primary logic full-power domain supply voltage		-0.5-1.0V
#define VCCPSINTFP					0X0A				//U46 0X0A MAX15301 VCCPSINTFP
//PS primary logic low-power domain supply voltage		-0.5-1.0V
#define VCCPSINTLP					0X0B				//U4 0X0B MAX15303 VCCPSINTLP

/* PMBUS Commands */
#define CMD_PAGE            	0x00
#define CMD_OPERATION       	0x01
#define CMD_VOUT_MODE			0x20
#define CMD_VOUT_COMMAND    	0x21
#define CMD_VOUT_MAX        	0x24
#define CMD_READ_VOUT       	0x8B
#define CMD_READ_IOUT       	0x8C

#define POWER_GOOD_OFF				0x5F
#define POWER_GOOD_ON				0x5E
#define VOUT_UV_FAULT_LIMIT			0x44
#define VOUT_MARGIN_LOW				0x26
#define VOUT_MARGIN_HIGH			0x25
#define VOUT_MAX					0x24
#define VOUT_OV_FAULT_LIMIT			0x40

#define STATUS_VOUT					0x7A
#define STATUS_WORD					0x79

/* Operating modes for operation command */
#define OP_MODE_NOM         	0x80
#define OP_MODE_MAR_LOW     	0x94
#define OP_MODE_MAR_HIGH    	0xA4

/* Control values for the IIC mux */
#define CMD_OUTPUT_0_REG		0x02U
#define DATA_OUTPUT				0x0U
#define IOEXPANDER1_ADDR		0x20
#define IIC_SCLK_RATE_I2CMUX 	600000
#define CMD_CH_2_REG			0x06U
#define PCA9544A_ADDR			0x75U

/* Function Declarations */
unsigned int pmBusWrite(unsigned char address, unsigned char command, unsigned char data);
unsigned char pmBusWriteWord(unsigned  char address, unsigned char command, unsigned char *data);
unsigned int pmBusRead(unsigned char address, unsigned char command, unsigned char byteCount, unsigned char *buffer);
unsigned int read_iic(unsigned char address, unsigned char byteCount, unsigned char *buffer);
double linear11ToFloat(unsigned char highByte, unsigned char lowByte);
unsigned char readVoltage(unsigned char deviceAddress, unsigned char *receiveBuf, unsigned char command);
unsigned char readCurrent(unsigned char deviceAddress,  unsigned char *receiveBuf);
float readVoltage_real(unsigned char deviceAddress, unsigned char command);
double 	readCurrent_real(unsigned char deviceAddress);
int setupIic(void);
int myusleep(unsigned int useconds);
void power_mon(unsigned char device_adr, char *voltageName);
int setupHardwareSubsystem(void);
int setupGpio(void);
int programVoltage(float targetVoltage, unsigned char deviceAddress, unsigned char command);
void voltageScaling(float targetVoltage, unsigned char deviceAddress);
void resetVoltageLevel(unsigned char deviceAddress, float targetVoltage);
int checkError(unsigned char deviceAddress);
#endif /* PMBUS_IIC_H_ */
