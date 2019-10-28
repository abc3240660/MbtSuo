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

#include <stdio.h>
#include <string.h>
#include <p24fxxxx.h>

#include "001_Tick_10ms.h"
#include "003_BG96.h"
#include "006_Gpio.h"
#include "007_Uart.h"
#include "008_RingBuffer.h"
#include "013_Protocol.h"

// --
// ---------------------- local variables -------------------- //
// --
static int errorCode = -1;
static unsigned int bufferHead = 0;

// Read data from RingBuffer into rxBuffer
// buffer for searched result
static char rxBuffer[RX_BUFFER_LENGTH] = "";

// ringbuf for at acks
static char atRingbuf[RX_RINGBUF_MAX_LEN] = {0};

// ringbuf for net recved
static char netRingbuf[RX_RINGBUF_MAX_LEN] = {0};

// BIT7: have ever connected at least once
// BIT6: CMCC registered
// BIT0: current connect status
static u8 gs_net_sta = 0;

static u8 gs_ftp_sta = 0;

static u8 gs_gnss_pos[LEN_BYTE_SZ128] = "";

// TODO: Delete Temp Use
static u8 gs_ftp_test = 0;

// --
// ---------------------- global variables -------------------- //
// --
ringbuffer_t g_at_rbuf;
ringbuffer_t g_net_rbuf;

u8 g_devtime_str[LEN_COMMON_USE+1] = "";
u8 g_devzone_str[LEN_COMMON_USE+1] = "";

u8 g_imei_str[LEN_COMMON_USE+1] = "";
u8 g_iccid_str[LEN_COMMON_USE+1] = "";
u8 g_rssi_str[LEN_COMMON_USE+1] = "";
u8 g_net_mode[LEN_COMMON_USE+1] = "";

void InitRingBuffers(void)
{
    ringbuffer_init(&g_at_rbuf,atRingbuf,RX_RINGBUF_MAX_LEN);
    ringbuffer_init(&g_net_rbuf,netRingbuf,RX_RINGBUF_MAX_LEN);
}

/////////////////////////////////// Wait Implement ///////////////////////////////////

//******************************************************************************************
// FuncName: SetPinDir
// Descriptions: configure the direction of GPIO
// Params:   (IN) pin_num: the pointed GPIO
//           (IN) dir: the pointed direction
// Return:   NONE
//******************************************************************************************
void SetPinDir(u8 pin_num, u8 dir)
{
}

//******************************************************************************************
// FuncName: SetPinVal
// Descriptions: configure the value of GPIO
// Params:   (IN) pin_num: the pointed GPIO
//           (IN) value: the pointed value
// Return:   NONE
//******************************************************************************************
void SetPinVal(u8 pin_num, u8 val)
{
}

//******************************************************************************************
// FuncName: WriteToBG96
// Descriptions: PIC24F send data to BG96
//           (IN) data_buf: the data to be send to BG96
// Return:   the number of bytes written
//******************************************************************************************
static int WriteToBG96(const char *data_buf)
{
    printf("%s", data_buf);

    return Uart2_Printf("%s",data_buf);
}

//******************************************************************************************
// FuncName: IsRingBufferAvailable
// Descriptions: get the number of bytes available to read
// Return:   the number of bytes available in RingBuffer
//******************************************************************************************
static int IsRingBufferAvailable()
{
    int ret;

    ret = ringbuffer_buf_use_size(&g_at_rbuf);

    return ret;
}

int IsNetRingBufferAvailable()
{
    int ret;

    ret = ringbuffer_buf_use_size(&g_net_rbuf);

    return ret;
}

//******************************************************************************************
// FuncName: ReadByteFromRingBuffer
// Descriptions: PIC24F read one byte from RingBuffer
// Return:   the first byte of incoming data available(or -1 if no data is available)
//******************************************************************************************
static char ReadByteFromRingBuffer()
{
    char dat = 0;
    int len = 1;
    if(IsRingBufferAvailable()<=0){
        return -1;
    }
    len = ringbuffer_read_len(&g_at_rbuf,&dat,len);
    return dat;
}

char ReadByteFromNetRingBuffer()
{
    char dat = 0;
    int len = 1;
    if(IsNetRingBufferAvailable()<=0){
        return -1;
    }
    len = ringbuffer_read_len(&g_net_rbuf,&dat,len);
    return dat;
}

bool WaitUartNetRxIdle()
{
    int size1 = 0;
    int size2 = 0;

    size1 = ringbuffer_buf_use_size(&g_net_rbuf);

    while (1) {
        delay_ms(50);
        size2 = ringbuffer_buf_use_size(&g_net_rbuf);

        if ((size1 == size2)) {// RX stopped for 50MS
            break;
        }

        size1 = size2;
    }

    return true;
}

static void CleanBuffer(void)
{
    memset(rxBuffer, '\0', RX_BUFFER_LENGTH);
    bufferHead = 0;
}

unsigned int ReadResponseByteToBuffer()
{
    char c = ReadByteFromRingBuffer();

    rxBuffer[bufferHead] = c;
    
    if (1 == gs_ftp_test) {
        printf("rxBuffer = %s\n", rxBuffer);
    }
    
    bufferHead = (bufferHead + 1) % RX_BUFFER_LENGTH;

#if 0// defined UART_DEBUG
    if (c == '\n'){
        printf("%c", c);
        printf("<--- ");
    } else {
        printf("%c", c);
    }
#endif

    return 1;
}

// Return: Only return after timeout
unsigned int ReadResponseToBuffer(unsigned int timeout)
{
    unsigned long start_time = GetTimeStamp();
    unsigned int recv_len = 0;

    CleanBuffer();

    if (888 == timeout) {
        unsigned int timeoutx = 1000;

        while (!isDelayTimeout(start_time, timeoutx)) {// 500ms
            if (IsRingBufferAvailable()) {
                recv_len += ReadResponseByteToBuffer();
                
                if (timeoutx < 20) {
                    timeoutx += 20;
                }
            }
        }        
    } else {
        while (!isDelayTimeout(start_time,timeout*1000)) {
            if (IsRingBufferAvailable()) {
                recv_len += ReadResponseByteToBuffer();
            }
        }
    }

    return recv_len;
}

