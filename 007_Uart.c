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

#define UART3_BUFFER_MAX_LEN 32

static uint8_t Uart3Buffer[UART3_BUFFER_MAX_LEN] = {0};
static uint16_t Uart3Length = 0;

static char Uart2_Send_Buf[UART2_BUFFER_MAX_LEN] = {0};

static ringbuffer_t tmp_rbuf;

// ringbuf for temp recved
static char tmpRingbuf[RX_RINGBUF_MAX_LEN] = {0};

uint8_t Uart3_Buffer[64] = {0};
int Uart3_Use_Len = 0;

// extern u8 g_ftp_enable;

extern int rx_debug_flag;

// 115200 for Debug
void Uart1_Init(void)
{
#ifndef OSC_20M_USE
    _RP22R = 3;// RD3
    _U1RXR = 23;// RD2

    _LATD3 = 1;
    _TRISD2 = 1;
    _TRISD3 = 0;

    U1MODE = 0X8808;
    U1STA = 0X2400;
    // 4M/(34+1) = 114285
    U1BRG = 34;
#else
    _RP22R = 3;// RD3
    _U1RXR = 24;// RD1

    _LATD3 = 1;
    _TRISD1 = 1;
    _TRISD3 = 0;

    U1MODE = 0X8808;
    U1STA = 0X2400;
    // 4M/(34+1) = 114285
    U1BRG = 0x15;
#endif

    _U1TXIP = 3;
    _U1RXIP = 7;
    _U1TXIF = 0;
    _U1RXIF = 0;
    _U1TXIE = 0;
    _U1RXIE = 1;
}

// 115200 for BG96
void Uart2_Init(void)
{
#ifndef OSC_20M_USE
    _RP17R = 5;// RF5
    _U2RXR = 10;// RF4

    _LATF5 = 1;
    _TRISF4 = 1;
    _TRISF5 = 0;
    U2MODE = 0X8808;
    U2STA = 0X2400;
    U2BRG = 34;
#else
#if 0
    _RP17R = 5;// RF5
    _U2RXR = 10;// RF4

    _LATF5 = 1;
    _TRISF4 = 1;
    _TRISF5 = 0;
#else
    _RP14R = 5;// RB14
    _U2RXR = 21;// RG6

    _LATB14 = 1;
    _TRISG6 = 1;
    _TRISB14 = 0;
    
    _ANSG6 = 0;
#endif
    U2MODE = 0X8808;
    U2STA = 0X2400;
    U2BRG = 0x15;
#endif

    _U2TXIP = 1;
    _U2RXIP = 2;
    _U2TXIF = 0;
    _U2RXIF = 0;
    _U2TXIE = 0;
    _U2RXIE = 1;

    ringbuffer_init(&tmp_rbuf,tmpRingbuf,RX_RINGBUF_MAX_LEN);
}

// 115200 for CLRC663
void Uart3_Init(void)
{
    // PIC24FJ256 is 28
    // PIC24FJ1024 is 19
    _RP3R = 19;     //RP3(RD10) = U3TX
    _U3RXR = 4;     //U3RXR     = RP4(RD9)

    _LATD10 = 1;
    _TRISD9 = 1;
    _TRISD10 = 0;
    U3MODE = 0X8808;
    U3STA = 0X2400;
    U3BRG = 34;

    _U3TXIP = 1;
    _U3RXIP = 2;
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
        _U3RXIF = 0;
        if (U3STAbits.OERR) {
            U3STAbits.OERR = 0;
        } else {
            Uart3Buffer[Uart3Length++] = temp;
        }
    } while (U3STAbits.URXDA);
}

void Uart3_Clear(void)
{
    memset(Uart3Buffer,0,UART3_BUFFER_MAX_LEN);
    Uart3Length = 0;
}

uint8_t Uart3_Read(uint16_t postion)
{
    if(postion > Uart3Length){
        return 0x00;
    }
    return Uart3Buffer[postion];
}

uint16_t Uart3_GetSize(void)
{
    return Uart3Length;
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

u8 SCISendDataOnISR(u8 *sendbuf,u16 size)
{
    while(size--)
    {
        Uart1_Putc(*sendbuf);
        sendbuf++;
    }

    return 1;
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

//    printf("%.2X\r\n", (uint8_t)ch);

    return ch;
}

int uart3_write_bytes(char * buf,int len)
{
    int i = 0;
    for(i=0;i<len;i++){
        Uart3_Putc(buf[i]);
    }
    return len;
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

void __attribute__((__interrupt__,no_auto_psv)) _U2RXInterrupt(void)
{
    char temp = 0;

    do {
        temp = U2RXREG;
        //if (rx_debug_flag) {
        //    printf("%.2X-%c\n", temp, temp);
        //}
        //Uart1_Putc(temp);
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
        // Uart1_Putc(temp);
        _U1RXIF = 0;
        if (U1STAbits.OERR) {
            U1STAbits.OERR = 0;
        } else {
            // printf("%.2X\n", temp);
            // ringbuffer_write_byte(&tmp_rbuf,temp);
            // printf("recv len = %d\n", IsTmpRingBufferAvailable());
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
    if(IsTmpRingBufferAvailable()<=0){
        return -1;
    }
    len = ringbuffer_read_len(&tmp_rbuf,&dat,len);
    return dat;
}

bool WaitUartTmpRxIdle(void)
{
    int size1 = 0;
    int size2 = 0;

    size1 = ringbuffer_buf_use_size(&tmp_rbuf);

    while (1) {
        delay_ms(50);
        size2 = ringbuffer_buf_use_size(&tmp_rbuf);

        if ((size1 == size2)) {// RX stopped for 50MS
            break;
        }

        size1 = size2;
    }

    return true;
}
