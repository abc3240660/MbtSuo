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
#ifdef DEMO_BOARD
    CM2CON = 0;
    ODCB &= 0xFFF0;// Open-Drain Control
    // AD1PCFGL |= 0x000F;
    LATB |= 0X0000;// Output Value:0-OFF 1-ON
    TRISB &= 0XFF00;// Direction:0-OUT 1-IN
#else
    CM2CON = 0;
    ODCD &= 0xFFF0;// Open-Drain Control
    // AD1PCFGL |= 0x000F;
    LATD |= 0X0000;// Output Value:0-OFF 1-ON
    TRISD &= 0X0000;// Direction:0-OUT 1-IN
#endif
}

void GPIOB_SetPin(short pin,char value)
{
#ifdef DEMO_BOARD
    // BANKB: for read
    // LATB: for read -> modify -> write
    if(value){
        LATB |= (1<<pin);
    }else{
        LATB &= ~(1<<pin);
    }
#else
    if(value){
        LATD |= (1<<pin);
    }else{
        LATD &= ~(1<<pin);
    }
#endif
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
    LATD |= ~0xFF8F;// Output Value:0-OFF 1-ON
}

void LEDs_AllOff(void)
{
    LATD &= 0xFF8F;// Output Value:0-OFF 1-ON
}

void LEDs_Init(void)
{
    // config RD1/2/3/4/5/6 into output
    TRISD &= 0xFF8F;// Direction:0-OUT 1-IN

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


void InOutPurpose_Init(void)
{
    // IN:  RB0  for Voltage Mesurement(using ADC)
    // OUT: RB1  for CLRC663 IF3: output 1
    // OUT: RB2  for CLRC663 TX(config in later UARTx_Init)
    // IN:  RB3  for BG96: 1-power on finished
    // = 9
    // XX:  RB4  for CLRC663 IF1: unused
    // IN:  RB5  for CLRC663 RX(config in later UARTx_Init)
    // XX:  RB6  for ICSP use
    // XX:  RB7  for ICSP use
    // = F

    // OUT: RB8  for BG96: to detect if host is active or sleep
    // OUT: RB9  for BG96: to power up BG96
    // OUT: RB10 for BG96: 1-normal mode, 0-airplane mode
    // OUT: RB11 for BG96: reset
    // = 0

    // IN:  RB12 for BG96: 1-normal mode, 0-PSM mode
    // OUT: RB13 for BUZZER
    // OUT: RB14 for BG96 TX(config in later UARTx_Init)
    // IN:  RB15 for TouchPad IRQ
    // = 9

    TRISB &= 0x90F9;// Direction:0-OUT 1-IN

    // OUT: RB13 for BNO055: reset
    // OUT: RB14 for BNO055: 1-normal mode, 0-IAP
    // XX:  Other for unused
    TRISC &= 0xFF81;// Direction:0-OUT 1-IN

    // IN:  RD0 for BNO055 IRQ
    // OUT: RD1/2/3/4/5/6 for LEDs
    // XX:  RD7 for unused
    // OUT: RD8/9 for Motor
    // IN:  RD10 for BNO055 RX(config in later UARTx_Init)
    // OUT: RD11 for BNO055 TX(config in later UARTx_Init)
    // XX:  RD12~RD15 for unused
    TRISD &= 0xFF81;// Direction:0-OUT 1-IN

    // IN:  RE0 for BG96 UART DCD
    // OUT: RE1 for BG96 UART RTS
    // IN:  RE2 for BG96 URC IRQ
    // IN:  RE3 for CLRC663 IRQ
    // = D
    // IN:  RE4 for SW1 IRQ
    // OUT: RE5/6/7 for CLRC663
    // = 1
    // XX:  RE8~RE15 for unused
    // FF
    TRISE &= 0xFF81;// Direction:0-OUT 1-IN

    // IN:  RF0 for BG96 UART CTS
    // OUT: RF1 for BG96 UART DTR
    // IN:  RF2 for Charge Status(need internally pull-up): 0-during charging, 1-normal mode
    // XX:  RF3 for unused
    // OUT: RF4 for TouchPad: 0-low power mode, 1-normal mode
    // IN:  RF5 for SW2 IRQ
    // XX:  RF6~RD15 for unused
    TRISF &= 0xFF81;// Direction:0-OUT 1-IN

    // XX:  RG0~RG2 for unused
    // OUT: RG3 for Charge Enable: 1-disable charge, 0-allow charge
    // XX:  RG4~RG5 for unused
    // OUT: RG6 for BG96 RX(config in later UARTx_Init)
    // IN:  RG7 for BG96_GPS RX(config in later UARTx_Init)
    // OUT: RG8 for BG96_GPS TX(config in later UARTx_Init)
    // OUT: RG9 for TouchPad: sensitivity PWM
    // XX:  RG10~RG15 for unused
    TRISG &= 0xFF81;// Direction:0-OUT 1-IN
}
