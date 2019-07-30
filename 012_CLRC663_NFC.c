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

static unsigned long start_time_nfc = 0;
static u8 g_tmp_card_id[LEN_BYTE_SZ32] = {0};
static u8 g_tmp_serial_nr[LEN_BYTE_SZ32] = {0};
static u8 g_bind_cards[LEN_MAX_CARD][LEN_BYTE_SZ64] = {{0}};

void print_block(uint8_t * block, uint8_t length);

// maybe is not correct
void __delay_us(uint16_t ms)
{
    int i=0,j=0;
    for(i=0;i<ms;i++){
        for(j=0;j<200;j++);
    }
}

uint16_t clrc663_SPI_transfer(const uint8_t* tx, uint8_t* rx, uint16_t len)
{
    uint16_t tempLen = 0;
    tempLen = SPI2_Exchange8bitBuffer((uint8_t*)tx, len, rx);
    // print_block(rx,tempLen);
    __delay_us(2);
    return tempLen;
}

void clrc663_SPI_select()
{
    PORTEbits.RE7 = 0;
}

void clrc663_SPI_unselect()
{
    PORTEbits.RE7 = 1;
}

//      *** *** ***     Print identification numbers     *** *** ***       //
//      *** *** ***     Card Invalidation     *** *** ***       //
// Hex print for blocks without printf.
void print_block(uint8_t * block, uint8_t length){
  int i = 0;
  for (i = 0; i < length; i++){
      if (block[i] < 16){  printf("%02x ",block[i]); }
      else printf("%02x ",block[i]);
  }
  printf("\r\n");
}

void print_block_01(uint8_t* block, uint8_t lower_lim, uint8_t upper_lim){
  int i = 0;
  for(i = lower_lim; i < upper_lim; i++){
    if (block[i] < 16){ printf("%02x ",block[i]); }
    else printf("%02x ",block[i]);
  }
  printf(" \r\n");
}

void print_card_ID_block(uint8_t * block, uint8_t length){
  printf("CardId: ");
  int i = 0;
  for (i = 3; i < 12; i++){
      if (block[i] < 16){printf("%02x ",block[i]); }
      else  printf("%02x ",block[i]); }
  printf("%02x ",block[12]>>4);
  printf(" \r\n");
}

//  Card Blacklist and Card Invalidation
void check_valid_card(uint8_t * buf){
  if(buf[40] == 0x62 || buf [41] == 0x83) printf(" ----INVALID CARD: card is locked----");
  else printf("---- VALID CARD ----");
}

//  Print Pseudo-Unique PICC Identifier, Type B
void print_PUPI(uint8_t* block){
  printf("PUPI: ");
  print_block_01(block, 1, 5);
}

//  Print MOBIB Hardcoded Serial Number
void print_serial_nr(uint8_t* block){
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
void shift(uint8_t* buf, uint8_t buf_size){
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
void print_card_ID(uint8_t* buf, uint8_t bufsize){
    shift(buf, bufsize);
    print_card_ID_block(buf, bufsize);
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
      print_card_ID(apdu_receive_buffer_1, sizeof(apdu_receive_buffer_1));
  }
  
  return 0;
}

u8 ReadMobibNFCCard(void)
{
    u8 i = 0;
    u8 tmp_len = 0;

    memset(g_tmp_card_id, 0, LEN_BYTE_SZ16);
    memset(g_tmp_serial_nr, 0, LEN_BYTE_SZ16);

    if (IsDuringBind()) {
        if (0 == start_time_nfc) {
            start_time_nfc = GetTimeStamp();
        }
    } else {
        start_time_nfc = 0;
    }

    if (read_iso14443B_nfc_card(g_tmp_card_id, g_tmp_serial_nr) > 0) {
        tmp_len = strlen((const char*)g_tmp_card_id);
        tmp_len = strlen((const char*)g_tmp_serial_nr);

         TcpReadedOneCard(g_tmp_card_id, g_tmp_serial_nr);

        if (IsDuringBind()) {
            start_time_nfc = GetTimeStamp();
            AddNewMobibCard(g_tmp_card_id, g_tmp_serial_nr);
        } else {
            for (i=0; i<LEN_MAX_CARD; i++) {
                if (0 == strncmp((const char*)g_bind_cards[i], (const char*)g_tmp_card_id, LEN_CARD_ID)) {
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
        if (start_time_nfc != 0) {
            if (isDelayTimeout(start_time_nfc,10*1000UL)) {
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

    for (i=0; i<LEN_MAX_CARD; i++) {
        if (0 == strlen((const char*)g_bind_cards[i])) {
            memcpy(g_bind_cards[i], card_id, LEN_BYTE_SZ32);
            memcpy(g_bind_cards[i]+32, serial_nr, LEN_BYTE_SZ32);
        }
    }
}

void DeleteMobibCard(u8* card_id, u8* serial_nr)
{
    u8 i = 0;

    if (NULL == card_id) {
        return;
    }

    for (i=0; i<LEN_MAX_CARD; i++) {
        if (0 == strncmp((const char*)g_bind_cards[i], (const char*)card_id, LEN_CARD_ID)) {
            memset(g_bind_cards[i], 0, LEN_BYTE_SZ64);
        }
    }
}

void DeleteAllMobibCard(void)
{
    u8 i = 0;

    for (i=0; i<LEN_MAX_CARD; i++) {
        memset(g_bind_cards[i], 0, LEN_BYTE_SZ64);
    }
}

u8 QueryMobibCard(u8* card_dats)
{
    u8 i = 0;
    u8 count = 0;
    u8 offset = 0;

    if (NULL == card_dats) {
        return 0;
    }

    for (i=0; i<LEN_MAX_CARD; i++) {
        if (strlen((const char*)g_bind_cards[i]) > 0) {
            if (count != 0) {
                card_dats[offset++] = '|';
            }

            memcpy(card_dats+offset, g_bind_cards[i], LEN_CARD_ID);

            count++;
        }
    }

    return count;
}
