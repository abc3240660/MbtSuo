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

// Must be multiple of 10ms
// Default 10ms ON + 10ms OFF
static u32 gs_beep_all_time = 100;// Beep ON + OFF period
static u32 gs_beep_on_time  = 50; // Beep Only ON time

// Only for blink period if blink enabled
// Must be multiple of 10ms
// Default 100ms PWM + 100ms all OFF
static u32 gs_leds_all_time = 200;// LED PWM ON+IDEL time
static u32 gs_leds_on_time  = 100; // LED PWM ON time

static unsigned long MobitTimesT1 = 0UL;// unit: ms
static unsigned long MobitTimesT2 = 0UL;// unit: ms
static unsigned long MobitTimesT3 = 0UL;// unit: ms
static unsigned long MobitTimesT4 = 0UL;// unit: ms


static char tmpuse_buf[RX_RINGBUF_MAX_LEN+1] = {0};

// --
// ---------------------- global variables -------------------- //
// --
extern ringbuffer_t g_at_rbuf;
extern ringbuffer_t g_net_rbuf;
extern u8 g_ring_times;
extern u32 g_led_times;
extern u32 g_led_always_on;

//******************************************************************************
//* Timer 1
//* Timer to make 2ms Tick
//******************************************************************************
void Configure_Tick1(void)
{
    TMR1 = 0x00;              // Clear timer register
    T1CONbits.TCKPS = 2;      // Select 1:64 Prescaler

    // Fcy = Fosc/2 = 16M
    // 250*(1/(16M/64)) = 250*4us = 1ms
    PR1 = 500;                // Load the period value

    IPC0bits.T1IP = IPL_MID;  // Set Timer1 Interrupt Priority Level
    IFS0bits.T1IF = 0;        // Clear Timer1 Interrupt Flag
    IEC0bits.T1IE = 1;        // Enable Timer1 interrupt
    T1CONbits.TON = 1;        // Start Timer
}

//******************************************************************************
//* Timer 2
//* Timer to make 10ms Tick
//******************************************************************************
void Configure_Tick2(void)
{
    T2CONbits.T32 = 0;        // Timer2/3 -> Two 16 Bit Timer

    TMR2 = 0x00;              // Clear timer register
    T2CONbits.TCKPS = 2;      // Select 1:64 Prescaler

    // Fcy = Fosc/2 = 16M
    // 2500*(1/(16M/64)) = 2500*4us = 10ms
    PR2 = 2500;               // Load the period value

    IPC1bits.T2IP = IPL_MID;  // Set Timer2 Interrupt Priority Level
    IFS0bits.T2IF = 0;        // Clear Timer2 Interrupt Flag
    IEC0bits.T2IE = 1;        // Enable Timer2 interrupt
    T2CONbits.TON = 1;        // Start Timer
}

//******************************************************************************
//* Timer 3
//* Timer to make 100ms Tick
//******************************************************************************
void Configure_Tick3(void)
{
    T2CONbits.T32 = 0;        // Timer2/3 -> Two 16 Bit Timer

    TMR3 = 0x00;              // Clear timer register
    T3CONbits.TCKPS = 2;      // Select 1:64 Prescaler

    // Fcy = Fosc/2 = 16M
    // 25000*(1/(16M/64)) = 25000*4us = 100ms
    PR3 = 25000;              // Load the period value

    _T3IP = IPL_LOW;          // Set Timer2 Interrupt Priority Level
    _T3IF = 0;                // Clear Timer2 Interrupt Flag
    _T3IE = 1;                // Enable Timer2 interrupt
    T3CONbits.TON = 1;        // Start Timer
}

void Enable_Tick1(void)
{
	return;
    TMR1 = 0x00;              // Clear timer register
    PR1 = 250;                // Load the period value
    T1CONbits.TON = 1;
}

void Disable_Tick1(void)
{
	return;
    T1CONbits.TON = 0;
}

void Enable_Tick2(void)
{
	return;
    TMR2 = 0x00;              // Clear timer register
    PR2 = 2500;               // Load the period value
    T2CONbits.TON = 1;
}

void Disable_Tick2(void)
{
	return;
    T2CONbits.TON = 0;
}


void Enable_Tick3(void)
{
	return;
    TMR3 = 0x00;              // Clear timer register
    PR3 = 25000;              // Load the period value
    T3CONbits.TON = 1;
}

void Disable_Tick3(void)
{
	return;
    T3CONbits.TON = 0;
}

