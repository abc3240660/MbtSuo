//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * A library for PIC24F UART1(for debug) & UART2(for BG96)
 * This file is about the UART API of PIC24
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 03/11/2019
******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <p24fxxxx.h>

#include "001_Tick_10ms.h"
#include "003_BG96.h"
#include "007_Uart.h"
#include "008_RingBuffer.h"

#define U4BufSize   LEN_BYTE_SZ64

static u16 Uart3_Rx_Len = 0;
static u8 Uart3_Rx_Buffer[LEN_BYTE_SZ64+1] = {0};

u8 U4RxBuf[U4BufSize] = {0};
u8 *pU4Start;
u8 *pU4End;

static char Uart2_Send_Buf[UART2_BUFFER_MAX_LEN] = {0};

static ringbuffer_t tmp_rbuf;

// ringbuf for temp recved
static char tmpRingbuf[RX_RINGBUF_MAX_LEN] = {0};

u32 g_ftp_dat_cnt = 0;
u8 g_ftp_buf[1536] = {0};

// extern u8 g_ftp_enable;
extern int rx_debug_flag;

// 115200 for Debug
void Uart1_Init(void)
{
    _RP22R = 3;// RD3
#if UART1_MANUAL_DBG
    _U1RXR = 24;// RD1
#endif
    _LATD3 = 1;
#if UART1_MANUAL_DBG
    _TRISD1 = 1;
#endif
    _TRISD3 = 0;

    U1MODE = 0x8808;
    U1STA = 0x2400;

#ifdef OSC_32M_USE
    // 4M/(34+1) = 114285
    U1BRG = 34;
#else// OSC_20M_USE
    U1BRG = 21;
#endif

    _U1TXIP = IPL_DIS;
#if UART1_MANUAL_DBG
    _U1RXIP = IPL_MID;
#endif
    _U1TXIF = 0;
    _U1RXIF = 0;
    _U1TXIE = 0;
#if UART1_MANUAL_DBG
    _U1RXIE = 1;
#endif
}

// 115200 for BG96
void Uart2_Init(void)
{
    _RP14R = 5;// RB14
    _U2RXR = 21;// RG6

    _LATB14 = 1;
    _TRISG6 = 1;
    _TRISB14 = 0;

    _ANSG6 = 0;
    _ANSB14 = 0;

    U2MODE = 0x8808;
    U2STA = 0x2400;

#ifdef OSC_32M_USE
    // 4M/(34+1) = 114285
    U2BRG = 34;
#else// OSC_20M_USE
    U2BRG = 21;
#endif

    _U2TXIP = IPL_DIS;
    _U2RXIP = IPL_MID;
    _U2TXIF = 0;
    _U2RXIF = 0;
    _U2TXIE = 0;
    _U2RXIE = 1;
    
    ringbuffer_init(&tmp_rbuf,tmpRingbuf,RX_RINGBUF_MAX_LEN);
}

// 115200 for CLRC663
void Uart3_Init(void)
{
    _RP18R = 19;     //RB5 = U3TX
    _U3RXR = 13;     //RB2 = U3RXR

    _LATB5 = 1;
    _TRISB2 = 1;
    _TRISB5 = 0;

    _ANSB2 = 0;
    _ANSB5 = 0;

    U3MODE = 0x8808;
    U3STA = 0x2400;

#ifdef OSC_32M_USE
    // 4M/(34+1) = 114285
    U3BRG = 34;
#else// OSC_20M_USE
    U3BRG = 21;
#endif

    _U3TXIP = IPL_DIS;
    _U3RXIP = IPL_MID;
    _U3TXIF = 0;
    _U3RXIF = 0;
    _U3TXIE = 0;
    _U3RXIE = 1;
}

void __attribute__((__interrupt__,no_auto_psv)) _U3RXInterrupt(void)
{
    char temp = 0;

    do {
        temp = U3RXREG;
        // DEBUG("-%.2X", (u8)temp);
        _U3RXIF = 0;
        if (U3STAbits.OERR) {
            U3STAbits.OERR = 0;
        } else {
            Uart3_Rx_Buffer[Uart3_Rx_Len++] = temp;
        }
    } while (U3STAbits.URXDA);
}

