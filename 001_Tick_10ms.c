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
#include "003_BG96.h"
#include "006_Gpio.h"
#include "007_Uart.h"
#include "008_RingBuffer.h"
#include "015_Common.h"

static unsigned long MobitTimesT1 = 0UL;// unit: ms
static unsigned long MobitTimesT3 = 0UL;// unit: ms
static unsigned long MobitTimesT4 = 0UL;// unit: ms
static unsigned long MobitTimesT2 = 0UL;// unit: ms

static char tmpuse_buf[RX_RINGBUF_MAX_LEN+1] = {0};

// --
// ---------------------- global variables -------------------- //
// --
extern ringbuffer_t g_at_rbuf;
extern ringbuffer_t g_net_rbuf;
extern u8 g_ring_times;

//******************************************************************************
//* Timer 1
//* Timer to make 10ms Tick
//******************************************************************************
void Configure_Tick_10ms(void)
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
    IFS0bits.T1IF = false;
    IEC0bits.T1IE = true;
#endif
}

//******************************************************************************
//* Timer 2
//* Timer to make 10ms Tick
//******************************************************************************
void Configure_Tick2_10ms(void)
{
#if 1//ndef OSC_20M_USE
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
    PR2 = 0x186;                //Period = 0.0100096 s; Frequency = 10000000 Hz; PR1 390;
    T2CON = 0x8030;             //TCKPS 1:256; TON enabled; TSIDL disabled; TCS FOSC/2; TECS SOSC; TSYNC disabled; TGATE disabled;
    IFS0bits.T2IF = false;
    IEC0bits.T2IE = true;
#endif
}

//******************************************************************************
//* Timer 1
//* Timer to make 10ms Tick
//******************************************************************************
void Configure_Tick3_10ms(void)
{
#if 0//ndef OSC_20M_USE
    T1CONbits.TCKPS = 2;  // Select 1:64 Prescaler
    TMR1 = 0x00;          // Clear timer register
    PR1 = 2500;           // Load the period value
    IPC0bits.T1IP = T1IPL;// Set Timer 1 Interrupt Priority Level
    IFS0bits.T1IF = 0;    // Clear Timer 1 Interrupt Flag
    IEC0bits.T1IE = 1;    // Enable Timer1 interrupt
    T1CONbits.TON = 1;    // Start Timer
#else
    TMR3 = 0x00;                // Clear timer register
    PR3 = 0x186;                //Period = 0.0100096 s; Frequency = 10000000 Hz; PR1 390;
    T3CON = 0x8030;             //TCKPS 1:256; TON enabled; TSIDL disabled; TCS FOSC/2; TECS SOSC; TSYNC disabled; TGATE disabled;
    IFS0bits.T3IF = false;
    IEC0bits.T3IE = true;
#endif
}

//******************************************************************************
//* Timer 1
//* Timer to make 10ms Tick
//******************************************************************************
void Configure_Tick4_10ms(void)
{
    _T4IP = 2;
    TMR4 = 0x00;                // Clear timer register
//    PR4 = 0x1F4;// 500us -> 2500 Hz
//    PR4 = 0x1A9;// 340us -> 2941 Hz
    PR4 = 0x180;// 340us -> 3255 Hz
//    PR4 = 0x188;// 340us -> 3188 Hz
//    PR4 = 0x177;// 300us
//    PR4 = 0xFA;// 200us
//    PR4 = 0x7D;// 100us
    T4CON = 0x8010;             //TCKPS 1:256; TON enabled; TSIDL disabled; TCS FOSC/2; TECS SOSC; TSYNC disabled; TGATE disabled;
    IFS1bits.T4IF = false;
    IEC1bits.T4IE = true;
}

//******************************************************************************
//* Timer 1 IRQ
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
    MobitTimesT1 += 1;
    if(MobitTimesT1 > 100000UL){
        MobitTimesT1 = 0;
    }

    if (GetLedsStatus(MAIN_LED_B)) {
        if (0 == (MobitTimesT1%2)) {
            LEDs_Ctrl(MAIN_LED_B, LED_ON);
        } else {
            LEDs_Ctrl(MAIN_LED_B, LED_OFF);
        }
    }

    if (GetLedsStatus(MAIN_LED_R)) {
        if (0 == (MobitTimesT1%2)) {
            LEDs_Ctrl(MAIN_LED_R, LED_ON);
        } else {
            LEDs_Ctrl(MAIN_LED_R, LED_OFF);
        }
    }

    if (GetLedsStatus(MAIN_LED_G)) {
        if (0 == (MobitTimesT1%2)) {
            LEDs_Ctrl(MAIN_LED_G, LED_ON);
        } else {
            LEDs_Ctrl(MAIN_LED_G, LED_OFF);
        }
    }

#if 0
    if (GetNetStatus() != 0x81) {
        if (0 == (MobitTimesT1%21)) {
            GPIOB_SetPin(1, 1);
        }

        if (0 == (MobitTimesT1%41)) {
            GPIOB_SetPin(1, 0);
        }
    }
#endif

