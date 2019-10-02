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

// static u8 is_mode_nb = 0;
static u32 start_time_hbeat = 0;

static u8 gs_charge_sta = 0;

u8 g_svr_ip[LEN_NET_TCP+1]  = "192.168.1.105";
u8 g_svr_port[LEN_NET_TCP+1] = "10211";
u8 g_svr_apn[LEN_NET_TCP+1] = "sentinel.m2mmobi.be";

int main(void)
{
    u8 net_sta = 0;
    u32 boot_times = 0;

    u16 hbeat_gap = DEFAULT_HBEAT_GAP;

    unsigned long task_cnt = 0;

    u8 params_dat[LEN_BYTE_SZ64+1] = "";

    System_Config();
    GPIOB_Init();
    Configure_Tick_10ms();
    Configure_Tick2_10ms();
    Uart1_Init();
    Uart2_Init();
#if 0
//    Uart3_Init();
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
#endif

    InitRingBuffers();
    printf("Application running...\r\n");

#if 0
    while(1) {
        if ((task_cnt%8) < 4) {
            GPIOB_SetPin(task_cnt%4, 1);
        } else {
            GPIOB_SetPin(task_cnt%4, 0);
        }
        
        Uart1_Putc('3');
        Uart1_Putc('4');
        Uart1_Putc('5');
        Uart1_Putc('6');

        if (0 == task_cnt) {
            TRISB &= 0x90F9;// Direction:0-OUT 1-IN

            if (0 == GPIOx_Input(BANKB, 3)) {
                printf("PB3 is Low\n");
            } else {
                printf("PB3 is High\n");
            }

            GPIOx_Output(BANKB, 9, 0);// PWRKEY low
            delay_ms(50);
            GPIOx_Output(BANKB, 9, 1);// PWRKEY High
            delay_ms(1000);
            GPIOx_Output(BANKB, 9, 0);// PWRKEY low

            delay_ms(10000);

            if (0 == GPIOx_Input(BANKB, 3)) {
                printf("PB3 is Low\n");
            } else {
                printf("PB3 is High\n");
            }

//            GPIOx_Output(BANKB, 10, 1);// not airplane mode
//            GPIOx_Output(BANKB, 11, 1);// RST(default HIGH))

            Configure_BG96();
        }

        task_cnt++;

        delay_ms(1000);
    }
#endif

    TRISB &= 0x90F9;// Direction:0-OUT 1-IN
    GPIOx_Output(BANKB, 9, 0);// PWRKEY low
    printf("test001...\n");
    delay_ms(50);
    GPIOx_Output(BANKB, 9, 1);// PWRKEY High
    printf("test002...\n");
    delay_ms(5000);
    GPIOx_Output(BANKB, 9, 0);// PWRKEY low
    printf("test003...\n");
    delay_ms(2000);
    GPIOx_Output(BANKB, 9, 1);// PWRKEY High
    printf("test004...\n");

    delay_ms(5000);
    printf("After BG96 PowerOn...\r\n");

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
                // ConnectToTcpServer(g_svr_ip, g_svr_port, g_svr_apn);
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

        if (0 == (task_cnt%20)) {// every 1.0s
        }

        delay_ms(50);
    }

    return 0;
}
