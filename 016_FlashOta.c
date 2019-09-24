#include <stdio.h>
#include <string.h>
#include <p24fxxxx.h>
#include "001_Tick_10ms.h"
#include "013_Protocol.h"
#include "015_Common.h"
#include "016_FlashOta.h"

extern u8 g_svr_ip[LEN_NET_TCP];
extern u8 g_svr_port[LEN_NET_TCP];
extern u8 g_svr_apn[LEN_NET_TCP];

// NOTE: all address in this file = Actual Flash Address in datasheet
//                                = InstructionWords Address Gap
//                                = InstructionWords's Number * 2
//                                = Every 2B Adress Gap can store 3B data

// len = number of InstructionWord to be read
u16 FlashRead_InstructionWordsToByteArray(u16 flash_page, u16 flash_offset, u16 len, u8 *pdata)
{
    int i = 0;
    FlashAddr_t flashAddr;

    for(i=0;i<len;i++)
    {
        flashAddr.Uint16Addr.HighAddr = flash_page;
        flashAddr.Uint16Addr.LowAddr = flash_offset+i*2;

        pdata[3*i+0]=InnerFlash_ReadInstructionLow(flashAddr);
        pdata[3*i+1]=InnerFlash_ReadInstructionLow(flashAddr)>>8;
        pdata[3*i+2]=InnerFlash_ReadInstructionHigh(flashAddr);
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

        if ((tmp_card[0]!=0x00) && (tmp_card[0]!=0xFF)) {
            printf("Found CARD ID: %s from 0x1-%.4X\n", (char*)tmp_card, FLASH_BASE_CARD_ID+CNTR_INWORD_PER_CARD*2*i);

            if (card_dat != NULL) {
                memcpy(card_dat+card_cnt*(LEN_CARD_ID+1), tmp_card, LEN_CARD_ID+1);
            }

            card_cnt++;
        }
    }

    return card_cnt;
}

void FlashErase_OnePage(u16 flash_page, u16 flash_offset)
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
    u8 is_used = 1;
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

    printf("Store CardID %s into 0x1-%.4X\n", (char*)card_dat, FLASH_BASE_CARD_ID+CNTR_INWORD_PER_CARD*2*i);
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
            if (0 == memcmp(card_dat, tmpdata, LEN_CARD_ID)) {
                for (j=0; j<CNTR_INWORD_PER_CARD; j++) {
                    pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.LowWord = 0;
                    pageData[i*CNTR_INWORD_PER_CARD+j].HighLowUINT16s.HighWord = 0;
                }

                printf("Delete CardID %s from 0x1-%.4X\n", (char*)card_dat, FLASH_BASE_CARD_ID+CNTR_INWORD_PER_CARD*2*i);
            }
        }
    }

    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);

    return 0;
}