//******************************************************************************
//* Timer 1 IRQ: 2ms
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
    MobitTimesT1 += 1;
    if(MobitTimesT1 > 10000000UL){
        MobitTimesT1 = 0;
    }

    if (0 == MobitTimesT1%10000) {
        // DEBUG("T1 10s...\n");
    }

    IFS0bits.T1IF = 0;// Clear Timer1 interrupt flag
}

//******************************************************************************
//* Timer 2 IRQ: 10ms
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void)
{
    static u8 start_flag = 0;
    static u32 loop_time = 0;
    int jg=100;//1s
    MobitTimesT2 += 1;
    if(MobitTimesT2 > 10000000UL){
        MobitTimesT2 = 0;
    }

#if 1// Beep Ctrl
    if (start_flag) {
        if (loop_time%(gs_beep_all_time/10) < (gs_beep_on_time/10)) {
            Beep_High();
        } else {
            Beep_Low();
        }

        loop_time++;

        if (loop_time >= (gs_beep_all_time/10)) {
            start_flag = 0;
            loop_time = 0;
        }
    } else {
        Beep_Low();
        loop_time = 0;
    }

    // Wait till a whole period's begin
    if (0 == MobitTimesT2%(gs_beep_all_time/10)) {
        if (g_ring_times > 0) {
            g_ring_times--;
            loop_time = 0;
            start_flag = 1;
        }
    }
#endif

#if 1// LEDs Ctrl
    if (0 == MobitTimesT2%jg) {// 10ms ON + 10ms OFF
        if (GetLedsStatus(MAIN_LED_B)) {
            if (GetLedsMode(MAIN_LED_B)) {
                if ((MobitTimesT2%(gs_leds_all_time/10) < (gs_leds_on_time/10))) {
                    LEDs_Ctrl(MAIN_LED_B, LED_ON);
                }
            } else {
                LEDs_Ctrl(MAIN_LED_B, LED_ON);
            }
        }

        if (GetLedsStatus(MAIN_LED_R)) {
            if (GetLedsMode(MAIN_LED_R)) {
                if ((MobitTimesT2%(gs_leds_all_time/10) < (gs_leds_on_time/10))) {
                    LEDs_Ctrl(MAIN_LED_R, LED_ON);
                }
            } else {
                LEDs_Ctrl(MAIN_LED_R, LED_ON);
            }
        }

        if (GetLedsStatus(MAIN_LED_G)) {
            if (GetLedsMode(MAIN_LED_G)) {
                if ((MobitTimesT2%(gs_leds_all_time/10) < (gs_leds_on_time/10))) {
                    LEDs_Ctrl(MAIN_LED_G, LED_ON);
                }
            } else {
                LEDs_Ctrl(MAIN_LED_G, LED_ON);
            }
        }
    } else if( (jg/2 == MobitTimesT2%jg) && (g_led_always_on == 0)){
        LEDs_Ctrl(MAIN_LED_B, LED_OFF);
        LEDs_Ctrl(MAIN_LED_R, LED_OFF);
        LEDs_Ctrl(MAIN_LED_G, LED_OFF);
    }
#endif

#if 0// Purple
    if (0 == MobitTimesT2%2) {
        GPIOx_Output(BANKD, MAIN_LED_R, 1);
    } else {
        GPIOx_Output(BANKD, MAIN_LED_R, 0);
    }
    if (0 == MobitTimesT2%2) {
        GPIOx_Output(BANKD, MAIN_LED_B, 1);
    } else {
        GPIOx_Output(BANKD, MAIN_LED_B, 0);
    }
#endif

#if 0// Orange
    if (0 == MobitTimesT2%3) {
        GPIOx_Output(BANKD, MAIN_LED_R, 1);
    } else {
        GPIOx_Output(BANKD, MAIN_LED_R, 0);
    }
    if (0 == MobitTimesT2%5) {
        GPIOx_Output(BANKD, MAIN_LED_G, 1);
    } else {
        GPIOx_Output(BANKD, MAIN_LED_G, 0);
    }
#endif

    if (g_led_times > 0) {
        if (g_led_times < 10) {
            g_led_times = 0;
        } else {
            g_led_times -= 10;// TM2 Unit = 10ms
        }
    }
    
    if (0 == g_led_times) {
        SetLedsStatus(MAIN_LED_B, LED_OFF);
        SetLedsStatus(MAIN_LED_G, LED_OFF);
        SetLedsStatus(MAIN_LED_R, LED_OFF);
        g_led_always_on=0;
    }

    if (0 == MobitTimesT2%1000) {
        // DEBUG("T2 10s...\n");
    }

    IFS0bits.T2IF = 0;// Clear Timer2 interrupt flag
}