void Uart3_Clear(void)
{
    Uart3_Rx_Len = 0;
    memset(Uart3_Rx_Buffer, 0, LEN_BYTE_SZ64);
}

u8 Uart3_Read(u16 postion)
{
    if (postion > Uart3_Rx_Len) {
        return 0x00;
    }

    return Uart3_Rx_Buffer[postion];
}

u16 Uart3_GetSize(void)
{
    return Uart3_Rx_Len;
}

int fputc(int ch,FILE * f)
{
    U1TXREG = ch;
    while(_U1TXIF == 0);
    _U1TXIF = 0;
    return ch;
}

int Uart1_Putc(char ch)
{
    U1TXREG = ch;
    while(_U1TXIF == 0);
    _U1TXIF = 0;
    return ch;
}

int Uart2_Putc(char ch)
{
    U2TXREG = ch;
    while(_U2TXIF == 0);
    _U2TXIF = 0;
    return ch;
}

int Uart3_Putc(char ch)
{
    U3TXREG = ch;
    while(_U3TXIF == 0);
    _U3TXIF = 0;

    // DEBUG("%.2X\r\n", (u8)ch);

    return ch;
}

int Uart2_String(char *ch)
{
    int i=0;

    int len = strlen(ch);
    for(i=0;i<len;i++){
        Uart2_Putc(ch[i]);
    }
    return len;
}

int Uart2_Printf(char *fmt,...)
{
    short i,len;
    va_list ap;
    va_start(ap,fmt);
    len = vsprintf((char*)Uart2_Send_Buf,fmt,ap);
    va_end(ap);
    for(i=0;i<len;i++)
    {
        U2TXREG = Uart2_Send_Buf[i];
        while(_U2TXIF == 0);
        _U2TXIF = 0;
    }
    return len;
}

void CleaFtpBuffer(void)
{
    g_ftp_dat_cnt = 0;
    memset(g_ftp_buf, 0, 1536);
}

void PrintFtpBuffer(void)
{
    u32 i = 0;
    u32 j = 0;
    u8 flag = 0; 

    DEBUG("U2 Recv(%ld Byte):\n", g_ftp_dat_cnt);
#if 0
    for (i=0; i<g_ftp_dat_cnt; i++) {
        if (0 == flag) {
            if (('T'==g_ftp_buf[i])&&(0x0D==g_ftp_buf[i+1])&&(0x0A==g_ftp_buf[i+2])) {
                flag = 1;
                i += 2;
                DEBUG("Found Connect Begin...\n");
            }
        } else {
            DEBUG("%.2X", (u8)g_ftp_buf[i+3]);
            DEBUG("%.2X", (u8)g_ftp_buf[i+2]);
            DEBUG("%.2X", (u8)g_ftp_buf[i+1]);
            DEBUG("%.2X ", (u8)g_ftp_buf[i+0]);
            if (3 == j%4) {
                DEBUG("\n");
            }
            j++;
            i += 3;
        }
    }
#endif
}

void __attribute__((__interrupt__,no_auto_psv)) _U2RXInterrupt(void)
{
    char temp = 0;

    do {
        temp = U2RXREG;
        g_ftp_buf[g_ftp_dat_cnt++%1536] = (u8)temp;
#ifdef BG96_MANUAL_DBG
        Uart1_Putc(temp);
#endif
        _U2RXIF = 0;
        if (U2STAbits.OERR) {
            U2STAbits.OERR = 0;
        } else {
            ringbuffer_write_byte(&tmp_rbuf,temp);
        }
    } while (U2STAbits.URXDA);
}

void __attribute__((__interrupt__,no_auto_psv)) _U1RXInterrupt(void)
{
    char temp = 0;

    do {
        temp = U1RXREG;
        // g_ftp_enable = 1;
#ifdef BG96_MANUAL_DBG
        Uart2_Putc(temp);
#endif
        _U1RXIF = 0;
        if (U1STAbits.OERR) {
            U1STAbits.OERR = 0;
        } else {
        }
    } while (U1STAbits.URXDA);
}

