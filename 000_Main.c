//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * Entrance of Application
 * This file is entrance of APP
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 03/11/2019
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <p24fxxxx.h>

#include "001_Tick_10ms.h"
#include "002_CLRC663.h"
#include "004_LB1938.h"
#include "005_BNO055.h"
#include "006_Gpio.h"
#include "007_Uart.h"
#include "009_System.h"
#include "011_Spi.h"
#include "012_CLRC663_NFC.h"
#include "016_FlashOta.h"
#include "017_InnerFlash.h"
#include "019_ADC0.h"
u8 bno055_int_flag=0;
// static u8 is_mode_nb = 0;
static u32 start_time_nfc = 0;

static u8 gs_charge_sta = 0;

u8 g_led_times;
u8 g_ring_times;

// 0-normal 1-low power
u8 g_bno055_sta;

// 0-normal 1-move interrupt
u8 g_bno055_move;

/*
static void __delay_usx(uint16_t ms)
{
    int i=0,j=0;
    for(i=0;i<ms;i++){
        for(j=0;j<20;j++);
    }
}
*/

static void PowerOffBG96Module(void)
{
    _ANSB9 = 0;

    GPIOx_Config(BANKB, 9, OUTPUT_DIR);// PWRKEY
    GPIOx_Output(BANKB, 9, 0);// default SI2302 HIGH
    delay_ms_nop(100);
    GPIOx_Output(BANKB, 9, 1);// MCU High -> SI2302 Low  -> PowerOff
    delay_ms_nop(800);// SI2302 Low >= 0.65s :  On -> Off
    GPIOx_Output(BANKB, 9, 0);// release to default SI2302 HIGH
}

