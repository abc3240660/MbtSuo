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
#include "012_CLRC663_NFC.h"
#include "013_Protocol.h"
#include "015_Common.h"
#include "016_FlashOta.h"
#include "017_InnerFlash.h"
#include "019_ADC0.h"

// static u8 is_mode_nb = 0;
static u32 start_time_hbeat = 0;

static u8 gs_charge_sta = 0;

// 0-During Powering 1-ACK OK 2-CGATT OK
static u8 gs_bg96_sta = 0;

u8 g_svr_ip[LEN_NET_TCP+1]  = "192.168.1.105";
u8 g_svr_port[LEN_NET_TCP+1] = "10212";// 10211-TCP 10212-UDP
u8 g_svr_apn[LEN_NET_TCP+1] = "sentinel.m2mmobi.be";

u32 g_led_times = 0;
u8 g_ring_times = 0;

// #define GPS_DEBUG 1

static void SwitchToLowClock(void)
{
    _GIE = 0;
    __builtin_write_OSCCONH(5);// LPRC
    __builtin_write_OSCCONL(1);// 32MHz change to 32KHz
    while(OSCCONbits.OSWEN);
    _GIE = 1;
}

static void SwitchToNormalClock(void)
{
    _GIE = 0;

#ifdef EXT_CRYSTAL
    __builtin_write_OSCCONH(3);// PRIPLL
#else
    __builtin_write_OSCCONH(1);// FRCPLL
#endif

    __builtin_write_OSCCONL(1);// 32KHz change to 32MHz
    while(OSCCONbits.OSWEN);
    _GIE = 1;
}

static void EnterToSleep(void)
{
    Sleep();
    Nop();
}

int main(void)
{
    float cur_pitch = 0;
    float cur_yaw = 0;
    float cur_roll = 0;
    u8 nfc_ret = 0;
    u8 net_sta = 0;
//    u32 boot_times = 0;

    u8 trycnt = 0;
    u32 adcValue = 0;
#ifdef GPS_DEBUG
    u16 hbeat_gap = 5;
#else
    u16 hbeat_gap = DEFAULT_HBEAT_GAP;
#endif

    unsigned long task_cnt = 0;

//    u8 params_dat[LEN_BYTE_SZ64+1] = "";

    System_Config();

    Uart1_Init();
    Uart2_Init();
    Configure_Tick1();

    delay_ms(1000);
    DEBUG("Ver11261313V2 Application running...\r\n");
    
    BG96_PowerUp();

//    delay_ms(5000);
    LEDs_Init();
    ADC0_Init();
    Beep_Init();
    Charge_Init();
    LB1938_Init();
    LockSwitch_Init();

    Configure_Tick2();
    Configure_Tick3();

    CLRC663_PowerUp();

    gs_bg96_sta = 0;

    Uart3_Init();
    Uart4_Init();
    
    BNO055_init();

#if 0// ADC Battery Voltage Testing
    while(1) {
        if(ADC0_GetValue(&adcValue)){
            DEBUG("ADC0:%ld\r\n",adcValue);
        }
        
        DEBUG("test...\n");
        delay_ms(5000);
    }
#endif

#if 0// GPS Locate Testing
    TurnOffGNSS();
    TurnOnGNSSDamon();

    while(1) {
        TurnTryLocate();
        delay_ms(5000);
    }
#endif

#if 0
    // Write params into flash when the first time boot
    FlashRead_SysParams(PARAM_ID_1ST_BOOT, params_dat, 64);
    if (strncmp((const char*)params_dat, (const char*)"1", 1) != 0) {
        FlashWrite_SysParams(PARAM_ID_SVR_IP, (u8*)g_svr_ip, strlen((const char*)g_svr_ip));
        FlashWrite_SysParams(PARAM_ID_SVR_PORT, (u8*)g_svr_port, strlen((const char*)g_svr_port));
        FlashWrite_SysParams(PARAM_ID_SVR_APN, (u8*)g_svr_apn, strlen((const char*)g_svr_apn));
        FlashWrite_SysParams(PARAM_ID_IAP_MD5, (u8*)"11112222333344445555666677778888", 32);
        FlashWrite_SysParams(PARAM_ID_1ST_BOOT, (u8*)"1", 1);
    }

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_BOOT_TM, params_dat, 64);
    if (strspn((const char*)params_dat, (const char*)"0123456789") == strlen((const char*)params_dat)) {
        boot_times = atoi((const char*)params_dat);
    }

    boot_times += 1;
    sprintf((char *)params_dat, "%ld", boot_times);
    FlashWrite_SysParams(PARAM_ID_BOOT_TM, params_dat, 3);

    ProtocolParamsInit();
    CardIDFlashBankInit();

    gs_charge_sta = GPIOx_Input(BANKG, 3);
