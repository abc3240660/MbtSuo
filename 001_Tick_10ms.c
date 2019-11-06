//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

//******************************************************************************
// File:   001_Tick_10ms.h
// Author: Hans Desmet
// Comments: Initial Real time 
// Revision history: Version 01.00
// Date 26/12/2018
//******************************************************************************

#include "001_Tick_10ms.h"
#include "006_Gpio.h"
#include "007_Uart.h"
#include "015_Common.h"

static unsigned long MobitTimesT1 = 0UL;// unit: ms
static unsigned long MobitTimesT2 = 0UL;// unit: ms
// static unsigned long MobitTimesT3 = 0UL;// unit: ms
// static unsigned long MobitTimesT4 = 0UL;// unit: ms

// --
// ---------------------- global variables -------------------- //
// --
extern u8 g_ring_times;

//******************************************************************************
//* Timer 1
//* Timer to make 10ms Tick
//******************************************************************************
void Configure_Tick1_10ms(void)
{
#if 0//ndef OSC_20M_USE
    T1CONbits.TCKPS = 2;  // Select 1:64 Prescaler
    TMR1 = 0x00;          // Clear timer register
    // Fcy = Fosc/2 = 16M
    // 2500*(1/(16M/64)) = 2500*4us = 10ms
    PR1 = 2500;           // Load the period value
    IPC0bits.T1IP = T1IPL;// Set Timer 1 Interrupt Priority Level
    IFS0bits.T1IF = 0;    // Clear Timer 1 Interrupt Flag
    IEC0bits.T1IE = 1;    // Enable Timer1 interrupt
    T1CONbits.TON = 1;    // Start Timer
#else
    TMR1 = 0x00;                // Clear timer register
    // Fcy = Fosc/2 = 10M
    // 390*(1/(10M/256)) = 9984us = 10ms
    PR1 = 0x186;                //Period = 0.0100096 s; Frequency = 10000000 Hz; PR1 390;
    T1CON = 0x8030;             //TCKPS 1:256; TON enabled; TSIDL disabled; TCS FOSC/2; TECS SOSC; TSYNC disabled; TGATE disabled;
    IPC0bits.T1IP = IPL_MID;// Set Timer 1 Interrupt Priority Level
    IFS0bits.T1IF = false;
    IEC0bits.T1IE = true;
#endif
}

//******************************************************************************
//* Timer 2
//* Timer to make 100ms Tick
//******************************************************************************
void Configure_Tick2_10ms(void)
{
#if 0//ndef OSC_20M_USE
    T2CONbits.T32 = 0;
    T2CONbits.TCKPS = 2;  // Select 1:64 Prescaler
    TMR2 = 0x00;          // Clear timer register
    PR2 = 2500;           // Load the period value
    IPC1bits.T2IP = T2IPL;// Set Timer 2 Interrupt Priority Level
    IFS0bits.T2IF = 0;    // Clear Timer 2 Interrupt Flag
    IEC0bits.T2IE = 1;    // Enable Timer2 interrupt
    T2CONbits.TON = 1;    // Start Timer
#else
    TMR2 = 0x00;                // Clear timer register
    // 3906*(1/(10M/256)) = 100ms
    PR2 = 3906;
    T2CON = 0x8038;             //TCKPS 1:256; TON enabled; TSIDL disabled; TCS FOSC/2; TECS SOSC; TSYNC disabled; TGATE disabled;
    IPC1bits.T2IP = IPL_LOW;// Set Timer 2 Interrupt Priority Level
    IFS0bits.T2IF = false;
    IEC0bits.T2IE = true;
#endif
}

void Enable_Tick2(void)
{
    TMR2 = 0x00;// Clear timer register
    PR2 = 3906; // reload the count

    T2CONbits.TON = 1;
}

void Disable_Tick2(void)
{
    T2CONbits.TON = 0;
}