// Return: Only return after timeout
static unsigned int ReadResponseToBufferUnclear(unsigned int timeout)
{
    unsigned long start_time = GetTimeStamp();
    unsigned int recv_len = 0;

    if (888 == timeout) {
        unsigned int timeoutx = 300;

        while (!isDelayTimeout(start_time, timeoutx)) {// 500ms
            if (IsRingBufferAvailable()) {
                recv_len += ReadResponseByteToBuffer();
                
                if (timeoutx < 20) {
                    timeoutx += 20;
                }
            }
        }        
    } else {
        while (!isDelayTimeout(start_time,timeout*1000)) {
            if (IsRingBufferAvailable()) {
                recv_len += ReadResponseByteToBuffer();
            }
        }
    }

    return recv_len;
}

static char *SearchChrBuffer(const char test_chr)
{
    int buf_len = strlen((const char *)rxBuffer);

    if (0<buf_len && buf_len < RX_BUFFER_LENGTH) {
        return strrchr((const char *)rxBuffer, test_chr);
    } else {
        return NULL;
    }
}

// Search the last words of the while reply
// AT Reply: XXXs + Keywords
//         : Keywords must be the end of the whole reply
// Return: Will return after got the Keywords or timeout
static Cmd_Response_t ReadResponseAndSearchChr(const char test_chr, unsigned int timeout)
{
    unsigned long start_time = GetTimeStamp();
    unsigned int recv_len = 0;

    CleanBuffer();
    while (!isDelayTimeout(start_time,timeout*1000)) {
        if (IsRingBufferAvailable()) {
            recv_len += ReadResponseByteToBuffer();
            if (SearchChrBuffer(test_chr)) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    if (recv_len > 0){
        return UNKNOWN_RESPONSE;
    } else {
        return TIMEOUT_RESPONSE;
    }
}


// The below call path will read reply into rxBuffer
// So SearchStrBuffer can search someting expect
// ReadResponseToBuffer
//   -->ReadResponseByteToBuffer
// SendAndSearchChr
//   -->ReadResponseAndSearchChr
//        -->ReadResponseByteToBuffer
// SendAndSearch
//   -->ReadResponseAndSearch
//        -->ReadResponseByteToBuffer
// SendAndSearch_multi
//   -->ReadResponseAndSearch_multi
//        -->ReadResponseByteToBuffer
static char *SearchStrBuffer(const char *test_str)
{
#if 1
    u16 i = 0;
    u16 valid_len = 150;

    for (i=0; i<RX_BUFFER_LENGTH; i++) {
        if (test_str[i] != 0) {
            valid_len = i;
        }
    }

    for (i=0; i<valid_len; i++) {
        if (rxBuffer[i] == test_str[0]) {
            if (0 == memcmp(rxBuffer+i, test_str, strlen(test_str))) {
                return (rxBuffer+i);
            }
        }
    }
    
    return NULL;
#else
    int buf_len = strlen((const char *)rxBuffer);

    if (buf_len < RX_BUFFER_LENGTH) {
        return strstr((const char *)rxBuffer, test_str);
    } else {
        return NULL;
    }
#endif
}

// Search the last words of the while reply
// AT Reply: XXXs + Keywords
//         : Keywords must be the end of the whole reply
// Return: Will return after got the Keywords or timeout
static Cmd_Response_t ReadResponseAndSearch(const char *test_str, unsigned int timeout)
{
    unsigned long start_time = GetTimeStamp();
    unsigned int recv_len = 0;
    CleanBuffer();
    while (!isDelayTimeout(start_time,timeout*1000UL)) {
        if (IsRingBufferAvailable()) {
            recv_len += ReadResponseByteToBuffer();
            if (SearchStrBuffer(test_str)) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    if (recv_len > 0) {
        printf("rxBuffer = %s\n", rxBuffer);
        printf("UNKNOWN_RESPONSE...\n");
        return UNKNOWN_RESPONSE;
    } else {
        printf("TIMEOUT_RESPONSE...\n");
        return TIMEOUT_RESPONSE;
    }
}

static Cmd_Response_t ReadResponseAndSearch_multi(const char *test_str, const char *e_test_str, unsigned int timeout)
{
    unsigned long start_time = GetTimeStamp();
    unsigned int recv_len = 0;
    unsigned int temp_len = 0;

    errorCode = -1;
    CleanBuffer();
    while (!isDelayTimeout(start_time,timeout*1000UL)) {
        if (IsRingBufferAvailable()) {
            temp_len = ReadResponseByteToBuffer();
//            printf("temp_len = %d\n", temp_len);
            recv_len += temp_len;
//            printf("recv_len = %d\n", recv_len);
            if (SearchStrBuffer(test_str)) {
                printf("\nSuccess Response\n");
                return SUCCESS_RESPONSE;
            } else if (SearchStrBuffer(e_test_str)) {
                start_time = GetTimeStamp();
                // only break when timeout
                // to ensure have got the whole infomation about error code
                while (!isDelayTimeout(start_time,1000UL)) {
                    if (IsRingBufferAvailable()) {
                        recv_len += ReadResponseByteToBuffer();
                    }
                }
                char *str_buf = SearchStrBuffer(": ");
                if (str_buf != NULL) {
                    char err_code[LEN_BYTE_SZ16+1];
                    strncpy(err_code, str_buf+2, LEN_BYTE_SZ16);
                    char *end_buf = strstr(err_code, "\r\n");
                    *end_buf = '\0';
                    errorCode = atoi(err_code);
                }
                printf("\nFail Response P1\n");
                return FAIL_RESPONSE;
            }

            // You may receive "ERROR" when waiting for "SEND_FAIL"
            if (0 == strcmp(e_test_str, RESPONSE_SEND_FAIL)) {
                if (SearchStrBuffer(RESPONSE_ERROR)) {
                    start_time = GetTimeStamp();
                    // TODO: to ensure if need to parse the fail code
                    // only break when timeout
                    // to ensure have got the whole infomation about error code
                    while (!isDelayTimeout(start_time,1000UL)) {
                        if (IsRingBufferAvailable()) {
                            recv_len += ReadResponseByteToBuffer();
                        }
                    }
                    char *str_buf = SearchStrBuffer(": ");
                    if (str_buf != NULL) {
                        char err_code[LEN_BYTE_SZ16+1];
                        strncpy(err_code, str_buf+2, LEN_BYTE_SZ16);
                        char *end_buf = strstr(err_code, "\r\n");
                        *end_buf = '\0';
                        errorCode = atoi(err_code);
                    }
                    printf("\nError Response\n");
                    return FAIL_RESPONSE;
                }
            }
        }
    }

    printf("recv_len = %d\n", recv_len);
    if (recv_len > 0){
        printf("\nFail Response P2\n");
        return FAIL_RESPONSE;
    } else {
        printf("\nTimeout Response\n");
        return TIMEOUT_RESPONSE;
    }
}

// Only used for get '>' when after AT+SEND before send datas
Cmd_Response_t SendAndSearchChr(const char *command, const char test_chr, unsigned int timeout)
{
    int i = 0;

    for (i = 0; i < 3; i++) {
        if (SendATcommand(command)) {
            if (ReadResponseAndSearchChr(test_chr, timeout) == SUCCESS_RESPONSE) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    return TIMEOUT_RESPONSE;
}

Cmd_Response_t SendAndSearch(const char *command, const char *test_str, unsigned int timeout)
{
    int i = 0;

    for (i = 0; i < 3; i++) {
        if (SendATcommand(command)) {
            if (ReadResponseAndSearch(test_str, timeout) == SUCCESS_RESPONSE) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    return TIMEOUT_RESPONSE;
}

Cmd_Response_t SendAndSearch_multi(const char *command, const char *test_str, const char *e_test_str, unsigned int timeout)
{
    int i = 0;
    Cmd_Response_t resp_status = UNKNOWN_RESPONSE;

    for (i = 0; i < 3; i++) {
        if (SendATcommand(command)) {
            resp_status = ReadResponseAndSearch_multi(test_str, e_test_str, timeout);
            return resp_status;
        }
    }
    return resp_status;
}

bool GetDevVersion(char *ver, u8 buf_len)
{
    if (SUCCESS_RESPONSE == SendAndSearch(DEV_VERSION, RESPONSE_OK, 2)) {
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strncpy(ver, rxBuffer, buf_len);
        return true;
    }
    return false;
}

static bool GetDevIMEI(void)
{
    if (SUCCESS_RESPONSE == SendAndSearch(DEV_IMEI, RESPONSE_OK, 2)) {
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        memset(g_imei_str, 0, LEN_COMMON_USE);
        strncpy((char*)g_imei_str, (const char*)rxBuffer, LEN_COMMON_USE);
        printf("g_imei_str = %s\n", g_imei_str);

        return true;
    }

    return false;
}

static bool DevSimPIN(char *pin, Cmd_Status_t status)
{
    char cmd[LEN_BYTE_SZ16] = "";

    strncpy(cmd, DEV_SIM_PIN, LEN_BYTE_SZ16);
    if (status == READ_MODE) {
        strcat(cmd, "?");
        if (SendAndSearch(cmd, "READY",2)) {
            //pin = "READY";
            return true;
        }
    } else if (status == WRITE_MODE) {
        char buf[LEN_BYTE_SZ16+1] = "";
        sprintf(buf, "=\"%s\"", pin);
        strcat(cmd, buf);
        if (SendAndSearch(cmd, RESPONSE_OK, 2)) {
            return true;
        }
    }
    return false;
}

bool GetDevRSSI(void)
{
    u8 i = 0;
    u8 len = 0;

    memset(g_rssi_str, 0, LEN_COMMON_USE);

    g_rssi_str[0] = 'F';
    if (SUCCESS_RESPONSE == SendAndSearch(DEV_NET_RSSI, RESPONSE_OK, 2)) {
        char *sta_buf = SearchStrBuffer(": ");

        len = strlen(sta_buf);
        for (i=0; i<LEN_COMMON_USE; i++) {

            if (i > len) {
                break;
            }

            if (',' == sta_buf[i+2]) {
                break;
            }

            g_rssi_str[i] = sta_buf[i+2];
        }

        printf("g_rssi_str = %s\n", g_rssi_str);

        return true;
    }

    return false;
}

static bool GetDevSimICCID(void)
{
    if (SUCCESS_RESPONSE == SendAndSearch(DEV_SIM_ICCID, RESPONSE_OK, 2)) {
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = SearchStrBuffer(": ");
        memset(g_iccid_str, 0, LEN_COMMON_USE);
        strncpy((char*)g_iccid_str, sta_buf+2, LEN_COMMON_USE);
        printf("g_iccid_str = %s\n", g_iccid_str);

        return true;
    }

    return false;
}

static bool GetCurrentTimeZone(void)
{
    char cmd[LEN_BYTE_SZ16+1] = "";

    strncpy(cmd, DEV_TIME_ZONE, LEN_BYTE_SZ16);
    strcat(cmd, "?");

    if (SUCCESS_RESPONSE == SendAndSearch(cmd, RESPONSE_OK, 2)) {
        u8 i = 0;
        u8 j = 0;
        u8 k = 0;
        u8 m = 0;
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = SearchStrBuffer(": ");
        memset(g_devzone_str, 0, LEN_COMMON_USE);
        memset(g_devtime_str, 0, LEN_COMMON_USE);

        for (i=0; i<strlen(sta_buf); i++) {
            if ('"' == sta_buf[i]) {
                j++;
                continue;
            } else {
                if (1 == j) {
                    if ((sta_buf[i]>='0') && (sta_buf[i]<='9')) {
                        g_devtime_str[k++] = sta_buf[i];

                        if (12 == k) {
                            continue;
                        }
                    }

                    if (k >= 12) {
                        g_devzone_str[m++] = sta_buf[i];
                    }
                } else {
                    k = 0;
                }
            }
        }

        printf("\ndevzone = %s\n", g_devzone_str);
        printf("devtime = %s\n", g_devtime_str);

        return true;
    }

    return false;
}

static Net_Status_t DevNetRegistrationStatus()
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    Net_Status_t n_status = NOT_REGISTERED;

    strncpy(cmd, DEV_NET_STATUS_G, LEN_BYTE_SZ16);
    strcat(cmd, "?");
    if (SendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = SearchChrBuffer(',');
        n_status = atoi(sta_buf + 1);
        switch (n_status)
        {
            case REGISTERED:
            case REGISTERED_ROAMING:
                return n_status;
            default:
                break;
        }
    }

    strncpy(cmd, DEV_EPS_NET_STATUS, LEN_BYTE_SZ16);
    strcat(cmd, "?");
    if (SendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = SearchChrBuffer(',');
        n_status = atoi(sta_buf + 1);
        switch (n_status)
        {
            case REGISTERED:
            case REGISTERED_ROAMING:
                return n_status;
            default:
                break;
        }
    }
    return n_status;
}

/////////////////////////////////// BG96 TCPIP ///////////////////////////////////
static bool SetDevAPNParameters(unsigned int pdp_index, Protocol_Type_t type, char *apn, char *usr, char *pwd, Authentication_Methods_t met)
{
    char cmd[LEN_BYTE_SZ64+1] = "";
    char buf[LEN_BYTE_SZ64+1] = "";

    strncpy(cmd, APN_PARAMETERS, LEN_BYTE_SZ64);
    sprintf(buf, "=%d,%d,\"%s\",\"%s\",\"%s\",%d", pdp_index, type, apn, usr, pwd, met);
    strcat(cmd, buf);
    if (SendAndSearch(cmd, RESPONSE_OK, 2)) {
        return true;
    }
    return false;
}

static Cmd_Response_t ActivateDevAPN(unsigned int pdp_index)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    char buf[LEN_BYTE_SZ8+1] = "";

    strncpy(cmd, ACTIVATE_APN, LEN_BYTE_SZ16);
    sprintf(buf, "=%d", pdp_index);
    strcat(cmd, buf);
    return SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 150);
}

static bool GetDevAPNIPAddress(unsigned int pdp_index, char *ip)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    char buf[LEN_BYTE_SZ8+1] = "";

    strncpy(cmd, GET_APN_IP_ADDRESS, LEN_BYTE_SZ16);
    sprintf(buf, "=%d", pdp_index);
    strcat(cmd, buf);
    if (SendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = SearchChrBuffer(',');
        if (strcmp(sta_buf + 1, "0.0.0.0") <= 0) {
            return false;
        }
        strncpy(ip, sta_buf+1, LEN_BYTE_SZ16);
        return true;
    }
    return false;
}

static bool InitAPN(unsigned int pdp_index, char *apn, char *usr, char *pwd, char *err_code, u8 buf_len)
{
    Net_Status_t i_status;
    Cmd_Response_t init_status;
    const char *e_str;
    unsigned long start_time = GetTimeStamp();

    while (!DevSimPIN("", READ_MODE)) {
        if (isDelayTimeout(start_time,10*1000UL)) {
            e_str = "\r\nAPN ERROR: No SIM card detected!\r\n";
            strncpy(err_code, e_str, buf_len);
            return false;
        }
    }
    start_time = GetTimeStamp();
    while (i_status != REGISTERED && i_status != REGISTERED_ROAMING) {
        i_status = DevNetRegistrationStatus();
        if (isDelayTimeout(start_time,120*1000UL)) {
            e_str = "\r\nAPN ERROR: Can't registered to the Operator network!\r\n";
            strncpy(err_code, e_str, buf_len);
            return false;
        }
    }
    start_time = GetTimeStamp();
    while (!isDelayTimeout(start_time,3000UL)) {
        if (SetDevAPNParameters(pdp_index, IPV4, apn, usr, pwd, PAP_OR_CHAP))
        {
            char i_ip[LEN_BYTE_SZ16+1] = "";
            if (GetDevAPNIPAddress(pdp_index, i_ip)) {
                sprintf(err_code, "\r\nAPN OK: The IP address is %s\r\n", i_ip);
                return true;
            } else {
                init_status = ActivateDevAPN(pdp_index);
                if (init_status == TIMEOUT_RESPONSE) {
                    e_str = "\r\nAPN ERROR: Please reset your device!\r\n";
                    strncpy(err_code, e_str, buf_len);
                    return false;
                }
            }
        }
        e_str = "\r\nAPN ERROR: Activate APN file!\r\n";
        strncpy(err_code, e_str, buf_len);
    }
    return false;
}

static bool OpenSocketService(unsigned int pdp_index, unsigned int socket_index, Socket_Type_t socket, char *ip, char *port, unsigned int local_port, Access_Mode_t mode)
{
    char cmd[LEN_BYTE_SZ128+1] = "";
    char buf[LEN_BYTE_SZ128+1] = "";

    strncpy(cmd, OPEN_SOCKET, LEN_BYTE_SZ128);
    switch (socket)
    {
        case TCP_CLIENT:
            sprintf(buf, "=%d,%d,\"TCP\",\"%s\",%s,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        case TCP_SEVER:
            sprintf(buf, "=%d,%d,\"TCP LISTENER\",\"%s\",%s,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        case UDP_CLIENT:
            sprintf(buf, "=%d,%d,\"UDP\",\"%s\",%s,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        case UDP_SEVER:
            sprintf(buf, "=%d,%d,\"UDP SERVICE\",\"%s\",%s,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        default:
            return false;
    }
    strcat(cmd, buf);
    switch (mode)
    {
        case BUFFER_MODE:
        case DIRECT_PUSH_MODE:
            if (SendAndSearch_multi(cmd, OPEN_SOCKET, RESPONSE_ERROR, 150) > 0) {
                unsigned long start_time = GetTimeStamp();
                while (!isDelayTimeout(start_time,1000UL)) {
                    if (IsRingBufferAvailable()) {
                        ReadResponseByteToBuffer();
                    }
                }
                errorCode = -1;
                char *sta_buf = SearchChrBuffer(',');

                printf("sta_buf:%s\r\n",sta_buf);
                if (0 == atoi(sta_buf + 1)) {
                    return true;
                } else {
                    errorCode = atoi(sta_buf + 1);
                }
            }
            break;
        case TRANSPARENT_MODE:
            if (SendAndSearch_multi(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 150) > 0) {
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool CloseSocketService(unsigned int socket_index)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    char buf[LEN_BYTE_SZ8+1] = "";

    strncpy(cmd, CLOSE_SOCKET, LEN_BYTE_SZ16);
    sprintf(buf, "=%d", socket_index);
    if (SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 150)) {
        return true;
    }
    return false;
}

static bool SocketSendData(unsigned int socket_index, Socket_Type_t socket, char *data_buf, char *ip, unsigned int port)
{
    char cmd[LEN_BYTE_SZ512+1] = "";
    char buf[LEN_BYTE_SZ512+1] = "";

    strncpy(cmd, SOCKET_SEND_DATA, LEN_BYTE_SZ512);
    switch (socket)
    {
        case TCP_CLIENT:
        case TCP_SEVER:
        case UDP_CLIENT:
            sprintf(buf, "=%d,%d", socket_index, strlen(data_buf));
            break;
        case UDP_SEVER:
            sprintf(buf, "=%d,%d,\"%s\",%d", socket_index, strlen(data_buf), ip, port);
            break;
        default:
            return false;
    }
    strcat(cmd, buf);

    if (SendAndSearchChr(cmd, '>', 2)) {// 2s
        if (SendDataAndCheck(data_buf, RESPONSE_SEND_OK, RESPONSE_SEND_FAIL, 10)) {
            return true;
        }
    }

    return false;
}

bool BG96TcpSendHead(unsigned int socket_index, Socket_Type_t socket)
{
    char cmd[LEN_BYTE_SZ64+1] = "";
    char buf[LEN_BYTE_SZ64+1] = "";

    strncpy(cmd, SOCKET_SEND_DATA, LEN_BYTE_SZ64);
    switch (socket)
    {
        case TCP_CLIENT:
        case UDP_CLIENT:
            sprintf(buf, "=%d", socket_index);
            break;
        default:
            return false;
    }
    strcat(cmd, buf);

    if (SendAndSearchChr(cmd, '>', 2)) {// 2s
        return true;
    }

    return false;
}

bool BG96TcpSendMiddle(char *data_buf)
{
    // to read recv fifo till empty
    while (ReadByteFromRingBuffer() >= 0);

    if (SendATcommand(data_buf)) {
        return true;
    }

    return false;
}

bool BG96TcpSendTail(void)
{
    // to read recv fifo till empty
    while (ReadByteFromRingBuffer() >= 0);

    Uart2_Putc(0x1A);

    if (ReadResponseAndSearch_multi(RESPONSE_SEND_OK, RESPONSE_SEND_FAIL, 10)) {
        return true;
    }

    return false;
}

bool BG96TcpSendCancel(void)
{
    Uart2_Putc(0x1B);

    if (ReadResponseAndSearch(RESPONSE_SEND_OK, 1000)) {
        return true;
    }

    return false;
}

/////////////////////////////////// BG96 Serial ///////////////////////////////////
void InitSerial()
{
    CleanBuffer();
}

bool SendDataAndCheck(const char *data_buf, const char *ok_str, const char *err_str, unsigned int timeout)
{
    DelayMs(100);

    // to read recv fifo till empty
    while (ReadByteFromRingBuffer() >= 0);

    int data_len = strlen(data_buf);

    printf("SND: ");
    int send_bytes = WriteToBG96(data_buf);
    printf("===\r\n");
#ifdef UART_DEBUG
    printf("\r\n");
    printf("%s", data_buf);
    printf("\r\n");
    printf("Send Data len :");
    printf("%d", send_bytes);
    printf("\r\n");
#endif

    if (send_bytes == data_len) {
        if (ReadResponseAndSearch_multi(ok_str, err_str, timeout)) {
            return true;
        }
    }

    return false;
}


bool SendATcommand(const char *command)
{
    DelayMs(100);
    // to read recv fifo till empty
    while (ReadByteFromRingBuffer() >= 0);
    printf("SND: ");
    WriteToBG96("AT");

    int cmd_len = strlen(command);
    int send_bytes = WriteToBG96(command);

#if defined UART_DEBUG
    printf("\r\n");
    printf("-> ");
    printf("AT");
    printf("%s", command);
    printf("\r\n");
#endif
    if (send_bytes != cmd_len){
        return false;
    }
    WriteToBG96("\r\n");

    printf("===\r\n");

    return true;
}

bool ReturnErrorCode(int *s_err_code)
{
    *s_err_code = -1;

    if (errorCode != -1) {
        *s_err_code = errorCode;
        return true;
    }

    return false;
}

bool TurnOnGNSS(GNSS_Work_Mode_t mode, Cmd_Status_t status)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    char buf[LEN_BYTE_SZ8+1] = "";

    strncpy(cmd, GNSS_TURN_ON, LEN_BYTE_SZ16);

    if (status == READ_MODE){
        strcat(cmd, "?");
        if(SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
            char *sta_buf = SearchStrBuffer(": ");
            char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
            *end_buf = '\0';
            if(atoi(sta_buf + 2) == 1){
                return true;
            }
        }
    }else if (status == WRITE_MODE){
        sprintf(buf, "=%d", mode);
        strcat(cmd, buf);
        if(SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
            return true;
        }
    }
    return false;
}

bool TurnOffGNSS(void)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    strncpy(cmd, GNSS_TURN_OFF, LEN_BYTE_SZ16);

    if (SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        return true;
    }
    return false;
}

bool TurnTryLocate(void)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    strncpy(cmd, "+QGPSLOC?", LEN_BYTE_SZ16);

    if (SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        return true;
    }
    return false;
}

bool TurnOnGNSSDamon(void)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    strncpy(cmd, GNSS_TURN_ON, LEN_BYTE_SZ16);

    strcat(cmd, "=1");
    if (SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        return true;
    }
    return false;
}

static bool GetGNSSPositionInformation(char *position)
{
    char cmd[LEN_BYTE_SZ16+1] = "";
    strncpy(cmd, GNSS_GET_POSITION, LEN_BYTE_SZ16);

    strcat(cmd, "=2");
    if(SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = SearchStrBuffer(": ");
        strncpy(position, sta_buf+2, LEN_BYTE_SZ128);
        return true;
    }
    return false;
}

bool CloseTcpService(void)
{
    const char *cmd = "+QICLOSE=0";

    if(SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        gs_net_sta = 0x80;
        return true;
    }
    return false;
}

bool CloseFtpService(void)
{
    const char *cmd = "+QFTPCLOSE";

    if(SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return true;
    }
    return false;
}

static bool QueryNetStatus(void)
{
    const char *cmd = "+CGATT?";

    if (SUCCESS_RESPONSE == SendAndSearch(cmd, "+CGATT: 1", 2)) {
//        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);

        return true;
    }

    return false;
}

static bool SetDevCommandEcho(bool echo)
{
    const char *cmd;

    if (echo == true) {
        cmd = "E1";
    } else {
        cmd = "E0";
    }
    if (SUCCESS_RESPONSE == SendAndSearch(cmd, RESPONSE_OK, 2)) {
        return true;
    }
    return false;
}

bool QueryNetMode(void)
{
    u8 i = 0;

    if (SUCCESS_RESPONSE == SendAndSearch(DEV_NET_INFORMATION, RESPONSE_OK, 5)) {
        char *end_buf = SearchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = SearchStrBuffer(": ");
 
        memset(g_net_mode, 0, LEN_COMMON_USE);        
        if (sta_buf != NULL) {
            for (i=3; i<strlen(sta_buf); i++) {
                if (',' == sta_buf[i]) {
                    g_net_mode[i-3-1] = 0;
                    break;
                }
                g_net_mode[i-3] = sta_buf[i];
            }
        }

        printf("g_net_mode = %s\n", g_net_mode);

        return true;
    }

    return false;
}

static bool SetAutoNetMode(void)
{
    const char *cmd;

    cmd = "+QCFG=\"NWSCANSEQ\"";;

    if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }
    cmd = "+QCFG=\"NWSCANMODE\"";

    if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }
    cmd = "+QCFG=\"IOTOPMODE\"";

    if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }

    cmd = "+QCFG=\"BAND\",F,400A0E189F,A0E189F,1";

    if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }

    // 00-Automatic = 020301
    // 01-GSM
    // 02-LTE CAT M1
    // 03-LTE CAT NB1
    cmd = "+QCFG=\"NWSCANSEQ\",030201,1";
//    cmd = "+QCFG=\"NWSCANSEQ\",02,1";

    if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }

    // 0-Automatic
    // 1-GSM Only
    // 3-LTE Only
//   cmd = "+QCFG=\"NWSCANMODE\",1,1";
   cmd = "+QCFG=\"NWSCANMODE\",0,1";
//   cmd = "+QCFG=\"NWSCANMODE\",3,1";

   if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }

   // 0-LTE CAT M1
   // 1-LTE Cat NB1
   // 2-LTE Cat M1 and Cat NB1
   cmd = "+QCFG=\"IOTOPMODE\",2,1";

    if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }

   cmd = "+QICSGP=1,1,\"sentinel.m2mmobi.be\",\"\",\"\",1";

    if (SendAndSearch(cmd, RESPONSE_OK, 2) != SUCCESS_RESPONSE) {
        return false;
    }

    return true;
}

