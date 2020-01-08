//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

//******************************************************************************
// File:   ---_Main.h
// Author: Hans Desmet
// Comments: Initial Main file
// Revision history: Version 01.00
// Date 26/12/2018
//******************************************************************************

#ifndef _000_MAIN_H
#define _000_MAIN_H

//******************************************************************************
// Includes
//******************************************************************************
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <p24fxxxx.h>
#include <xc.h>                                                                 // include processor files - each processor file is guarded.  
#include "001_Tick_10ms.h"
#include "002_CLRC663.h"
#include "003_BG96.h"
#include "004_LB1938.h"
#include "005_BNO055.h"
#include "006_Gpio.h"
#include "007_Uart.h"
#include "009_System.h"
#include "012_CLRC663_NFC.h"
#include "013_Protocol.h"
#include "015_Common.h"
//#include "019_ADC0.h"

//******************************************************************************
// System functionality config
//******************************************************************************
//CPU crystel 20MHz is present
//#define OSC_20M_USE
//Enable backend communication 
#define BG96COMM
#define LOWPOWER

//******************************************************************************
// Defines
//******************************************************************************
#define HeartBeatPeriod     30UL  //120

//******************************************************************************
// Backend default parameters
//******************************************************************************
u8 srvIP[]  = "192.168.1.105";
u8 svrPort[] = "10212";                 // 10211-TCP 10212-UDP
u8 svrAPN[] = "sentinel.m2mmobi.be";

#endif /* _000_MAIN_H */
//******************************************************************************
//* END OF FILE
//******************************************************************************

