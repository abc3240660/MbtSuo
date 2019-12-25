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
#include "015_Common.h"

#include "001_Tick_10ms.h"
#include "003_BG96.h"
#include "004_LB1938.h"
#include "006_GPIO.h"
#include "007_Uart.h"
#include "012_CLRC663_NFC.h"
#include "019_ADC0.h"
#include "013_Protocol.h"
#include "014_Md5.h"
#include "016_FlashOta.h"

// ring times = X / 2
extern u8 g_ring_times;// 6-DD 200-Alarm
static u8 special_test = 0;

static u8 gs_iap_buf[LEN_BYTE_SZ1024] = "";
extern u8 g_ring_times;
extern u32 g_led_times;
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

//static u8 gs_msg_md5[LEN_MD5_HEXSTR+1] = "";

static u8 gs_alarm_on[LEN_COMMON_USE+1] = "1";
static u8 gs_beep_on[LEN_COMMON_USE+1] = "1";
static u8 gs_alarm_level[LEN_COMMON_USE+1] = "100";

static u8 gs_iap_waiting = 0;
static u8 gs_iap_md5[LEN_MD5_HEXSTR+1] = "";
static u8 gs_iap_file[LEN_DW_URL+1] = "Mbtsuo_1218_1012.bin";

static u16 gs_hbeat_gap = DEFAULT_HBEAT_GAP;

static u8 gs_during_bind = 0;
static unsigned long start_time_reg = 0;
static unsigned long start_time_risk = 0;
static unsigned long start_time_locked = 0;
static unsigned long start_time_unlocked = 0;
static unsigned long start_time_finish_addc = 0;

static u8 gs_communit_key[LEN_MD5_HEXSTR+1] = "";

//static char gs_send_md5[LEN_MD5_HEXSTR+1] = "e10adc3949ba59abbe56e057f20f883e";

static unsigned long gs_dw_size_total = 0;
static unsigned long gs_dw_recved_sum = 0;

static char tcp_send_buf[LEN_MAX_SEND+1] = "";

static u8 gs_tmp_buf_big[LEN_BYTE_SZ512+1] = "";

static char gs_gnss_part[LEN_BYTE_SZ128+1] = "";

static u32 gs_ftp_offset = 0;

//static u8 gs_ftp_ip[LEN_NET_TCP]  = "122.4.233.119";
//static u8 gs_ftp_port[LEN_NET_TCP] = "10218";

// "101.132.150.94" "21"
static u8 gs_ftp_ip[LEN_NET_TCP+1]  = "192.168.1.107";
static u8 gs_ftp_port[LEN_NET_TCP+1] = "21";

// TODO: Need to reset into 0 if IAP failed for ReIAP Request
static u32 gs_ftp_sum_got = 0;
static u8 gs_is_erased = 0;

static u8 gs_change_apn = 0;

static u8 gs_during_ship = 0;

// static MD5_CTX g_ftp_md5_ctx;
// static u8 gs_ftp_res_md5[LEN_COMMON_USE+1] = "";

static u8 gs_addordel_cards[LEN_BYTE_SZ512+1] = {0};

static char gs_one_tcpcmds[RX_RINGBUF_MAX_LEN+1] = {0};

// 0-opened 1-closed
static u8 gs_lock_sta = 0;

// --
// ---------------------- global variables -------------------- //
// --
extern u8 g_imei_str[LEN_COMMON_USE+1];
extern u8 g_iccid_str[LEN_COMMON_USE+1];
extern u8 g_rssi_str[LEN_COMMON_USE+1];

extern u8 g_devtime_str[LEN_COMMON_USE+1];
extern u8 g_devzone_str[LEN_COMMON_USE+1];
extern u8 g_net_mode[LEN_COMMON_USE+1];

extern u8 g_svr_ip[LEN_NET_TCP+1];
extern u8 g_svr_port[LEN_NET_TCP+1];
extern u8 g_svr_apn[LEN_NET_TCP+1];

#if 1// FTP Upgrade
extern u32 g_ftp_dat_cnt;
extern u8 g_ftp_buf[1536];
void CleaFtpBuffer(void);
void PrintFtpBuffer(void);
#endif

#if 0
void CalcRegisterKey(void)
{
    u8 i = 0;
    MD5_CTX reg_md5_ctx;
    u8 reg_md5_hex[LEN_MD5_HEX+1] = "";

    GAgent_MD5Init(&reg_md5_ctx);
    GAgent_MD5Update(&reg_md5_ctx, g_imei_str, strlen((char*)g_imei_str));
    GAgent_MD5Update(&reg_md5_ctx, g_iccid_str, strlen((char*)g_iccid_str));
    GAgent_MD5Final(&reg_md5_ctx, reg_md5_hex);

    for (i=0; i<LEN_MD5_HEX; i++) {
        sprintf((char*)gs_communit_key+2*i, "%.2X", reg_md5_hex[i]);
    }

    DEBUG("MD5 = %s\n", gs_communit_key);
}

static void EncodeTcpPacket(u8* in_dat)
{
    u8 i = 0;
    MD5_CTX encode_md5_ctx;
    u8 encoded_md5_hex[LEN_MD5_HEX+1] = "";

    if (NULL == in_dat) {
        return;
    }

    GAgent_MD5Init(&encode_md5_ctx);
    GAgent_MD5Update(&encode_md5_ctx, in_dat, strlen((char*)in_dat));
    GAgent_MD5Final(&encode_md5_ctx, encoded_md5_hex);

    for (i=0; i<LEN_MD5_HEX; i++) {
        sprintf(gs_send_md5+2*i, "%.2X", encoded_md5_hex[i]);
    }

    DEBUG("MD5 = %s\n", gs_send_md5);
}
#endif

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

    DEBUG("pos = %d\n", pos);
    for (i=pos; i<cmd_count; i++) {
        if ((0==pos) && (i>=QUERY_PARAMS)) {
            break;
        }

        if (0 == strncmp(str, cmd_list[i], strlen(cmd_list[i]))) {
            break;
        }
    }

    if (i != UNKNOWN_CMD) {
        DEBUG("Recved CMD/ACK %s\n", str);
    }

    return i;
}

