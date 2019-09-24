//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * A library for CLRC663 Module
 * This file is about the CLRC663 API
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 04/14/2019
******************************************************************************/

#include <xc.h>

#include "001_Tick_10ms.h"
#include "004_LB1938.h"
#include "007_Uart.h"
#include "011_Spi.h"
#include "012_CLRC663_NFC.h"
#include "013_Protocol.h"
#include "016_FlashOta.h"

static u8 gs_tmp_card_id[LEN_BYTE_SZ32+1] = {0};
static u8 gs_tmp_serial_nr[LEN_BYTE_SZ32+1] = {0};
static u8 gs_bind_cards[CNTR_MAX_CARD][LEN_BYTE_SZ64] = {{0}};

static u32 gs_start_time_nfc = 0;

void print_block(uint8_t * block, uint8_t length);

// maybe is not correct
void __delay_us(uint16_t ms)
{
    int i=0,j=0;
    for(i=0;i<ms;i++){
        for(j=0;j<200;j++);
    }
}

static void __delay_usx(uint16_t ms)
{
    int i=0,j=0;
    for(i=0;i<ms;i++){
        for(j=0;j<20;j++);
    }
}

//      *** *** ***     Print identification numbers     *** *** ***       //
//      *** *** ***     Card Invalidation     *** *** ***       //
// Hex print for blocks without printf.
void print_block(uint8_t * block, uint8_t length)
{
  int i = 0;

  for (i = 0; i < length; i++){
      if (block[i] < 16){  printf("%02x ",block[i]); }
      else printf("%02x ",block[i]);
  }

  printf("\r\n");
}

void print_block_01(uint8_t* block, uint8_t lower_lim, uint8_t upper_lim)
{
  int i = 0;

  for(i = lower_lim; i < upper_lim; i++){
    if (block[i] < 16){ printf("%02x ",block[i]); }
    else printf("%02x ",block[i]);
  }

  printf(" \r\n");
}

void print_card_ID_block(uint8_t * block, uint8_t length)
{
    int i = 0;

    printf("CardId: ");

    for (i = 3; i < 12; i++) {
        if (block[i] < 16) {
            printf("%02x ",block[i]);
        } else {
            printf("%02x ",block[i]);
        }
    }

    printf("%02x ",block[12]>>4);
    printf(" \r\n");
}

//  Card Blacklist and Card Invalidation
void check_valid_card(uint8_t * buf)
{
  if (buf[40] == 0x62 || buf [41] == 0x83) printf(" ----INVALID CARD: card is locked----");
  else printf("---- VALID CARD ----");
}

//  Print Pseudo-Unique PICC Identifier, Type B
void print_PUPI(uint8_t* block)
{
  printf("PUPI: ");
  print_block_01(block, 1, 5);
}

//  Print MOBIB Hardcoded Serial Number
void print_serial_nr(uint8_t* block)
{
  printf("Serial number: ");
  print_block_01(block, 23, 31);
}

int bitRead(uint8_t value,int pos)
{
    uint8_t temp;
    temp = (value & (0x01 << pos)) >> pos;
    return temp;
}

//  Shift buffer with two bits (to determine MOBIB Original Card ID)
void shift(uint8_t* buf, uint8_t buf_size)
{
    int i = 0;
    buf [3] = buf [3] << 2;
    for(i = 4; i < 13; i++){
      uint8_t lastbit = bitRead(buf[i], 7);
      uint8_t secondlastbit = bitRead(buf[i], 6);
      if(lastbit == 1) bitSet(buf[i-1], 1);
      if(secondlastbit == 1) bitSet(buf[i-1], 0);
      buf[i] = buf[i] << 2;
    }
}


//  Print MOBIB Original Card ID
void print_card_ID(uint8_t* buf, uint8_t bufsize, u8* card_id, u8* serial_nr)
{
    u8 i = 0;
    u8 k = 0;
    u8 tmp = 0;

    shift(buf, bufsize);

    for (i = 3; i < 13; i++){
        tmp = (buf[i]>>4)&0x0F;
        if ((tmp>=0x00) && (tmp<=0x09)) {
            card_id[k++] = tmp + '0';
        } else if ((tmp>=0x0A) && (tmp<=0x0F)) {
            card_id[k++] = tmp-0x0A + 'A';
        }
        if (i != 12) {
            tmp = buf[i]&0x0F;
            if ((tmp>=0x00) && (tmp<=0x09)) {
                card_id[k++] = tmp + '0';
            } else if ((tmp>=0x0A) && (tmp<=0x0F)) {
                card_id[k++] = tmp-0x0A + 'A';
            }
        }
//        printf("%02X\n", buf[i]);
    }

    printf("CardId = %s\n", card_id);
 //   print_card_ID_block(buf, bufsize);
}

