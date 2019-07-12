//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * A library for MOBIT Protocol
 * This file is about the MOBIT Protocol API
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 02/20/2019
******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "013_Protocol.h"
#include "014_md5.h"
#include "003_BG96.h"

const char* cmd_list[] = {
    // DEV Auto CMDs
    CMD_DEV_REGISTER,
    CMD_HEART_BEAT,
    CMD_DOOR_LOCKED,
    CMD_DOOR_UNLOCKED,
    CMD_CALYPSO_UPLOAD,
    CMD_INVALID_MOVE,
    CMD_REPORT_GPS,
    CMD_IAP_SUCCESS,
    CMD_CHARGE_STARTED,
    CMD_CHARGE_STOPED,
    CMD_FINISH_ADDNFC,
    CMD_FINISH_RST,
    CMD_EXIT_SLEEP,
    CMD_RISK_REPORT,

    // SVR Auto CMDs
    CMD_QUERY_PARAMS,
    CMD_RING_ALARM,
    CMD_UNLOCK_DOOR,
    CMD_FACTORY_RST,
    CMD_ENTER_SLEEP,
    CMD_QUERY_GPS,
    CMD_IAP_UPGRADE,
    CMD_CHANGE_APN,
    CMD_QUERY_NFC,
    CMD_DELETE_NFC,
    CMD_ADD_NFC,
    CMD_QUERY_ALARM,
    CMD_MODIFY_ALARM,
    CMD_QUERY_ICCID,
    NULL
};

u32 g_need_ack = 0;
u32 g_till_svr_ack = 0;

// DOOR Sta:
u8 g_door_state = 0;

// Door LOCK Sta:
// BIT7: 0-idle, 1-changed
// BIT0: 0-locked, 1-unlocked
u8 g_drlock_sta_chged = 0;

// Door Open/Close Sta:
// BIT7: 0-idle, 1-changed
// BIT0: 0-closed, 1-opened
u8 g_dropen_sta_chged = 0;

// HandBrake LOCK Sta:
// BIT7: 0-idle, 1-changed
// BIT0: 0-locked, 1-unlocked
u8 g_hbrake_sta_chged = 0;

u16 g_gps_trace_gap = 0;

u8 g_server_time[LEN_SYS_TIME+1] = "";

u8 g_msg_md5[LEN_DW_MD5+1] = "";
u8 g_card_ids[(LEN_CARD_ID+1)*10] = "";

u8 g_net_ip[LEN_NET_TCP+1] = "";
u8 g_net_port[LEN_NET_TCP+1] = "";
u8 g_net_apn[LEN_NET_TCP+1] = "";

u8 g_alarm_on[LEN_COMMON_USE+1] = "1";
u8 g_beep_on[LEN_COMMON_USE+1] = "1";
u8 g_alarm_level[LEN_COMMON_USE+1] = "80";

u8 g_iap_update = 0;
u8 g_iap_update_md5[LEN_DW_MD5+1] = "";
u8 g_iap_update_url[LEN_DW_URL+1] = "";

u8 g_hbeat_gap = 6;// default 6s

u8 g_CHANGE_APN_name[LEN_FILE_NAME+1];

extern u8 g_first_md5[LEN_COMMON_USE];
extern u8 g_imei_str[LEN_COMMON_USE];
extern u8 g_iccid_str[LEN_COMMON_USE];

char bg96_send_buf[LEN_MAX_SEND] = "";
char send_md5[LEN_DW_MD5] = "e10adc3949ba59abbe56e057f20f883e";

void calc_first_md5()
{
    u8 i = 0;
    MD5_CTX g_ota_md5_ctx;

    GAgent_MD5Init(&g_ota_md5_ctx);
    GAgent_MD5Update(&g_ota_md5_ctx, g_imei_str, strlen((char*)g_imei_str));
    GAgent_MD5Update(&g_ota_md5_ctx, g_iccid_str, strlen((char*)g_iccid_str));
    GAgent_MD5Final(&g_ota_md5_ctx, g_first_md5);

    printf("MD5 = ");
    for (i=0; i<16; i++) {
        printf("%.2X", g_first_md5[i]);
    }
    printf("\r\n");
}

