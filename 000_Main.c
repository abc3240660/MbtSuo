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
u8 bno055_int_flag=0;
// static u8 is_mode_nb = 0;
static u32 start_time_hbeat = 0;

static u8 gs_charge_sta = 0;

u8 g_svr_ip[LEN_NET_TCP+1]  = "192.168.1.105";
u8 g_svr_port[LEN_NET_TCP+1] = "10211";
u8 g_svr_apn[LEN_NET_TCP+1] = "sentinel.m2mmobi.be";

static void __delay_usx(uint16_t ms)
{
    int i=0,j=0;
    for(i=0;i<ms;i++){
        for(j=0;j<20;j++);
    }
}

int main(void)
{
	float cur_pitch;
	float cur_yaw;
	float cur_roll;
    u8 onebyte=0;
    u8 bno055_mode=0;
    u8 test_cnt = 0;
    u8 net_sta = 0;
    u32 boot_times = 0;

    u16 hbeat_gap = DEFAULT_HBEAT_GAP;

    unsigned long task_cnt = 0;

    u8 params_dat[LEN_BYTE_SZ64+1] = "";

    System_Config();
//    GPIOB_Init();
    Configure_Tick1_10ms();
    Configure_Tick2_10ms();

#ifndef DEMO_BOARD
//    CLRC663_PowerUp();
//    BG96_PowerUp();
#endif

    Uart1_Init();
    Uart2_Init();
    Uart3_Init();
    Uart4_Init();
	
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

#if 1// BNO055 Testing
    GPIOx_Config(BANKC, 13, OUTPUT_DIR);// BNO055 
    GPIOx_Config(BANKC, 14, OUTPUT_DIR);// BNO055
    GPIOx_Output(BANKC, 14, 1);// nTILT_BOOT_LOAD
   
    delay_ms(2000);
    GPIOx_Output(BANKC, 13, 0);// nTILT_RST
    delay_ms(2000);
    GPIOx_Output(BANKC, 13, 1);// nTILT_RST

    delay_ms(4000);
	BNO055_init();
	 if(0 == bno055_get_euler(&cur_pitch, &cur_yaw, &cur_roll))
		 printf("base : %f        %f       %f \n",cur_pitch, cur_yaw, cur_roll);
	 else
		 printf("Get base eurl failed \n");
    EXT_INT_Initialize();
     bno055_clear_int();//clear INT
    //bno055_enter_normal_mode();//
    //Configure_BNO055();
   // printf("After BG96 PowerOn...\r\n");
    
//    while(1) {
//        printf("BNO055 Testing...\r\n");
//        delay_ms(2000);
//    }
#endif

#if 0// CLRC663 LOOP Testing
    while(1)
    {
//        GPIOx_Output(BANKE, 7, 0);// IFSEL1 Low
//        delay_ms(1000);
//        GPIOx_Output(BANKE, 7, 1);// IFSEL1 Low
//        Uart3_Putc(0xFF);
        ReadMobibNFCCard();
//        read_iso14443A_nfc_card();

        delay_ms(2000);
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
        
//        if (IsDuringShip()) {
//            if (GPIOx_Input(BANKE, 4)) {// locked status mean to quit shiping mode
//                PowerOnMainSupply();
//                ClearShipStatus();
//            }
//
//            delay_ms(1000);
//
//            continue;
//        }
//
//        // If detect uncharge->charge, do reset once
//        if ((0==gs_charge_sta) && (1==GPIOx_Input(BANKG, 3))) {
//            printf("Charge Reset...\n");
//            asm("reset");
//        }
//
//        hbeat_gap = GetHeartBeatGap();
//        net_sta = GetNetStatus();
//
//        if (1 == IsApnChangeWait()) {
//            CloseTcpService();
//
//            delay_ms(200);
//            if (0x80 == net_sta) {
//                ResetApnChange();
//
//                ConnectToTcpServer(g_svr_ip, g_svr_port, g_svr_apn);
//                if (0x81 == net_sta) {
//                    FlashWrite_SysParams(PARAM_ID_SVR_IP, (u8*)g_svr_ip, strlen((const char*)g_svr_ip));
//                    FlashWrite_SysParams(PARAM_ID_SVR_PORT, (u8*)g_svr_port, strlen((const char*)g_svr_port));
//                    FlashWrite_SysParams(PARAM_ID_SVR_APN, (u8*)g_svr_apn, strlen((const char*)g_svr_apn));
//                }
//            }
//        }

        // -- TODO: cannot waste long time which will affect NFC-Read
        // -------------------- Re-Connect ------------------ //
        // --
        // retry initial or connection every 10s
        // if net-register failed or lost connection
        
        if (0 == (task_cnt++%200)) {
//            if (0 == net_sta) {
//                Configure_BG96();
//            }
//
//            net_sta = GetNetStatus();
//            if ((0x80==net_sta) || (0x40==net_sta)) {// lost connection
//                // ConnectToTcpServer(g_svr_ip, g_svr_port, g_svr_apn);
//            }
        }

        // -- use accuate time gap to hbeat
        // ---------------------- H Beat -------------------- //
        // --
//        if (0x81 == net_sta) {
//            if (0 == start_time_hbeat) {
//                start_time_hbeat = GetTimeStamp();
//            }
//
//            if (start_time_hbeat != 0) {
//                if (isDelayTimeout(start_time_hbeat,hbeat_gap*1000UL)) {
//                    start_time_hbeat = GetTimeStamp();
//                    TcpHeartBeat();
//                }
//            }
//        } else {
//            start_time_hbeat = 0;
//        }

        // --
        // ---------------------- TASK 1 -------------------- //
        // --
        if (0 == (task_cnt%2)) {// every 0.1s
           // ProcessIapRequest();// If IAP requested, will cost 1s every time
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
            //ProcessTcpSvrCmds();
            
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
//            if ((task_cnt%16) < 8) {
//                GPIOB_SetPin(task_cnt%4, 1);
//            } else {
//                GPIOB_SetPin(task_cnt%4, 0);
//            }
        } else {
//            if ((task_cnt%16) < 8) {
//                GPIOB_SetPin(task_cnt%2+1, 1);
//            } else {
//                GPIOB_SetPin(task_cnt%2+1, 0);
//            }
        }

        if (0 == (task_cnt%20)) {// every 1.0s
        }

        delay_ms(50);
    }

    return 0;
}
