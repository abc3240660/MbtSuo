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
#include "000_Main.h"

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
u32 g_led_always_on=0;

// 0-normal 1-move interrupt
u8 g_bno055_move = 0;

// #define GPS_DEBUG 1

static void SwitchToLowClock(void)
{
    
    _GIE = 0;
    /*
    __builtin_write_OSCCONH(5);// LPRC
    __builtin_write_OSCCONL(1);// 32MHz change to 32KHz
    while(OSCCONbits.OSWEN);
    */
    _GIE = 1;
    
}

static void SwitchToNormalClock(void)
{
    /*
         _GIE = 0;
    __builtin_write_OSCCONH(0x01);

    // Start clock switching
   __builtin_write_OSCCONL(0x01);    

    // Wait for Clock switch to occur (COSC = 0b011)
    while (OSCCONbits.COSC != 0b001);

    // Wait for PLL to lock
    while(OSCCONbits.LOCK!=1);
*/
#if 0
#ifdef EXT_CRYSTAL
    __builtin_write_OSCCONH(3);// PRIPLL
#else
    __builtin_write_OSCCONH(1);// FRCPLL
#endif

    __builtin_write_OSCCONL(1);// 32KHz change to 32MHz
    while(OSCCONbits.OSWEN);
#endif
    /*
       // Initiate Clock Switch to Primary OSC with PLL (NOSC=0b011)
    __builtin_write_OSCCONH(0x01);

    // Start clock switching
    __builtin_write_OSCCONL(0x01);    

    // Wait for Clock switch to occur (COSC = 0b011)
    while (OSCCONbits.COSC != 0b011);

    // Wait for PLL to lock
    while(OSCCONbits.LOCK!=1);

    _GIE = 1;
     */
}

static void EnterToSleep(void)
{
    Sleep();
    Nop();
}

