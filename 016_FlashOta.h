#ifndef _MCU_DRIVERS_DATARECORD_H_
#define _MCU_DRIVERS_DATARECORD_H_
 
#include "015_Common.h" 
#include "017_InnerFlash.h"
 
void FlashErase_LargePage(u16 pageIndex, u32 offsetaddr);
void FlashErase_SomePage(u16 highAddr, u16 lowAddr, u16 pageNum);
void FlashErase_OnePage(u16 flash_page, u16 flash_offset);
u16 FlashRead_SomeInstructionWords(u16 flash_page, u16 flash_offset, u16 len, u8 *pdata);

void FlashWriteRead_Test(void);
u16 FlashRead_AllNFCCards(u8 *card_dat);
u16 FlashWrite_OneNFCCard(u8 *card_dat);
u16 FlashWrite_DeleteOneCard(u8 *card_dat);

#endif
