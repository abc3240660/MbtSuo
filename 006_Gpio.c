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

#include <stdio.h>
#include <p24fxxxx.h>

#include "006_Gpio.h"

void GPIOB_Init(void)
{
    CM2CON = 0;
    ODCB &= 0xFFF0;// Open-Drain Control
    // AD1PCFGL |= 0x000F;
    LATB |= 0X0000;// Output Value:0-OFF 1-ON
    TRISB &= 0XFF00;// Direction:0-OUT 1-IN
}

void GPIOB_SetPin(short pin,char value)
{
    // BANKB: for read
    // LATB: for read -> modify -> write
    if(value){
        LATB |= (1<<pin);
    }else{
        LATB &= ~(1<<pin);
    }
}

void GPIOx_Config(GPIO_BANKx port, u8 pin, GPIO_DIR dir)
{
    if (BANKB == port) {
        if(dir){
            TRISB |= (1<<pin);
        } else {
            TRISB &= ~(1<<pin);
        }
    } else if (BANKC == port) {
        if(dir){
            TRISC |= (1<<pin);
        } else {
            TRISC &= ~(1<<pin);
        }
    } else if (BANKD == port) {
        if(dir){
            TRISD |= (1<<pin);
        } else {
            TRISD &= ~(1<<pin);
        }
    } else if (BANKE == port) {
        if(dir){
            TRISE |= (1<<pin);
        } else {
            TRISE &= ~(1<<pin);
        }
    } else if (BANKF == port) {
        if(dir){
            TRISF |= (1<<pin);
        } else {
            TRISF &= ~(1<<pin);
        }
    } else if (BANKG == port) {
        if(dir){
            TRISG |= (1<<pin);
        } else {
            TRISG &= ~(1<<pin);
        }
    }
}

void GPIOx_Output(GPIO_BANKx port, u8 pin, u8 value)
{
    if (BANKB == port) {
        if(value){
            LATB |= (1<<pin);
        } else {
            LATB &= ~(1<<pin);
        }
    } else if (BANKC == port) {
        if(value){
            LATC |= (1<<pin);
        } else {
            LATC &= ~(1<<pin);
        }
    } else if (BANKD == port) {
        if(value){
            LATD |= (1<<pin);
        } else {
            LATD &= ~(1<<pin);
        }
    } else if (BANKE == port) {
        if(value){
            LATE |= (1<<pin);
        } else {
            LATE &= ~(1<<pin);
        }
    } else if (BANKF == port) {
        if(value){
            LATF |= (1<<pin);
        } else {
            LATF &= ~(1<<pin);
        }
    } else if (BANKG == port) {
        if(value){
            LATG |= (1<<pin);
        } else {
            LATG &= ~(1<<pin);
        }
    }
}

u8 GPIOx_Input(GPIO_BANKx port, u8 pin)
{
    if (BANKB == port) {
        if (PORTB & (1<<pin)) {
            return 1;
        }
    } else if (BANKC == port) {
        if (PORTC & (1<<pin)) {
            return 1;
        }
    } else if (BANKD == port) {
        if (PORTD & (1<<pin)) {
            return 1;
        }
    } else if (BANKE == port) {
        if (PORTE & (1<<pin)) {
            return 1;
        }
    } else if (BANKF == port) {
        if (PORTF & (1<<pin)) {
            return 1;
        }
    } else if (BANKG == port) {
        if (PORTG & (1<<pin)) {
            return 1;
        }
    }

    return 0;
}

/*
RD3 -> LED_RED_E 	-> 	EXT_RED
RD2 -> LED_GREEN_E -> 	EXT_GREEN
RD1 -> LED_BLUE_E 	-> 	EXT_BLUE

RD6 -> LED_GREEN
RD5 -> LED_RED
RD4 -> LED_BLUE
*/

void LEDs_AllON(void)
{
    LATD |= ~0xFF81;// Output Value:0-OFF 1-ON
}

void LEDs_AllOff(void)
{
    LATD &= 0xFF81;// Output Value:0-OFF 1-ON
}

void LEDs_Init(void)
{
    // config RD1/2/3/4/5/6 into output
    TRISD &= 0xFF81;// Direction:0-OUT 1-IN

    // ON all to indicate powerup
    LEDs_AllON();
}

// PORTx: just for read
// LATx: read -> modify -> write
void LEDs_Ctrl(LED_INDEX led_id,LED_STA led_sta)
{
    if(led_sta){
        LATD |= (1<<led_id);
    }else{
        LATD &= (~1<<led_id);
    }
}