// length = Bytes Count
// data = must change into string type
u16 FlashWrite_SysParams(PARAM_ID params_id, u8 *data, u16 length)
{
    u16 i = 0;
    u16 j = 0;
    FlashAddr_t flashAddr;
    u16 flash_offset = 0;
    u16 flash_length = 0;// InstructionWord Count
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = FLASH_PAGE_PARAMS;

    // Read out the whole One Page
    for(i=0;i<1024;i++)// One Page = 1024 InstructionWord
    {
        flashAddr.Uint16Addr.LowAddr = FLASH_BASE_PARAMS+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }

    flashAddr.Uint16Addr.LowAddr = FLASH_BASE_PARAMS;
    InnerFlash_EraseFlashPage(flashAddr);

    delay_ms(200);
    switch (params_id) {
        case PARAM_ID_SVR_IP:
            flash_offset = FLASH_BASE_IP;
            flash_length = FLASH_SIZE_IP;
            break;
        case PARAM_ID_SVR_PORT:
            flash_offset = FLASH_BASE_PORT;
            flash_length = FLASH_SIZE_PORT;
            break;
        case PARAM_ID_SVR_APN:
            flash_offset = FLASH_BASE_APN;
            flash_length = FLASH_SIZE_APN;
            break;
        case PARAM_ID_IAP_MD5:
            flash_offset = FLASH_BASE_IAP_MD5;
            flash_length = FLASH_SIZE_IAP_MD5;
            break;
        case PARAM_ID_1ST_BOOT:
            flash_offset = FLASH_BASE_1ST;
            flash_length = FLASH_SIZE_1ST;
            break;
        case PARAM_ID_ALM_ON:
            flash_offset = FLASH_BASE_ALM_ON;
            flash_length = FLASH_SIZE_ALM_ON;
            break;
        case PARAM_ID_BEEP_ON:
            flash_offset = FLASH_BASE_BEP_ON;
            flash_length = FLASH_SIZE_BEP_ON;
            break;
        case PARAM_ID_BEEP_LEVEL:
            flash_offset = FLASH_BASE_BEP_LV;
            flash_length = FLASH_SIZE_BEP_LV;
            break;
        case PARAM_ID_BOOT_TM:
            flash_offset = FLASH_BASE_BOT_TM;
            flash_length = FLASH_SIZE_BOT_TM;
            break;
        case PARAM_ID_IAP_FLAG:
            flash_offset = FLASH_BASE_IAP_FG;
            flash_length = FLASH_SIZE_IAP_FG;
            break;
        case PARAM_ID_IAP_STA:
            flash_offset = FLASH_BASE_IAP_STA;
            flash_length = FLASH_SIZE_IAP_STA;
            break;
        case PARAM_ID_IAP_CNT:
            flash_offset = FLASH_BASE_IAP_CNT;
            flash_length = FLASH_SIZE_IAP_CNT;
            break;
        case PARAM_ID_RSVD_U1:
            flash_offset = FLASH_BASE_RSVD_U1;
            flash_length = FLASH_SIZE_RSVD_U1;
            break;
        case PARAM_ID_RSVD_U2:
            flash_offset = FLASH_BASE_RSVD_U2;
            flash_length = FLASH_SIZE_RSVD_U2;
            break;
        case PARAM_ID_RSVD_U3:
            flash_offset = FLASH_BASE_RSVD_U3;
            flash_length = FLASH_SIZE_RSVD_U3;
            break;
        case PARAM_ID_RSVD_U4:
            flash_offset = FLASH_BASE_RSVD_U4;
            flash_length = FLASH_SIZE_RSVD_U4;
            break;
        case PARAM_ID_RSVD_U5:
            flash_offset = FLASH_BASE_RSVD_U5;
            flash_length = FLASH_SIZE_RSVD_U5;
            break;
        case PARAM_ID_RSVD_U6:
            flash_offset = FLASH_BASE_RSVD_U6;
            flash_length = FLASH_SIZE_RSVD_U6;
            break;
        case PARAM_ID_RSVD_U7:
            flash_offset = FLASH_BASE_RSVD_U7;
            flash_length = FLASH_SIZE_RSVD_U7;
            break;
        case PARAM_ID_RSVD_U8:
            flash_offset = FLASH_BASE_RSVD_U8;
            flash_length = FLASH_SIZE_RSVD_U8;
            break;
        default :
            break;
    }

    flashAddr.Uint16Addr.LowAddr = flash_offset;

    printf("flash_offset = 0x%.4X, flash_length = %d, index = %d\n", flash_offset, flash_length, (flash_offset-FLASH_BASE_IP)/2);

    // Modify from the pointed offset
    // index maybe >= 1024, so need to ensure that not overflow pageData's size
    for (i=0; i<flash_length; i++) {
        if ((j*3+0) >= length) {
            break;
        }

        pageData[i+(flash_offset-FLASH_BASE_IP)/2].HighLowUINT16s.HighWord = data[j*3];

        if ((j*3+1) >= length) {
            break;
        }

        pageData[i+(flash_offset-FLASH_BASE_IP)/2].HighLowUINT16s.LowWord = data[j*3+1] << 8;

        if ((j*3+2) >= length) {
            break;
        }

        pageData[i+(flash_offset-FLASH_BASE_IP)/2].HighLowUINT16s.LowWord += data[j*3+2];

        j++;
    }

    flashAddr.Uint16Addr.LowAddr = FLASH_BASE_PARAMS;
    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);

    return 0;
}