int IsTmpRingBufferAvailable(void)
{
    int ret;

    ret = ringbuffer_buf_use_size(&tmp_rbuf);

    return ret;
}

char ReadByteFromTmpRingBuffer(void)
{
    char dat = 0;
    int len = 1;

    len = ringbuffer_read_len(&tmp_rbuf,&dat,len);
    return dat;
}

bool WaitUartTmpRxIdle(void)
{
    int size1 = 0;
    int size2 = 0;

    size1 = ringbuffer_buf_use_size(&tmp_rbuf);

    while (1) {
        delay_ms(2);
        size2 = ringbuffer_buf_use_size(&tmp_rbuf);

        if ((size1 == size2)) {// RX stopped for 50MS
            break;
        }

        size1 = size2;
    }

    return true;
}

/******************************************************************************
 * UART4 Init
 *****************************************************************************/
void Uart4_Init(void)
{
    pU4Start = U4RxBuf;
    pU4End = U4RxBuf;

    _RP12R = 21;
    _U4RXR = 3;
            
    _LATD11 = 1;
    _TRISD10 = 1;
    _TRISD11 = 0;

    U4MODE=0x8808;
    U4STA = 0x2400;

#ifdef OSC_32M_USE
    // 4M/(34+1) = 114285
    U4BRG = 34;
#else// OSC_20M_USE
    U4BRG = 21;
#endif
    _U4TXIP = IPL_DIS;
    _U4RXIP = IPL_HIGH;
    _U4TXIF = 0;
    _U4RXIF = 0;
    _U4TXIE = 0;
    _U4RXIE = 1;
}

void Uart4_DeInit(void)
{
    pU4Start = U4RxBuf;
    pU4End = U4RxBuf;

    _RP12R = 0;
    _U4RXR = 0;
            
    _LATD11 = 0;
    _TRISD10 = 0;
    _TRISD11 = 0;

    U4MODE=0;
    U4STA = 0;

#ifdef OSC_32M_USE
    // 4M/(34+1) = 114285
    U4BRG = 0;
#else// OSC_20M_USE
    U4BRG = 21;
#endif
    _U4TXIP = IPL_DIS;
    _U4RXIP = IPL_HIGH;
    _U4TXIF = 0;
    _U4RXIF = 0;
    _U4TXIE = 0;
    _U4RXIE = 0;
}

/******************************************************************************
 * UART4 Send byte
 *****************************************************************************/
void U4Putc(char ch) {
    U4TXREG = ch;
    while( !_U4TXIF)
        ;
    _U4TXIF = 0;
}

/******************************************************************************
 * UART4 reade data size in circular buffer
 *****************************************************************************/
u16 U4GetSize(void) {
    if (pU4End < pU4Start)
        return(pU4Start - pU4End);
    else
        return(pU4End - pU4Start);
}

/******************************************************************************
 * UART4 Read byte from circular buffer
 *****************************************************************************/
u8 U4Getc(u8 *data) {
    int cnt = 100;

    do {
        if (U4GetSize()) {
            *data = *(pU4Start++);
            if (pU4Start > (U4RxBuf+U4BufSize) )
                pU4Start = U4RxBuf;
            return 1;
        }
        delay_ms_nop(10);
    } while(cnt--);
    return 0;
}

/******************************************************************************
 * UART4 reset circular buffer pointers
 *****************************************************************************/
void U4ResetBuffer(void)
{
    pU4Start = pU4End = U4RxBuf;
}

/******************************************************************************
 * UART4 ISR
 *****************************************************************************/
void __attribute__((__interrupt__,no_auto_psv)) _U4RXInterrupt(void)
{
    if (U4STAbits.OERR) {
        DEBUG("U4 OEER \n");
        U4STAbits.OERR = 0;
    } else {
        _U4RXIF = 0;    // clear IRQ flag
        *(pU4End++) = U4RXREG;
        // if at end of the buffer, go back to start
        if (pU4End > (U4RxBuf+U4BufSize))
            pU4End = U4RxBuf;
    }
}