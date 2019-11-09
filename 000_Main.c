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
#include "012_CLRC663_NFC.h"
#include "016_FlashOta.h"
#include "017_InnerFlash.h"
#include "019_ADC0.h"

// static u8 is_mode_nb = 0;
static u32 start_time_nfc = 0;
static u8 gs_charge_sta = 0;

u8 g_led_times = 0;
u8 g_ring_times = 0;
u8 bno055_int_flag = 0;

// 0-normal 1-low power
u8 g_bno055_sta = 0;

// 0-normal 1-move interrupt
u8 g_bno055_move = 0;

extern u16 recv_total_len;
extern u8 BNO_055_RECV_BUFF[64];

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
    float cur_pitch = 0;
    float cur_yaw = 0;
    float cur_roll = 0;

    u8 bno055_intr_mode = 0;

    u8 try_cnt = 0;
    u8 nfc_ret = 0;
    u8 net_sta = 0;
    u8 nfc_enable = 0;

    u32 adc_val = 0;
    u32 task_cnt = 0;
    u16 nfc_time = 10;

    System_Config();

    Beep_Init();
    PowerOffBG96Module();

    LEDs_Init();
    Charge_Init();
    LB1938_Init();
    LockSwitch_Init();

#if 0// TypeC Check
    while (1) {
        if (Charge_InsertDetect()) {
            break;
        }

        delay_ms_nop(1000);
    }
#endif

    g_ring_times = 3;
    Configure_Tick1_10ms();
    Configure_Tick2_10ms();
    
    while (0) {
//        GPIOx_Output(BANKD, MAIN_LED_B, 1);
        GPIOx_Output(BANKD, MAIN_LED_R, 1);
        GPIOx_Output(BANKD, MAIN_LED_G, 1);
        delay_ms(10);
        GPIOx_Output(BANKD, MAIN_LED_B, 0);
        GPIOx_Output(BANKD, MAIN_LED_R, 0);
        GPIOx_Output(BANKD, MAIN_LED_G, 0);
        delay_ms(990);
    }

    CLRC663_PowerUp();
    BNO055_PowerUp();

    Uart1_Init();// Debug
    Uart3_Init();// CLRC663
    Uart4_Init();// BNO055

    DEBUG("ART Application running...\r\n");

#if 0// soft delay test
    while (1) {

        DEBUG("ART Application running...\r\n");
        delay_ms_nop(1000);
    }
#endif
    
#if 1// CLRC663 LOOP Testing
    while(1) {
        nfc_ret = ReadMobibNFCCard();
        if (0 == nfc_ret) {
            g_ring_times = 2;
        }

        // read_iso14443A_nfc_card();

        if (0 == nfc_ret) {
            LEDs_AllON();
        }
        delay_ms(1000);

        if (0 == nfc_ret) {
            LEDs_AllOff();
        }
        delay_ms(1000);
        
        if (try_cnt++ >= 5) {
            break;
        }
    }
#endif

#if 0// Buzzer LOOP Testing
    GPIOx_Config(BANKB, 13, OUTPUT_DIR);// Beep

    u8 beep_loop = 0;

    while(1)
    {
        GPIOx_Output(BANKB, 13, 1);
        __delay_usx(10);
        GPIOx_Output(BANKB, 13, 0);
        __delay_usx(10);
        
        if (beep_loop++ >= 100) {
            beep_loop = 0;
            DEBUG("XBeep Testing...\r\n");
        }
    }
#endif

#if 0
    u8 i = 0;
    u16 len1 = 0;
    u16 len2 = 0;

    while (1) {
        len1 = recv_total_len;
        delay_ms(100);
        len2 = recv_total_len;
        
        if ((len1 == len2) && (len1 != 0)) {
            recv_total_len = 0;
            for (i=0; i<len1; i++) {
                DEBUG("[%.2X] ", BNO_055_RECV_BUFF[i]);
            }
        }
    }
#endif

#if 1// BNO055 Testing
    delay_ms(4000);
    BNO055_init();

    if (0 == bno055_get_euler(&cur_pitch, &cur_yaw, &cur_roll))
        DEBUG("base : %f        %f       %f \n",(double)cur_pitch, (double)cur_yaw, (double)cur_roll);
    else
        DEBUG("Get base eurl failed \n");

    EXT_INT_Initialize();
    bno055_clear_int();//clear INT
    // bno055_enter_normal_mode();//
    // Configure_BNO055();
    // DEBUG("After BG96 PowerOn...\r\n");
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
            if (bno055_int_flag == 1) {
                // INT occur
                bno055_int_flag=0;
                bno055_intr_mode = bno055_get_int_src();// get the INT source
                bno055_clear_int();// clear INT
                if (bno055_intr_mode&0x80) {
                    DEBUG("BNO055 enter low power mode\n");
                    bno055_enter_lower_mode();
                    // delay_ms(500);
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
                    __builtin_write_OSCCONH(5);// LPRC
                    __builtin_write_OSCCONL(0x01);
                    while(OSCCONbits.OSWEN);
                    _GIE = 1;
#endif

                    Sleep();

#if 0
                    _GIE = 0;
                    __builtin_write_OSCCONH(3);// PRIPLL
                    __builtin_write_OSCCONL(0x01);
                    while(OSCCONbits.OSWEN);
                    _GIE = 1;
#endif
                } else if (bno055_intr_mode&0x40) {
                    DEBUG("BNO055 enter normal mode\n");
                    bno055_enter_normal_mode();
                }
            }

            // Normal mode
            if (bno055_intr_mode&0x40) {
#if 0
                if (0 == bno055_euler_check(cur_pitch, cur_yaw, cur_roll))
                    DEBUG("Euler OK\n");
                else
                    DEBUG("Euler NG\n");
#endif
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
            DEBUG("task active\n");
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