//    if (0 == MobitTimesT1%1000) {
//        printf("1000 count...\n");
//    }

    IFS0bits.T1IF = 0;// Clear Timer1 interrupt flag
}

//******************************************************************************
//* Timer 2 IRQ
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void)
{
    int i = 0;
    static char c = 0;
    static int tmp_len = 0;
    static int cnt_after = 0;
    static int cnt_tail = 0;

    static int cnt_tail_exp = 0;
    static int msg_done = 0;

    char *p = NULL;

    MobitTimesT2 += 1;
    if(MobitTimesT2 > 100000UL){
        MobitTimesT2 = 0;
    }

    while (1) {
        // process till RX empty for a while
        if (0 == IsTmpRingBufferAvailable()) {
            WaitUartTmpRxIdle();
            break;
        }

        // printf("recv len = %d\n", IsTmpRingBufferAvailable());

        c = ReadByteFromTmpRingBuffer();

#if 0
        // when MSG1 lost tail
        // need next msg(MSG2 head) to complete MSG1
        if ('+' == c) {
            cnt_after = 1;
        } else {
            if (cnt_after != 0) {
                cnt_after++;
            }
        }
#endif

        if (('C'==c) && (tmp_len>=4)) {
            if (('U'==tmpuse_buf[tmp_len-2]) && ('R'==tmpuse_buf[tmp_len-1]) &&
                ('Q'==tmpuse_buf[tmp_len-4]) && ('I'==tmpuse_buf[tmp_len-3]) &&
                ('+'==tmpuse_buf[tmp_len-5])) {

                if (0 == cnt_after) {
                    // process uncompleted ACKs + MSGs
                    // or skip the head "\r\n" too
                    if (tmp_len > 5) {// XXXs + "+QIUR"
                        printf("Receive ACKs: ");

                        for (i=0; i<(tmp_len-5); i++) {
                            printf("%c", tmpuse_buf[i]);
                            ringbuffer_write_byte(&g_at_rbuf,tmpuse_buf[i]);
                        }
                        printf("<<<\n");

                        // if tail->head, printf may NG
                        // printf("Total ACKs = %s<<<\n", g_at_rbuf.head);
                    }
                } else {// process uncompleted MSGs + new MSGs again:
                    // just skip the previous uncompleted MSGs
                }

                // skip or redirection the previous head MSGs/ACKs
                tmp_len = 5;
                cnt_after = 5;

                msg_done = 0;
                cnt_tail = 0;
                cnt_tail_exp = 0;

                memset(tmpuse_buf, 0, RX_RINGBUF_MAX_LEN);
                strncpy(tmpuse_buf, "+QIUR", strlen("+QIUR"));
            }
        } else {
            if (cnt_after != 0) {
                cnt_after++;
            }
        }

        if (tmp_len < (RX_RINGBUF_MAX_LEN-1)) {
            tmpuse_buf[tmp_len++] = c;
        }

        // "+QQQ"
        // "\r\n+QQQ"
        // "\r\n\r\n+QQQ"
        if (cnt_after >= sizeof("+QIURC: \"recv\"")) {
            if ((p=strstr(tmpuse_buf, "+QIURC: ")) != NULL) {
                if (0 == strncmp(p+sizeof("+QIURC: \"")-1, "recv", sizeof("recv")-1)) {
                    cnt_tail_exp = 2;
                } else {
                    cnt_tail_exp = 1;
                }

                // printf("len=%d, str = %s\n", sizeof("recv"), p+sizeof("+QIURC: \""));
            }
        }

        if (tmp_len >= 2) {
            if (('>'==tmpuse_buf[tmp_len-2]) && (' '==tmpuse_buf[tmp_len-1])) {
                if (tmp_len < 100) {
                    printf("Receive XACKs(%dB): %s<<<\n", tmp_len, tmpuse_buf);
                }

                for (i=0; i<tmp_len; i++) {
                    ringbuffer_write_byte(&g_at_rbuf,tmpuse_buf[i]);
                }

                //printf("Total XACKs = %s\n", g_at_rbuf.head);

                msg_done = 0;
                cnt_tail = 0;
                cnt_tail_exp = 0;
                tmp_len = 0;
                memset(tmpuse_buf, 0, RX_RINGBUF_MAX_LEN);
            }
        }

        if (tmp_len > 4) {// skip "\r\n" & "\r\n\r\n"
            if (('\r'==tmpuse_buf[tmp_len-2]) && ('\n'==tmpuse_buf[tmp_len-1])) {
                if (cnt_tail_exp != 0) {
                    cnt_tail++;

                    if (cnt_tail_exp == cnt_tail) {
                        cnt_after = 0;
                        msg_done = 1;
                    }
                } else {
                    msg_done = 1;
                }

                if (1 == msg_done) {
                    if ((cnt_tail_exp==cnt_tail) && (cnt_tail_exp!=0)) {
                        printf("Receive MSGs: %s<<<\n", tmpuse_buf);

                        if ((p=strstr(tmpuse_buf, "+QIURC: ")) != NULL) {
                            if ((p=strstr(tmpuse_buf, "closed")) != NULL) {// TCP mode only
                                SetNetStatus(0x80);
                                printf("******* TCP Closed *******\n");
                            } else if ((p=strstr(tmpuse_buf, "recv")) != NULL) {// TCP or UDP
                                cnt_tail = 0;

                                ringbuffer_write_byte(&g_net_rbuf,'S');
                                ringbuffer_write_byte(&g_net_rbuf,'T');
                                ringbuffer_write_byte(&g_net_rbuf,'A');

                                for (i=(p-tmpuse_buf)+1; i<tmp_len; i++) {
                                    if (('\r'==tmpuse_buf[i-1]) && ('\n'==tmpuse_buf[i])) {
                                        cnt_tail++;
                                        if (cnt_tail >= 2) {
                                            break;
                                        }
                                        continue;
                                    }

                                    if (cnt_tail >= 1) {
                                        if (('\r'!=tmpuse_buf[i]) && ('\n'!=tmpuse_buf[i])) {
                                            ringbuffer_write_byte(&g_net_rbuf,tmpuse_buf[i]);
                                        }
                                    }
                                }

                                ringbuffer_write_byte(&g_net_rbuf,'E');
                                ringbuffer_write_byte(&g_net_rbuf,'N');
                                ringbuffer_write_byte(&g_net_rbuf,'D');

                                // printf("MSGs = %s\n", g_net_rbuf.head);
                            } else if ((p=strstr(tmpuse_buf, "incoming full")) != NULL) {// BG96 act as TCP server
                                printf("******* incoming full *******\n");
                            } else if ((p=strstr(tmpuse_buf, "incoming")) != NULL) {// BG96 act as TCP server
                                printf("******* incoming *******\n");
                            } else if ((p=strstr(tmpuse_buf, "pdpdeact")) != NULL) {
                                printf("******* pdpdeact *******\n");
                            } else {// invalid MSGs
                                printf("******* invalid MSGs *******\n");
                            }
                        } else {
                            // invalid MSGs
                        }
                    } else {
                        if (tmp_len < 100) {
                            printf("Receive2 XACKs(%dB): %s<<<\n", tmp_len, tmpuse_buf);
                        }

                        for (i=0; i<tmp_len; i++) {
                            ringbuffer_write_byte(&g_at_rbuf,tmpuse_buf[i]);
                            // printf("ringbuffer->writepos=%d\n", g_at_rbuf.writepos);
                        }

                        //printf("Total XACKs = %s\n", g_at_rbuf.head);
                    }

                    msg_done = 0;
                    cnt_tail = 0;
                    cnt_tail_exp = 0;
                    tmp_len = 0;
                    memset(tmpuse_buf, 0, RX_RINGBUF_MAX_LEN);
                }
            }
        }
    }

    if (0 == MobitTimesT2 % 300) {
        // printf("timer2 trigger\r\n");
        printf("recv len = %d\n", IsTmpRingBufferAvailable());
    }

    IFS0bits.T2IF = 0;// Clear Timer2 interrupt flag
}