u8 read_iso14443B_nfc_card(u8* card_id, u8* serial_nr)
{
  //  Configure CLRC663
  CLRC663_configure_communication_protocol(CLRC630_PROTO_ISO14443B_106_NRZ_BPSK);

  //  WUPB
  uint8_t wupb_buffer [] = {CLRC663_ISO14443B_CMD_APF, CLRC663_ISO14443B_CMD_AFI, CLRC663_ISO14443B_CMD_PARAM};
  uint8_t atqb_buffer [12] = {0};
  uint8_t atqb_length = clrc663_communicate(wupb_buffer, sizeof(wupb_buffer), atqb_buffer);

  if(atqb_length != 0) {
    //  ATTRIB
    uint8_t transmit_buffer [] = {0x1D, atqb_buffer[1], atqb_buffer[2], atqb_buffer[3], atqb_buffer[4], 0x00, 0x08, 0x01, 0x00};
    uint8_t receive_buffer [1] = {0};
    clrc663_communicate(transmit_buffer, sizeof(transmit_buffer), receive_buffer);

    //  CALYPSO --- Select Global Data Application 
    uint8_t apdu_transmit_buffer [] = APDU_SELECT_GLOBAL_DATA_APP_MOBIB_CARD;
    uint8_t apdu_receive_buffer [42] = {0};
    clrc663_communicate(apdu_transmit_buffer, sizeof(apdu_transmit_buffer), apdu_receive_buffer);

    //  CALYPSO --- Read record 1
    uint8_t apdu_transmit_buffer_1 [] = CARDISSUING_FILE_READ_RECORD_1;
    uint8_t apdu_receive_buffer_1 [32] = {0};
    clrc663_communicate(apdu_transmit_buffer_1, sizeof(apdu_transmit_buffer_1), apdu_receive_buffer_1);


    #ifdef DEBUG_ISO14443B
      //  Print sended en received bytes
      printf("  ---   WUPB   --- \n");
      printf("Send: "); print_block(wupb_buffer, sizeof(wupb_buffer));
      printf("Receive: "); print_block(atqb_buffer, sizeof(atqb_buffer));
      printf("  ---   ATTRIB   --- \n");
      printf("Send: "); print_block(transmit_buffer, sizeof(transmit_buffer));
      printf("Receive: "); print_block(receive_buffer, sizeof(receive_buffer));
      printf("  ---   CALYPSO --- APDU select global data application   --- \n");
      printf("Send: "); print_block(apdu_transmit_buffer, sizeof(apdu_transmit_buffer));
      printf("Receive: "); print_block(apdu_receive_buffer, sizeof(apdu_receive_buffer));
      printf("  ---   CALYPSO --- APDU read record 1  --- \n");
      printf("Send: "); print_block(apdu_transmit_buffer_1, sizeof(apdu_transmit_buffer_1));
      printf("Receive: "); print_block(apdu_receive_buffer_1, sizeof(apdu_receive_buffer_1));
    #endif

    #ifdef PRINT_IDENTIFICATION_NUMBERS
      printf(" \r\n");
      check_valid_card(apdu_receive_buffer);
      print_PUPI(atqb_buffer);
      print_serial_nr(apdu_receive_buffer);
      print_card_ID(apdu_receive_buffer_1, sizeof(apdu_receive_buffer_1));
    #endif

      print_serial_nr(apdu_receive_buffer);
      print_card_ID(apdu_receive_buffer_1, sizeof(apdu_receive_buffer_1), card_id, serial_nr);

      if (('0'==card_id[0]) && ('0'==card_id[1]) && ('0'==card_id[2])) {
          return 0;
      }

      return 1;
  }

  return 0;
}

