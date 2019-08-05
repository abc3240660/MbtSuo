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

static u8 is_mode_nb = 0;
static unsigned long start_time_hbeat = 0;
static char one_svr_cmds[RX_RINGBUF_MAX_LEN] = {0};

void process_bg96(void)
{
    int i = 0;
    int skip_flag = 0;

    if (0 == IsNetRingBufferAvailable()) {
        return;
    }

    while(1) {
        if (0 == IsNetRingBufferAvailable()) {
            WaitUartNetRxIdle();

            // buffer is empty for 50MS
            if (0 == IsNetRingBufferAvailable()) {
                break;
            }
        }
        one_svr_cmds[i++] = ReadByteFromNetRingBuffer();

        if (3 == i) {
            if ((one_svr_cmds[0]=='S')&&(one_svr_cmds[1]=='T')&&(one_svr_cmds[2]=='A')) {
                skip_flag = 0;
            } else {
                // invalid MSGs or overwritten: "XTA" -> just skip till net "STA"
                skip_flag = 1;
            }
        }

        if (i > 3) {
            if ((one_svr_cmds[i-3]=='S')&&(one_svr_cmds[i-2]=='T')&&(one_svr_cmds[i-3]=='A')) {
                if (i != 3) {
                    i = 3;
                    one_svr_cmds[0] = 'S';
                    one_svr_cmds[1] = 'T';
                    one_svr_cmds[2] = 'A';
                }

                skip_flag = 0;
            }

            if ((one_svr_cmds[i-3]=='E')&&(one_svr_cmds[i-2]=='N')&&(one_svr_cmds[i-1]=='D')) {
                if ((one_svr_cmds[0]=='S')&&(one_svr_cmds[1]=='T')&&(one_svr_cmds[2]=='A')) {
                    // process the valid MSGs
                    printf("process MSGs: %s\n", one_svr_cmds);
                    one_svr_cmds[i-3] = '\0';// D
                    one_svr_cmds[i-2] = '\0';// N
                    one_svr_cmds[i-1] = '\0';// E
                    if (('#'==one_svr_cmds[3]) && ('$'==one_svr_cmds[i-4])) {
                        one_svr_cmds[i-4] = '\0';// $
                        ParseMobitMsg(one_svr_cmds+3);
                    } else {
                        printf("Error MSGs %c:%c\n", one_svr_cmds[3], one_svr_cmds[i-4]);
                    }
                } else {
                    // skip the invalid MSGs
                    i = 0;
                }

                skip_flag = 0;
                memset(one_svr_cmds, 0, RX_RINGBUF_MAX_LEN);

                continue;
            }
        }

        if (1 == skip_flag) {
            i = 0;
        }
    }
}

int main(void)
{
    u8 test_cnt = 0;
    u8 net_sta = 0;
    u16 hbeat_gap = DEFAULT_HBEAT_GAP;

    // unsigned long xxx = 0x1345678;
    unsigned long task_cnt = 0;

    System_Config();

    GPIOB_Init();
    Configure_Tick_10ms();
    Configure_Tick2_10ms();
    Uart1_Init();
    Uart2_Init();
    Uart3_Init();
    LB1938_Init();
    SPI2_Init();

    // BNO055 Testing
    // Configure_BNO055();
    // bno055_calibrate_demo();
    // bno055_demo();

    // printf("Hello PIC24F Uart1... 0x%.8lX\r\n", xxx);
    printf("Hello PIC24F Uart1...\r\n");

    CalcFirstMd5();

    InitRingBuffers();

    while(1)
    {
        hbeat_gap = GetHeartBeatGap();
        net_sta = GetNetStatus();

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
                ConnectToTcpServer();
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
        if (0 == (task_cnt%4)) {// every 0.2s
        }

        // --
        // ---------------------- TASK 2 -------------------- //
        // --
        if (0 == (task_cnt%11)) {  // every 0.5s
            process_bg96();
            ReadMobibNFCCard();
        }

        // --
        // ---------------------- TASK 3 -------------------- //
        if (0 == (task_cnt%21)) {  // every 1.0s
        // --
        }

        // --
        // ---------------------- TASK 4 -------------------- //
        // --
        if (0 == (task_cnt%31)) {  // every 1.5s
            ProcessTcpServerCommand();
        }

        // --
        // ---------------------- TASK 5 -------------------- //
        // --
        if (0 == (task_cnt%41)) {  // every 2.0s
            ReadMobibNFCCard();
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

        delay_ms(50);
    }
}