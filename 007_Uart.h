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

#ifndef MUART_H
#define MUART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "015_Common.h"

#define UART2_BUFFER_MAX_LEN 512

#define RX_RINGBUF_MAX_LEN 1280

uint8_t Uart3_Read(uint16_t postion);
uint16_t Uart3_GetSize(void);
    
// for debug
void Uart1_Init(void);
void Uart2_Init(void);

u8 SCISendDataOnISR(u8 *sendbuf,u16 size);

// for BG96
int Uart2_Putc(char ch);
int Uart2_String(char *ch);
int Uart2_Printf(char *fmt,...);
int IsTmpRingBufferAvailable(void);
char ReadByteFromTmpRingBuffer(void);
bool WaitUartTmpRxIdle(void);

// for CLRC663
void Uart3_Init(void);
int Uart3_Putc(char ch);
void Uart3_Clear(void);

// for BNO055
void Uart4_Init(void);
void Uart4_Putc(char ch);
// void Uart4_Clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MUART_H */