u8 ReadMobibNFCCard(void)
{
    u8 i = 0;
    u8 tmp_len = 0;

    memset(gs_tmp_card_id, 0, LEN_BYTE_SZ16);
    memset(gs_tmp_serial_nr, 0, LEN_BYTE_SZ16);

    if (IsDuringBind()) {
        if (0 == gs_start_time_nfc) {
            for (i=0; i<20; i++) {
                if(i%2){
                    LATD |= (1<<8);
                }else{
                    LATD &= ~(1<<8);
                }
                __delay_usx(25UL);
            }
            gs_start_time_nfc = GetTimeStamp();
        }
    } else {
        gs_start_time_nfc = 0;
    }

    if (read_iso14443B_nfc_card(gs_tmp_card_id, gs_tmp_serial_nr) > 0) {
        tmp_len = strlen((const char*)gs_tmp_card_id);
        tmp_len = strlen((const char*)gs_tmp_serial_nr);

//        LATB |= (1<<4);
//        delay_ms(5000);
//        LATB &= ~(1<<4);

        for (i=0; i<20; i++) {
            if(i%2){
                LATD |= (1<<8);
            }else{
                LATD &= ~(1<<8);
            }
            __delay_usx(25UL);
        }

        TcpReadedOneCard(gs_tmp_card_id, gs_tmp_serial_nr);

        if (IsDuringBind()) {
            gs_start_time_nfc = GetTimeStamp();
            AddNewMobibCard(gs_tmp_card_id, gs_tmp_serial_nr);
        } else {
            for (i=0; i<CNTR_MAX_CARD; i++) {
                if (0 == strncmp((const char*)gs_bind_cards[i], (const char*)gs_tmp_card_id, LEN_CARD_ID)) {
                    LB1938_MotorCtrl(MOTOR_LEFT, MOTOR_HOLD_TIME);
                    ReportLockerUnlocked();
                }
            }
        }

        return 0;
    }

    // cannot readout any MOBIB card for 10s
    // so just finish this ADDC command
    if (IsDuringBind()) {
        if (gs_start_time_nfc != 0) {
            if (isDelayTimeout(gs_start_time_nfc,10*1000UL)) {
                for (i=0; i<60; i++) {
                    if (0 == i%20) {
                        delay_ms(200);
                    }

                    if(i%2){
                        LATD |= (1<<8);
                    }else{
                        LATD &= ~(1<<8);
                    }
                    __delay_usx(25UL);
                }
                ReportFinishAddNFC();
            }
        }
    }

    return 1;
}

void AddNewMobibCard(u8* card_id, u8* serial_nr)
{
    u8 i = 0;

    if (NULL == card_id) {
        return;
    }

    for (i=0; i<CNTR_MAX_CARD; i++) {
        if (0 == strncmp((const char*)gs_bind_cards[i], (const char*)card_id, strlen((const char*)card_id))) {
            return;
        }
    }

    FlashWrite_OneNFCCard(card_id);

    for (i=0; i<CNTR_MAX_CARD; i++) {
        if (0 == strlen((const char*)gs_bind_cards[i])) {
            memcpy(gs_bind_cards[i], card_id, LEN_BYTE_SZ32);
            memcpy(gs_bind_cards[i]+32, serial_nr, LEN_BYTE_SZ32);
            break;
        }
    }

    for (i=0; i<CNTR_MAX_CARD; i++) {
        if (strlen((const char*)gs_bind_cards[i]) != 0) {
            printf("Binded CardId[%d]: %s\n", i, gs_bind_cards[i]);
        }
    }
}

void DeleteMobibCard(u8* card_id, u8* serial_nr)
{
    u8 i = 0;

    if (NULL == card_id) {
        return;
    }

    for (i=0; i<CNTR_MAX_CARD; i++) {
        if (0 == strncmp((const char*)gs_bind_cards[i], (const char*)card_id, LEN_CARD_ID)) {
            memset(gs_bind_cards[i], 0, LEN_BYTE_SZ64);
        }
    }

    FlashWrite_DeleteOneCard(card_id);
}

void DeleteAllMobibCard(void)
{
    u8 i = 0;

    for (i=0; i<CNTR_MAX_CARD; i++) {
        memset(gs_bind_cards[i], 0, LEN_BYTE_SZ64);
    }

    FlashErase_OnePage(FLASH_PAGE_CARD_ID, FLASH_BASE_CARD_ID);
}

u8 QueryMobibCard(u8* card_dats)
{
    u8 i = 0;
    u8 count = 0;
    u8 offset = 0;

    if (NULL == card_dats) {
        return 0;
    }

    for (i=0; i<CNTR_MAX_CARD; i++) {
        if (strlen((const char*)gs_bind_cards[i]) > 0) {
            if (count != 0) {
                card_dats[offset++] = '|';
            }

            memcpy(card_dats+offset, gs_bind_cards[i], LEN_CARD_ID);

            count++;
        }
    }

    return count;
}

void CardIDFlashBankInit(void)
{
    FlashRead_AllNFCCards(&gs_bind_cards[0]);
}
