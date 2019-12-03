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
static u32 start_time_nfc = 0;

static u8 gs_charge_sta = 0;

// 0-During Powering 1-ACK OK 2-CGATT OK
static u8 gs_bg96_sta = 0;

u8 g_svr_ip[LEN_NET_TCP+1]  = "192.168.1.105";
u8 g_svr_port[LEN_NET_TCP+1] = "10212";// 10211-TCP 10212-UDP
u8 g_svr_apn[LEN_NET_TCP+1] = "sentinel.m2mmobi.be";

u32 g_led_times = 0;
u8 g_ring_times = 0;

// 0-normal 1-move interrupt
u8 g_bno055_move = 0;

#define GPS_DEBUG 0

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
    u8 nfc_enable = 0;
    u32 adc_val = 0;
    u16 nfc_time = 10;
    u32 boot_times = 0;

    u8 trycnt = 0;
    u32 adcValue = 0;
#ifdef GPS_DEBUG
    u16 hbeat_gap = 30;
#else
    u16 hbeat_gap = DEFAULT_HBEAT_GAP;
#endif

    u8 intr_type = 0;

    // 0-normal 1-low power
    u8 bno055_mode = 0;

    u32 task_cnt = 0;

    u8 params_dat[LEN_BYTE_SZ64+1] = "";

    System_Config();

    Uart1_Init();
    Uart2_Init();
    Configure_Tick1();

    delay_ms(1000);
    DEBUG("Test12031728 Application running...\r\n");
    
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

	SetLedsStatus(MAIN_LED_G, LED_ON);
	g_led_times = 500;

#if 0
	LB1938_OpenLock();

	while(1);
#endif

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
#if 0
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
#endif

        if (IsDuringShip()) {
            if (GPIOx_Input(BANKE, 4)) {// locked status mean to quit shiping mode
                PowerOnMainSupply();
                ClearShipStatus();
            }

            delay_ms(1000);

            continue;
        }

#if 0
        // If detect uncharge->charge, do reset once
        if ((0==gs_charge_sta) && (1==GPIOx_Input(BANKG, 3))) {
            DEBUG("Charge Reset...\n");
            asm("reset");
        }
#endif

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

#if 0
        if (!g_ring_times && !g_led_times) {
            Disable_Tick2();
        } else {
			Enable_Tick2();
		}
		
        if (1 == nfc_enable) {
            if (0 == start_time_nfc) {
                DEBUG("start_time_nfc ...\n");
                g_ring_times = 2;
                g_led_times = 10000;
                SetLedsStatus(MAIN_LED_P, LED_ON);
                Enable_Tick2();
                start_time_nfc = GetTimeStamp();
            }

            if (start_time_nfc != 0) {
                if (isDelayTimeout(start_time_nfc,nfc_time*1000UL)) {
                    nfc_enable = 0;
                    DEBUG("nfc_enable = 0\n");

                    g_ring_times = 3;
                    Enable_Tick2();

                    g_led_times = 0;
                    delay_ms(15);

                    g_led_times = 5000;
                    SetLedsStatus(MAIN_LED_R, LED_ON);
                    // SetLedsStatus(MAIN_LED_G, LED_OFF);
                    // SetLedsStatus(MAIN_LED_B, LED_OFF);
                }
            }
        } else {
            start_time_nfc = 0;
        }
#endif

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
                    
                    // ManualIapRequested();
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
						
			if (true == IsIapRequested()) {
				ReadMobibNFCCard();
			}
        }

        // --
        // ---------------------- TASK 2 -------------------- //
        // --
        if (0 == (task_cnt%11)) {  // every 0.5s
			if (false == IsIapRequested()) {
				ReadMobibNFCCard();
			}
        }

        // --
        // ---------------------- TASK 3 -------------------- //
        // --
        if (0 == (task_cnt%21)) {  // every 1.0s
            DequeueTcpRequest();
            ProcessTcpRequest();
        }

        // --
        // ---------------------- TASK 4 -------------------- //
        // --
#if 0
        if (0 == (task_cnt%22)) {  // every 0.5s
            if (nfc_enable) {
                DEBUG("before ReadMobibNFCCard\n");
                if (0 == ReadMobibNFCCard()) {
                    g_ring_times = 1;
                    Enable_Tick2();

                    LB1938_OpenLock();

                    g_led_times = 0;
                    delay_ms(15);

                    g_led_times = 3000;
                    SetLedsStatus(MAIN_LED_G, LED_ON);

                    nfc_enable = 0;
                }
                DEBUG("after ReadMobibNFCCard\n");
            }
        }
#endif

        // --
        // ---------------------- TASK 5 -------------------- //
        // --
#if 0
        if (0 == (task_cnt%31)) {  // every 1.5s
            if (1 == GetBNOIntrFlag()) {
                // INT occur
                ClearBNOIntrFlag();
                DEBUG("New Intr Occur...\n");
                intr_type = bno055_get_int_src();//get the INT source
                
                DEBUG("Intr Reg = %.2X\n", intr_type);
                bno055_clear_int();//clear INT
                
                if (!nfc_enable) {
                    if (intr_type&0x80) {
                        bno055_mode = 1;// During Low Power Mode
                        Disable_Tick2();

                        CLRC663_PowerOff();
                        DEBUG("Enter low power\n");
                        bno055_enter_lower_mode();

                        if (0 == GetBNOIntrFlag()) {
                            SwitchToLowClock();

                            Sleep();
                            Nop();

#if 0
                            LEDs_Ctrl(MAIN_LED_G, LED_ON);
                            delay_us_nop(100);
                            LEDs_Ctrl(MAIN_LED_G, LED_OFF);
#endif

                            SwitchToNormalClock();

                            // Enable_Tick1();
                            DEBUG("Wake Up...\r\n");
                        } else {
                            DEBUG("Intr happen again...\n");
                        }

                        Enable_Tick2();
                    } else if (intr_type&0x40) {
                        DEBUG("Enter normal\n");
                        bno055_enter_normal_mode();//
                        bno055_mode = 0;// During Normal Mode

                        if (!IsLockSwitchOpen()) {
                            CLRC663_PowerUp();
                            nfc_enable = 1;
                            DEBUG("Ready to read NFC...\n");
                        } else {
                            CLRC663_PowerOff();
                            DEBUG("Lock is already open.\n");
                        }
                    }
                }
            } else if (0 == bno055_mode) {//
#if 0
                if (0 == bno055_euler_check(cur_pitch, cur_yaw, cur_roll))
                    DEBUG("Euler OK\n");
                else
                    DEBUG("Euler NG\n");
#endif
            }
        }
#endif

        // --
        // ---------------------- TASK 6 -------------------- //
        // --
        if (0 == (task_cnt%41)) {  // every 2.0s
            if (0 == gs_bg96_sta) {
                trycnt++;
                if (BG96EnsureRxOk()) {
                    trycnt = 0;
                    gs_bg96_sta = 1;

                    DumpNetCfg();
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
        // ---------------------- TASK 7 -------------------- //
        // --
        if (0 == (task_cnt%99)) {  // every 5.0s
        }

        // -- just for engineer testing
        // ---------------------- TASK 7 -------------------- //
        // --
        if (0 == (task_cnt%199)) { // every 10.0s
            // g_ring_times = 2;
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