u8 get_mobit_cmd_count()
{
    u8 cnt = 0;
    while(1) {
        if (NULL == cmd_list[cnt]) {
            break;
        }
        cnt++;
    }

    return cnt;
}

u8 is_supported_mobit_cmd(u8 pos, char* str)
{
    u8 i = 0;
    u8 cmd_count = get_mobit_cmd_count();

    printf("pos = %d\n", pos);
    for (i=pos; i<cmd_count; i++) {
        if ((0==pos) && (i>=QUERY_PARAMS)) {
            break;
        }

        if (0 == strncmp(str, cmd_list[i], strlen(cmd_list[i]))) {
            break;
        }
    }

    if (i != UNKNOWN_CMD) {
        printf("Recved CMD/ACK %s\n", str);
    }

    return i;
}

void parse_mobit_msg(char* msg)
{
    u8 k = 0;
    u16 i = 0;
    u8 is_run = 0;
    int index = 0;
    char delims[] = ",";
    char* split_str = NULL;

    enum CMD_TYPE cmd_type = UNKNOWN_CMD;

#ifdef DEBUG_USE
    //printf("Support %d CMDs\n", cmd_count);
#endif

    // MD5 Validation
    memcpy(g_msg_md5, msg+strlen(msg)-32, LEN_DW_MD5);
    g_msg_md5[LEN_DW_MD5] = '\0';
    printf("g_msg_md5 = %s\n", g_msg_md5);

    if (0) {// MD5 is invalid
        return;
    }

    split_str = strtok(msg, delims);
    while(split_str != NULL) {
#ifdef DEBUG_USE
        //printf("split_str = %s\n", split_str);
#endif
        // index = 3: SVR CMD
        // index = 4: SVR ACK
        if (1 == index) {
            if (0 == strncmp(split_str, "Run", strlen("Run")-1)) {
                is_run = 1;
            }

            printf("is_run = %d\n", is_run);
        } else if (2 == index) {
            cmd_type =  UNKNOWN_CMD;

            if (0 == is_run) {// SVR Re for Dev Auto CMDs
                cmd_type = (enum CMD_TYPE)is_supported_mobit_cmd(0, split_str);
            } else {// SVR Auto CMDs
                cmd_type = (enum CMD_TYPE)is_supported_mobit_cmd(CMD_QUERY_PARAMS, split_str);

                // need to ack for every SVR CMDs
                if (cmd_type != UNKNOWN_CMD) {
                    g_need_ack |= (1<<cmd_type);
                }
            }

            printf("cmd_type = %d\r\n", cmd_type);

            if (cmd_type != UNKNOWN_CMD) {
                break;
            }

            // No need Parse extra params
            if (cmd_type >= QUERY_PARAMS) {// SVR Auto CMDs
                // Need do some action ASAP

                if (QUERY_PARAMS == cmd_type) {
                    // Needn't do anything, just return params by tcp
                } else if (RING_ALARM == cmd_type) {
                    DoRingAlarmFast();
                } else if (UNLOCK_DOOR == cmd_type) {
                    DoUnLockTheLockerFast();
                } else if (FACTORY_RST == cmd_type) {
                    DoFactoryResetFast();// Delete every CardIDs
                } else if (ENTER_SLEEP == cmd_type) {
                    DoEnterSleepFast();
                } else if (QUERY_GPS == cmd_type) {
                    DoQueryGPSFast();
                } else if (QUERY_NFC == cmd_type) {
                    DoQueryNFCFast();
                } else if (ADD_NFC == cmd_type) {
                    DoAddNFCFast();
                } else if (QUERY_ALARM == cmd_type) {
                    // Needn't do anything, just return parms by tcp
                } else if (QUERY_ICCID == cmd_type) {
                    // Needn't do anything, just return parms by tcp
                }
            } else {// SVR ACKs for DEV's Auto CMDs
                // Dev thread will re-send DEV important Report till server received

                if (DOOR_LOCKED == cmd_type) {
                    // clear the loop send flag
                    g_till_svr_ack &= ~(1<<cmd_type);
                } else if (DOOR_UNLOCKED == cmd_type) {
                    g_till_svr_ack &= ~(1<<cmd_type);
                } else if (FINISH_ADDNFC == cmd_type) {
                    g_till_svr_ack &= ~(1<<cmd_type);
                } else if (RISK_REPORT == cmd_type) {
                    g_till_svr_ack &= ~(1<<cmd_type);
                } else {
                    // Needn't do anything
                    // Just Skip
                }
            }
        } else if (index > 2) {// Need to Parse extra params
            // Parse CMD or ACK with params
            if (DEV_REGISTER == cmd_type) {// only one SVR ACK with params
                if (3 == index) {
                    g_hbeat_gap = atoi(split_str);
                    printf("g_hbeat_gap = %d\n", g_hbeat_gap);
                    if (g_hbeat_gap < 5) {
                        g_hbeat_gap = 5;
                        printf("change g_hbeat_gap = %d\n", g_hbeat_gap);
                    }
                } else if (4 == index) {
                    strncpy((char*)g_server_time, split_str, LEN_SYS_TIME);
                    g_server_time[LEN_SYS_TIME] = '\0';
                    printf("g_swap_pw = %s\n", g_server_time);
                }
            // below is all SVR CMDs with params
            } else if (IAP_UPGRADE == cmd_type) {
                if (3 == index) {
                    g_iap_update = 1;
                    memset(g_iap_update_url, 0, LEN_DW_URL);
                    strncpy((char*)g_iap_update_url, split_str, LEN_DW_URL);
                    printf("g_iap_update_url = %s\n", g_iap_update_url);
                } else if (4 == index) {
                    memset(g_iap_update_md5, 0, LEN_DW_URL);
                    strncpy((char*)g_iap_update_md5, split_str, LEN_DW_URL);
                    printf("g_iap_update_md5 = %s\n", g_iap_update_md5);
                }
            } else if (CHANGE_APN == cmd_type) {
                if (3 == index) {
                    memset(g_net_ip, 0, LEN_NET_TCP);
                    strncpy((char*)g_net_ip, split_str, LEN_NET_TCP);
                    printf("g_net_ip = %s\n", g_net_ip);
                } else if (4 == index) {
                    memset(g_net_port, 0, LEN_NET_TCP);
                    strncpy((char*)g_net_port, split_str, LEN_NET_TCP);
                    printf("g_net_port = %s\n", g_net_port);
                } else if (5 == index) {
                    memset(g_net_apn, 0, LEN_NET_TCP);
                    strncpy((char*)g_net_apn, split_str, LEN_NET_TCP);
                    printf("g_net_apn = %s\n", g_net_apn);
                }
            } else if (DELETE_NFC == cmd_type) {
                if (3 == index) {
                    u8 tmp_card[32]="";
                    memset(g_card_ids, 0, LEN_CARD_ID);
                    strncpy((char*)g_card_ids, split_str, LEN_CARD_ID);
                    printf("g_card_ids = %s\n", g_card_ids);

                    for (i=0; i<strlen((char*)g_card_ids);i++) {
                        if ('|' == g_card_ids[i]) {
                            // do delete
                            printf("To deleteX %s\n", tmp_card);

                            k = 0;
                            memset(tmp_card, 0, 32);
                        } else {
                            tmp_card[k++] = g_card_ids[i];
                        }
                    }

                    // do delete
                    printf("To delete %s\n", tmp_card);
                }
            } else if (MODIFY_ALARM == cmd_type) {
                if (3 == index) {
                    memset(g_alarm_on, 0, LEN_NET_TCP);
                    strncpy((char*)g_alarm_on, split_str, LEN_NET_TCP);
                    printf("g_alarm_on = %s\n", g_alarm_on);
                } else if (4 == index) {
                    memset(g_beep_on, 0, LEN_NET_TCP);
                    strncpy((char*)g_beep_on, split_str, LEN_NET_TCP);
                    printf("g_beep_on = %s\n", g_beep_on);
                } else if (5 == index) {
                    memset(g_alarm_level, 0, LEN_NET_TCP);
                    strncpy((char*)g_alarm_level, split_str, LEN_NET_TCP);
                    printf("g_alarm_level = %s\n", g_alarm_level);
                }
            }
        }

        split_str = strtok(NULL, delims);

        index++;
    }
}

