#include <string.h>
#include <p24fxxxx.h>
#include "001_Tick_10ms.h"
#include "015_Common.h"
#include "016_FlashOta.h"

// NOTE: all address in this file = Actual Flash Address in datasheet
//                                = InstructionWords Address Gap
//                                = InstructionWords's Number * 2
//                                = Every 2B Adress Gap can store 3B data

// len = number of InstructionWord to be read
u16 FlashRead_SomeInstructionWords(u16 flash_page, u16 flash_offset, u16 len, u8 *pdata)
{
    int i = 0;
    FlashAddr_t flashAddr;

    for(i=0;i<len;i++)
    {
        flashAddr.Uint16Addr.HighAddr = flash_page;
        flashAddr.Uint16Addr.LowAddr = flash_offset+i*2;

        pdata[4*i]=InnerFlash_ReadInstructionLow(flashAddr);
        pdata[4*i+1]=InnerFlash_ReadInstructionLow(flashAddr)>>8;
        pdata[4*i+2]=InnerFlash_ReadInstructionHigh(flashAddr);
        pdata[4*i+3]=InnerFlash_ReadInstructionHigh(flashAddr)>>8;
    }

    return 0;
}

// Each CardID = 19B Data = give 8 * InstructionWords = 24B Data
// Most 64*8 = 512 * InstructionWords = 1024B Address GAP = store MAX 1536B(actual 64*19=1216B)
// Eevery 8*InstructionWords = 16B Adress GAP = Can Store MAX 24B Data = Actual Store 19B Data
u16 FlashRead_AllNFCCards(u8 *card_dat)
{
    int i = 0;
    int j = 0;
    u16 tmpdata = 0;
    int card_cnt = 0;
    FlashAddr_t flashAddr;
    u8 tmp_card[LEN_CARD_ID+1] = "";

    for (i=0; i<CNTR_MAX_CARD; i++) {
        for (j=0; j<CNTR_INWORD_PER_CARD; j++) {
            flashAddr.Uint16Addr.HighAddr = FLASH_PAGE_CARD_ID;
            flashAddr.Uint16Addr.LowAddr = FLASH_BASE_CARD_ID+i*CNTR_INWORD_PER_CARD*2+j*2;

            tmpdata = InnerFlash_ReadInstructionHigh(flashAddr);
            tmp_card[3*j+0] = tmpdata;

            // Max Len = 19B
            if (j >= (LEN_CARD_ID/3)) {
                break;
            }

            tmpdata = InnerFlash_ReadInstructionLow(flashAddr);
            tmp_card[3*j+1] = tmpdata >> 8;
            tmp_card[3*j+2] = tmpdata;
        }

        tmp_card[LEN_CARD_ID] = 0;

        if (LEN_CARD_ID == strlen(tmp_card)) {
            printf("Found CARD ID: %s\n", tmp_card);

            if (card_dat != NULL) {
                memcpy(card_dat+card_cnt*(LEN_CARD_ID+1), tmp_card, LEN_CARD_ID+1);
            }

            card_cnt++;
        }
    }

    return card_cnt;
}

void FlashErase_OnePage(u16 flash_page, u16 flash_offset )
{
    FlashAddr_t flashAddr;

    flashAddr.Uint16Addr.HighAddr = flash_page;
    flashAddr.Uint16Addr.LowAddr = flash_offset;
    InnerFlash_EraseFlashPage(flashAddr);
}