// length = the max length of data buffer
u16 FlashRead_SysParams(PARAM_ID params_id, u8 *data, u8 length)
{
    u16 i = 0;
    u16 dat_len = 0;
    u16 tmpdata = 0;
    FlashAddr_t flashAddr;
    u16 flash_offset = 0;
    u16 flash_length = 0;// InstructionWord Count
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord

    memset(data, 0, length+1);
    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = FLASH_PAGE_PARAMS;

    delay_ms(200);

    switch (params_id) {
        case PARAM_ID_SVR_IP:
            flash_offset = FLASH_BASE_IP;
            flash_length = FLASH_SIZE_IP;
            break;
        case PARAM_ID_SVR_PORT:
            flash_offset = FLASH_BASE_PORT;
            flash_length = FLASH_SIZE_PORT;
            break;
        case PARAM_ID_SVR_APN:
            flash_offset = FLASH_BASE_APN;
            flash_length = FLASH_SIZE_APN;
            break;
        case PARAM_ID_IAP_MD5:
            flash_offset = FLASH_BASE_IAP_MD5;
            flash_length = FLASH_SIZE_IAP_MD5;
            break;
        case PARAM_ID_1ST_BOOT:
            flash_offset = FLASH_BASE_1ST;
            flash_length = FLASH_SIZE_1ST;
            break;
        case PARAM_ID_ALM_ON:
            flash_offset = FLASH_BASE_ALM_ON;
            flash_length = FLASH_SIZE_ALM_ON;
            break;
        case PARAM_ID_BEEP_ON:
            flash_offset = FLASH_BASE_BEP_ON;
            flash_length = FLASH_SIZE_BEP_ON;
            break;
        case PARAM_ID_BEEP_LEVEL:
            flash_offset = FLASH_BASE_BEP_LV;
            flash_length = FLASH_SIZE_BEP_LV;
            break;
        case PARAM_ID_BOOT_TM:
            flash_offset = FLASH_BASE_BOT_TM;
            flash_length = FLASH_SIZE_BOT_TM;
            break;
        case PARAM_ID_IAP_FLAG:
            flash_offset = FLASH_BASE_IAP_FG;
            flash_length = FLASH_SIZE_IAP_FG;
            break;
        case PARAM_ID_IAP_STA:
            flash_offset = FLASH_BASE_IAP_STA;
            flash_length = FLASH_SIZE_IAP_STA;
            break;
        case PARAM_ID_IAP_CNT:
            flash_offset = FLASH_BASE_IAP_CNT;
            flash_length = FLASH_SIZE_IAP_CNT;
            break;
        case PARAM_ID_RSVD_U1:
            flash_offset = FLASH_BASE_RSVD_U1;
            flash_length = FLASH_SIZE_RSVD_U1;
            break;
        case PARAM_ID_RSVD_U2:
            flash_offset = FLASH_BASE_RSVD_U2;
            flash_length = FLASH_SIZE_RSVD_U2;
            break;
        case PARAM_ID_RSVD_U3:
            flash_offset = FLASH_BASE_RSVD_U3;
            flash_length = FLASH_SIZE_RSVD_U3;
            break;
        case PARAM_ID_RSVD_U4:
            flash_offset = FLASH_BASE_RSVD_U4;
            flash_length = FLASH_SIZE_RSVD_U4;
            break;
        case PARAM_ID_RSVD_U5:
            flash_offset = FLASH_BASE_RSVD_U5;
            flash_length = FLASH_SIZE_RSVD_U5;
            break;
        case PARAM_ID_RSVD_U6:
            flash_offset = FLASH_BASE_RSVD_U6;
            flash_length = FLASH_SIZE_RSVD_U6;
            break;
        case PARAM_ID_RSVD_U7:
            flash_offset = FLASH_BASE_RSVD_U7;
            flash_length = FLASH_SIZE_RSVD_U7;
            break;
        case PARAM_ID_RSVD_U8:
            flash_offset = FLASH_BASE_RSVD_U8;
            flash_length = FLASH_SIZE_RSVD_U8;
            break;
        default :
            break;
    }

    // Modify from the pointed offset
    // index maybe >= 1024, so need to ensure that not overflow pageData's size
    for (i=0; i<flash_length; i++) {
        flashAddr.Uint16Addr.LowAddr = flash_offset+i*2;

        tmpdata = InnerFlash_ReadInstructionHigh(flashAddr);

        if (0xFF == (tmpdata&0xFF)) {
            break;
        }

        if (dat_len >= length) {
            break;
        }

        data[dat_len++] = tmpdata;

        tmpdata = InnerFlash_ReadInstructionLow(flashAddr);

        if (0xFF == (tmpdata>>8)) {
            break;
        }

        if (dat_len >= length) {
            break;
        }

        data[dat_len++] = tmpdata >> 8;

        if (0xFF == (tmpdata>>8)) {
            break;
        }

        if (dat_len >= length) {
            break;
        }

        data[dat_len++] = tmpdata;
    }

    data[dat_len] = 0;

    printf("ReadOut Params(%dB): %s from 0x1-%.4X\n", dat_len, data, flash_offset);

    return dat_len;
}

// Will not erase page
// Firstly read out the whole page, then modify from the pointed offset
// flash_base = LargePage's HighAddr, flash_offset = SmallPage's LowAddr
// index = InstructionWord's offset during One Page
// length = InstructionWord's count
u16 FlashWrite_InstructionWords(u16 flash_base, u16 flash_offset, OneInstruction_t *data, u16 length)
{
    u16 i = 0;
    FlashAddr_t flashAddr;
    OneInstruction_t pageData[1024];// One Page = 1024 InstructionWord

    memset(pageData, 0, sizeof(OneInstruction_t)*1024);
    flashAddr.Uint16Addr.HighAddr = flash_base;

    // Read out the whole One Page
    for(i=0;i<1024;i++)// One Page = 1024 InstructionWord
    {
        flashAddr.Uint16Addr.LowAddr = flash_offset+i*2;
        pageData[i] = InnerFlash_ReadOneInstruction(flashAddr);
    }

    flashAddr.Uint16Addr.LowAddr = flash_offset;

    // Modify from the pointed offset
    // index maybe >= 1024, so need to ensure that not overflow pageData's size
    for(i=0;i<length;i++)
    {
        //printf("XDAT = %.4X,%.4X\n", data[i].HighLowUINT16s.HighWord, data[i].HighLowUINT16s.LowWord);
        pageData[i].UINT32 = data[i].UINT32;
    }

    InnerFlash_WriteInstructionsToFlash(flashAddr,pageData,1024);

    return 0;
}