void ProcessTcpServerCommand(void)
{
    // during download mode, skip other operations
    // till download success or failed
    if (g_iap_update != 0) {
        sim7500e_http_iap();

        // try twice NG, skip this request
        if (g_iap_update != 0) {
            g_iap_update = 0;

            memset(g_iap_update_md5, 0, LEN_DW_MD5);
            memset(g_iap_update_url, 0, LEN_DW_URL);

            g_dw_size_total = 0;
            g_dw_recved_sum = 0;
        }

        return;
    }

    if (g_need_ack & (1<<QUERY_PARAMS)) {
        g_need_ack &= ~(1<<QUERY_PARAMS);
        TcpReQueryParams();
    } else if (g_need_ack & (1<<QUERY_GPS)) {
        g_need_ack &= ~(1<<QUERY_GPS);
        TcpReQueryGPS();
    } else if (g_need_ack & (1<<QUERY_NFC)) {
        g_need_ack &= ~(1<<QUERY_NFC);
        TcpReQueryNFCs();
    } else if (g_need_ack & (1<<QUERY_ALARM)) {
        g_need_ack &= ~(1<<QUERY_ALARM);
        TcpReQueryAlarm();
    } else if (g_need_ack & (1<<QUERY_ICCID)) {
        g_need_ack &= ~(1<<QUERY_ICCID);
        TcpReQueryIccid();
    } else {
        for (i=0 ;i<32; i++) {
            if (g_need_ack & (1<<i)) {
                if (QUERY_PARAMS == i) {
                    continue;
                } else if (QUERY_GPS == i) {
                    continue;
                } else if (QUERY_NFC == i) {
                    continue;
                } else if (QUERY_ALARM == i) {
                    continue;
                } else if (QUERY_ICCID == i) {
                    continue;
                } else {
                    TcpReNormalAck();
                    g_need_ack &= ~(1<<i);
                }
            }
        }
    }

    // TODO: add some time delay between twice tcp-send
    // always send till received ACK from server
    if (g_till_svr_ack & (1<<DOOR_LOCKED)) {
        TcpLockerLocked();
    } else if (g_till_svr_ack & (1<<DOOR_UNLOCKED)) {
        TcpLockerUnlocked();
    } else if (g_till_svr_ack & (1<<FINISH_ADDNFC)) {
        TcpFinishAddNFCCard();
    } else if (g_till_svr_ack & (1<<RISK_REPORT)) {
        TcpRiskAlarm();
    }
}