#if 0
static bool ValidateTcpMsg(char* msg)
{
    u16 i = 0;
    u16 end_pos = 0;
    MD5_CTX msg_md5_ctx;
    u8 msg_md5_hex[LEN_MD5_HEX+1] = "";
    u8 calc_msg_md5[LEN_MD5_HEXSTR+1] = "";

    if (NULL == msg) {
        return false;
    }

    // MD5 Validation
    memset(gs_msg_md5, 0, LEN_MD5_HEXSTR);
    memcpy(gs_msg_md5, msg+strlen(msg)-LEN_MD5_HEXSTR, LEN_MD5_HEXSTR);
    DEBUG("gs_msg_md5 = %s\n", gs_msg_md5);

    for (i=0; i<strlen(msg); i++) {
        if (',' == msg[i]) {
            end_pos = i;
        }
    }

    if (0 == end_pos) {
        return false;
    }

    msg[end_pos] = '$';

    GAgent_MD5Init(&msg_md5_ctx);
    GAgent_MD5Update(&msg_md5_ctx, (u8*)msg, end_pos+1);
    GAgent_MD5Final(&msg_md5_ctx, msg_md5_hex);

    for (i=0; i<LEN_MD5_HEX; i++) {
        sprintf((char*)calc_msg_md5+2*i, "%.2X", msg_md5_hex[i]);
    }

    DEBUG("MD5 = %s\n", calc_msg_md5);

    msg[end_pos] = ',';

    if (0 == strncmp((const char*)gs_msg_md5, (const char*)calc_msg_md5, LEN_MD5_HEXSTR)) {
        return true;
    } else {
        return false;
    }
}
#endif

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

    if (NULL == msg) {
        return;
    }

    if (strlen((const char*)msg) < 6) {
        return;
    }

#ifdef DEBUG_USE
    //DEBUG("Support %d CMDs\n", cmd_count);
#endif

//    if (false == ValidateTcpMsg(msg)) {// MD5 is invalid
//        return;
//    }

    split_str = strtok(msg, delims);
    while(split_str != NULL) {
#ifdef DEBUG_USE
        //DEBUG("split_str = %s\n", split_str);
#endif
        // index = 3: SVR CMD
        // index = 4: SVR ACK
        if (1 == index) {
            if (0 == strncmp(split_str, "Run", strlen("Run")-1)) {
                is_run = 1;
            }

            DEBUG("is_run = %d\n", is_run);
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

            DEBUG("cmd_type = %d\r\n", cmd_type);

            if (UNKNOWN_CMD == cmd_type) {
                break;
            }

            // No need Parse extra params
            if (cmd_type >= QUERY_PARAMS) {// SVR Auto CMDs
                // Need do some action ASAP

                if (QUERY_PARAMS == cmd_type) {
                    // Needn't do anything, just return params by tcp
                } else if (RING_ALARM == cmd_type) {
                    gs_need_ack &= ~(temp<<cmd_type);// no need to do TCPACK to server
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
                    gs_till_svr_ack &= ~(temp<<cmd_type);

                    gs_hbeat_gap = atoi(split_str);
                    DEBUG("gs_hbeat_gap = %d\n", gs_hbeat_gap);
                    if (gs_hbeat_gap < 5) {
                        gs_hbeat_gap = 5;
                        DEBUG("change gs_hbeat_gap = %d\n", gs_hbeat_gap);
                    }
                } else if (4 == index) {
                    memset(gs_communit_key, 0, LEN_MD5_HEXSTR);
                    strncpy((char*)gs_communit_key, split_str, LEN_MD5_HEXSTR);
                    DEBUG("gs_communit_key = %s\n", gs_communit_key);
                }
            // below is all SVR CMDs with params
            } else if (IAP_UPGRADE == cmd_type) {
                if (3 == index) {
                    gs_iap_waiting = 1;
                    memset(gs_iap_file, 0, LEN_DW_URL);
                    strncpy((char*)gs_iap_file, split_str, LEN_DW_URL);
                    DEBUG("gs_iap_file = %s\n", gs_iap_file);
                } else if (4 == index) {
                    memset(gs_iap_md5, 0, LEN_DW_URL);
                    strncpy((char*)gs_iap_md5, split_str, LEN_DW_URL);
                    DEBUG("gs_iap_md5 = %s\n", gs_iap_md5);
                }
            } else if (CHANGE_APN == cmd_type) {
                if (3 == index) {
                    memset(g_svr_ip, 0, LEN_NET_TCP);
                    strncpy((char*)g_svr_ip, split_str, LEN_NET_TCP);
                    DEBUG("g_svr_ip = %s\n", g_svr_ip);
                } else if (4 == index) {
                    memset(g_svr_port, 0, LEN_NET_TCP);
                    strncpy((char*)g_svr_port, split_str, LEN_NET_TCP);
                    DEBUG("g_svr_port = %s\n", g_svr_port);
                } else if (5 == index) {
                    memset(g_svr_apn, 0, LEN_NET_TCP);
                    strncpy((char*)g_svr_apn, split_str, LEN_NET_TCP);
                    DEBUG("g_svr_apn = %s\n", g_svr_apn);

                    gs_change_apn = 1;
                }
            } else if (DELETE_NFC == cmd_type) {
                if (3 == index) {
                    u16 offset = 0;
                    u8 del_count = 0;
                    u8 tmp_card[LEN_BYTE_SZ32+1] = "";
                    memset(gs_tmp_buf_big, 0, LEN_BYTE_SZ512);
                    strncpy((char*)gs_tmp_buf_big, split_str, LEN_BYTE_SZ512);
                    DEBUG("gs_delete_cards = %s\n", gs_tmp_buf_big);

                    for (i=0; i<strlen((char*)gs_tmp_buf_big);i++) {
                        if ('|' == gs_tmp_buf_big[i]) {
                            DeleteMobibCard(tmp_card, NULL);
                            DEBUG("To deleteX %s\n", tmp_card);

                            offset = strlen((const char*)gs_addordel_cards);

                            if (offset > (CNTR_MAX_CARD*(LEN_CARD_ID+1))) {
                                break;
                            }

                            if (0 == del_count) {
                                sprintf((char*)gs_addordel_cards+offset, "%s", tmp_card);
                            } else {
                                sprintf((char*)gs_addordel_cards+offset, "|%s", tmp_card);
                            }

                            del_count++;

                            k = 0;
                            memset(tmp_card, 0, LEN_BYTE_SZ512);
                        } else {
                            tmp_card[k++] = gs_tmp_buf_big[i];
                        }
                    }

                    offset = strlen((const char*)gs_addordel_cards);

                    if (offset < (CNTR_MAX_CARD*(LEN_CARD_ID+1))) {
                        if (0 == del_count) {
                            sprintf((char*)gs_addordel_cards+offset, "%s", tmp_card);
                        } else {
                            sprintf((char*)gs_addordel_cards+offset, "|%s", tmp_card);
                        }
                    }

                    DeleteMobibCard(tmp_card, NULL);
                    DEBUG("To delete %s\n", tmp_card);
                }
            } else if (MODIFY_ALARM == cmd_type) {
                if (3 == index) {
                    memset(gs_alarm_on, 0, LEN_NET_TCP);
                    strncpy((char*)gs_alarm_on, split_str, LEN_NET_TCP);
                    DEBUG("gs_alarm_on = %s\n", gs_alarm_on);
                    FlashWrite_SysParams(PARAM_ID_ALM_ON, (u8*)gs_alarm_on, 1);
                } else if (4 == index) {
                    memset(gs_beep_on, 0, LEN_NET_TCP);
                    strncpy((char*)gs_beep_on, split_str, LEN_NET_TCP);
                    DEBUG("gs_beep_on = %s\n", gs_beep_on);
        			FlashWrite_SysParams(PARAM_ID_BEEP_ON, (u8*)gs_beep_on, 1);
                } else if (5 == index) {
                    memset(gs_alarm_level, 0, LEN_NET_TCP);
                    strncpy((char*)gs_alarm_level, split_str, LEN_NET_TCP);
                    DEBUG("gs_alarm_level = %s\n", gs_alarm_level);
        			FlashWrite_SysParams(PARAM_ID_BEEP_LEVEL, (u8*)gs_alarm_level, 2);
                }
            }
        }

        split_str = strtok(NULL, delims);

        index++;
    }
}

