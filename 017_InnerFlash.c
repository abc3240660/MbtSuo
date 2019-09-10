#include <xc.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "017_InnerFlash.h"

// NOTE: all address in this file = Actual Flash Address in datasheet
//                                = InstructionWords Address Gap
//                                = InstructionWords's Number * 2
//                                = Every 2B Adress Gap can store 3B data

// Read High 1B data of one InstructionWord
extern unsigned int MemReadHigh (unsigned int TablePage, unsigned int TableOffset);

// Read Low 2B data of one InstructionWord
extern unsigned int MemReadLow (unsigned int TablePage, unsigned int TableOffset);

// One PAGE = 1K InstructionWords = 2K InstructionWords Address Gap = Can Store 3K Data
// TablePage = HighAddr, TableOffset = LowAddr
extern unsigned int MemEraseOnePage (unsigned int TablePage, unsigned int TableOffset);


extern void MemWriteDoubleInstructionWords(volatile unsigned int TablePage, volatile unsigned int TableOffset, volatile OneInstruction_t *temp);

OneInstruction_t InnerFlash_ReadOneInstruction(FlashAddr_t flashAddr)
{
    unsigned int Data1;
    OneInstruction_t Data;

    Data1 = MemReadLow (flashAddr.Uint16Addr.HighAddr, flashAddr.Uint16Addr.LowAddr);
    Data.HighLowUINT16s.LowWord = Data1;
    Data1 = MemReadHigh (flashAddr.Uint16Addr.HighAddr, flashAddr.Uint16Addr.LowAddr);
    Data.HighLowUINT16s.HighWord = Data1;

    return Data;
}
 
unsigned int InnerFlash_ReadInstructionLow(FlashAddr_t flashAddr)
{
    unsigned int Data1;

    Data1 = MemReadLow (flashAddr.Uint16Addr.HighAddr, flashAddr.Uint16Addr.LowAddr);

    return Data1;
}

unsigned int InnerFlash_ReadInstructionHigh(FlashAddr_t flashAddr)
{
    unsigned int Data1;
 
    Data1 = MemReadHigh (flashAddr.Uint16Addr.HighAddr, flashAddr.Uint16Addr.LowAddr);

    return Data1;
}

// flashAddr = InstructionWords Address Gap = InstructionWords's Number * 2
unsigned int InnerFlash_EraseFlashPage(FlashAddr_t flashAddr)
{
    _GIE = 0;// Disable all Interrupt

    MemEraseOnePage (flashAddr.Uint16Addr.HighAddr, flashAddr.Uint16Addr.LowAddr);

    _GIE = 1;// Re-Enable all Interrupt

    return 1;
}

// dataLength = Number of InstructionWords
// Internally every loop write two InstructionWords
void InnerFlash_WriteInstructionsToFlash(volatile FlashAddr_t flashAddr, volatile OneInstruction_t *data, volatile u16 dataLength) 
{
    volatile OneInstruction_t dataTmp[2];
    volatile uint16_t i;

    for(i=0;i<dataLength/2*2;i+=2)
    {
        dataTmp[0]= data[i];
        dataTmp[1] = data[i+1];

        _GIE = 0;// Disable all Interrupt
        MemWriteDoubleInstructionWords(flashAddr.Uint16Addr.HighAddr, flashAddr.Uint16Addr.LowAddr+i*2, dataTmp);
        _GIE = 1;// Re-Enable all Interrupt
    }

    if(dataLength%2==1)
    {
        dataTmp[0]= data[i];
        dataTmp[1].UINT32 = 0xFFFFFF;

        _GIE = 0;// Disable all Interrupt
        MemWriteDoubleInstructionWords(flashAddr.Uint16Addr.HighAddr, flashAddr.Uint16Addr.LowAddr+i*2, dataTmp);
        _GIE = 1;// Re-Enable all Interrupt
    }
}