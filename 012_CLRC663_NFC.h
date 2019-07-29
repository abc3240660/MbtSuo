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

#ifndef CLRC663_NFC_H
#define CLRC663_NFC_H

#ifdef __cplusplus
extern "C" {
#endif


#include "002_CLRC663.h"

#define bitSet(x,n) (x|=0x01<<n)
    
//#define DEBUG_ISO14443B
//#define PRINT_IDENTIFICATION_NUMBERS
    
uint16_t clrc663_SPI_transfer(const uint8_t* tx, uint8_t* rx, uint16_t len);
void clrc663_SPI_select();
void clrc663_SPI_unselect();
void read_iso14443B_nfc_card();
uint16_t clrc663_SPI_transfer(const uint8_t* tx, uint8_t* rx, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* CLRC663_NFC_H */