bool BG96ATInitialize(void)
{
    int trycnt = 10;

    printf("This is the Mobit Debug Serial!\n");

    while(trycnt--) {
        if (SetDevCommandEcho(false)) {
            break;
        }
    }

    if (trycnt < 1) {
        return false;
    }

    delay_ms(10000);

    QueryNetMode();

    trycnt = 50;
    while(trycnt--) {
        if (true == QueryNetStatus()) {
            break;
        }
    }

    if (trycnt < 1) {
        SetAutoNetMode();
        asm("reset");
        return false;
    }

    gs_net_sta = 0x40;

    trycnt = 10;
    while(trycnt--) {
        if (true == GetDevIMEI()) {
            break;
        }
    }

    if (trycnt < 1) {
        return false;
    }

    trycnt = 10;
    while(trycnt--) {
        if (true == GetDevSimICCID()) {
            break;
        }
    }

    if (trycnt < 1) {
        return false;
    }

    trycnt = 10;
    while(trycnt--) {
        if (true == GetCurrentTimeZone()) {
            break;
        }
    }

    if (trycnt < 1) {
        return false;
    }

    TurnOffGNSS();
    TurnOnGNSS(STAND_ALONE, WRITE_MODE);

    return true;
}

bool BG96TcpSend(char* send_buf)
{
    if (NULL == send_buf) {
        return false;
    }

    if (gs_net_sta != 0x81) {
        return false;
    }

    unsigned int comm_socket_index = 0;  // The range is 0 ~ 11
    Socket_Type_t socket = TCP_CLIENT;

    if(SocketSendData(comm_socket_index, socket, (char *)send_buf, "", 88)){
        printf("Socket Send Data Success!\n");

        return true;
    } else {
        return false;
    }
}