u16 FlashWrite_OneNFCCard(u8 *card_dat)
{
    u16 i = 0;
    u16 j = 0;
    FlashAddr_t flashAddr;
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord
    u8 tmpdata[CNTR_INWORD_PER_CARD*3] = "";

    if (NULL == card_dat) {
        return 1;
    }

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = FLASH_PAGE_CARD_ID;

    // Read out the whole One Page
    for(i=0;i<1024;i++)// One Page = 1024 InstructionWord
    {
        flashAddr.Uint16Addr.LowAddr = FLASH_BASE_CARD_ID+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }

    flashAddr.Uint16Addr.LowAddr = FLASH_BASE_CARD_ID;
    InnerFlash_EraseFlashPage(flashAddr);

    delay_ms(200);

    // Modify from the pointed offset
    // index maybe >= 1024, so need to ensure that not overflow pageData's size
    for (i=0; i<CNTR_MAX_CARD; i++) {
        for (j=0; j<CNTR_INWORD_PER_CARD; j++) {
            tmpdata[3*j+0] = pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.HighWord;
            tmpdata[3*j+1] = pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord >> 8;
            tmpdata[3*j+2] = (u8)pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord;

            // Check if has been used
            if ((0==j) && (tmpdata[0]!=0x00) && (tmpdata[0]!=0xFF)) {
                is_used = 1;
                break;
            }

            pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.HighWord = card_dat[j*3];

            // Max Len = 19B
            if (j >= (LEN_CARD_ID/3)) {
                break;
            }

            pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord = card_dat[j*3+1] << 8;
            pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord += card_dat[j*3+2];
        }

        // Has been written in the just sub-loop
        if (0 == is_used) {
            break;
        }
    }

    printf("Store CardID %s into 0x2-%.4X\n", data, FLASH_BASE_CARD_ID+CNTR_INWORD_PER_CARD*2*i);
    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);

    return 0;
}

u16 FlashWrite_DeleteOneCard(u8 *card_dat)
{
    u16 i = 0;
    u16 j = 0;
    u16 is_used = 0;
    FlashAddr_t flashAddr;
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord
    u8 tmpdata[CNTR_INWORD_PER_CARD*3] = "";

    if (NULL == card_dat) {
        return 1;
    }

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = FLASH_PAGE_CARD_ID;

    // Read out the whole One Page
    for(i=0;i<1024;i++)// One Page = 1024 InstructionWord
    {
        flashAddr.Uint16Addr.LowAddr = FLASH_BASE_CARD_ID+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }

    flashAddr.Uint16Addr.LowAddr = FLASH_BASE_CARD_ID;
    InnerFlash_EraseFlashPage(flashAddr);

    delay_ms(200);

    // Modify from the pointed offset
    // index maybe >= 1024, so need to ensure that not overflow pageData's size
    for (i=0; i<CNTR_MAX_CARD; i++) {
        is_used = 0;
        for (j=0; j<CNTR_INWORD_PER_CARD; j++) {
            tmpdata[3*j+0] = pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.HighWord;
            tmpdata[3*j+1] = pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord >> 8;
            tmpdata[3*j+2] = (u8)pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord;
        }

        // Clear CardID
        if ((tmpdata[0]!=0x00) && tmpdata[0]!=0xFF) {
            if (0 == memncmp(data, tmpdata, LEN_CARD_ID)) {
                for (j=0; j<CNTR_INWORD_PER_CARD; j++) {
                    pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord = 0;
                    pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.HighWord = 0;
                }
            }
        }
    }

    printf("Store CardID %s into 0x2-%.4X\n", data, FLASH_BASE_CARD_ID+CNTR_INWORD_PER_CARD*2*i);
    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);

    return 0;
}

void FlashWriteRead_Test(void)
{
    FlashWrite_OneNFCCard("3080021000000255613", 19);
    FlashWrite_OneNFCCard("3080021001000255614", 19);
    FlashWrite_OneNFCCard("3080021002000255615", 19);
    FlashWrite_OneNFCCard("3080021003000255616", 19);

    FlashRead_AllNFCCards(NULL);
}

#if 0
u16 FlashWrite_UpdateParams(u16 index, u8 *data, u16 length)
{
    u16 i = 0;
    FlashAddr_t flashAddr;
    u16 flash_offset = 0;
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = FLASH_PAGE_PARAMS;

    // Read out the whole One Page
    for(i=0;i<1024;i++)// One Page = 1024 InstructionWord
    {
        flashAddr.Uint16Addr.LowAddr = flash_offset+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }

    flashAddr.Uint16Addr.LowAddr = flash_offset;

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
#endif

// One LargePage = 32 * SmallPage
// One LargePage = 64K InstructionWords Address Gap = 32K InstructionWords
void FlashErase_LargePage(u16 pageIndex)
{
    u16 offset = 0;
    FlashAddr_t flashAddr;

    flashAddr.Uint16Addr.HighAddr = pageIndex;

    for (offset=0; offset<=0xF800; offset+=0x800) {
        flashAddr.Uint16Addr.LowAddr = offset;
        InnerFlash_EraseFlashPage(flashAddr);
    }
}

void FlashErase_SomePage(u16 highAddr, u16 lowAddr, u16 pageNum)
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