static u32 GetIapBinSize(void)
{
    u16 i = 0;
    u32 size = 0;
    u8 strs_array[LEN_BYTE_SZ64+1] = "";

    FlashRead_SysParams(PARAM_ID_RSVD_U4, strs_array, LEN_BYTE_SZ64);

    for (i=0; i<8; i++) {
        if ((strs_array[i]<'0') || (strs_array[i]>'9')) {// Invalid Data In "BIN SIZE" Flash Section
            strs_array[i] = 0;
            break;
        }
    }

	DEBUG("Re-Start IAP Got SIZEV1 = %s\n", strs_array);

    for (i=0; i<strlen((const char*)strs_array); i++) {
        size *= 10;
        size += strs_array[i] - '0';
    }

	DEBUG("Re-Start IAP Got SIZEV = %ld\n", size);

	return size;
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
    u16 hbeat_gap = 5;
#else
    u16 hbeat_gap = DEFAULT_HBEAT_GAP;
#endif
    u8 flag_led=0;
    u8 intr_type = 0;
	u8 wdt_reset = 0;
    u8 ftp_restart = 0;

    // 0-normal 1-low power
    u8 bno055_mode = 0;

    u32 task_cnt = 0;

    u8 params_dat[LEN_BYTE_SZ64+1] = "";

	if (_WDTO) {
		wdt_reset = 1;
	}

    System_Config();
    BNO055_PowerUp();
    BoardPowerInit();
    BoardPower(0);


    InitRingBuffers();
    delay_ms_nop(500);
    BoardPower(1);
    LEDs_Init();
    ADC0_Init();
    Beep_Init();
    Charge_Init();
    LB1938_Init();
    LockSwitch_Init();
   
    _SWDTEN = 1;// wait 8.192s
    //?????
    SetLedsStatus(MAIN_LED_G, LED_ON);
    g_led_times=3000;

    CLRC663_PowerUp();

    gs_bg96_sta = 0;
    Uart1_Init();
	Uart3_Init();
    Uart4_Init();
#ifdef BG96COMM
	Uart2_Init();
    BG96_Config_HW();
#endif
    DEBUG("Test%s Application running...\r\n", SW_VER);

    Configure_Tick1();
    Configure_Tick2();
    Configure_Tick3();

#ifdef BG96COMM
    PowerOnBG96Module(false);
#endif
    
    BNO055_init();


#if 0	
	if (1 == wdt_reset) {
		DEBUG("Reset by WDT Timeout...\r\n");
#if 1
		memset(params_dat, 0, LEN_BYTE_SZ64);
		FlashRead_SysParams(PARAM_ID_IAP_FLAG, params_dat, 64);
		if (0 == strncmp((const char*)params_dat, (const char*)IAP_REQ_RN, 4)) {
			DEBUG("Re-Start IAP Manually...\r\n");
			ManualIapRequested();
		}
		
		memset(params_dat, 0, LEN_BYTE_SZ64);
		FlashRead_SysParams(PARAM_ID_RSVD_U3, params_dat, 64);
		if (strlen((const char*)params_dat) != 0) {
			DEBUG("Re-Start IAP File = %s\n", params_dat);
			ManualIapSetFileName(params_dat);
		}
		
		ManualIapSetGotSize(GetIapBinSize());
#endif
	}
#else
	memset(params_dat, 0, LEN_BYTE_SZ64);
	FlashRead_SysParams(PARAM_ID_IAP_FLAG, params_dat, 64);
	if (0 == strncmp((const char*)params_dat, (const char*)IAP_REQ_RN, 4)) {		
		memset(params_dat, 0, LEN_BYTE_SZ64);
		FlashRead_SysParams(PARAM_ID_RSVD_U3, params_dat, 64);
		if (strlen((const char*)params_dat) != 0) {
            ftp_restart = 1;
			DEBUG("Re-Start IAP File = %s\n", params_dat);
			ManualIapSetFileName(params_dat);

			ManualIapSetGotSize(GetIapBinSize());
		}
	}
#endif

#if 1
    // Write params into flash when the first time boot
    FlashRead_SysParams(PARAM_ID_1ST_BOOT, params_dat, 64);
    if (strncmp((const char*)params_dat, (const char*)"1", 1) != 0) {
        FlashWrite_SysParams(PARAM_ID_SVR_IP, (u8*)g_svr_ip, strlen((const char*)g_svr_ip));
        FlashWrite_SysParams(PARAM_ID_SVR_PORT, (u8*)g_svr_port, strlen((const char*)g_svr_port));
        FlashWrite_SysParams(PARAM_ID_SVR_APN, (u8*)g_svr_apn, strlen((const char*)g_svr_apn));
        FlashWrite_SysParams(PARAM_ID_IAP_MD5, (u8*)"11112222333344445555666677778888", 32);
       
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
    FlashRead_SysParams(PARAM_ID_1ST_BOOT, params_dat, 64);
    if (strncmp((const char*)params_dat, (const char*)"1", 1) != 0) {
    	FlashWrite_SysParams(PARAM_ID_1ST_BOOT, (u8*)"1", 1);
    }
    gs_charge_sta = GPIOx_Input(BANKG, 3);
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
                } else {
					DEBUG("Connect new IP/PORT failed, reset...\n");
					asm("reset");
				}
            }
        }

        // -- TODO: cannot waste long time which will affect NFC-Read
        // -------------------- Re-Connect ------------------ //
        // --
        // retry initial or connection every 10s
        // if net-register failed or lost connection
        if (0 == (task_cnt++%200)) {
            if (false == IsIapRequested()) {
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
            if(flag_led == 0)
            {
                flag_led = 1;
                SetLedsStatus(MAIN_LED_B, LED_ON);
                g_led_times=3000;
                //g_ring_times=g_led_times/10;
            }
            if (0 == start_time_hbeat) {
                start_time_hbeat = GetTimeStamp();
            }

            if (start_time_hbeat != 0) {
                if (isDelayTimeout(start_time_hbeat,hbeat_gap*1000UL)) {
                    start_time_hbeat = GetTimeStamp();
#ifdef GPS_DEBUG
                    // DoQueryGPSFast();
                    // TcpReportGPS();
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
                    
                    if (1 == ftp_restart) {
                        ftp_restart = 0;
                        
                        DEBUG("Re-Start IAP Manually...\r\n");
                        ManualIapRequested();
                    }
                }
            }

            if (1 == gs_bg96_sta) {
				// SetAutoNetMode();

                trycnt++;
                if (QueryNetStatus()) {
                    gs_bg96_sta = 2;
                    trycnt = 0;
                }
            }
            
            if (trycnt >= 100) {
                trycnt = 0;
                SetAutoNetMode();
                PowerOnBG96Module();
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