// One LargePage = 32 * SmallPage
// One LargePage = 64K InstructionWords Address Gap = 32K InstructionWords
void FlashErase_LargePage(u16 pageIndex, u16 start_addr)
{
    u16 offset = 0;
    FlashAddr_t flashAddr;

    flashAddr.Uint16Addr.HighAddr = pageIndex;

    for (offset=start_addr; offset<=0xF800; offset+=0x800) {
        flashAddr.Uint16Addr.LowAddr = offset;
        InnerFlash_EraseFlashPage(flashAddr);

        // if commented , will goto endless  loop
        if (offset >= 0xF800) {
            break;
        }
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

void FlashWriteRead_CardIDTest(void)
{
    FlashWrite_OneNFCCard((u8*)"3080021000000255613");
    FlashWrite_OneNFCCard((u8*)"3080021001000255614");
    FlashWrite_OneNFCCard((u8*)"3080021002000255615");
    FlashWrite_OneNFCCard((u8*)"3080021003000255616");

    FlashRead_AllNFCCards(NULL);

    FlashWrite_DeleteOneCard((u8*)"3080021002000255615");

    FlashRead_AllNFCCards(NULL);
}

void FlashWriteRead_ParamsTest(void)
{
    u8 data[64+1] = "";

    FlashWrite_SysParams(PARAM_ID_SVR_IP, (u8*)g_svr_ip, strlen((const char*)g_svr_ip));
    FlashWrite_SysParams(PARAM_ID_SVR_PORT, (u8*)g_svr_port, strlen((const char*)g_svr_port));
    FlashWrite_SysParams(PARAM_ID_SVR_APN, (u8*)g_svr_apn, strlen((const char*)g_svr_apn));
    FlashWrite_SysParams(PARAM_ID_IAP_MD5, (u8*)"11112222333344445555666677778888", 32);
    FlashWrite_SysParams(PARAM_ID_1ST_BOOT, (u8*)"1", 1);
    FlashWrite_SysParams(PARAM_ID_ALM_ON, (u8*)"1", 1);
    FlashWrite_SysParams(PARAM_ID_BEEP_ON, (u8*)"1", 1);
    FlashWrite_SysParams(PARAM_ID_BEEP_LEVEL, (u8*)"18", 2);
    FlashWrite_SysParams(PARAM_ID_BOOT_TM, (u8*)"102", 3);
    FlashWrite_SysParams(PARAM_ID_IAP_FLAG, (u8*)"1A2B", 4);
    FlashWrite_SysParams(PARAM_ID_IAP_STA, (u8*)"5821", 4);
    FlashWrite_SysParams(PARAM_ID_IAP_CNT, (u8*)"1", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U1, (u8*)"1", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U2, (u8*)"2", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U3, (u8*)"3", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U4, (u8*)"4", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U5, (u8*)"5", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U6, (u8*)"6", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U7, (u8*)"7", 1);
    FlashWrite_SysParams(PARAM_ID_RSVD_U8, (u8*)"8", 1);


    FlashRead_SysParams(PARAM_ID_SVR_IP, data, 64);
    FlashRead_SysParams(PARAM_ID_SVR_PORT, data, 64);
    FlashRead_SysParams(PARAM_ID_SVR_APN, data, 64);
    FlashRead_SysParams(PARAM_ID_IAP_MD5, data, 64);
    FlashRead_SysParams(PARAM_ID_1ST_BOOT, data, 64);
    FlashRead_SysParams(PARAM_ID_ALM_ON, data, 64);
    FlashRead_SysParams(PARAM_ID_BEEP_ON, data, 64);
    FlashRead_SysParams(PARAM_ID_BEEP_LEVEL, data, 64);
    FlashRead_SysParams(PARAM_ID_BOOT_TM, data, 64);
    FlashRead_SysParams(PARAM_ID_IAP_FLAG, data, 64);
    FlashRead_SysParams(PARAM_ID_IAP_STA, data, 64);
    FlashRead_SysParams(PARAM_ID_IAP_CNT, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U1, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U2, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U3, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U4, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U5, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U6, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U7, data, 64);
    FlashRead_SysParams(PARAM_ID_RSVD_U8, data, 64);
}