#else
    InitRingBuffers();
#endif

#if 0// BNO055 Testing
	if (0 == bno055_get_euler(&cur_pitch, &cur_yaw, &cur_roll)) {
		DEBUG("Euler Base: %f %f %f\n", (double)cur_pitch, (double)cur_yaw, (double)cur_roll);
	} else {
		DEBUG("Get base eurl failed \n");
    }

    ExtIntr_Initialize();
    bno055_clear_int();// clear INT
    
    while(1) {
        DEBUG("BNO055 Testing...\r\n");
        
        ReadMobibNFCCard();

        delay_ms(5000);
    }
#endif

#if 0// CLRC663 LOOP Testing
    LEDs_AllOff();

    while(1)
    {
//        GPIOx_Output(BANKE, 7, 0);// IFSEL1 Low
//        delay_ms(1000);
//        GPIOx_Output(BANKE, 7, 1);// IFSEL1 Low
//        Uart3_Putc(0xFF);
        nfc_ret = ReadMobibNFCCard();
        if (0 == nfc_ret) {
            g_ring_times = 2;
        }
//        read_iso14443A_nfc_card();

        if (0 == nfc_ret) {
            LEDs_AllON();
        }
        delay_ms(1000);

        if (0 == nfc_ret) {
            LEDs_AllOff();
        }
        delay_ms(1000);
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

    while(1)
    {
        if (IsDuringShip()) {
            if (GPIOx_Input(BANKE, 4)) {// locked status mean to quit shiping mode
                PowerOnMainSupply();
                ClearShipStatus();
            }

            delay_ms(1000);

            continue;
        }

        // If detect uncharge->charge, do reset once
        if ((0==gs_charge_sta) && (1==GPIOx_Input(BANKG, 3))) {
            DEBUG("Charge Reset...\n");
            asm("reset");
        }

#ifndef GPS_DEBUG
        hbeat_gap = GetHeartBeatGap();
#endif
        net_sta = GetNetStatus();

        if (1 == IsApnChangeWait()) {
            CloseTcpService();

            delay_ms(200);
            if (0x80 == net_sta) {
                ResetApnChange();

                ConnectToTcpServer(g_svr_ip, g_svr_port, g_svr_apn);
                if (0x81 == net_sta) {
                    FlashWrite_SysParams(PARAM_ID_SVR_IP, (u8*)g_svr_ip, strlen((const char*)g_svr_ip));
                    FlashWrite_SysParams(PARAM_ID_SVR_PORT, (u8*)g_svr_port, strlen((const char*)g_svr_port));
                    FlashWrite_SysParams(PARAM_ID_SVR_APN, (u8*)g_svr_apn, strlen((const char*)g_svr_apn));
                }
            }
        }

        // -- TODO: cannot waste long time which will affect NFC-Read
        // -------------------- Re-Connect ------------------ //
        // --
        // retry initial or connection every 10s
        // if net-register failed or lost connection
        if (0 == (task_cnt++%200)) {
            if (0 == net_sta) {
                if (2 == gs_bg96_sta) {
                    Configure_BG96();
                }
            }

            net_sta = GetNetStatus();
            if ((0x80==net_sta) || (0x40==net_sta)) {// lost connection
                ConnectToTcpServer(g_svr_ip, g_svr_port, g_svr_apn);
            }
        }

        // -- use accuate time gap to hbeat
        // ---------------------- H Beat -------------------- //
        // --
        if (0x81 == net_sta) {
            if (0 == start_time_hbeat) {
                start_time_hbeat = GetTimeStamp();
            }

            if (start_time_hbeat != 0) {
                if (isDelayTimeout(start_time_hbeat,hbeat_gap*1000UL)) {
                    start_time_hbeat = GetTimeStamp();
#ifdef GPS_DEBUG
                    DoQueryGPSFast();
                    TcpReportGPS();
#endif
                    TcpHeartBeat();
                    
                    ManualIapRequested();
                }
            }
        } else {
            start_time_hbeat = 0;
        }

        // --
        // ---------------------- TASK 1 -------------------- //
        // --
        if (0 == (task_cnt%2)) {// every 0.1s
            ProcessIapRequest();// If IAP requested, will cost 1s every time
        }

        // --
        // ---------------------- TASK 2 -------------------- //
        // --
        if (0 == (task_cnt%11)) {  // every 0.5s
            // ReadMobibNFCCard();
        }

        // --
        // ---------------------- TASK 3 -------------------- //
        // --
        if (0 == (task_cnt%21)) {  // every 1.0s
            ProcessTcpSvrCmds();
            ProcessTcpServerCommand();
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
            if (0 == gs_bg96_sta) {
                trycnt++;
                if (BG96EnsureRxOk()) {
                    trycnt = 0;
                    gs_bg96_sta = 1;

                    DumpNetMode();
                    QueryNetMode();
                }
            }

            if (1 == gs_bg96_sta) {
                trycnt++;
                if (QueryNetStatus()) {
                    gs_bg96_sta = 2;
                    trycnt = 0;
                }
            }
            
            if (trycnt >= 100) {
                trycnt = 0;
                SetAutoNetMode();
                BG96_PowerUp();
                // asm("reset");
            }
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
            // g_ring_times = 2;
            DEBUG("task active\n");
#if 0
            if (0x81 == net_sta) {
                // Auto Dev Send Test
                if (0 ==  test_cnt) {
                    TcpHeartBeat();
                } else if (1 ==  test_cnt) {
                    TcpExitCarriageSleep();
                } else if (2 ==  test_cnt) {
                    DoQueryGPSFast();
                    TcpReportGPS();
                } else if (3 ==  test_cnt) {
                    TcpInvalidMovingAlarm();
                } else if (4 ==  test_cnt) {
                    TcpRiskAlarm();
                } else if (5 ==  test_cnt) {
                    TcpFinishIAP();
                } else if (6 ==  test_cnt) {
                    TcpFinishAddNFCCard();
                } else if (7 ==  test_cnt) {
                    TcpReadedOneCard(NULL, NULL);
                } else if (8 ==  test_cnt) {
                    TcpLockerLocked();
                } else if (9 ==  test_cnt) {
                    TcpChargeStarted();
                } else if (10 ==  test_cnt) {
                    TcpChargeStoped();
                } else if (11 ==  test_cnt) {
                    TcpFinishFactoryReset();
                }
            }

            test_cnt++;
            if (test_cnt > 12) {
                test_cnt = 0;
            }
#endif
        }

        if (10000 == task_cnt) {
            task_cnt = 0;
        }

#if 0
        if (0x81 == net_sta) {
            if ((task_cnt%16) < 8) {
                GPIOB_SetPin(task_cnt%4, 1);
            } else {
                GPIOB_SetPin(task_cnt%4, 0);
            }
        } else {
            if ((task_cnt%16) < 8) {
                GPIOB_SetPin(task_cnt%2+1, 1);
            } else {
                GPIOB_SetPin(task_cnt%2+1, 0);
            }
        }
#endif

        if (0 == (task_cnt%20)) {// every 1.0s
        }

        delay_ms(50);
    }

    return 0;
}
