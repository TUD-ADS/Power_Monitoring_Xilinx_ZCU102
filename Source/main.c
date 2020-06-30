#include <stdio.h>
#include "xil_printf.h"
#include "xil_exception.h"
#include "xil_cache.h"
#include "xscugic.h"
#include "xipipsu.h"
#include "xipipsu_hw.h"


#include "pmbus_iic.h"
#include "xgpiops.h"

// const float targetVoltage = 0.85;

int main() {
	
	//------------------------------------------------------------------------------------------------------

	int status = setupHardwareSubsystem();
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: setupHardwareSubsystem()\r\n");
	}

	power_mon(VCCINT, "VCCINT");
//	voltageScaling(targetVoltage, VCCINT);
	power_mon(VCCINT, "VCCINT");
		xil_printf("--------------------------------------------------------------\n");
	while (*ptr != XST_SUCCESS) {
		power_mon(VCCINT, "VCCINT");
		}
	//------------------------------------------------------------------------------------------------------
	return 0;
}

