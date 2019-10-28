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
static u32 start_time_hbeat = 0;

static u8 gs_charge_sta = 0;

u8 g_svr_ip[LEN_NET_TCP+1]  = "192.168.1.105";
u8 g_svr_port[LEN_NET_TCP+1] = "10212";// 10211-TCP 10212-UDP
u8 g_svr_apn[LEN_NET_TCP+1] = "sentinel.m2mmobi.be";

extern u8 g_ring_times;

/*
static void __delay_usx(uint16_t ms)
{
    int i=0,j=0;
    for(i=0;i<ms;i++){
        for(j=0;j<20;j++);
    }
}
*/

void delay_debug(u32 cnt)
{
    u32 i = 0;
    u32 j = 0;
    
    for (i=0; i<cnt; i++)
        for (j=0; j<1000; j++);
}
#define GPS_DEBUG 1

int main(void)
{
#if 0
#if 0
    _ANSB0 = 0;
    _ANSB1 = 0;
    _ANSB2 = 0;
    _ANSB3 = 0;

    _ANSB10 = 0;
    _ANSB11 = 0;
    _ANSB12 = 0;
    _ANSB13 = 0;
#else
    ANSB = 0;
    ANSC = 0;
    ANSD = 0;
    ANSE = 0;
    ANSF = 0;
    ANSG = 0;
#endif

#if 0
    GPIOx_Config(BANKB, 0, OUTPUT_DIR);// Beep
    GPIOx_Config(BANKB, 1, OUTPUT_DIR);// Beep
    GPIOx_Config(BANKB, 2, OUTPUT_DIR);// Beep
    GPIOx_Config(BANKB, 3, OUTPUT_DIR);// Beep

    GPIOx_Config(BANKB, 10, OUTPUT_DIR);// Beep
    GPIOx_Config(BANKB, 11, OUTPUT_DIR);// Beep
    GPIOx_Config(BANKB, 12, OUTPUT_DIR);// Beep
    GPIOx_Config(BANKB, 13, OUTPUT_DIR);// Beep
    
    IOCPDB = 0xF + (0xF<<10);

#if 0
    GPIOx_Output(BANKB, 0, 0);
    GPIOx_Output(BANKB, 1, 0);
    GPIOx_Output(BANKB, 2, 0);
    GPIOx_Output(BANKB, 3, 0);

    GPIOx_Output(BANKB, 10, 0);
    GPIOx_Output(BANKB, 11, 0);
    GPIOx_Output(BANKB, 12, 0);
    GPIOx_Output(BANKB, 13, 0);
#endif
#else
    TRISB = 0xFF;
    TRISC = 0xFF;
    TRISD = 0xFF;
    TRISE = 0xFF;
    TRISF = 0xFF;
    TRISG = 0xFF;
    
    IOCPDB = 0xFF;
    IOCPDC = 0xFF;
    IOCPDD = 0xFF;
    IOCPDE = 0xFF;
    IOCPDF = 0xFF;
    IOCPDG = 0xFF;
    
    PMD1 = 0xFF;
    PMD2 = 0xFF;
    PMD3 = 0xFF;
    PMD4 = 0xFF;
    PMD5 = 0xFF;
    PMD6 = 0xFF;
    PMD7 = 0xFF;
    PMD8 = 0xFF;
#endif
#if 0
    while(1) {
#if 0
        GPIOx_Output(BANKB, 12, 0);
        GPIOx_Output(BANKB, 13, 1);
//        delay_debug(1);// LPRC
//        delay_debug(100);// FRC
        delay_debug(300);// PRI 20MHz
        GPIOx_Output(BANKB, 12, 1);
        GPIOx_Output(BANKB, 13, 0);
//        delay_debug(1);// LPRC
//        delay_debug(100);// FRC
        delay_debug(300);// PRI 20MHz
#endif
    }
#endif

    Sleep();
    Nop();
    while(1);
#endif
    u8 nfc_ret = 0;
    u8 net_sta = 0;
//    u32 boot_times = 0;

    u32 adcValue = 0;
#ifdef GPS_DEBUG
    u16 hbeat_gap = 5;
#else
    u16 hbeat_gap = DEFAULT_HBEAT_GAP;
#endif

    unsigned long task_cnt = 0;

//    u8 params_dat[LEN_BYTE_SZ64+1] = "";

#if 1
    _ANSB13 = 0;
    GPIOx_Config(BANKB, 13, OUTPUT_DIR);// Beep
    GPIOx_Output(BANKB, 13, 0);
#endif

    System_Config();
//    GPIOB_Init();
    LEDs_Init();
    ADC0_Init();
    Configure_Tick_10ms();
    Configure_Tick2_10ms();
//    Configure_Tick3_10ms();
//    Configure_Tick4_10ms();

#ifndef DEMO_BOARD
    CLRC663_PowerUp();
    BG96_PowerUp();
#endif

    Uart1_Init();
    LEDs_AllOff();

#if 0
    while(1) {
        printf("test001XApplication running......\n");
        delay_ms(3000);
    }
#endif
    Uart2_Init();
    Uart3_Init();
    Uart4_Init();
    
    LEDs_AllOff();

#if 1
    while(1) {
        if(ADC0_GetValue(&adcValue)){
            printf("ADC1:%ld\r\n",adcValue);
        }
        
        delay_ms(5000);
    }
#endif

#if 0
    TurnOffGNSS();
    TurnOnGNSSDamon();

    while(1) {
        TurnTryLocate();
        delay_ms(5000);
    }
#endif

#if 0
//    LB1938_Init();
//    SPI2_Init();

    // BNO055 Testing
    // Configure_BNO055();
    // bno055_calibrate_demo();
    // bno055_demo();

    delay_ms(3000);

    printf("Application running...\r\n");

    InitRingBuffers();

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
    printf("XApplication running...\r\n");
#endif
    
    // Diable Beep
    GPIOx_Config(BANKB, 13, OUTPUT_DIR);// Beep
    GPIOx_Output(BANKB, 13, 0);

#if 1// BNO055 Testing
    GPIOx_Config(BANKC, 13, OUTPUT_DIR);// BNO055
    GPIOx_Config(BANKC, 14, OUTPUT_DIR);// BNO055
    GPIOx_Output(BANKC, 13, 1);
    GPIOx_Output(BANKC, 14, 1);

    delay_ms(2000);
    Configure_BNO055();
    printf("After BG96 PowerOn...\r\n");
    
    while(1) {
        printf("BNO055 Testing...\r\n");
        delay_ms(2000);
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
            printf("XBeep Testing...\r\n");
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
            printf("Charge Reset...\n");
            asm("reset");
        }

        hbeat_gap = GetHeartBeatGap();
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
                Configure_BG96();
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