//******************************************************************************
//* Timer 1 IRQ: 10ms
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
    MobitTimesT1 += 1;
    if(MobitTimesT1 > 100000UL){
        MobitTimesT1 = 0;
    }

    // LED: ON 10ms then OFF 40ms
    if (GetLedsStatus(MAIN_LED_B)) {
        if (0 == (MobitTimesT1%5)) {
            // Blink: ON 200ms then OFF 800ms
            if (GetLedsMode(MAIN_LED_B)) {
                if ((MobitTimesT1%100 < 20)) {
                    LEDs_Ctrl(MAIN_LED_B, LED_ON);
                }
            } else {
                LEDs_Ctrl(MAIN_LED_B, LED_ON);
            }
        } else {
            LEDs_Ctrl(MAIN_LED_B, LED_OFF);
        }
    }

    // LED: ON 10ms then OFF 40ms
    if (GetLedsStatus(MAIN_LED_R)) {
        if (0 == (MobitTimesT1%5)) {
            // Blink: ON 200ms then OFF 800ms
            if (GetLedsMode(MAIN_LED_R)) {
                if ((MobitTimesT1%100 < 20)) {
                    LEDs_Ctrl(MAIN_LED_R, LED_ON);
                }
            } else {
                LEDs_Ctrl(MAIN_LED_R, LED_ON);
            }
        } else {
            LEDs_Ctrl(MAIN_LED_R, LED_OFF);
        }
    }

    // LED: ON 10ms then OFF 40ms
    if (GetLedsStatus(MAIN_LED_G)) {
        if (0 == (MobitTimesT1%5)) {
            // Blink: ON 200ms then OFF 800ms
            if (GetLedsMode(MAIN_LED_G)) {
                if ((MobitTimesT1%100 < 20)) {
                    LEDs_Ctrl(MAIN_LED_G, LED_ON);
                }
            } else {
                LEDs_Ctrl(MAIN_LED_G, LED_ON);
            }
        } else {
            LEDs_Ctrl(MAIN_LED_G, LED_OFF);
        }
    }

//    if (0 == MobitTimesT1%1000) {
//        printf("1000 count...\n");
//    }

    IFS0bits.T1IF = 0;// Clear Timer1 interrupt flag
}

//******************************************************************************
//* Timer 2 IRQ: 100ms
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void)
{
    u32 loop_time = 0;

    MobitTimesT2 += 1;
    if(MobitTimesT2 > 100000UL){
        MobitTimesT2 = 0;
    }

    // 330us * 3000 = 1s
    loop_time = g_ring_times * 3000;

    // Period 330us -> 3030Hz
    while (loop_time--) {
        // ON 330ms then OFF 660ms
        if (loop_time%3000 < 1000) {
            Beep_High();
        }

        delay_us_nop(165);
        Beep_Low();
        delay_us_nop(165);
    }

    Disable_Tick2();

    IFS0bits.T2IF = 0;// Clear Timer2 interrupt flag
}

//******************************************************************************************
// FuncName: delay_ms
// Descriptions: delay some time(unit: millisecond)
//           (IN) val: how long to delay
// Return:   NONE
//******************************************************************************************
void delay_ms(unsigned long val)
{
    unsigned long start_time = GetTimeStamp();

    while (!isDelayTimeout(start_time,val));
}

// Delay Unit: 1ms
//******************************************************************************************
// FuncName: delay_ms_nop
// Descriptions: delay some time(unit: millisecond)
//           (IN) val: how long to delay
// Return:   NONE
//******************************************************************************************
void delay_ms_nop(u32 cnt)
{
    u32 i = 0;
    u32 j = 0;

    for (i=0; i<cnt; i++)
        for (j=0; j<1150; j++);
}

// Delay Unit: 1us
//******************************************************************************************
// FuncName: delay_us_nop
// Descriptions: delay some time(unit: millisecond)
//           (IN) val: how long to delay
// Return:   NONE
//******************************************************************************************
void delay_us_nop(u32 cnt)
{
    u32 i = 0;
    u32 j = 0;

    for (i=0; i<cnt; i++)
        for (j=0; j<1; j++);
}


//******************************************************************************************
// FuncName: GetTimeStamp
// Descriptions: get the time stamp after program started
// Return:   number of milliseconds passed since the program started
//******************************************************************************************
unsigned long GetTimeStamp()
{
    return MobitTimesT1;
}

//******************************************************************************************
// FuncName: isDelayTimeout
// Descriptions: determine whether the timeout
// Return:   timeout->true,no timeout->false
//******************************************************************************************
bool isDelayTimeout(unsigned long start_time,unsigned long delayms)
{
    // delay at least 10ms
    if(delayms < 10){
        delayms = 10;
    }
    if(MobitTimesT1 >= start_time){
        if((MobitTimesT1 - start_time) > (delayms/10UL)){
            return 1;
        }
    }else{
        if((100000UL-start_time+MobitTimesT1) > (delayms/10UL)){
            return 1;
        }
    }
    return 0;
}

//******************************************************************************
//* END OF FILE
//******************************************************************************