// ============================================ DEV TCP Host ============================================ //
bool TcpHeartBeat(void)
{
    // const char send_data[] = "#MOBIT,868446032285351,HB,4.0,1,20,e10adc3949ba59abbe56e057f20f883e$";

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s,%s,%s$", g_imei_str, CMD_HEART_BEAT, "4.0", "1", "20", send_md5);

    return BG96TcpSend();
}

bool TcpDeviceRegister(void)
{
    // const char send_data[] = "#MOBIT,868446032285351,REG,898602B4151830031698,1.0.0,1.0.0,4.0,1561093302758,2,e10adc3949ba59abbe56e057f20f883e$";

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s,%s,%s,%s,%s,%s$", g_imei_str, CMD_DEV_REGISTER, g_iccid_str, "1.0.0", "1.00", "4.0", "20190709180030", "2", send_md5);

    return BG96TcpSend();
}

bool TcpFinishFactoryReset(void)
{
    // DEV: #MOBIT,868446032285351,RSOK,1,111111111,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_FINISH_RST, "1", "111111111", send_md5);

    return BG96TcpSend();
}

// after lock the locker by hand
bool TcpExitCarriageSleep(void)
{
    // #MOBIT,868446032285351,WU,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_EXIT_SLEEP, send_md5);

    return BG96TcpSend();
}

bool TcpReportGPS(void)
{
    // #MOBIT,868446032285351,GEO,51.106922|3.702681|20|180,0,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_REPORT_GPS, "51.106922|3.702681|20|180", "0", send_md5);

    return BG96TcpSend();
}

bool TcpInvalidMovingAlarm(void)
{
    // #MOBIT,868446032285351,AL,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_INVALID_MOVE, send_md5);

    return BG96TcpSend();
}

bool TcpRiskAlarm(void)
{
    // #MOBIT,868446032285351,FA,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_RISK_REPORT, send_md5);

    return BG96TcpSend();
}

