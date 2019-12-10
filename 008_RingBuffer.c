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

#include "008_RingBuffer.h"

void ringbuffer_init(ringbuffer_t *rb, char *buf, unsigned int len)
{
    if(NULL == rb)
    {
		DEBUG("ERR: %s null pointer found.\n", __func__);
        return;
    }

    rb->rbBuff = (u8*)buf;
    rb->rbHead = rb->rbBuff;
    rb->rbTail = rb->rbBuff;

    rb->rbCapacity = len;

    return;
}

void ringbuffer_clear(ringbuffer_t* rb)
{
    if(NULL == rb)
    {
		DEBUG("ERR: %s null pointer found.\n", __func__);
        return;
    }

    rb->rbHead = rb->rbBuff;
    rb->rbTail = rb->rbBuff;

	if (rb->rbCapacity > 0) {
		memset(rb->rbBuff, 0, rb->rbCapacity);
	}

	return;
}

u32 ringbuffer_capacity(ringbuffer_t *rb)
{
    if(NULL == rb)
    {
		DEBUG("ERR: %s null pointer found.\n", __func__);
        return 0;
    }

    return rb->rbCapacity;
}

unsigned int ringbuffer_can_read(ringbuffer_t *rb)
{
    if(NULL == rb)
    {
		DEBUG("ERR: %s null pointer found.\n", __func__);
        return 0;
    }

    if (rb->rbHead == rb->rbTail)
    {
        return 0;
    }

    if (rb->rbHead < rb->rbTail)
    {
        return rb->rbTail - rb->rbHead;
    }

    return ringbuffer_capacity(rb) - (rb->rbHead - rb->rbTail);
}

u32 ringbuffer_can_write(ringbuffer_t *rb)
{
    if(NULL == rb)
    {
		DEBUG("ERR: %s null pointer found.\n", __func__);
        return 0;
    }

    return ringbuffer_capacity(rb) - ringbuffer_buf_use_size(rb);
}

unsigned int ringbuffer_buf_use_size(ringbuffer_t* rb)
{
	return ringbuffer_can_read(rb);
}

int ringbuffer_read_len(ringbuffer_t* rb, char* data, int count)
{
    u32 copySz = 0;

    if((NULL == rb)||(NULL == data))
    {
		DEBUG("ERR: %s null pointer found.\n", __func__);
        return 0;
    }

    if (rb->rbHead < rb->rbTail)
    {
        copySz = min(count, ringbuffer_buf_use_size(rb));
        memcpy(data, rb->rbHead, copySz);
        rb->rbHead += copySz;
        return copySz;
    }
    else
    {
        if (count < ringbuffer_capacity(rb)-(rb->rbHead - rb->rbBuff))
        {
            copySz = count;
            memcpy(data, rb->rbHead, copySz);
            rb->rbHead += copySz;
            return copySz;
        }
        else
        {
            copySz = ringbuffer_capacity(rb) - (rb->rbHead - rb->rbBuff);
            memcpy(data, rb->rbHead, copySz);
            rb->rbHead = rb->rbBuff;
            copySz += ringbuffer_read_len(rb, (char*)data+copySz, count-copySz);
            return copySz;
        }
    }
}

u32 ringbuffer_write_array(ringbuffer_t *rb, const void *data, u32 count)
{
    u32 tailAvailSz = 0;

    if((NULL == rb)||(NULL == data))
    {
		DEBUG("ERR: %s null pointer found.\n", __func__);
        return 0;
    }

    if (count >= ringbuffer_can_write(rb))
    {
		DEBUG("ERR: %s buffer is not enough.\n", __func__);
        return 0;
    }

    if (rb->rbHead <= rb->rbTail)
    {
        tailAvailSz = ringbuffer_capacity(rb) - (rb->rbTail - rb->rbBuff);
        if (count <= tailAvailSz)
        {
            memcpy(rb->rbTail, data, count);
            rb->rbTail += count;
            if (rb->rbTail == rb->rbBuff+ringbuffer_capacity(rb))
            {
                rb->rbTail = rb->rbBuff;
            }
            return count;
        }
        else
        {
            memcpy(rb->rbTail, data, tailAvailSz);
            rb->rbTail = rb->rbBuff;

            return tailAvailSz + ringbuffer_write_array(rb, (char*)data+tailAvailSz, count-tailAvailSz);
        }
    }
    else
    {
        memcpy(rb->rbTail, data, count);
        rb->rbTail += count;
        return count;
    }
}

int ringbuffer_write_byte(ringbuffer_t* rb, char buf)
{
	return (int)ringbuffer_write_array(rb, &buf, 1);
}
