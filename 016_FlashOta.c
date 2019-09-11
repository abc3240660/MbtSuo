#include <string.h>
#include <p24fxxxx.h>
#include "001_Tick_10ms.h"
#include "016_FlashOta.h"

// NOTE: all address in this file = Actual Flash Address in datasheet
//                                = InstructionWords Address Gap
//                                = InstructionWords's Number * 2
//                                = Every 2B Adress Gap can store 3B data

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

#define FLASH_BASE_CARD_ID  0x800
#define FLASH_SIZE_CARD_ID  0x800

#define CNTR_MAX_CARD_ID    64
#define NUM_INWORD_PER_CARD 8

// Each CardID = 19B Data = give 8 * InstructionWords = 24B Data
// Most 64*8 = 512 * InstructionWords = 1024B Address GAP = store MAX 1536B(actual 64*19=1216B)
u16 DataRecord_ReadCards(u8 *pdata)
{
    int i = 0;
    int j = 0;
    int k = 0;
    u16 tmpdata = 0;
    FlashAddr_t flashAddr;
    u8 data[NUM_INWORD_PER_CARD*3] = "";

    for (i=0; i<CNTR_MAX_CARD_ID; i++) {
        for (j=0; j<NUM_INWORD_PER_CARD; j++) {
            flashAddr.Uint16Addr.HighAddr = 0;
            flashAddr.Uint16Addr.LowAddr = FLASH_BASE_CARD_ID+i*2;

            tmpdata = InnerFlash_ReadInstructionLow(flashAddr);
            data[3*j+0] = tmpdata;
            data[3*j+1] = tmpdata >> 8;

            tmpdata = InnerFlash_ReadInstructionHigh(flashAddr);
            data[3*j+2] = tmpdata;
        }

        if ((data[0]!=0x00) && (data[0]!=0xFF)) {
            memcpy(pdata+k*NUM_INWORD_PER_CARD*3, data, NUM_INWORD_PER_CARD*3);
            k++;
        }
    }

    return k;
}

void DataRecord_ErasePage(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR )
{
    FlashAddr_t flashAddr;

    flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;
    flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR;
    InnerFlash_EraseFlashPage(flashAddr);
}

// Each Line = 128 * InstructionWords
// Line Base Address: DATA_RECORD_START_PAGE = LargePage's HighAddr, DATA_RECORD_START_ADDR = SmallPage's LowAddr
// Firstly read out the whole page, then modify one InstructionWord in the pointed offset
u16 DataRecord_WriteData(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, volatile OneInstruction_t data)
{  
    u16 i = 0;
    FlashAddr_t flashAddr;
    OneInstruction_t pageData[1024];
    
    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;    

    for(i=0;i<1024;i++)
    {
        flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }

    flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR;
    InnerFlash_EraseFlashPage(flashAddr);
    
    delay_ms(200);

    pageData[index] = data;

    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);
 
    return 0;
}

// length = Byte's count
// DATA_RECORD_START_PAGE = LargePage's HighAddr, DATA_RECORD_START_ADDR = SmallPage's LowAddr
u16 DataRecord_WriteBytesArray(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, u8 *data, u16 length)
{
    u16 i = 0;
    FlashAddr_t flashAddr;
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord
    
    if (DATA_RECORD_START_ADDR%0x100 != 0) {
        DATA_RECORD_START_ADDR = (DATA_RECORD_START_ADDR/0x100)*0x100;
    }

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;

    // Read out the whole One Page
    for(i=0;i<1024;i++)// One Page = 1024 InstructionWord
    {
        flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }
 
    flashAddr.Uint16Addr.LowAddr = DATA_RECORD_START_ADDR;
    InnerFlash_EraseFlashPage(flashAddr);
    
    delay_ms(200);
    
    // Modify from the pointed offset
    // index maybe >= 1024, so need to ensure that not overflow pageData's size
    for(i=index;i<(length+2)/3;i++)
    {
        pageData[i].HighLowUINT16s.HighWord = data[i*3];
        if ((i*3+1) >= length) {
            break;
        }
        pageData[i].HighLowUINT16s.LowWord = data[i*3+1] << 8;
        if ((i*3+2) >= length) {
            break;
        }
        pageData[i].HighLowUINT16s.LowWord += data[i*3+2];
    }

    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);

    return 0;
}

// Firstly read out the whole page, then modify from the pointed offset
// DATA_RECORD_START_PAGE = LargePage's HighAddr, DATA_RECORD_START_ADDR = SmallPage's LowAddr
// index = InstructionWord's offset during One Page
// length = InstructionWord's count
u16 DataRecord_WriteDataArray(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, volatile OneInstruction_t *data, u16 length)
{
    u16 i = 0;
    FlashAddr_t flashAddr;
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = DATA_RECORD_START_PAGE;

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
        //printf("XDAT = %.4X,%.4X\n", data[i].HighLowUINT16s.HighWord, data[i].HighLowUINT16s.LowWord);
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

void EraseSomePage(u16 highAddr, u16 lowAddr, u16 pageNum)
{
    u16 i = 0;
    FlashAddr_t flashAddr;

    if (lowAddr%0x800 != 0) {
        lowAddr = (lowAddr/0x800)*0x800;
    }
    
    if (lowAddr >= 0xF800) {
        lowAddr = 0xF800;
    }
    
    flashAddr.Uint16Addr.HighAddr = highAddr;

    for (i=0; i<pageNum; i++) {
        flashAddr.Uint16Addr.LowAddr = lowAddr;
        InnerFlash_EraseFlashPage(flashAddr);
        
        if (lowAddr >= 0xF800) {
            lowAddr = 0;
            flashAddr.Uint16Addr.HighAddr++;
        } else {
            lowAddr += 0x800;
        }
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
