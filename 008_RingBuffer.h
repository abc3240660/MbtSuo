//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * A library for RingBuffer of UART2(for BG96)
 * This file is about the RingBuffer of UART2
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 03/11/2019
******************************************************************************/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef __cplusplus
extern "C"{
#endif //__cplusplus

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <p24fxxxx.h>
#include "015_Common.h"

#define min(a, b) (a)<(b)?(a):(b)                   ///< Calculate the minimum value

#pragma pack(1)
typedef struct {
    u8  *rbHead;
    u8  *rbTail;
    u8  *rbBuff;
    unsigned int rbCapacity;
}ringbuffer_t;
#pragma pack()

void ringbuffer_init(ringbuffer_t *rb, char *buf, unsigned int len);
int ringbuffer_write_byte(ringbuffer_t* rb, char buf);
int ringbuffer_read_len(ringbuffer_t* rb, char* data, int count);
void ringbuffer_clear(ringbuffer_t* rb);
unsigned int ringbuffer_buf_use_size(ringbuffer_t* rb);

#ifdef __cplusplus
}
#endif
	
#endif //RINGBUFFER_H
