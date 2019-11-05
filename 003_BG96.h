//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * A library for Quectel BG96 Module
 * This file is about the BG96 API
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 02/20/2019
******************************************************************************/

#ifndef __BG96_H
#define __BG96_H

#include <string.h>
#include <stdlib.h>
#include <p24fxxxx.h>

#include "015_Common.h"

/////////////////////////////////// BG96 AT CMDs ///////////////////////////////////
// AT commands response
static const char RESPONSE_READY[] = "RDY";
static const char RESPONSE_OK[] = "OK";
static const char RESPONSE_CRLF_OK[] = "\r\n\r\nOK";
static const char RESPONSE_ERROR[] = "ERROR";
static const char RESPONSE_POWER_DOWN[] = "POWERED DOWN";
static const char RESPONSE_CONNECT[] = "CONNECT";
static const char RESPONSE_SEND_OK[] = "SEND OK";
static const char RESPONSE_SEND_FAIL[] = "SEND FAIL";

// common AT commands
static const char DEV_AT[] = "";
static const char DEV_INFORMATION[] = "I";
static const char DEV_VERSION[] = "+CGMR";
static const char DEV_IMEI[] = "+CGSN";
static const char DEV_FUN_LEVEL[] = "+CFUN";
static const char DEV_LOCAL_RATE[] = "+IPR";
static const char DEV_SIM_IMSI[] = "+CIMI";
static const char DEV_SIM_PIN[] = "+CPIN";
static const char DEV_SIM_ICCID[] = "+QCCID";
static const char DEV_TIME_ZONE[] = "+CCLK";
static const char DEV_NET_STATUS[] = "+CREG";
static const char DEV_NET_STATUS_G[] = "+CGREG";
static const char DEV_EPS_NET_STATUS[] = "+CEREG";
static const char DEV_NET_RSSI[] = "+CSQ";
static const char DEV_NET_MODE_RSSI[] = "+QCSQ";
static const char DEV_NET_OPERATOR[] = "+COPS";
static const char DEV_NET_INFORMATION[] = "+QNWINFO";
static const char DEV_NET_PACKET_COUNTER[] = "+QGDCNT";
static const char DEV_POWER_DOWN[] = "+QPOWD";
static const char DEV_CLOCK[] = "+CCLK";

// TCPIP AT Commands
static const char APN_PARAMETERS[] = "+QICSGP";
static const char ACTIVATE_APN[] = "+QIACT";
static const char DEACTIVATE_APN[] = "+QIDEACT";
static const char GET_APN_IP_ADDRESS[] = "+CGPADDR";
static const char OPEN_SOCKET[] = "+QIOPEN";
static const char CLOSE_SOCKET[] = "+QICLOSE";
static const char SOCKET_STATUS[] = "+QISTATE";
static const char SOCKET_SEND_DATA[] = "+QISEND";
static const char SOCKET_READ_DATA[] = "+QIRD";
static const char SOCKET_SEND_HEX_DATA[] = "+QISENDEX";
static const char DATA_ACCESS_MODES[] = "+QISWTMD";
static const char QUERY_ERROR_CODE[] = "+QIGETERROR";
static const char RECV_SOCKET_EVENT[] = "+QIURC";

// FTP AT Commands
static const char FTP_CONFIG_PARAMETER[] = "+QFTPCFG";
static const char FTP_OPEN_SESSION[] = "+QFTPOPEN";
static const char FTP_DOWNLOAD_DAT[] = "+QFTPGET";
static const char FTP_GET_FIL_SIZE[] = "+QFTPSIZE";

// MQTT AT Commands
static const char MQTT_CONFIG_PARAMETER[] = "+QMTCFG";
static const char MQTT_OPEN_NETWORK[] = "+QMTOPEN";
static const char MQTT_CLOSE_NETWORK[] = "+QMTCLOSE";
static const char MQTT_CREATE_CLIENT[] = "+QMTCONN";
static const char MQTT_CLOSE_CLIENT[] = "+QMTDISC";
static const char MQTT_SUBSCRIBE_TOPICS[] = "+QMTSUB";
static const char MQTT_UNSUBSCRIBE_TOPICS[] = "+QMTUNS";
static const char MQTT_PUBLISH_MESSAGES[] = "+QMTPUB";
static const char MQTT_STATUS[] = "+QMTSTAT";
static const char MQTT_RECV_DATA[] = "+QMTRECV";

