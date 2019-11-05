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
#include "003_BG96.h"
#include "004_LB1938.h"
#include "005_BNO055.h"
#include "006_Gpio.h"
#include "007_Uart.h"
#include "009_System.h"
#include "011_Spi.h"
#include "012_CLRC663_NFC.h"
#include "013_Protocol.h"
#include "016_FlashOta.h"
#include "017_InnerFlash.h"
#include "019_ADC0.h"

// static u8 is_mode_nb = 0;
static u32 start_time_nfc = 0;

static u8 gs_charge_sta = 0;

u8 g_svr_ip[LEN_NET_TCP+1]  = "192.168.1.105";
u8 g_svr_port[LEN_NET_TCP+1] = "10212";// 10211-TCP 10212-UDP
u8 g_svr_apn[LEN_NET_TCP+1] = "sentinel.m2mmobi.be";

u8 g_led_times;
extern u8 g_ring_times;

// 0-normal 1-low power
extern u8 g_bno055_sta;

// 0-normal 1-move interrupt
extern u8 g_bno055_move;

/*
static void __delay_usx(uint16_t ms)
{
    int i=0,j=0;
    for(i=0;i<ms;i++){
        for(j=0;j<20;j++);
    }
}
*/


int main(void)
{
    u8 nfc_ret = 0;
    u8 net_sta = 0;
    u32 task_cnt = 0;
    u32 adc_val = 0;
    u8 nfc_enable = 0;

    u16 nfc_time = 10;

    System_Config();

    Beep_Init();
    BG96_PowerOff();

    LEDs_Init();
    Charge_Init();
    LB1938_Init();

    while (1) {
        if (Charge_InsertDetect()) {
            break;
        }

        delay_ms_nop(1000);
    }

    g_ring_times = 6;
    Configure_Tick1_10ms();
    Configure_Tick2_10ms();

    CLRC663_PowerUp();
    BNO055_PowerUp();

    Uart1_Init();// Debug
    Uart3_Init();// CLRC663
    Uart4_Init();// BNO055

    printf("ART Application running...\r\n");

    Configure_BNO055();

    while(1)
    {
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