bool ConnectToTcpServer(u8* svr_ip, u8* svr_port, u8* svr_apn)
{
    u8 trycnt = 10;

    unsigned int comm_pdp_index = 1;  // The range is 1 ~ 16
    unsigned int comm_socket_index = 0;  // The range is 0 ~ 11
//    Socket_Type_t socket = TCP_CLIENT;
    Socket_Type_t socket = UDP_CLIENT;

    char apn_error[LEN_BYTE_SZ64+1] = "";

    while(trycnt--) {
        if (InitAPN(comm_pdp_index, (char *)svr_apn, "", "", apn_error, LEN_BYTE_SZ64)) {
            break;
        }

        printf("apn_error :%s\n", apn_error);
    }

    if (trycnt < 1) {
        return false;
    }

    CloseTcpService();

    trycnt = 10;
    while(trycnt--) {
        if (OpenSocketService(comm_pdp_index, comm_socket_index, socket, (char *)svr_ip, (char *)svr_port, 0, DIRECT_PUSH_MODE)){
            break;
        }

        printf("Open Socket Service Fail!\n");
        CloseTcpService();
    }

    if (trycnt < 1) {
        return false;
    }

    gs_net_sta = 0x81;
    printf("Open Socket Service Success!\n");

    GetCurrentTimeZone();
    StartCommunication();

    return true;
}