//******************************************************************************
//* Timer 1 IRQ
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T3Interrupt(void)
{
    MobitTimesT3 += 1;
    if(MobitTimesT3 > 100000UL){
        MobitTimesT3 = 0;
    }
    
    if (0 == MobitTimesT3%1000) {
        printf("1000 count...\n");
    }

    IFS0bits.T3IF = 0;// Clear Timer1 interrupt flag
}

static u8 gs_beep_ring = 0;
//******************************************************************************
//* Timer 4 IRQ -> 340us
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T4Interrupt(void)
{
    MobitTimesT4 += 1;
    if(MobitTimesT4 > 1000000UL){
        MobitTimesT4 = 0;
    }
    
    if (0 == MobitTimesT4%100000) {
        printf("T4 1000 count...\n");
    }
    
    if (0 == MobitTimesT4%300) {// 102ms
        if (g_ring_times > 0) {
            g_ring_times--;
        }
    }
    
    if ((g_ring_times>0) && (0 == g_ring_times%2)) {
        if (0 == MobitTimesT4%2) {
            GPIOx_Output(BANKB, 13, 1);
        }
        if (1 == MobitTimesT4%2) {
            GPIOx_Output(BANKB, 13, 0);
        }
    }

    IFS1bits.T4IF = 0;// Clear Timer1 interrupt flag
}

//******************************************************************************************
// FuncName: DelayMs
// Descriptions: delay some time(unit: millisecond)
//           (IN) val: how long to delay
// Return:   NONE
//******************************************************************************************
void delay_ms(unsigned long val)
{
    unsigned long start_time = GetTimeStamp();

    while (!isDelayTimeout(start_time,val));
}

//******************************************************************************************
// FuncName: DelayMs
// Descriptions: delay some time(unit: millisecond)
//           (IN) val: how long to delay
// Return:   NONE
//******************************************************************************************
void DelayMs(unsigned long val)
{
    unsigned long start_time = GetTimeStamp();

    while (!isDelayTimeout(start_time,val));
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