bool TcpFinishIAP(void)
{
    // #MOBIT,868446032285351,UP,1,1.0.0,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_IAP_SUCCESS, "1", "1.0.0", send_md5);

    return BG96TcpSend();
}

bool TcpFinishAddNFCCard(void)
{
    // #MOBIT,868446032285351,ADDCR,a|b|c|d|e|f,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s$", g_imei_str, CMD_FINISH_ADDNFC, "a|b|c|d|e|f", send_md5);

    return BG96TcpSend();
}

bool TcpReadedOneCard(void)
{
    // #MOBIT,868446032285351,RC,1234567,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s$", g_imei_str, CMD_CALYPSO_UPLOAD, "1234567", send_md5);

    return BG96TcpSend();
}

bool TcpLockerLocked(void)
{
    // #MOBIT,868446032285351,LC,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_DOOR_LOCKED, send_md5);

    return BG96TcpSend();
}

bool TcpLockerUnlocked(void)
{
    // #MOBIT,868446032285351,OL,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_DOOR_LOCKED, send_md5);

    return BG96TcpSend();
}

bool TcpChargeStarted(void)
{
    // #MOBIT,868446032285351,CG,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_CHARGE_STARTED, send_md5);

    return BG96TcpSend();
}

bool TcpChargeStoped(void)
{
    // #MOBIT,868446032285351,CGE,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_CHARGE_STARTED, send_md5);

    return BG96TcpSend();
}

// ============================================ DEV TCP Slave ============================================ //

// #MOBIT,868446032285351,UG,1,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,RS,1,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,SLEEP,1,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,OL,0,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,UP,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,ADDC,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,ICCID,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,1,1,80,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,MALC,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,DD,Re,e10adc3949ba59abbe56e057f20f883e$
bool TcpReNormalAck(u8* cmd_str, u8* sta)
{
    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,Re,%s,%s$", g_imei_str, cmd_str, sta, send_md5);

    return BG96TcpSend();
}

bool TcpReQueryParams(void)
{
    // #MOBIT,868446032285351,QG,lock.mobit.eu,1000,mobit.apn,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_PARAMS, g_svr_ip, g_svr_port, g_svr_apn, send_md5);

    return BG96TcpSend();
}

bool TcpReQueryGPS(void)
{
    // #MOBIT,868446032285351,GGEO,51.106922|3.702681|20|180,0,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s|%s|%s|%s,%s,Re,%s$", g_imei_str, CMD_QUERY_GPS, "51.106922", "3.702681", "20", "180", "0", send_md5);

    return BG96TcpSend();
}

bool TcpReQueryNFCs(void)
{
    // #MOBIT,868446032285351,QCL,a|b|c|d|e|f|g|h,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_NFC, "a|b|c|d|e|f|g|h", send_md5);

    return BG96TcpSend();
}

bool TcpReQueryAlarm(void)
{
    // #MOBIT,868446032285351,ALC,1,1,80,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_ALARM, g_alarm_on, g_beep_on, g_alarm_level, send_md5);

    return BG96TcpSend();
}

bool TcpReDeleteNFCs(void)
{
    // #MOBIT,868446032285351,RMC,a|b|c|d|e,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(bg96_send_buf, 0, LEN_MAX_SEND);
    sprintf(bg96_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_NFC, "a|b|c|d|e|f|g|h", send_md5);

    return BG96TcpSend();
}

// ============================================ DEV Action ============================================ //
bool DoUnLockTheLockerFast(void)
{
    printf("DoUnLockTheLockerFast...\n");

    return true;
}

bool DoRingAlarmFast(void)
{
    printf("DoRingAlarmFast...\n");

    return true;
}

bool DoFactoryResetFast(void)
{
    printf("DoFactoryResetFast...\n");

    return true;
}

bool DoEnterSleepFast(void)
{
    printf("DoEnterSleepFast...\n");

    return true;
}

bool DoEnterSleepFast(void)
{
    printf("DoEnterSleepFast...\n");

    return true;
}

bool DoQueryGPSFast(void)
{
    printf("DoEnterSleepFast...\n");

    return true;
}

bool DoQueryNFCFast(void)
{
    printf("DoEnterSleepFast...\n");

    return true;
}

bool DoAddNFCFast(void)
{
    printf("DoEnterSleepFast...\n");

    return true;
}