//******************************************************************************
//* Timer 3 IRQ: 100ms
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T3Interrupt(void)
{
    int i = 0;
    static char c = 0;
    static int tmp_len = 0;
    static int cnt_after = 0;
    static int cnt_tail = 0;

    static int cnt_tail_exp = 0;
    static int msg_done = 0;

    char *p = NULL;

    MobitTimesT3 += 1;
    if(MobitTimesT3 > 10000000UL){
        MobitTimesT3 = 0;
    }

    while (1) {
        // process till RX empty for a while
        if (0 == IsTmpRingBufferAvailable()) {
            WaitUartTmpRxIdle();

			if(0 == IsTmpRingBufferAvailable()){
				break;
			}
        }

        // DEBUG("recv len = %d\n", IsTmpRingBufferAvailable());

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
                        DEBUG("Receive ACKs: ");

                        for (i=0; i<(tmp_len-5); i++) {
                            DEBUG("%c", tmpuse_buf[i]);
                            ringbuffer_write_byte(&g_at_rbuf,tmpuse_buf[i]);
                        }
                        DEBUG("<<<\n");

                        // if tail->head, DEBUG may NG
                        // DEBUG("Total ACKs = %s<<<\n", g_at_rbuf.head);
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

                // DEBUG("len=%d, str = %s\n", sizeof("recv"), p+sizeof("+QIURC: \""));
            }
        }

        if (tmp_len >= 2) {
            if (('>'==tmpuse_buf[tmp_len-2]) && (' '==tmpuse_buf[tmp_len-1])) {
                if (tmp_len < 100) {
                    DEBUG("Receive XACKs(%dB): %s<<<\n", tmp_len, tmpuse_buf);
                }

                for (i=0; i<tmp_len; i++) {
                    ringbuffer_write_byte(&g_at_rbuf,tmpuse_buf[i]);
                }

                //DEBUG("Total XACKs = %s\n", g_at_rbuf.head);

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
                        DEBUG("Receive MSGs: %s<<<\n", tmpuse_buf);

                        if ((p=strstr(tmpuse_buf, "+QIURC: ")) != NULL) {
                            if ((p=strstr(tmpuse_buf, "closed")) != NULL) {// TCP mode only
                                SetNetStatus(0x80);
                                DEBUG("******* TCP Closed *******\n");
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

                                // DEBUG("MSGs = %s\n", g_net_rbuf.head);
                            } else if ((p=strstr(tmpuse_buf, "incoming full")) != NULL) {// BG96 act as TCP server
                                DEBUG("******* incoming full *******\n");
                            } else if ((p=strstr(tmpuse_buf, "incoming")) != NULL) {// BG96 act as TCP server
                                DEBUG("******* incoming *******\n");
                            } else if ((p=strstr(tmpuse_buf, "pdpdeact")) != NULL) {
                                DEBUG("******* pdpdeact *******\n");
                            } else {// invalid MSGs
                                DEBUG("******* invalid MSGs *******\n");
                            }
                        } else {
                            // invalid MSGs
                        }
                    } else {
                        if (tmp_len < 100) {
                            DEBUG("Receive2 XACKs(%dB): %s<<<\n", tmp_len, tmpuse_buf);
                        }

                        for (i=0; i<tmp_len; i++) {
                            ringbuffer_write_byte(&g_at_rbuf,tmpuse_buf[i]);
                            // DEBUG("ringbuffer->writepos=%d\n", g_at_rbuf.writepos);
                        }

                        //DEBUG("Total XACKs = %s\n", g_at_rbuf.head);
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

    if (0 == MobitTimesT3%100) {
        // DEBUG("tm3 recv len = %d\n", IsTmpRingBufferAvailable());
    }

    IFS0bits.T3IF = 0;// Clear Timer3 interrupt flag
}

//******************************************************************************
//* Timer 4 IRQ -> 100ms
//******************************************************************************
void __attribute__((__interrupt__, no_auto_psv)) _T4Interrupt(void)
{
    MobitTimesT4 += 1;
    if(MobitTimesT4 > 1000000UL){
        MobitTimesT4 = 0;
    }

	if (0 == MobitTimesT4%100) {
        // DEBUG("T4 10s...\n");
    }

    IFS1bits.T4IF = 0;// Clear Timer4 interrupt flag
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
    // delay at least 1ms
    if(delayms < 2){
        delayms = 2;
    }
    if(MobitTimesT1 >= start_time){
        if((MobitTimesT1 - start_time) > delayms/2){
            return 1;
        }
    }else{
        if((10000000UL-start_time+MobitTimesT1) > delayms/2){
            DEBUG("Timer1 Overload...\n");
            return 1;
        }
    }
    return 0;
}

//******************************************************************************
//* END OF FILE
//******************************************************************************
