//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * A library for PIC24F GPIO Configuration
 * This file is about the GPIO API of PIC24
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 03/11/2019
******************************************************************************/

#ifndef GPIO_H
#define GPIO_H

#include "015_Common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIN_LED_R = 0,
    MAIN_LED_G,
    MAIN_LED_B,
    EXTE_LED_R,
    EXTE_LED_G,
    EXTE_LED_B,
} LED_INDEX;

typedef enum {
    LED_OFF = 0,
    LED_ON,
} LED_STA;

typedef enum {
    BANKB = 0,
    BANKC,
    BANKD,
    BANKE,
    BANKF,
    BANKG,
} GPIO_BANKx;

typedef enum {
    OUTPUT_DIR = 0,
    INPUT_DIR,
} GPIO_DIR;

void GPIOB_Init(void);
void GPIOB_SetPin(short pin,char value);
void GPIOx_Config(GPIO_BANKx port, u8 pin, GPIO_DIR dir);
void GPIOx_Output(GPIO_BANKx port, u8 pin, u8 value);
u8 GPIOx_Input(GPIO_BANKx port, u8 pin);

void LEDs_Init(void);
void LEDs_Ctrl(LED_INDEX led_id,LED_STA led_sta);
void LEDs_AllON(void);
void LEDs_AllOff(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
