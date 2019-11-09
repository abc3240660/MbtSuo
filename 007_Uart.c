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
#include "007_Uart.h"
#include "015_Common.h"

#define UART3_BUFFER_MAX_LEN 32

static uint8_t Uart3Buffer[UART3_BUFFER_MAX_LEN] = {0};
static uint16_t Uart3Length = 0;

uint8_t Uart4_Buffer[UART3_BUFFER_MAX_LEN] = {0};
uint16_t Uart4_Use_Len = 0;

static char Uart2_Send_Buf[UART2_BUFFER_MAX_LEN] = {0};


//static ringbuffer_t tmp_rbuf;

// ringbuf for temp recved
// static char tmpRingbuf[RX_RINGBUF_MAX_LEN] = {0};

uint8_t Uart3_Buffer[64] = {0};
int Uart3_Use_Len = 0;

// extern u8 g_ftp_enable;

extern int rx_debug_flag;
extern u8 BNO_055_RECV_BUFF[64];
extern u16 recv_total_len;

int Uart1_Putc(char ch);

// 115200 for Debug
void Uart1_Init(void)
{
    _RP22R = 3;// RD3
    _U1RXR = 24;// RD1

    _LATD3 = 1;
    _TRISD1 = 1;
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
    _U1RXIP = IPL_DIS;
    _U1TXIF = 0;
    _U1RXIF = 0;
    _U1TXIE = 0;
    _U1RXIE = 0;
}

// 115200 for BG96
void Uart2_Init(void)
{
    _RP14R = 5;// RB14
    _U2RXR = 21;// RG6

    _LATB14 = 1;
    _TRISG6 = 1;
    _TRISB14 = 0;

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

	// DEBUG("%.2X\r\n", (uint8_t)ch);

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
        //    DEBUG("%.2X-%c\n", temp, temp);
        //}
        //Uart1_Putc(temp);
        _U2RXIF = 0;
        if (U2STAbits.OERR) {
            U2STAbits.OERR = 0;
        } else {
        }
    } while (U2STAbits.URXDA);
}

void __attribute__((__interrupt__,no_auto_psv)) _U1RXInterrupt(void)
{
    char temp = 0;

    do {
        temp = U1RXREG;
        // g_ftp_enable = 1;
        // Uart4_Putc(temp);
        _U1RXIF = 0;
        if (U1STAbits.OERR) {
            U1STAbits.OERR = 0;
        } else {
        }
    } while (U1STAbits.URXDA);
}

int IsTmpRingBufferAvailable(void)
{
    int ret = 0;

    //ret = ringbuffer_buf_use_size(&tmp_rbuf);

    return ret;
}

char ReadByteFromTmpRingBuffer(void)
{
    char dat = 0;
    int len = 1;

    if(IsTmpRingBufferAvailable()<=0){
        return -1;
    }
    //len = ringbuffer_read_len(&tmp_rbuf,&dat,len);
    return dat;
}

bool WaitUartTmpRxIdle(void)
{
    int size1 = 0;
    int size2 = 0;

    //size1 = ringbuffer_buf_use_size(&tmp_rbuf);

    while (1) {
        delay_ms(50);
        //size2 = ringbuffer_buf_use_size(&tmp_rbuf);

        if ((size1 == size2)) {// RX stopped for 50MS
            break;
        }

        size1 = size2;
    }

    return true;
}

void Uart4_Init(void)
{
    RPOR6bits.RP12R = 0x0015;    //RD11->UART4:U4TX
    RPINR27bits.U4RXR = 0x0003;    //RD10->UART4:U4RX

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

    IPC22bits.U4TXIP = IPL_DIS;
    IPC22bits.U4RXIP = IPL_HIGH;
    IEC5bits.U4TXIE = 0;
    IEC5bits.U4RXIE = 1;
}

void Uart4_Putc(char ch)
{
    U4TXREG = ch;
    //DEBUG("TX[%.2X] ", (u8)ch);
    while(_U4TXIF == 0);
    _U4TXIF = 0;
}

void __attribute__((__interrupt__,no_auto_psv)) _U4RXInterrupt(void)
{
    do {

        if (U4STAbits.OERR) {
            DEBUG("U4 OEER \n");
            U4STAbits.OERR = 0;
        } else {
            _U4RXIF = 0;
            BNO_055_RECV_BUFF[recv_total_len++]=U4RXREG;
        }
    } while (U4STAbits.URXDA);
}