// GNSS AT Commands
static const char GNSS_CONFIGURATION[] = "+QGPSCFG";
static const char GNSS_TURN_ON[] = "+QGPS";
static const char GNSS_TURN_OFF[] = "+QGPSEND";
static const char GNSS_GET_POSITION[] = "+QGPSLOC";
static const char GNSS_ACQUIRE_NMEA[] = "+QGPSGNMEA";

/////////////////////////////////// Wait Implement ///////////////////////////////////
// For GPIO Ctrl
#define LOW 0
#define HIGH 1
#define OUTPUT 1
#define POWKEY_PIN  6
#define RESET_PIN   5

/////////////////////////////////// BG96 MACRO ///////////////////////////////////
typedef enum functionality {
    MINIMUM_FUNCTIONALITY = 0,
    FULL_FUNCTIONALITY = 1,
    DISABLE_RF = 4,
} Functionality_t;

typedef enum cmd_status {
    READ_MODE = 0,
    WRITE_MODE = 1,
} Cmd_Status_t;

typedef enum net_status {
    NOT_REGISTERED = 0,
    REGISTERED = 1,
    SEARCHING = 2,
    REGISTRATION_DENIED = 3,
    UNKNOWN = 4,
    REGISTERED_ROAMING = 5,
} Net_Status_t;

typedef enum net_type {
    GSM = 0,
    LTE_CAT_M1 = 8,
    LTE_CAT_NB1 = 9,
} Net_Type_t;

// For FILE
typedef enum open_file_mode {
    CREATE_OR_OPEN = 0,
    CREATE_OR_CLEAR = 1,
    ONLY_READ = 2,
} Open_File_Mode_t;

typedef enum pointer_mode {
    FILE_BEGINNING = 0,
    FILE_CURRENT = 1,
    FILE_ENDING = 2,
} Pointer_Mode_t;

// For GNSS
typedef enum gnss_work_mode {
    STAND_ALONE = 1,
    MS_BASED = 2,
    MS_ASSISTED = 3,
    SPEED_OPTIMAL = 4,
} GNSS_Work_Mode_t;

typedef enum gnss_constellation {
    GPS_ONLY = 0,
    GPS_GLONASS_BEIDOU_GALILEO = 1,
    GPS_GLONASS_BEIDOU = 2,
    GPS_GLONASS_GALILEO = 3,
    GPS_GLONASS = 4,
    GPS_BEIDOU_GALILEO = 5,
    GPS_GALILEO = 6,
} GNSS_Constellation_t;

typedef enum nmea_sentences_type {
    GPGGA = 1,
    GPRMC = 2,
    GPGSV = 4,
    GPGSA = 8,
    GPVTG = 16,
} NMEA_Type_t;

// For TCPIP
typedef enum protocol_type {
    IPV4 = 1,
    IPV4V6 = 2,
} Protocol_Type_t;

typedef enum authentication_methods {
    NONE = 0,
    PAP = 1,
    CHAP = 2,
    PAP_OR_CHAP = 3,
} Authentication_Methods_t;

typedef enum socket_type {
    TCP_CLIENT = 0,
    TCP_SEVER = 1,
    UDP_CLIENT = 2,
    UDP_SEVER = 3,
} Socket_Type_t;

typedef enum access_mode {
    BUFFER_MODE = 0,
    DIRECT_PUSH_MODE = 1,
    TRANSPARENT_MODE = 2,
} Access_Mode_t;

typedef enum socket_event {
    SOCKET_RECV_DATA_EVENT = 1,
    SOCKET_CLOSE_EVENT = 2,
    SOCKET_PDP_DEACTIVATION_EVENT = 3,
    SOCKET_INCOMING_CONNECTION_EVENT = 4,
    SOCKET_CONNECTION_FULL_EVENT = 5,
} Socket_Event_t;