int main(void)
{
	float cur_pitch;
	float cur_yaw;
	float cur_roll;
    u8 onebyte=0;
    u8 bno055_mode=0;

    u8 nfc_ret = 0;
    u8 net_sta = 0;
    u32 task_cnt = 0;
    u32 adc_val = 0;
    u8 nfc_enable = 0;

    u16 nfc_time = 10;

    System_Config();

    Beep_Init();
    PowerOffBG96Module();

    LEDs_Init();
    Charge_Init();
    LB1938_Init();

#if 0
    while (1) {
        if (Charge_InsertDetect()) {
            break;
        }

        delay_ms_nop(1000);
    }
#endif

    g_ring_times = 6;
    Configure_Tick1_10ms();
    Configure_Tick2_10ms();

//    CLRC663_PowerUp();
    BNO055_PowerUp();

    Uart1_Init();// Debug
    Uart3_Init();// CLRC663
    Uart4_Init();// BNO055

    printf("ART Application running...\r\n");

//    while (1) {
//        printf("ART Application running...\r\n");
//        delay_ms_nop(1000);
//    }

#if 1// BNO055 Testing
    delay_ms(4000);
	BNO055_init();
    if(0 == bno055_get_euler(&cur_pitch, &cur_yaw, &cur_roll))
        printf("base : %f        %f       %f \n",(double)cur_pitch, (double)cur_yaw, (double)cur_roll);
	else
	    printf("Get base eurl failed \n");
    EXT_INT_Initialize();
    bno055_clear_int();//clear INT
    //bno055_enter_normal_mode();//
    //Configure_BNO055();
    //printf("After BG96 PowerOn...\r\n");
    
//    while(1) {
//        printf("BNO055 Testing...\r\n");
//        delay_ms(2000);
//    }
#endif


    while(1)
    {
#if 0
        if (g_bno055_sta) {// low power
            Sleep();
            Nop();
        }

        if (g_bno055_move) {// BNO055 wakeup
            if (IsLockSwitchOpen()) {
                Sleep();
                Nop();
            } else {
                nfc_enable = 1;
            }
        } else {// WDT wakeup for Charge Detect
            if (Charge_InsertDetect()) {
                gs_charge_sta |= 0x80;

                ADC0_Init();

                if (ADC0_GetValue(&adc_val)) {
                    if (adc_val > 1000) {
                        Charge_Disable();
                        SetLedsStatus(MAIN_LED_G, LED_ON);
                    } else {
                        SetLedsMode(MAIN_LED_B, LED_BLINK);
                        SetLedsStatus(MAIN_LED_B, LED_ON);
                    }
                }

                ADC0_Disable();
            } else {
                if (0x80 == gs_charge_sta) {
                    gs_charge_sta = 0;
                    SetLedsMode(MAIN_LED_B, LED_BLINK);
                    SetLedsStatus(MAIN_LED_B, LED_OFF);
                    SetLedsStatus(MAIN_LED_G, LED_OFF);
                }
            }
        }

        // -- use accuate time gap to hbeat
        // ---------------------- H Beat -------------------- //
        // --
        if (1 == nfc_enable) {
            if (0 == start_time_nfc) {
                g_ring_times = 6;
                Enable_Tick2();
                SetLedsStatus(MAIN_LED_B, LED_ON);
                start_time_nfc = GetTimeStamp();
            }

            if (start_time_nfc != 0) {
                if (isDelayTimeout(start_time_nfc,nfc_time*1000UL)) {
                    nfc_enable = 0;
                    SetLedsStatus(MAIN_LED_B, LED_OFF);
                }
            }
        } else {
            start_time_nfc = 0;
        }
#endif
        if (0 == (task_cnt++%200)) {
        }

        // --
        // ---------------------- TASK 1 -------------------- //
        // --
        if (0 == (task_cnt%2)) {// every 0.1s
        }

        // --
        // ---------------------- TASK 2 -------------------- //
        // --
        if (0 == (task_cnt%11)) {  // every 0.5s
            if (nfc_enable) {
                if (ReadMobibNFCCard()) {
                    g_ring_times = 6;
                    Enable_Tick2();

                    LB1938_OpenLock();

                    g_led_times = 6;
                    SetLedsStatus(MAIN_LED_G, LED_ON);
                } else {
                    g_ring_times = 6;
                    Enable_Tick2();

                    g_led_times = 6;
                    SetLedsStatus(MAIN_LED_R, LED_ON);
                }
            }
        }

        // --
        // ---------------------- TASK 3 -------------------- //
        // --

        if (0 == (task_cnt%21)) {  // every 1.0s
            if(bno055_int_flag == 1)//
            {
                //INT occur
                bno055_int_flag=0;
                onebyte = bno055_get_int_src();//get the INT source
                bno055_clear_int();//clear INT
                if(onebyte&0x80)
                {
                    printf("Enter low power \n");
                    bno055_enter_lower_mode();
                    bno055_mode=1;
                    //delay_ms(500);
                    
#if 0
                    PMD1 = 0xFF;
                    PMD2 = 0xFF;
                    PMD3 = 0xFF;
                    PMD4 = 0xFF;
                    PMD5 = 0xFF;
                    PMD6 = 0xFF;
                    PMD7 = 0xFF;
                    PMD8 = 0xFF;    

                    _GIE = 0;
                    __builtin_write_OSCCONH(5);
                    __builtin_write_OSCCONL(0x01);
                    while(OSCCONbits.OSWEN);
                    _GIE = 1;
#endif

                    Sleep(); 
                }else if(onebyte&0x40){
                    printf("Enter normal \n");
                    bno055_enter_normal_mode();//
                    bno055_mode=0;
                }
            }else if(bno055_mode == 0){//
                if(0 == bno055_euler_check(cur_pitch, cur_yaw, cur_roll))
                    printf("OK \n");
                else
                    printf("NG \n");
            }
        }

        // --
        // ---------------------- TASK 4 -------------------- //
        // --
        if (0 == (task_cnt%31)) {  // every 1.5s
        }

        // --
        // ---------------------- TASK 5 -------------------- //
        // --
        if (0 == (task_cnt%41)) {  // every 2.0s
        }

        // --
        // ---------------------- TASK 6 -------------------- //
        // --
        if (0 == (task_cnt%99)) {  // every 5.0s
        }

        // -- just for engineer testing
        // ---------------------- TASK 7 -------------------- //
        // --
        if (0 == (task_cnt%199)) { // every 10.0s
            printf("task active\n");
        }

        if (10000 == task_cnt) {
            task_cnt = 0;
        }

        if (0 == (task_cnt%20)) {// every 1.0s
        }

        delay_ms(50);
    }

    return 0;
}