/////////////////////////////////// BG96 FTP ///////////////////////////////////
bool OpenFtpSession(unsigned int pdp_index, u8* ftp_ip, u8* ftp_port)
{
    char cmd[LEN_BYTE_SZ64+1] = "";
    char buf[LEN_BYTE_SZ32+1] = "";

    if ((NULL==ftp_ip) || (NULL==ftp_port)) {
        return false;
    }

    strncpy(cmd, FTP_CONFIG_PARAMETER, LEN_BYTE_SZ64);
    sprintf(buf, "=\"contextid\",%d", pdp_index);
    strcat(cmd, buf);
    if(!SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }

    memset(cmd, '\0', LEN_BYTE_SZ64);
    strncpy(cmd, FTP_CONFIG_PARAMETER, LEN_BYTE_SZ64);
    // strcat(cmd, "=\"account\",\"lide\",\"Lide2019!@#\"");    
    // strcat(cmd, "=\"account\",\"Administrator\",\"AInijia88443628._\"");
    strcat(cmd, "=\"account\",\"upgrade\",\"SentineL_UpFm_Ftp\"");
    if(!SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }

    memset(cmd, '\0', LEN_BYTE_SZ64);
    strncpy(cmd, FTP_CONFIG_PARAMETER, LEN_BYTE_SZ64);
    strcat(cmd, "=\"filetype\",1");
    if(!SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }

    memset(cmd, '\0', LEN_BYTE_SZ64);
    strncpy(cmd, FTP_CONFIG_PARAMETER, LEN_BYTE_SZ64);
    // strcat(cmd, "=\"transmode\",1");
    strcat(cmd, "=\"transmode\",0");
    if(!SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }

    memset(cmd, '\0', LEN_BYTE_SZ64);
    strncpy(cmd, FTP_CONFIG_PARAMETER, LEN_BYTE_SZ64);
    strcat(cmd, "=\"rsptimeout\",90");
    if(!SendAndSearch_multi(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }

    memset(buf, '\0', LEN_BYTE_SZ32);
    memset(cmd, '\0', LEN_BYTE_SZ64);
    strncpy(cmd, FTP_OPEN_SESSION, LEN_BYTE_SZ64);
    sprintf(buf, "=\"%s\",%s", ftp_ip, ftp_port);
    strcat(cmd, buf);
    if(SendAndSearch_multi(cmd, "+QFTPOPEN: 0,0", RESPONSE_ERROR, 30) != SUCCESS_RESPONSE){
        printf("FTPOPEN failed...\n");
        return false;
    }

    return true;
}

bool ConnectToFtpServer(u8* iap_file, u8* ftp_ip, u8* ftp_port)
{
    u8 trycnt = 10;
    unsigned int pdp_id = 1;  // The range is 1 ~ 16

    if (NULL == iap_file) {
        return false;
    }

    if ((NULL==ftp_ip) || (NULL==ftp_port)) {
        return false;
    }

    CloseFtpService();

    while(trycnt--) {
        if (OpenFtpSession(pdp_id, ftp_ip, ftp_port)){
            break;
        }

        printf("Open FTP Service Fail!\n");
        CloseFtpService();
    }

    if (trycnt < 1) {
        return false;
    }

    // Close TCP temporarily
    // After FTP download finished(whatever success or failed))
    // Need to Re-OpenTCP or Reset directly
    CloseTcpService();

    gs_ftp_sta = 0x81;

    printf("Open FTP Service Success!\n");

    return true;
}

u16 BG96FtpGetData(u32 offset, u32 length, u8* iap_buf, u8* iap_file)
{
    u16 i = 0;
    char cmd[LEN_BYTE_SZ64+1] = "";
    char buf[LEN_BYTE_SZ32+1] = "";
    char size_str[8]= "";

    u16 size_pos = 0;
    u16 size_got = 0;

    static u8 trycnt = 0;

    if ((NULL==iap_buf) || (NULL==iap_file)) {
        return 0;
    }

//    gs_ftp_test = 1;

    strncpy(cmd, FTP_DOWNLOAD_DAT, LEN_BYTE_SZ64);
    // sprintf(buf, "=\"test.mp3\",\"COM:\",%ld,%ld", offset, length);
    sprintf(buf, "=\"%s\",\"COM:\",%ld,%ld", iap_file, offset, length);
    strcat(cmd, buf);

    if(!SendAndSearch_multi(cmd, "CONNECT\r\n", RESPONSE_ERROR, 30)){
        // ftp error
        if (errorCode > 600) {
            printf("---------errorCode = %d\n", errorCode);
            gs_ftp_sta = 0;
        }

        // other error
        if (++trycnt >= 2) {
            trycnt = 0;
            gs_ftp_sta = 0;
        }

        return 0;
    }

    trycnt = 0;
    ReadResponseToBuffer(888);

    printf("FTPGET Recv %d Bytes: %.2X%.2X%.2X\n", bufferHead, rxBuffer[0], rxBuffer[1], rxBuffer[2]);

    memset(size_str, 0, 8);
    for (i=0; i<bufferHead; i++) {
        if (('F'==rxBuffer[i+0]) && ('T'==rxBuffer[i+1]) && ('P'==rxBuffer[i+2]) &&
            ('G'==rxBuffer[i+3]) && (':'==rxBuffer[i+6]) && ('0'==rxBuffer[i+8])) {
            size_pos = i+10;
            printf("FTP DW size=%s\n", rxBuffer+size_pos);
        }

        if ((size_pos!=0) && (i>=size_pos)) {
            if (('\r'==rxBuffer[i+0]) && ('\n'==rxBuffer[i+1])) {
                break;
            }
            size_str[i-size_pos] = rxBuffer[i];
        }
    }

    if (size_pos != 0) {
        size_got = atoi(size_str);
        memcpy((char*)iap_buf, (const char*)rxBuffer, size_got);
    }

    printf("FTP Got Firm size: %d Bytes\n", size_got);

    return size_got;
}

u8 GetNetStatus(void)
{
    return gs_net_sta;
}

u8 GetFtpStatus(void)
{
    return gs_ftp_sta;
}

void SetNetStatus(u8 sta)
{
    gs_net_sta = sta;
}

void GetGPSInfo(char* gnss_part)
{
    u8 i = 0;
    u8 k = 0;
    u8 m = 0;

    if (NULL == gnss_part) {
        return;
    }

    memset(gs_gnss_pos, 0, LEN_BYTE_SZ128);
    if (!GetGNSSPositionInformation((char *)gs_gnss_pos)){
        printf("Get the GNSS Position Fail!\n");
        int e_code;
        if (ReturnErrorCode(&e_code)){
            printf("ERROR CODE: %d\n", e_code);
            printf("Please check the documentation for error details.\n");
        }

        return;
    }

    printf("Get the GNSS Position Success!\n");

    memset(gnss_part, 0, LEN_BYTE_SZ128);
    for (i=0; i<strlen((const char*)gs_gnss_pos); i++) {
        if (',' == gs_gnss_pos[i]) {
            k++;

            if ((2==k) || (3==k) || (7==k)) {
                gnss_part[m++] = '|';
            }

            continue;
        }

        if ((1==k) || (2==k) || (6==k) || (7==k)) {
            gnss_part[m++] = gs_gnss_pos[i];
        }
    }

    printf("gs_gnss_pos = %s\n", gs_gnss_pos);
    printf("gnss_defi = %s\n", gnss_part);
}

/////////////////////////////////// BG96 Common ///////////////////////////////////
static bool InitModule()
{
//    GPIOx_Config(BANKB, 11, OUTPUT_DIR);// RESET
//    GPIOx_Output(BANKB, 11, 0);
//    delay_ms(300);
//    GPIOx_Output(BANKB, 11, 1);

    GPIOx_Config(BANKB, 8, OUTPUT_DIR);// AP_READY
    GPIOx_Output(BANKB, 8, 0);

    GPIOx_Config(BANKB, 9, OUTPUT_DIR);// PWRKEY
    GPIOx_Output(BANKB, 9, 0);
    delay_ms(2000);
    GPIOx_Output(BANKB, 9, 1);
    
    printf("TRISB=%.4X\n", TRISB);

    return true;
}

void BG96_PowerUp(void)
{
    InitModule();
}

//******************************************************************************
// Configure BG96
//******************************************************************************
void Configure_BG96(void)
{
    u8 trycnt = 2;
//    bool resultBool = false;

    while(trycnt--) {
//        resultBool = InitModule();
//        if(resultBool){
//            printf("init bg96 success!\r\n");
//        }else{
//            printf("init bg96 failure!\r\n");
//        }

        if (BG96ATInitialize()) {
            break;
        }

        delay_ms(1000);
    }
}

static u32 ParseFTPFileSize(void)
{
    u32 i = 0;
    u32 size_pos = 0;
    u32 size_got = 0;
    char size_str[LEN_BYTE_SZ8+1] = "";

    memset(size_str, 0, 8);
    printf("FTP DW size=%s\n", rxBuffer);
    for (i=0; i<bufferHead; i++) {
        if (('F'==rxBuffer[i+0]) && ('T'==rxBuffer[i+1]) && ('P'==rxBuffer[i+2]) &&
            ('S'==rxBuffer[i+3]) && (':'==rxBuffer[i+7]) && ('0'==rxBuffer[i+9])) {
            size_pos = i+11;
            printf("FTP DW size=%s\n", rxBuffer+size_pos);
        }

        if ((size_pos!=0) && (i>=size_pos)) {
            if (('\r'==rxBuffer[i+0]) && ('\n'==rxBuffer[i+1])) {
                break;
            }
            size_str[i-size_pos] = rxBuffer[i];
        }
    }

    printf("iap_size1 = %s\n", size_str);
    if (size_pos != 0) {
        for (i=0; i<strlen(size_str); i++) {
            if ((size_str[i]<'0') || (size_str[i]>'9')) {
                break;
            }
            size_got *= 10;
            size_got += size_str[i] - '0';
        }
    }

    printf("iap_size2 = %ld\n", size_got);

    return size_got;
}


u32 GetFTPFileSize(u8* iap_file)
{
    char cmd[LEN_BYTE_SZ64+1] = "";
    char buf[LEN_BYTE_SZ32+1] = "";

    if (NULL == iap_file) {
        return 0;
    }

    memset(cmd, '\0', LEN_BYTE_SZ64);
    memset(buf, '\0', LEN_BYTE_SZ32);
    strncpy(cmd, FTP_GET_FIL_SIZE, LEN_BYTE_SZ64);
    sprintf(buf, "=\"%s\"", iap_file);
    strcat(cmd, buf);
    if (!SendAndSearch_multi(cmd, "+QFTPSIZE: 0,", RESPONSE_ERROR, 30)) {
        return 0;
    }

    ReadResponseToBufferUnclear(888);

    return ParseFTPFileSize();
}

//******************************************************************************
//* END OF FILE
//******************************************************************************
