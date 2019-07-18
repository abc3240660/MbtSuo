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

#ifdef __cplusplus
}
#endif

#endif /* GPIO_H */
