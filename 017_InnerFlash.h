#ifndef _MCU_DRIVERS_INNERFLASH_H_
#define _MCU_DRIVERS_INNERFLASH_H_

#include "015_Common.h" 

typedef union OneInstruction {
    u32 UINT32;
    struct {
        u16 LowWord;
        u16 HighWord;
    } HighLowUINT16s;
} OneInstruction_t;

typedef union FlashAddr {
    u32 Uint32Addr;
    struct {
        u16 LowAddr;
        u16 HighAddr;
    } Uint16Addr;
} FlashAddr_t;

unsigned int InnerFlash_EraseFlashPage(FlashAddr_t flashAddr);
unsigned int InnerFlash_ReadInstructionHigh(FlashAddr_t flashAddr);
unsigned int InnerFlash_ReadInstructionLow(FlashAddr_t flashAddr);
OneInstruction_t InnerFlash_ReadOneInstruction(FlashAddr_t flashAddr);
void InnerFlash_WriteInstructionsToFlash(volatile FlashAddr_t source_addr, volatile OneInstruction_t *data, volatile u16 dataLength);
 
#endif