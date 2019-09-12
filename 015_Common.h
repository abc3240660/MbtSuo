//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

//******************************************************************************
// File:   ---_Defines.h
// Author: Hans Desmet
// Comments: Initial Define file
// Revision history: Version 01.00
// Date 26/12/2018
//******************************************************************************

#ifndef __DEFINE_H
#define __DEFINE_H

#include <p24fxxxx.h>                                                                  // include processor files - each processor file is guarded.  

#define         SW_Major_Version        1
#define         SW_Minor_Version        1
#define         SW_Release              1

#define         T1IPL                   5                                       // define Timer 1 IRQ priority levell for real tome Tick
#define         T2IPL                   1                                       // define Timer 2 IRQ priority levell for real tome Tick

#define         _XTAL_FREQ              12000000

#define false 0
#define true 1

#define LEN_BYTE_SZ8     8
#define LEN_BYTE_SZ16    16
#define LEN_BYTE_SZ32    32
#define LEN_BYTE_SZ64    64
#define LEN_BYTE_SZ128   128
#define LEN_BYTE_SZ256   256
#define LEN_BYTE_SZ512   512

#define LEN_CARD_ID      19
#define LEN_SERIAL_NR    16

// if store too more, TCP send buffer will be
// not enough, currently LEN_MAX_SEND = 512
// 20*19 = 380, will be suitable
#define CNTR_MAX_CARD           20
#define CNTR_INWORD_PER_CARD    8

// for common use
#define CNTR_INWORD_PER_COMM    8
#define CNTR_INWORD_PER_SMALL   2
#define CNTR_INWORD_PER_BIG     16

#define DEFAULT_HBEAT_GAP 120

// 0x0800 ~ 0x0FFF(2K): Params
// 0x1000 ~ 0x1FFF(4K): CardIDs

#define FLASH_BASE_CARD_ID  0x1000
#define FLASH_PAGE_CARD_ID  0x0001
#define FLASH_PAGE_PARAMS   0x0000

#define FLASH_BASE_PARAMS   0x0800

#define FLASH_BASE_IP       FLASH_BASE_PARAMS
#define FLASH_SIZE_IP       CNTR_INWORD_PER_COMM

#define FLASH_BASE_PORT     (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*1)
#define FLASH_SIZE_PORT     CNTR_INWORD_PER_COMM

#define FLASH_BASE_APN      (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2)
#define FLASH_SIZE_APN      CNTR_INWORD_PER_BIG

#define FLASH_BASE_1ST      (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG)
#define FLASH_SIZE_1ST      CNTR_INWORD_PER_SMALL

#define FLASH_BASE_1ST      (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG)
#define FLASH_SIZE_1ST      CNTR_INWORD_PER_SMALL

#define FLASH_BASE_1ST      (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG)
#define FLASH_SIZE_1ST      CNTR_INWORD_PER_SMALL

#define FLASH_BASE_ALM_ON  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*1)
#define FLASH_SIZE_ALM_ON  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_BEP_ON  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*2)
#define FLASH_SIZE_BEP_ON  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_BEP_LV  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*3)
#define FLASH_SIZE_BEP_LV  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_BOT_TM  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*4)
#define FLASH_SIZE_BOT_TM  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_IAP_FG  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*5)
#define FLASH_SIZE_IAP_FG  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_IAP_STA  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*6)
#define FLASH_SIZE_IAP_STA  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_IAP_CNT  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*7)
#define FLASH_SIZE_IAP_CNT  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U1  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*8)
#define FLASH_SIZE_RSVD_U1  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U2  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*9)
#define FLASH_SIZE_RSVD_U2  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U3  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*10)
#define FLASH_SIZE_RSVD_U3  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U4  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*11)
#define FLASH_SIZE_RSVD_U4  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U5  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*12)
#define FLASH_SIZE_RSVD_U5  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U6  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*13)
#define FLASH_SIZE_RSVD_U7  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U7  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*14)
#define FLASH_SIZE_RSVD_U7  CNTR_INWORD_PER_SMALL

#define FLASH_BASE_RSVD_U8  (FLASH_BASE_PARAMS + CNTR_INWORD_PER_COMM*2 + CNTR_INWORD_PER_BIG + CNTR_INWORD_PER_SMALL*15)
#define FLASH_SIZE_RSVD_U8  CNTR_INWORD_PER_SMALL

typedef unsigned char bool;

// For Common
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

typedef struct {
    u8 svr_ip[LEN_BYTE_SZ32];
    u8 svr_port[LEN_BYTE_SZ32];
    u8 svr_apn[LEN_BYTE_SZ64];
    u8 iap_md5[LEN_BYTE_SZ32];
    u32 is_first_boot;
    u32 is_alarm_on;
    u32 is_beep_on;
    u32 beep_level;
    u32 boot_time;

    // (APP SET)0x1A1A2B2B - need do update from SD Card
    // (APP SET)0x5A5A6B6B - need do update from SPI Flash
    // (IAP SET)0x3C3C4D4D - update finished
    // (APP CLR)0x00000000 - idle(after app detect 0x3C3C4D4D)
    u32 need_iap_flag;

    // (IAP SET)0x52816695 - iap update NG
    // (APP CLR)0x00000000 - idle
    u32 iap_sta_flag;

    // try to jump into app
    // (IAP SET)0 -> 10 - if equal to 10, need do restore hex data from BAKOK sector into RUN sector
    // (APP CLR)0 - jump to app ok
    u32 iap_try_cnt;

    u32 reserved_use1;
    u32 reserved_use2;
    u32 reserved_use3;
    u32 reserved_use4;
    u32 reserved_use5;
    u32 reserved_use6;
    u32 reserved_use7;
    u32 reserved_use8;
} SYS_ENV;

typedef struct {
    // (APP SET)0x1A1A2B2B - need do update from SD Card
    // (APP SET)0x5A5A6B6B - need do update from SPI Flash
    // (IAP SET)0x3C3C4D4D - update finished
    // (APP CLR)0x00000000 - idle(after app detect 0x3C3C4D4D)
    u32 need_iap_flag;

    // (APP SET)0x51516821 - need backup hex data from RUN sector into BAKOK sector
    // (IAP CLR)0x00000000 - idle
    u32 need_bak_flag;

    // (APP SET)0x12345678 - APP is running
    // (IAP SET)0x61828155 - already done backup hex data from RUN sector into BAKOK sector
    u32 bak_sta_flag;

    // (IAP SET)0x52816695 - iap update NG
    // (APP CLR)0x00000000 - idle
    u32 iap_sta_flag;

    // (IAP SET)0 -> 10 - if equal to 10, need do restore hex data from BAKOK sector into RUN sector
    // (APP CLR)0 - jump to app ok
    u32 try_run_cnt;

    // (APP SET)0x51656191 - need restore hex data from BAKOK sector into RUN sector
    // (IAP CLR)0x00000000 - idle
    u32 need_rcv_flag;
} IAP_ENV;

#endif

//******************************************************************************
//* END OF FILE
//******************************************************************************