typedef enum ssl_cipher_suites {
    TLS_RSA_WITH_AES_256_CBC_SHA = 0,
    TLS_RSA_WITH_AES_128_CBC_SHA = 1,
    TLS_RSA_WITH_RC4_128_SHA = 2,
    TLS_RSA_WITH_RC4_128_MD5 = 3,
    TLS_RSA_WITH_3DES_EDE_CBC_SHA = 4,
    TLS_RSA_WITH_AES_256_CBC_SHA256 = 5,
    TLS_ECDHE_RSA_WITH_RC4_128_SHA = 6,
    TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA = 7,
    TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA = 8,
    TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA = 9,
    TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256 = 10,
    TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384 = 11,
    TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 = 12,
    SUPPORT_ALL_ABOVE = 13,
} SSL_Cipher_Suites_t;

typedef enum ssl_socket_event {
    SSL_SOCKET_RECV_EVENT = 1,
    SSL_SOCKET_CLOSE_EVENT = 2,
} SSL_Socket_Event_t;

static const char ssl_ca_cert_name[] = "ca_cert.pem";
static const char ssl_client_cert_name[] = "client_cert.pem";
static const char ssl_client_key_name[] = "client_key.pem";

// For BG96 Serial
#define RX_BUFFER_LENGTH  1024
#define RX_RINGBUFFER_LENGTH 512
// #define UART_DEBUG

static const unsigned long Band_list[] = {
    9600,
    19200,
    38400,
    57600,
    115200,
    230400,
    460800,
    921600
};

typedef enum cmd_response {
    UNKNOWN_RESPONSE  = -2,
    TIMEOUT_RESPONSE  = -1,
    FAIL_RESPONSE  =  0,
    SUCCESS_RESPONSE  = 1,
} Cmd_Response_t;

/////////////////////////////////// BG96 API ///////////////////////////////////
bool GetDevNetSignalQuality(unsigned int *rssi);

Cmd_Response_t ScanOperatorNetwork(char *net);

Cmd_Response_t DevOperatorNetwork(unsigned int *mode, unsigned int *format, char *oper, Net_Type_t *act, Cmd_Status_t status);

bool GetDevNetworkInformation(char *type, char *oper, char *band, char *channel);

bool DevNetPacketCounter(unsigned long *send_bytes, unsigned long *recv_bytes, bool clean);

bool DevPowerDown();

bool DevClock(char *d_clock, Cmd_Status_t status);

// For TCPIP
bool CloseTcpService(void);

bool SwitchAccessModes(unsigned int socket_index, Access_Mode_t mode);

bool QueryLastErrorCode(char *err_code);

// For Serial
bool SendDataAndCheck(const char *data_buf, const char *ok_str, const char *err_str, unsigned int timeout);

bool SendATcommand(const char *command);

bool ReturnErrorCode(int *s_err_code);

void Configure_BG96(void);

bool WaitUartNetRxIdle();
int IsNetRingBufferAvailable();
char ReadByteFromNetRingBuffer();

bool BG96ATInit(void);
bool HeartBeat(void);

void InitRingBuffers(void);

bool BG96ATInitialize(void);
bool ConnectToTcpServer(u8* svr_ip, u8* svr_port, u8* svr_apn);
bool ConnectToFtpServer(u8* iap_file, u8* ftp_ip, u8* ftp_port);
u16 BG96FtpGetData(u32 offset, u32 length, u8* iap_buf, u8* iap_file);
bool BG96TcpSend(char* send_buf);
bool CloseFtpService(void);

u8 GetNetStatus(void);
void SetNetStatus(u8 sta);
void GetGPSInfo(char* gnss_part);

u8 GetFtpStatus(void);

bool BG96TcpSendTail(void);
bool BG96TcpSendCancel(void);
bool BG96TcpSendMiddle(char *data_buf);
bool BG96TcpSendHead(unsigned int socket_index, Socket_Type_t socket);
u32 GetFTPFileSize(u8* iap_file);
void BG96_PowerUp(void);
bool QueryNetMode(void);
void GetGPSInfo(char* gnss_part);
bool TurnOnGNSS(GNSS_Work_Mode_t mode, Cmd_Status_t status);
bool TurnOffGNSS(void);
bool TurnTryLocate(void);
bool TurnOnGNSSDamon(void);
bool GetDevRSSI(void);
bool GetDevNetModeRSSI(void);
bool BG96EnsureRxOk(void);
bool QueryNetStatus(void);
bool SetAutoNetMode(void);
bool DumpNetMode(void);
void ResetBG96Module(void);
void PowerOffBG96Module(void);

#endif //__BG96_H

//******************************************************************************
//* END OF FILE
//******************************************************************************
