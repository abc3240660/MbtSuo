#include <string.h>
#include <p24fxxxx.h>
#include "016_FlashOta.h"

// NOTE: all address in this file = Actual Flash Address in datasheet
//                                = InstructionWords Address Gap
//                                = InstructionWords's Number * 2
//                                = Every 2B Adress Gap can store 3B data

static OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord

// len = number of InstructionWord to be read
u16 DataRecord_ReadData(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 len, u8 *pdata)
{
    int i = 0;
    FlashAddr_t flashAddr;
    
    for(i=0;i<len;i++)
    {  
        flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;
        flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+i*2;

        pdata[4*i]=InnerFlash_ReadInstructionLow(flashAddr);
        pdata[4*i+1]=InnerFlash_ReadInstructionLow(flashAddr)>>8;
        pdata[4*i+2]=InnerFlash_ReadInstructionHigh(flashAddr);
        pdata[4*i+3]=InnerFlash_ReadInstructionHigh(flashAddr)>>8;
    }
   
    return 0;
}
 
void DataRecord_ErasePage(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR )
{
    FlashAddr_t flashAddr;

    flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;
    flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR;
    InnerFlash_EraseFlashPage(flashAddr);
}

// Firstly read out the whole page, then modify one InstructionWord in the pointed offset
u16 DataRecord_WriteData(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, volatile OneInstruction_t data)
{  
    u16 i = 0;
    FlashAddr_t flashAddr;

    flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;
    // flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+index*2;

#if 0
    OneInstruction_t dataTmp = InnerFlash_ReadOneInstruction(flashAddr);
    if(dataTmp.UINT32==data.UINT32)
        return 0xffff;
#endif

    // OneInstruction_t pageData[1024];
    
    memset(pageData, 0, sizeof(OneInstruction_t)*1024);

    for(i=0;i<1024;i++)
    {
        flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }

    flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR;
    InnerFlash_EraseFlashPage(flashAddr);

    pageData[index] = data;

    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);
 
    return 0;
}

// Firstly read out the whole page, then modify from the pointed offset
// DATA_RECORD_START_PAGE = Page's HighAddr, DATA_RECORD_START_ADDR = Page's LowAddr
// index = InstructionWord's offset during One Page
u16 DataRecord_WriteDataArray(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, volatile OneInstruction_t *data, u16 length)
{
    u16 i = 0;
    // u16 cmpNZ = 0;
    FlashAddr_t flashAddr;
    // OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);

    flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;
    // flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+index*2;

    // To judge if the Flash data == to be written data
#if 0
    for(i=index;i<length;i++)
    {
        flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
        if(pageData[i].UINT32!=data[i-index].UINT32)
        {
            cmpNZ = 1;
            break;
        }
    }

    if(cmpNZ==0)
        return 0xffff;
#endif

    // Read out the whole One Page
    for(i=0;i<1024;i++)// One Page = 1024 InstructionWord
    {
        flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }
 
    flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR;
    
    // Modify from the pointed offset
    // index maybe >= 1024, so need to ensure that not overflow pageData's size
    for(i=index;i<length;i++)
    {
        pageData[index-(index/1024)*1024+i].UINT32 = data[i].UINT32;
    }

    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);

    return 0;
}

// One LargePage = 64K InstructionWords Address Gap = 32K InstructionWords
void EraseLargePage(u16 pageIndex, u32 offsetaddr)
{
    u16 offset = 0;
    FlashAddr_t flashAddr;

    flashAddr.Uint16Addr.HighAddr = pageIndex;
    for(offset=offsetaddr;offset<=0xF800;offset+=0x800)
    {
        flashAddr.Uint16Addr.LowAddr = offset;
        InnerFlash_EraseFlashPage(flashAddr);
        if(offset>=0xF800)
            break;
    }
}

// Fill some test pattern
void FillLagrePage(u16 pageIndex)
{
    volatile u16 offset = 0;
    volatile FlashAddr_t source_addr;
    volatile OneInstruction_t data[2];
    
    switch(pageIndex)
    {
        case 0:
            for(offset=0x8000;offset<=0xFFFC;offset+=4)
            {
                source_addr.Uint16Addr.HighAddr = 0;
                source_addr.Uint16Addr.LowAddr = offset;
                data[0].HighLowUINT16s.HighWord = 0xF0;
                data[0].HighLowUINT16s.LowWord = offset;
                data[1].HighLowUINT16s.HighWord = 0xF0;
                data[1].HighLowUINT16s.LowWord = offset+2;
                InnerFlash_WriteInstructionsToFlash(source_addr,data,2);

                if(offset>=0xFFFC)
                    break;
            }
    
            break;
        case 1:
            
            for(offset=0;offset<=0xFFFC;offset+=4)
            {
                source_addr.Uint16Addr.HighAddr = 1;
                source_addr.Uint16Addr.LowAddr = offset;
                data[0].HighLowUINT16s.HighWord = 0xF1;
                data[0].HighLowUINT16s.LowWord = offset;
                data[1].HighLowUINT16s.HighWord = 0xF1;
                data[1].HighLowUINT16s.LowWord = offset+2;
                InnerFlash_WriteInstructionsToFlash(source_addr,data,2);

                if(offset>=0xFFFC)
                    break;
            }
    
    
            break;
        case 2:
            for(offset=0;offset<0xA800;offset+=4)
            {
                source_addr.Uint16Addr.HighAddr = 2;
                source_addr.Uint16Addr.LowAddr = offset;
                data[0].HighLowUINT16s.HighWord = 0xF2;
                data[0].HighLowUINT16s.LowWord = offset;
                data[1].HighLowUINT16s.HighWord = 0xF2;
                data[1].HighLowUINT16s.LowWord = offset+2;
                InnerFlash_WriteInstructionsToFlash(source_addr,data,2);
            }
            break;

        default:
            break;
    }
}