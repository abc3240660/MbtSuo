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

#include "001_Tick_10ms.h"
#include "003_BG96.h"
#include "004_LB1938.h"
#include "012_CLRC663_NFC.h"
#include "013_Protocol.h"
#include "014_Md5.h"

static const char* cmd_list[] = {
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

static u32 gs_need_ack = 0;
static u32 gs_till_svr_ack = 0;

static u8 gs_swap_passwd[LEN_SYS_TIME+1] = "";

static u8 gs_msg_md5[LEN_DW_MD5+1] = "";

static u8 gs_alarm_on[LEN_COMMON_USE+1] = "1";
static u8 gs_beep_on[LEN_COMMON_USE+1] = "1";
static u8 gs_alarm_level[LEN_COMMON_USE+1] = "80";

static u8 gs_iap_update = 0;
static u8 gs_iap_update_md5[LEN_DW_MD5+1] = "";
static u8 gs_iap_update_url[LEN_DW_URL+1] = "";

static u16 gs_hbeat_gap = DEFAULT_HBEAT_GAP;

static u8 gs_during_bind = 0;
static unsigned long start_time_risk = 0;
static unsigned long start_time_locked = 0;
static unsigned long start_time_unlocked = 0;
static unsigned long start_time_finish_addc = 0;

static u8 gs_first_md5[LEN_COMMON_USE] = "";

static char gs_send_md5[LEN_DW_MD5] = "e10adc3949ba59abbe56e057f20f883e";

static unsigned long gs_dw_size_total = 0;
static unsigned long gs_dw_recved_sum = 0;

static char tcp_send_buf[LEN_MAX_SEND] = "";

static u8 tmp_buf_big[LEN_BYTE_SZ512] = "";

// --
// ---------------------- global variables -------------------- //
// --
extern u8 g_imei_str[LEN_COMMON_USE];
extern u8 g_iccid_str[LEN_COMMON_USE];

extern u8 g_svr_ip[LEN_NET_TCP];
extern u8 g_svr_port[LEN_NET_TCP];
extern u8 g_svr_apn[LEN_NET_TCP];

void CalcFirstMd5(void)
{
    u8 i = 0;
    MD5_CTX g_ota_md5_ctx;

    GAgent_MD5Init(&g_ota_md5_ctx);
    GAgent_MD5Update(&g_ota_md5_ctx, g_imei_str, strlen((char*)g_imei_str));
    GAgent_MD5Update(&g_ota_md5_ctx, g_iccid_str, strlen((char*)g_iccid_str));
    GAgent_MD5Final(&g_ota_md5_ctx, gs_first_md5);

    printf("MD5 = ");
    for (i=0; i<16; i++) {
        printf("%.2X", gs_first_md5[i]);
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

void ParseMobitMsg(char* msg)
{
    u8 k = 0;
    u16 i = 0;
    u8 is_run = 0;
    int index = 0;
    char delims[] = ",";
    char* split_str = NULL;
    unsigned long temp = 1;

    enum CMD_TYPE cmd_type = UNKNOWN_CMD;

#ifdef DEBUG_USE
    //printf("Support %d CMDs\n", cmd_count);
#endif

    // MD5 Validation
    // memcpy(gs_msg_md5, msg+strlen(msg)-32, LEN_DW_MD5);
    // gs_msg_md5[LEN_DW_MD5] = '\0';
    // printf("gs_msg_md5 = %s\n", gs_msg_md5);

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
                cmd_type = (enum CMD_TYPE)is_supported_mobit_cmd(QUERY_PARAMS, split_str);

                // need to ack for every SVR CMDs
                if (cmd_type != UNKNOWN_CMD) {
                    gs_need_ack |= (temp<<cmd_type);
                }
            }

            printf("cmd_type = %d\r\n", cmd_type);

            if (UNKNOWN_CMD == cmd_type) {
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
                    gs_till_svr_ack &= ~(temp<<cmd_type);
                } else if (DOOR_UNLOCKED == cmd_type) {
                    gs_till_svr_ack &= ~(temp<<cmd_type);
                } else if (FINISH_ADDNFC == cmd_type) {
                    gs_till_svr_ack &= ~(temp<<cmd_type);
                } else if (RISK_REPORT == cmd_type) {
                    gs_till_svr_ack &= ~(temp<<cmd_type);
                } else {
                    // Needn't do anything
                    // Just Skip
                }
            }
        } else if (index > 2) {// Need to Parse extra params
            // Parse CMD or ACK with params
            if (DEV_REGISTER == cmd_type) {// only one SVR ACK with params
                if (3 == index) {
                    gs_hbeat_gap = atoi(split_str);
                    printf("gs_hbeat_gap = %d\n", gs_hbeat_gap);
                    if (gs_hbeat_gap < 5) {
                        gs_hbeat_gap = 5;
                        printf("change gs_hbeat_gap = %d\n", gs_hbeat_gap);
                    }
                } else if (4 == index) {
                    strncpy((char*)gs_swap_passwd, split_str, LEN_SYS_TIME);
                    gs_swap_passwd[LEN_SYS_TIME] = '\0';
                    printf("gs_swap_passwd = %s\n", gs_swap_passwd);
                }
            // below is all SVR CMDs with params
            } else if (IAP_UPGRADE == cmd_type) {
                if (3 == index) {
                    gs_iap_update = 1;
                    memset(gs_iap_update_url, 0, LEN_DW_URL);
                    strncpy((char*)gs_iap_update_url, split_str, LEN_DW_URL);
                    printf("gs_iap_update_url = %s\n", gs_iap_update_url);
                } else if (4 == index) {
                    memset(gs_iap_update_md5, 0, LEN_DW_URL);
                    strncpy((char*)gs_iap_update_md5, split_str, LEN_DW_URL);
                    printf("gs_iap_update_md5 = %s\n", gs_iap_update_md5);
                }
            } else if (CHANGE_APN == cmd_type) {
                if (3 == index) {
                    memset(g_svr_ip, 0, LEN_NET_TCP);
                    strncpy((char*)g_svr_ip, split_str, LEN_NET_TCP);
                    printf("g_svr_ip = %s\n", g_svr_ip);
                } else if (4 == index) {
                    memset(g_svr_port, 0, LEN_NET_TCP);
                    strncpy((char*)g_svr_port, split_str, LEN_NET_TCP);
                    printf("g_svr_port = %s\n", g_svr_port);
                } else if (5 == index) {
                    memset(g_svr_apn, 0, LEN_NET_TCP);
                    strncpy((char*)g_svr_apn, split_str, LEN_NET_TCP);
                    printf("g_svr_apn = %s\n", g_svr_apn);
                }
            } else if (DELETE_NFC == cmd_type) {
                if (3 == index) {
                    u8 tmp_card[LEN_BYTE_SZ32] = "";
                    memset(tmp_buf_big, 0, LEN_BYTE_SZ512);
                    strncpy((char*)tmp_buf_big, split_str, LEN_BYTE_SZ512);
                    printf("gs_delete_cards = %s\n", tmp_buf_big);

                    for (i=0; i<strlen((char*)tmp_buf_big);i++) {
                        if ('|' == tmp_buf_big[i]) {
                            DeleteMobibCard(tmp_card, NULL);
                            printf("To deleteX %s\n", tmp_card);

                            k = 0;
                            memset(tmp_card, 0, LEN_BYTE_SZ512);
                        } else {
                            tmp_card[k++] = tmp_buf_big[i];
                        }
                    }

                    DeleteMobibCard(tmp_card, NULL);
                    printf("To delete %s\n", tmp_card);
                }
            } else if (MODIFY_ALARM == cmd_type) {
                if (3 == index) {
                    memset(gs_alarm_on, 0, LEN_NET_TCP);
                    strncpy((char*)gs_alarm_on, split_str, LEN_NET_TCP);
                    printf("gs_alarm_on = %s\n", gs_alarm_on);
                } else if (4 == index) {
                    memset(gs_beep_on, 0, LEN_NET_TCP);
                    strncpy((char*)gs_beep_on, split_str, LEN_NET_TCP);
                    printf("gs_beep_on = %s\n", gs_beep_on);
                } else if (5 == index) {
                    memset(gs_alarm_level, 0, LEN_NET_TCP);
                    strncpy((char*)gs_alarm_level, split_str, LEN_NET_TCP);
                    printf("gs_alarm_level = %s\n", gs_alarm_level);
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
    if (gs_iap_update != 0) {
        DoHttpIAP();

        // try twice NG, skip this request
        if (gs_iap_update != 0) {
            gs_iap_update = 0;

            memset(gs_iap_update_md5, 0, LEN_DW_MD5);
            memset(gs_iap_update_url, 0, LEN_DW_URL);

            gs_dw_size_total = 0;
            gs_dw_recved_sum = 0;
        }

        return;
    }

    unsigned long temp = 1;

    if (gs_need_ack & (temp<<QUERY_PARAMS)) {
        gs_need_ack &= ~(temp<<QUERY_PARAMS);
        TcpReQueryParams();
    } else if (gs_need_ack & (temp<<QUERY_GPS)) {
        gs_need_ack &= ~(temp<<QUERY_GPS);
        TcpReQueryGPS();
    } else if (gs_need_ack & (temp<<QUERY_NFC)) {
        gs_need_ack &= ~(temp<<QUERY_NFC);
        TcpReQueryNFCs();
    } else if (gs_need_ack & (temp<<QUERY_ALARM)) {
        gs_need_ack &= ~(temp<<QUERY_ALARM);
        TcpReQueryAlarm();
    } else if (gs_need_ack & (temp<<QUERY_ICCID)) {
        gs_need_ack &= ~(temp<<QUERY_ICCID);
        TcpReQueryIccid();
    } else {
        u8 i = 0;
        for (i=0 ;i<32; i++) {
            if (gs_need_ack & (temp<<i)) {
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
                    if ((IAP_UPGRADE==i) || (ADD_NFC==i) ||
                        (MODIFY_ALARM==i) || (RING_ALARM==i)) {
                        TcpReNormalAck((u8*)(cmd_list[i]), NULL);
                    } else if ((IAP_UPGRADE==i) || (ADD_NFC==i)) {
                        // TODO: must check if unlock success
                        // read the value of PIN connected to the locker position
                        if (1) {// TODO
                            TcpReNormalAck((u8*)(cmd_list[i]), (u8*)("1"));
                        } else {
                            TcpReNormalAck((u8*)(cmd_list[i]), (u8*)("0"));
                        }
                    } else if (DELETE_NFC == i) {
                        TcpReDeleteNFCs();
                    } else {
                        TcpReNormalAck((u8*)(cmd_list[i]), (u8*)("1"));
                    }

                    if (FACTORY_RST == i) {
                        TcpFinishFactoryReset();
                    }

                    gs_need_ack &= ~(temp<<i);
                }
            }
        }
    }

    // always send till received ACK from server
    // period: 10s
    if (gs_till_svr_ack & (temp<<DOOR_LOCKED)) {
        if (start_time_locked != 0) {
            if (isDelayTimeout(start_time_locked,10*1000UL)) {
                TcpLockerLocked();
            }
        }
    } else if (gs_till_svr_ack & (temp<<DOOR_UNLOCKED)) {
        if (start_time_unlocked != 0) {
            if (isDelayTimeout(start_time_unlocked,10*1000UL)) {
                TcpLockerUnlocked();
            }
        }
    } else if (gs_till_svr_ack & (temp<<FINISH_ADDNFC)) {
        if (start_time_finish_addc != 0) {
            if (isDelayTimeout(start_time_finish_addc,10*1000UL)) {
                TcpFinishAddNFCCard();
            }
        }
    } else if (gs_till_svr_ack & (temp<<RISK_REPORT)) {
        if (start_time_risk != 0) {
            if (isDelayTimeout(start_time_risk,10*1000UL)) {
                TcpRiskAlarm();
            }
        }
    }
}

// ============================================ DEV TCP Host ============================================ //
bool TcpHeartBeat(void)
{
    // const char send_data[] = "#MOBIT,868446032285351,HB,4.0,1,20,e10adc3949ba59abbe56e057f20f883e$";

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,%s$", g_imei_str, CMD_HEART_BEAT, "4.0", "1", "20", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpDeviceRegister(void)
{
    // const char send_data[] = "#MOBIT,868446032285351,REG,898602B4151830031698,1.0.0,1.0.0,4.0,1561093302758,2,e10adc3949ba59abbe56e057f20f883e$";

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,%s,%s,%s,%s$", g_imei_str, CMD_DEV_REGISTER, g_iccid_str, "1.0.0", "1.00", "4.0", "20190709180030", "2", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpFinishFactoryReset(void)
{
    // DEV: #MOBIT,868446032285351,RSOK,1,111111111,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_FINISH_RST, "1", "111111111", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

// after lock the locker by hand
bool TcpExitCarriageSleep(void)
{
    // #MOBIT,868446032285351,WU,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_EXIT_SLEEP, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReportGPS(void)
{
    // #MOBIT,868446032285351,GEO,51.106922|3.702681|20|180,0,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_REPORT_GPS, "51.106922|3.702681|20|180", "0", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpInvalidMovingAlarm(void)
{
    // #MOBIT,868446032285351,AL,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_INVALID_MOVE, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpRiskAlarm(void)
{
    // #MOBIT,868446032285351,FA,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_RISK_REPORT, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpFinishIAP(void)
{
    // #MOBIT,868446032285351,UP,1,1.0.0,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_IAP_SUCCESS, "1", "1.0.0", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpFinishAddNFCCard(void)
{
    // #MOBIT,868446032285351,ADDCR,a|b|c|d|e|f,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s$", g_imei_str, CMD_FINISH_ADDNFC, "a|b|c|d|e|f", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReadedOneCard(u8* card_id, u8* serial_nr)
{
    // #MOBIT,868446032285351,RC,1234567,e10adc3949ba59abbe56e057f20f883e$

    if ((NULL==card_id) || (NULL==serial_nr)) {
        return false;
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s$", g_imei_str, CMD_CALYPSO_UPLOAD, card_id, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpLockerLocked(void)
{
    // #MOBIT,868446032285351,LC,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_DOOR_LOCKED, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpLockerUnlocked(void)
{
    // #MOBIT,868446032285351,OL,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_DOOR_LOCKED, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpChargeStarted(void)
{
    // #MOBIT,868446032285351,CG,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_CHARGE_STARTED, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpChargeStoped(void)
{
    // #MOBIT,868446032285351,CGE,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_CHARGE_STARTED, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

// ============================================ DEV TCP Slave ============================================ //

// #MOBIT,868446032285351,UG,1,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,RS,1,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,SLEEP,1,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,OL,0,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,UP,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,ADDC,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,MALC,Re,e10adc3949ba59abbe56e057f20f883e$
// #MOBIT,868446032285351,DD,Re,e10adc3949ba59abbe56e057f20f883e$
bool TcpReNormalAck(u8* cmd_str, u8* sta)
{
    if (NULL == cmd_str) {
        return false;
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    if (NULL == sta) {
        sprintf(tcp_send_buf, "#MOBIT,%s,%s,Re,%s$", g_imei_str, cmd_str, gs_send_md5);
    } else {
        sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, cmd_str, sta, gs_send_md5);
    }

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryParams(void)
{
    // #MOBIT,868446032285351,QG,lock.mobit.eu,1000,mobit.apn,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_PARAMS, g_svr_ip, g_svr_port, g_svr_apn, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

// TODO 07/30
bool TcpReQueryGPS(void)
{
    // #MOBIT,868446032285351,GGEO,51.106922|3.702681|20|180,0,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s|%s|%s|%s,%s,Re,%s$", g_imei_str, CMD_QUERY_GPS, "51.106922", "3.702681", "20", "180", "0", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryNFCs(void)
{
    // #MOBIT,868446032285351,QCL,a|b|c|d|e|f|g|h,Re,e10adc3949ba59abbe56e057f20f883e$

    // Max Size: 19*16 = 304B
    memset(tmp_buf_big, 0, LEN_BYTE_SZ512);
    QueryMobibCard(tmp_buf_big);

    if (0 == strlen((const char*)tmp_buf_big)) {
        tmp_buf_big[0] = 'F';
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_NFC, tmp_buf_big, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryAlarm(void)
{
    // #MOBIT,868446032285351,ALC,1,1,80,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_ALARM, gs_alarm_on, gs_beep_on, gs_alarm_level, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryIccid(void)
{
    // #MOBIT,868446032285351,ICCID,898602B4151830031698,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_ICCID, g_iccid_str, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReDeleteNFCs(void)
{
    // #MOBIT,868446032285351,RMC,a|b|c|d|e,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_DELETE_NFC, "1", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

// ============================================ DEV Action ============================================ //
bool DoUnLockTheLockerFast(void)
{
    LB1938_MotorCtrl(MOTOR_LEFT, 10);

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

    DeleteAllMobibCard();

    return true;
}

bool DoEnterSleepFast(void)
{
    printf("DoEnterSleepFast...\n");

    // TODO: must check if unlock success
    // read the value of PIN connected to the locker position

    return true;
}

bool DoQueryGPSFast(void)
{
    printf("DoQueryGPSFast...\n");

    return true;
}

bool DoQueryNFCFast(void)
{
    printf("DoQueryNFCFast...\n");

    return true;
}

bool DoAddNFCFast(void)
{
    printf("DoAddNFCFast...\n");

    gs_during_bind = 1;

    return true;
}

bool DoHttpIAP(void)
{
    printf("DoHttpIAP...\n");

    return true;
}

u8 IsDuringBind(void)
{
    return (gs_during_bind ? 1 : 0);
}

u16 GetHeartBeatGap(void)
{
    return gs_hbeat_gap;
}

void ReportFinishAddNFC(void)
{
    unsigned long temp = 1;

    if (0 == start_time_finish_addc) {
        start_time_finish_addc = GetTimeStamp();
    }

    gs_during_bind = 0;
    gs_till_svr_ack |= (temp<<FINISH_ADDNFC);
}

void ReportLockerUnlocked(void)
{
    unsigned long temp = 1;

    if (0 == start_time_unlocked) {
        start_time_unlocked = GetTimeStamp();
    }

    gs_till_svr_ack |= (temp<<DOOR_UNLOCKED);
}