// ============================================ DEV TCP Host ============================================ //
bool TcpHeartBeat(void)
{
    u32 tmp_vol1 = 0;
    u32 tmp_vol2 = 0;
    u32 bat_vol = 0;
    u8 vol_str[8] = "";
    // const char send_data[] = "#MOBIT,868446032285351,HB,4.0,1,20,e10adc3949ba59abbe56e057f20f883e$";

    if (true == IsIapRequested()) {
        return false;
    }

    if (ADC0_GetValue(&bat_vol)) {
        if ((bat_vol>=0) && (bat_vol<=1023)) {
            tmp_vol1 = (((bat_vol*330*18)/10240)%1000)/100;
            tmp_vol2 = (((bat_vol*330*18)/10240)%100)/10;
            sprintf((char*)vol_str, "%ld.%ld", tmp_vol1, tmp_vol2);
        } else {
            strcpy((char*)vol_str, "F");
        }
    } else {
        strcpy((char*)vol_str, "F");
    }

    GetDevRSSI();
    QueryNetMode();
    //GetDevNetModeRSSI();
    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%d,%s,%s", g_imei_str, CMD_HEART_BEAT, vol_str, gs_lock_sta, g_rssi_str, g_net_mode);

//    EncodeTcpPacket((u8*)tcp_send_buf);
//    memset(tcp_send_buf, 0, LEN_MAX_SEND);

//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,%s$", g_imei_str, CMD_HEART_BEAT, "4.0", "1", "20", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpDeviceRegister(void)
{
    u32 tmp_vol1 = 0;
    u32 tmp_vol2 = 0;
    u32 bat_vol = 0;
    u8 vol_str[8] = "";
    // const char send_data[] = "#MOBIT,868446032285351,REG,898602B4151830031698,1.0.0,1.0.0,4.0,1561093302758,2,e10adc3949ba59abbe56e057f20f883e$";

//    CalcRegisterKey();

    if (0 == strlen((const char*)g_net_mode)) {
        strcpy((char*)g_net_mode, "CAT-NB1");
    }

    if (ADC0_GetValue(&bat_vol)) {
        if ((bat_vol>=0) && (bat_vol<=1023)) {
            tmp_vol1 = (((bat_vol*2*330)/1024)%1000)/100;
            tmp_vol2 = (((bat_vol*2*330)/1024)%100)/10;
            sprintf((char*)vol_str, "%ld.%ld", tmp_vol1, tmp_vol2);
        } else {
            strcpy((char*)vol_str, "F");
        }
    } else {
        strcpy((char*)vol_str, "F");
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,%s,%s,%s,%s$%s", g_imei_str, CMD_DEV_REGISTER, g_iccid_str, HW_VER, SW_VER, vol_str, g_devtime_str, g_devzone_str, g_net_mode, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,%s,%s,%s,%s$", g_imei_str, CMD_DEV_REGISTER, g_iccid_str, "1.0.0", "1.00", "4.0", g_devtime_str, g_devzone_str, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpFinishFactoryReset(void)
{
    // DEV: #MOBIT,868446032285351,RSOK,1,111111111,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s$%s", g_imei_str, CMD_FINISH_RST, "1", "111111111", gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_FINISH_RST, "1", "111111111", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

// after lock the locker by hand
bool TcpExitCarriageSleep(void)
{
    // #MOBIT,868446032285351,WU,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s$%s", g_imei_str, CMD_EXIT_SLEEP, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_EXIT_SLEEP, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReportGPS(void)
{
    // #MOBIT,868446032285351,GEO,51.106922|3.702681|20|180,0,e10adc3949ba59abbe56e057f20f883e$

    if (0 == strlen(gs_gnss_part)) {
        strcpy(gs_gnss_part, "F|F|F|F");
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s$%s", g_imei_str, CMD_REPORT_GPS, gs_gnss_part, "0", gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_REPORT_GPS, gs_gnss_part, "0", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

void ReportInvalidMovingAlarm(void)
{
    TcpInvalidMovingAlarm();
    TcpReportGPS();
}

bool TcpInvalidMovingAlarm(void)
{
    // #MOBIT,868446032285351,AL,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s$%s", g_imei_str, CMD_INVALID_MOVE, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_INVALID_MOVE, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

void ReportRiskAlarm(void)
{
    TcpRiskAlarm();
    TcpReportGPS();
}

bool TcpRiskAlarm(void)
{
    // #MOBIT,868446032285351,FA,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s$%s", g_imei_str, CMD_RISK_REPORT, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_RISK_REPORT, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpFinishIAP(void)
{
    // #MOBIT,868446032285351,UP,1,1.0.0,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s$%s", g_imei_str, CMD_IAP_SUCCESS, "1", "1.0.0", gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s$", g_imei_str, CMD_IAP_SUCCESS, "1", "1.0.0", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpFinishAddNFCCard(void)
{
    // #MOBIT,868446032285351,ADDCR,a|b|c|d|e|f,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$%s", g_imei_str, CMD_FINISH_ADDNFC, (char*)((char)gs_addordel_cards[0]==0?"F":(char*)gs_addordel_cards), gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s$", g_imei_str, CMD_FINISH_ADDNFC, gs_addordel_cards, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReadedOneCard(u8* card_id, u8* serial_nr)
{
    // #MOBIT,868446032285351,RC,1234567,e10adc3949ba59abbe56e057f20f883e$

    if (GetNetStatus() != 0x81) {
        return false;
    }

    if ((NULL==card_id) || (NULL==serial_nr)) {
        return false;
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$%s", g_imei_str, CMD_CALYPSO_UPLOAD, card_id, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s$", g_imei_str, CMD_CALYPSO_UPLOAD, card_id, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpLockerLocked(void)
{
    // #MOBIT,868446032285351,LC,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s$%s", g_imei_str, CMD_DOOR_LOCKED, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_DOOR_LOCKED, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpLockerUnlocked(void)
{
    // #MOBIT,868446032285351,OL,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s$%s", g_imei_str, CMD_DOOR_UNLOCKED, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_DOOR_UNLOCKED, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpChargeStarted(void)
{
    // #MOBIT,868446032285351,CG,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s$%s", g_imei_str, CMD_CHARGE_STARTED, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_CHARGE_STARTED, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpChargeStoped(void)
{
    // #MOBIT,868446032285351,CGE,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s$%s", g_imei_str, CMD_CHARGE_STARTED, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s$", g_imei_str, CMD_CHARGE_STARTED, gs_send_md5);

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
        sprintf(tcp_send_buf, "#MOBIT,%s,%s,Re$%s", g_imei_str, cmd_str, gs_communit_key);
    } else {
        sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re$%s", g_imei_str, cmd_str, sta, gs_communit_key);
    }

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);

//    if (NULL == sta) {
//        sprintf(tcp_send_buf, "#MOBIT,%s,%s,Re,%s$", g_imei_str, cmd_str, gs_send_md5);
//    } else {
//        sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, cmd_str, sta, gs_send_md5);
//    }

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryParams(void)
{
    // #MOBIT,868446032285351,QG,lock.mobit.eu,1000,mobit.apn,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re$%s", g_imei_str, CMD_QUERY_PARAMS, g_svr_ip, g_svr_port, g_svr_apn, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_PARAMS, g_svr_ip, g_svr_port, g_svr_apn, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

// TODO 07/30
bool TcpReQueryGPS(void)
{
    // #MOBIT,868446032285351,GGEO,51.106922|3.702681|20|180,0,Re,e10adc3949ba59abbe56e057f20f883e$

    if (0 == strlen(gs_gnss_part)) {
        //gs_gnss_part[0] = 'F';
        strcpy(gs_gnss_part, "F|F|F|F");
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,Re$%s", g_imei_str, CMD_QUERY_GPS, gs_gnss_part, "0", gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_GPS, gs_gnss_part, "0", gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryNFCs(void)
{
    // #MOBIT,868446032285351,QCL,a|b|c|d|e|f|g|h,Re,e10adc3949ba59abbe56e057f20f883e$

    // Max Size: 20*20 = 400B + 64BHead/Tail = 464B
    memset(gs_tmp_buf_big, 0, LEN_BYTE_SZ512);
    QueryMobibCard(gs_tmp_buf_big);

    if (0 == strlen((const char*)gs_tmp_buf_big)) {
        gs_tmp_buf_big[0] = 'F';
    }

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re$%s", g_imei_str, CMD_QUERY_NFC, gs_tmp_buf_big, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_NFC, gs_tmp_buf_big, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryAlarm(void)
{
    // #MOBIT,868446032285351,ALC,1,1,80,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re$%s", g_imei_str, CMD_QUERY_ALARM, gs_alarm_on, gs_beep_on, gs_alarm_level, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_ALARM, gs_alarm_on, gs_beep_on, gs_alarm_level, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReQueryIccid(void)
{
    // #MOBIT,868446032285351,ICCID,898602B4151830031698,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re$%s", g_imei_str, CMD_QUERY_ICCID, g_iccid_str, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_QUERY_ICCID, g_iccid_str, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

bool TcpReDeleteNFCs(void)
{
    // #MOBIT,868446032285351,RMC,a|b|c|d|e,Re,e10adc3949ba59abbe56e057f20f883e$

    memset(tcp_send_buf, 0, LEN_MAX_SEND);
    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re$%s", g_imei_str, CMD_DELETE_NFC, gs_addordel_cards, gs_communit_key);

//    EncodeTcpPacket((u8*)tcp_send_buf);

//    memset(tcp_send_buf, 0, LEN_MAX_SEND);
//    sprintf(tcp_send_buf, "#MOBIT,%s,%s,%s,Re,%s$", g_imei_str, CMD_DELETE_NFC, gs_addordel_cards, gs_send_md5);

    return BG96TcpSend(tcp_send_buf);
}

// ============================================ DEV Action ============================================ //
bool DoUnLockTheLockerFast(void)
{
    LB1938_OpenLock();

    DEBUG("DoUnLockTheLockerFast...\n");

    return true;
}

bool DoRingAlarmFast(void)
{
    g_ring_times = 6;// ring 3 times
    DEBUG("DoRingAlarmFast...\n");

    return true;
}

bool DoFactoryResetFast(void)
{
    DEBUG("DoFactoryResetFast...\n");

    // DeleteAllMobibCard();

    // One Page for params
    FlashErase_OnePage(FLASH_PAGE_PARAMS, FLASH_BASE_PARAMS);

    // Two Page for CardIDs
    FlashErase_OnePage(FLASH_PAGE_CARD_ID, FLASH_BASE_CARD_ID);
    FlashErase_OnePage(FLASH_PAGE_CARD_ID, FLASH_BASE_CARD_ID+BYTE_ADDR_PER_PAGE_SML);

    FlashWrite_SysParams(PARAM_ID_RSVD_U2, (u8*)"1", 1);

    asm("reset");

    return true;
}

bool DoEnterSleepFast(void)
{
    DEBUG("DoEnterSleepFast...\n");

    LB1938_MotorCtrl(MOTOR_LEFT, MOTOR_HOLD_TIME);

    return true;
}

bool DoQueryGPSFast(void)
{
    DEBUG("DoQueryGPSFast...\n");

    GetGPSInfo(gs_gnss_part);

    return true;
}

bool DoQueryNFCFast(void)
{
    DEBUG("DoQueryNFCFast...\n");

    return true;
}

bool DoAddNFCFast(void)
{
    DEBUG("DoAddNFCFast...\n");

    gs_during_bind = 1;

    return true;
}

bool DoFtpIAP(void)
{
    DEBUG("DoFtpIAP...\n");

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

bool IsIapRequested(void)
{
    if (1 == gs_iap_waiting) {
        return true;
    } else {
        return false;
    }
}

bool ManualIapRequested(void)
{
    gs_iap_waiting = 1;

    return true;
}

bool ManualIapSetGotSize(u32 got_size)
{
	gs_ftp_offset = got_size;
	gs_ftp_sum_got = got_size;

    return true;
}

bool ManualIapSetFileName(u8* file)
{
	if (file != NULL) {
		memset(gs_iap_file, 0, LEN_DW_URL);
		strncpy((char*)gs_iap_file, (const char*)file, LEN_DW_URL);
	}
	
	return true;
}

void ReportFinishAddNFC(u8 gs_bind_cards[][LEN_BYTE_SZ64], u8* index_array)
{
    u8 i = 0;
    u8 j = 0;
    u16 offset = 0;
    unsigned long temp = 1;

    DEBUG("ReportFinishAddNFC...\n");

    for (i=0; i<CNTR_MAX_CARD; i++) {
        if ((index_array[i]<1) || (index_array[i]>20)) {
            break;
        }

        offset = strlen((const char*)gs_addordel_cards);

        if (offset > (CNTR_MAX_CARD*(LEN_CARD_ID+1))) {
            break;
        }

        if (0 == j) {
            sprintf((char*)gs_addordel_cards+offset, "%s", gs_bind_cards[index_array[i-1]]);
        } else {
            sprintf((char*)gs_addordel_cards+offset, "|%s", gs_bind_cards[index_array[i-1]]);
        }
        j++;
    }

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

u8 IsApnChangeWait(void)
{
    return gs_change_apn;
}

void ResetApnChange(void)
{
    gs_change_apn = 0;
}

void DequeueTcpRequest(void)
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
        gs_one_tcpcmds[i++] = ReadByteFromNetRingBuffer();

        if (3 == i) {
            if ((gs_one_tcpcmds[0]=='S')&&(gs_one_tcpcmds[1]=='T')&&(gs_one_tcpcmds[2]=='A')) {
                skip_flag = 0;
            } else {
                // invalid MSGs or overwritten: "XTA" -> just skip till net "STA"
                skip_flag = 1;
            }
        }

        if (i > 3) {
            if ((gs_one_tcpcmds[i-3]=='S')&&(gs_one_tcpcmds[i-2]=='T')&&(gs_one_tcpcmds[i-3]=='A')) {
                if (i != 3) {
                    i = 3;
                    gs_one_tcpcmds[0] = 'S';
                    gs_one_tcpcmds[1] = 'T';
                    gs_one_tcpcmds[2] = 'A';
                }

                skip_flag = 0;
            }

            if ((gs_one_tcpcmds[i-3]=='E')&&(gs_one_tcpcmds[i-2]=='N')&&(gs_one_tcpcmds[i-1]=='D')) {
                if ((gs_one_tcpcmds[0]=='S')&&(gs_one_tcpcmds[1]=='T')&&(gs_one_tcpcmds[2]=='A')) {
                    // process the valid MSGs
                    DEBUG("process MSGs: %s\n", gs_one_tcpcmds);
                    gs_one_tcpcmds[i-3] = '\0';// D
                    gs_one_tcpcmds[i-2] = '\0';// N
                    gs_one_tcpcmds[i-1] = '\0';// E
                    if (('#'==gs_one_tcpcmds[3]) && ('$'==gs_one_tcpcmds[i-4])) {
                        gs_one_tcpcmds[i-4] = '\0';// $
                        ParseMobitMsg(gs_one_tcpcmds+3);
                    } else {
                        DEBUG("Error MSGs %c:%c\n", gs_one_tcpcmds[3], gs_one_tcpcmds[i-4]);
                    }
                } else {
                    // skip the invalid MSGs
                    i = 0;
                }

                skip_flag = 0;
                memset(gs_one_tcpcmds, 0, RX_RINGBUF_MAX_LEN);

                continue;
            }
        }

        if (1 == skip_flag) {
            i = 0;
        }
    }
}

void ProtocolParamsInit(void)
{
    u8 params_dat[LEN_BYTE_SZ64+1] = "";

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_SVR_IP, params_dat, 64);
    if (strspn((const char*)params_dat, (const char*)".0123456789") == strlen((const char*)params_dat)) {
        strncpy((char*)g_svr_ip, (const char*)params_dat, LEN_NET_TCP);
    }

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_SVR_PORT, params_dat, 64);
    if (strspn((const char*)params_dat, (const char*)"0123456789") == strlen((const char*)params_dat)) {
        strncpy((char*)g_svr_port, (const char*)params_dat, LEN_NET_TCP);
    }

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_SVR_APN, params_dat, 64);
    if (strlen((const char*)params_dat) > 0) {
        strncpy((char*)g_svr_apn, (const char*)params_dat, LEN_NET_TCP);
    }

    // Write params into flash when the first time boot
    FlashRead_SysParams(PARAM_ID_1ST_BOOT, params_dat, 64);
    if (strncmp((const char*)params_dat, (const char*)"1", 1) != 0) {
        FlashWrite_SysParams(PARAM_ID_ALM_ON, (u8*)"1", 1);
        FlashWrite_SysParams(PARAM_ID_BEEP_ON, (u8*)"1", 1);
        FlashWrite_SysParams(PARAM_ID_BEEP_LEVEL, (u8*)"100", 3);
    }

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_ALM_ON, params_dat, 64);
    if (strspn((const char*)params_dat, (const char*)"0123456789") == strlen((const char*)params_dat)) {
        strncpy((char*)gs_alarm_on, (const char*)params_dat, LEN_COMMON_USE);
    }

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_BEEP_ON, params_dat, 64);
    if (strspn((const char*)params_dat, (const char*)"0123456789") == strlen((const char*)params_dat)) {
        strncpy((char*)gs_beep_on, (const char*)params_dat, LEN_COMMON_USE);
    }

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_BEEP_LEVEL, params_dat, 64);
    if (strspn((const char*)params_dat, (const char*)"0123456789") == strlen((const char*)params_dat)) {
        strncpy((char*)gs_alarm_level, (const char*)params_dat, LEN_COMMON_USE);
    }

    memset(params_dat, 0, LEN_BYTE_SZ64);
    FlashRead_SysParams(PARAM_ID_RSVD_U2, params_dat, 64);
    if (0 == strncmp((const char*)params_dat, (const char*)"1", 1)) {
        u32 temp = 1;
        gs_need_ack |= (temp<<FACTORY_RST);
        FlashWrite_SysParams(PARAM_ID_RSVD_U2, (u8*)"0", 1);
    }
}

u8 bytes_array[CNTR_INWD_PER_PAGE*BINA_DATA_PER_INWD] = {0};

static bool CheckIapMd5(u32 bin_size, u32 inwd_count)
{
    u32 i = 0;
	u32 tmp_val = 0;
    u32 data_len = 0;
    u32 page_count = 0;
    u16 flash_page = 0;
    u32 flash_offset = 0;
    u8 strs_array[LEN_BYTE_SZ64 + 1] = "";

    MD5_CTX iapbin_md5_ctx;
    u8 iapbin_res_md5[LEN_COMMON_USE] = "";

    if ((bin_size < 512) || (0==inwd_count)) {
        return false;
    }

    GAgent_MD5Init(&iapbin_md5_ctx);

	// For Vector Partition
	data_len = CNTR_INWD_PER_BLK*BINA_DATA_PER_INWD;// Vector BIN SIZE = 512
	flash_offset = 0;
	flash_page = FLASH_PAGE_BAK;

	FlashRead_InstructionWordsToByteArray(flash_page, flash_offset, CNTR_INWD_PER_PAGE, bytes_array);

	bytes_array[7] = 0x05;
	DEBUG("MD5 flash_address = 0x%X-%.4lX-%ld\n", flash_page, (flash_offset % 0x10000), data_len);

	DEBUG("GAgent_MD5Update DATZ = %.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X\n", (u8)bytes_array[0], (u8)bytes_array[1], (u8)bytes_array[2], (u8)bytes_array[3], (u8)bytes_array[4], (u8)bytes_array[5], (u8)bytes_array[6], (u8)bytes_array[7]);
	DEBUG("GAgent_MD5Update DATF = %.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X\n", (u8)bytes_array[data_len-8], (u8)bytes_array[data_len-7], (u8)bytes_array[data_len-6], (u8)bytes_array[data_len-5], (u8)bytes_array[data_len-4], (u8)bytes_array[data_len-3], (u8)bytes_array[data_len-2], (u8)bytes_array[data_len-1]);
	GAgent_MD5Update(&iapbin_md5_ctx, bytes_array, data_len);

	// For Data Partition
    data_len = CNTR_INWD_PER_PAGE * BINA_DATA_PER_INWD;
	// 512 Vector BIN SIZE = 0x100 Flash Addr = 0x80 INWD
    page_count = ((inwd_count-0x80+CNTR_INWD_PER_PAGE-1) / CNTR_INWD_PER_PAGE);

	DEBUG("page_count = %ld, inwd_count = %ld\n", page_count, inwd_count);
    for (i=0; i<page_count; i++) {
		tmp_val = BYTE_ADDR_PER_PAGE_SML;
		tmp_val *= i;

        flash_offset = FLASH_BASE_BAK;
		flash_offset += tmp_val;

		flash_page = FLASH_PAGE_BAK + (flash_offset / 0x10000);

        flash_offset %= 0x10000;
        FlashRead_InstructionWordsToByteArray(flash_page, flash_offset, CNTR_INWD_PER_PAGE, bytes_array);

        if (i == (page_count-1)) {
			// Vector BIN SIZE = 512
            data_len = (bin_size - CNTR_INWD_PER_BLK*BINA_DATA_PER_INWD) % (CNTR_INWD_PER_PAGE*BINA_DATA_PER_INWD);
        }

		DEBUG("MD5 flash_address = 0x%X-%.4lX-%ld\n", flash_page, (flash_offset % 0x10000), data_len);

		DEBUG("GAgent_MD5Update DATZ = %.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X\n", (u8)bytes_array[0], (u8)bytes_array[1], (u8)bytes_array[2], (u8)bytes_array[3], (u8)bytes_array[4], (u8)bytes_array[5], (u8)bytes_array[6], (u8)bytes_array[7]);
		DEBUG("GAgent_MD5Update DATF = %.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X\n", (u8)bytes_array[data_len-8], (u8)bytes_array[data_len-7], (u8)bytes_array[data_len-6], (u8)bytes_array[data_len-5], (u8)bytes_array[data_len-4], (u8)bytes_array[data_len-3], (u8)bytes_array[data_len-2], (u8)bytes_array[data_len-1]);
        GAgent_MD5Update(&iapbin_md5_ctx, bytes_array, data_len);
    }

    GAgent_MD5Final(&iapbin_md5_ctx, iapbin_res_md5);
	
	DEBUG("FLASH MD5 = ");
    for (i=0; i<16; i++) {
		DEBUG("%.2X", iapbin_res_md5[i]);
		sprintf((char*)strs_array+i*2, "%.2X", iapbin_res_md5[i]);
    }
    DEBUG("\r\n");

	DEBUG("FLASH MD5V2 = %s\n", strs_array);
#if 1
    FlashRead_SysParams(PARAM_ID_IAP_MD5, gs_iap_md5, LEN_BYTE_SZ64);

	DEBUG("USER FTP MD5 = %s\n", gs_iap_md5);
    if (0 == strncmp((const char*)gs_iap_md5, (const char*)strs_array, LEN_BYTE_SZ32)) {
        return true;
    } else {
        return false;
    }
#else
	return true;
#endif
}

void ProcessIapRequest(void)
{
    if (gs_iap_waiting != 1) {
        return;
    }

    u16 i = 0;
    u32 j = 0;
    u8 ftp_sta = 0;
    u16 got_size = 0;
    u32 ftp_len_per = 1024;
    // u32 ftp_len_per = 512;
    static u32 iap_total_size = 0;
	static u8 trycnt = 0;

    ftp_sta = GetFtpStatus();

    // ----------------
    // --------
    // Erase BAK partition & Initialize MD5
    if (0 == gs_is_erased) {
        gs_is_erased = 1;
        // GAgent_MD5Init(&g_ftp_md5_ctx);

		if (0 == gs_ftp_sum_got) {
			FlashErase_LargePage(FLASH_PAGE_BAK, FLASH_BASE_BAK);// SIZE: 0xE000
			FlashErase_LargePage(FLASH_PAGE_BAK+1, 0);// SIZE: 0x10000
		}
    }

    // ----------------
    // --------
    // Start first Connect or Re-Connect
    if (0 == ftp_sta) {
		// re-connect 5 times, if all failed, abort IAP and report to TcpServer
		// TODO: Report IAP Failed into TcpServer
        if (++trycnt > 5) {
            trycnt = 0;
            // gs_ftp_sta = 0;
			gs_iap_waiting = 0;
        }
        ConnectToFtpServer(gs_iap_file, gs_ftp_ip, gs_ftp_port);

        iap_total_size = GetFTPFileSize(gs_iap_file);

        if (iap_total_size <= 0) {
            return;
        }
    }

    memset(gs_iap_buf, 0, LEN_BYTE_SZ1024);
    if (0x81 == ftp_sta) {
#if 1
		if (0 == gs_ftp_sum_got) {
			ftp_len_per = 0x200;// Intr Vector
			FlashWrite_SysParams(PARAM_ID_IAP_FLAG, (u8*)IAP_REQ_RN, 4);
			FlashWrite_SysParams(PARAM_ID_RSVD_U3, (u8*)gs_iap_file, strlen((const char*)gs_iap_file));
			FlashWrite_SysParams(PARAM_ID_IAP_MD5, (u8*)gs_iap_md5, 32);
		} else {
			ftp_len_per = 0x400;// 1024
		}
#endif

        CleaFtpBuffer();
        got_size = BG96FtpGetData(gs_ftp_offset, ftp_len_per, gs_iap_buf, gs_iap_file);

        if (got_size > 0) {
            u8 j = 0;
            u32 tmp_value = 10;
            u8 num_count = 0;
            u8 iap_size_str[8] = {0};
            u16 flash_page = FLASH_PAGE_BAK;// 0x2,2000
            u32 flash_offset = FLASH_BASE_BAK;

            OneInstruction_t dat[1024];

            gs_ftp_offset += got_size;
            DEBUG("gs_ftp_offset = %ld\n", gs_ftp_offset);
#if 1
#if 1
            PrintFtpBuffer();
#endif

#if 0
            DEBUG("HEX Dat:\n");
            for (i=0; i<got_size/4; i++) {
                DEBUG("%.2X%.2X%.2X    ", (u8)gs_iap_buf[4*i+2], (u8)gs_iap_buf[4*i+1], (u8)gs_iap_buf[4*i]);
                if (3 == i%4) {
                    DEBUG("\n");
                }
            }
#endif

#if 1
            for (i=0; i<g_ftp_dat_cnt; i++) {
                if (('T'==g_ftp_buf[i])&&(0x0D==g_ftp_buf[i+1])&&(0x0A==g_ftp_buf[i+2])) {
                    j = i+3;
                    DEBUG("Found Connect Begin...\n");
                    break;
                }
            }

			// GAgent_MD5Update(&g_ftp_md5_ctx, g_ftp_buf+j, got_size);

            for(i=0;i<(got_size/4);i++)
            {
                if ((i*4+2) >= got_size) {
                    break;
                }
                dat[i].HighLowUINT16s.HighWord = (u8)g_ftp_buf[j+i*4+2];
                if ((i*4+1) >= got_size) {
                    break;
                }
                dat[i].HighLowUINT16s.LowWord = (u8)g_ftp_buf[j+i*4+1] << 8;
                if ((i*4+0) >= got_size) {
                    break;
                }
                dat[i].HighLowUINT16s.LowWord += (u8)g_ftp_buf[j+i*4+0];
            }
#else
			// GAgent_MD5Update(&g_ftp_md5_ctx, gs_iap_buf, got_size);

            for(i=0;i<(got_size/4);i++)
            {
                if ((i*4+2) >= got_size) {
                    break;
                }
                dat[i].HighLowUINT16s.HighWord = (u8)gs_iap_buf[i*4+2];
                if ((i*4+1) >= got_size) {
                    break;
                }
                dat[i].HighLowUINT16s.LowWord = (u8)gs_iap_buf[i*4+1] << 8;
                if ((i*4+0) >= got_size) {
                    break;
                }
                dat[i].HighLowUINT16s.LowWord += (u8)gs_iap_buf[i*4+0];
            }
#endif            
            for (i=0; i<1; i++) {
                DEBUG("=%.8lX\n", (u32)dat[i].UINT32);
            }

            if ((0==gs_ftp_sum_got) && (0x200==got_size)) {
                flash_offset = 0;
                flash_page = FLASH_PAGE_BAK;
            } else {
                flash_offset = FLASH_BASE_BAK;
                flash_offset += gs_ftp_sum_got / 2;
                flash_offset -= 0x100;// =Intr Vector, 2B flash size = 4B bin size(3B data + 1B dummy)
                flash_page = FLASH_PAGE_BAK + (flash_offset / 0x10000);
            }
#endif
            // DEBUG("flash_offset = %.8lX, %ld\n", flash_offset, flash_offset);
            DEBUG("Before WR flash_address = 0x%X-%.4lX\n", flash_page, (flash_offset % 0x10000));
            FlashWrite_InstructionWords(flash_page, (u16)flash_offset, dat, ftp_len_per/4);
            // FlashWrite_InstructionWords(flash_page, (u16)flash_offset, dat, 256);
			
			DEBUG("After WR flash_address = 0x%X-%.4lX\n", flash_page, (flash_offset % 0x10000));
            gs_ftp_sum_got += got_size;
			
            tmp_value = gs_ftp_sum_got;
            do {
                tmp_value /= 10;
                num_count++;
            } while(tmp_value != 0);

            for (i=0; i<num_count; i++) {
                tmp_value = 1;
                for (j=0; j<(num_count-i-1); j++) {
                    tmp_value *= 10;
                }
                iap_size_str[i] = '0' + ((gs_ftp_sum_got/tmp_value)%10);
            }

            DEBUG("iap_got_size_str = %s\n", iap_size_str);

            FlashWrite_SysParams(PARAM_ID_RSVD_U4, (u8*)iap_size_str, 6);
        }

        if ((gs_ftp_offset>=iap_total_size) && (iap_total_size!=0)) {
            u8 j = 0;
			u32 inwd_count = 0;
            u32 tmp_value = 10;
            u8 num_count = 0;
            u8 iap_size_str[8] = {0};

            gs_iap_waiting = 0;

			inwd_count = ((iap_total_size+BINA_DATA_PER_INWD-1) / BINA_DATA_PER_INWD);
#if 0
            // GAgent_MD5Final(&g_ftp_md5_ctx, gs_ftp_res_md5);

            DEBUG("FTP MD5 = ");
            for (i=0; i<16; i++) {
                // DEBUG("%.2X", gs_ftp_res_md5[i]);
            }
            DEBUG("\r\n");
#endif
            DEBUG("FTP DW Finished...\n");

            tmp_value = iap_total_size;
            do {
                tmp_value /= 10;
                num_count++;
            } while(tmp_value != 0);

            for (i=0; i<num_count; i++) {
                tmp_value = 1;
                for (j=0; j<(num_count-i-1); j++) {
                    tmp_value *= 10;
                }
                iap_size_str[i] = '0' + ((iap_total_size/tmp_value)%10);
            }

            DEBUG("iap_size_str = %s\n", iap_size_str);

			if (true == CheckIapMd5(iap_total_size, inwd_count)) {
#if 1
				FlashWrite_SysParams(PARAM_ID_IAP_FLAG, (u8*)IAP_REQ_ON, 4);
				FlashWrite_SysParams(PARAM_ID_RSVD_U1, (u8*)iap_size_str, 6);

				DEBUG("Before APP Reset...\n");
				asm("reset");
			} else {
				DEBUG("IAP MD5 validate failed...\n");
			}
#endif
        }
    }
}

void PowerOffMainSupply(void)
{
    // close connectiton
    CloseTcpService();
    CloseFtpService();

    // clear flags into default

    // turn off power supply
    // or enter into low power consume mode
}

void PowerOnMainSupply(void)
{
    // turn on power supply
    // or quit from low power consume mode

    // wait some time for power sequence...
}

void ProcessTcpRequest(void)
{
    unsigned long temp = 1;
    u8 flag=0;
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
                    if ((IAP_UPGRADE==i) ||
                        (MODIFY_ALARM==i) || (RING_ALARM==i)) {
                        TcpReNormalAck((u8*)(cmd_list[i]), NULL);
                    }
                    else if (ADD_NFC==i){
                        TcpReNormalAck((u8*)(cmd_list[i]), NULL);
                        SetLedsStatus(MAIN_LED_G, LED_ON);
                        g_led_times=20000;
                        g_ring_times=1;
                    } else if ((CHANGE_APN==i) || (FACTORY_RST==i)) {
                        u8 run_ret = 1;// default success

                        if (run_ret) {
                            TcpReNormalAck((u8*)(cmd_list[i]), (u8*)("1"));
                        } else {
                            TcpReNormalAck((u8*)(cmd_list[i]), (u8*)("0"));
                        }
                    } else if (DELETE_NFC == i) {
                        TcpReDeleteNFCs();
                    } else {
                        u8 run_ret = 1;// default success

                        // must check if unlock success
                        // read the value of PIN connected to the locker position
                        if (UNLOCK_DOOR == i) {
                            if (GPIOx_Input(BANKE, 4)) {// unlock NG: maybe get stuck
                                run_ret = 0;
                            }
                        }

                        // must check if unlock success
                        // read the value of PIN connected to the locker position
                        if (ENTER_SLEEP == i) {
                            if (GPIOx_Input(BANKE, 4)) {// unlock NG: maybe get stuck
                                run_ret = 0;
                            }
                        }

                        if (run_ret) {
                            TcpReNormalAck((u8*)(cmd_list[i]), (u8*)("1"));
                            ReportLockerUnlocked();
                        } else {
                            TcpReNormalAck((u8*)(cmd_list[i]), (u8*)("0"));
                        }

                        if (ENTER_SLEEP == i) {
                            if (run_ret) {
                                PowerOffMainSupply();
                                gs_during_ship = 1;
                            }
                        }
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
                //????????1s
                g_ring_times=10;
                TcpLockerLocked();
            }
        }
    } else if (gs_till_svr_ack & (temp<<DOOR_UNLOCKED)) {
        if (start_time_unlocked != 0) {
            if (isDelayTimeout(start_time_unlocked,10*1000UL)) {
                //????????1s
                g_ring_times=10;
                TcpLockerUnlocked();
                TcpReportGPS();
                // TODO: delete this test
                gs_till_svr_ack = 0;
            }
        }
    } else if (gs_till_svr_ack & (temp<<FINISH_ADDNFC)) {
        if (start_time_finish_addc != 0) {
            if (isDelayTimeout(start_time_finish_addc,10*1000UL)) {
                TcpFinishAddNFCCard();
                // TODO: delete this test
                gs_till_svr_ack = 0;
                
                //???????
                g_led_times=0;
            }
        }
    } else if (gs_till_svr_ack & (temp<<RISK_REPORT)) {
        if (start_time_risk != 0) {
            if (isDelayTimeout(start_time_risk,10*1000UL)) {
                TcpRiskAlarm();
            }
        }
    } else if (gs_till_svr_ack & (temp<<DEV_REGISTER)) {
        if (start_time_reg != 0) {
            if (isDelayTimeout(start_time_reg,5*1000UL)) {
                QueryNetMode();
                TcpDeviceRegister();
                special_test++;
                start_time_reg = GetTimeStamp();
            }
        }
    }
}

u8 IsDuringShip(void)
{
    return gs_during_ship;
}

void ClearShipStatus(void)
{
    gs_during_ship = 0;
}

void StartCommunication(void)
{
    start_time_reg = GetTimeStamp();
    gs_till_svr_ack |= (1<<DEV_REGISTER);
}