//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

//******************************************************************************
// File:   004_LB1938.h
// Author: Hans Desmet
// Comments: File handles LB1938 motor controller 
// Revision history: Version 01.00
// Date 26/12/2018
//******************************************************************************

#include "001_Tick_10ms.h"
#include "004_LB1938.h"
#include "006_Gpio.h"
#include "007_Uart.h"

//******************************************************************************
// Configure LB1938
//******************************************************************************
void LB1938_Init(void)
{
    // Motor LB1938FA:
    // RD9 -> MOTOR_FWD
    // RD8 -> nMOTOR_FWD

    GPIOx_Config(BANKD, 8, OUTPUT_DIR);
    GPIOx_Config(BANKD, 9, OUTPUT_DIR);

    GPIOx_Output(BANKD, 8, 0);
    GPIOx_Output(BANKD, 9, 0);
}

void LB1938_MotorCtrl(MOTOR_FWD motor_fwd, u16 hold_ms)
{
    u8 pin = 0;
	u8 sw_sta = 0;
	u16 time_delay = 0;

    if (MOTOR_LEFT == motor_fwd) {
        pin = 8;
    } else {
        pin = 9;
    }

    GPIOx_Output(BANKD, pin, 1);
	
	while (1) {
		if (!GPIOx_Input(BANKF, 5)) {
			if (1 == sw_sta) {
				GPIOx_Output(BANKD, pin, 0);
			}
		} else {
			sw_sta = 1;
		}
		
		delay_ms(2);
	}

    // delay_ms(hold_ms);
    // GPIOx_Output(BANKD, pin, 0);
}

void LB1938_OpenLock(void)
{
    // Mbtsuo_OpenClockTestV1
    // LB1938_MotorCtrl(MOTOR_LEFT, 100);
    
    // Mbtsuo_OpenClockTestV2
    // LB1938_MotorCtrl(MOTOR_LEFT, 1000);

    // Mbtsuo_OpenClockTestV3
    // LB1938_MotorCtrl(MOTOR_RIGHT, 100);

    // Mbtsuo_OpenClockTestV4
    // LB1938_MotorCtrl(MOTOR_RIGHT, 1000);

    // Mbtsuo_OpenClockTestV5
    // LB1938_MotorCtrl(MOTOR_LEFT, 5000);

    // Mbtsuo_OpenClockTestV6
    LB1938_MotorCtrl(MOTOR_RIGHT, 5000);
}

//******************************************************************************
//* END OF FILE
//******************************************************************************
