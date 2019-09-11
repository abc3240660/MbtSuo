#ifndef _MCU_DRIVERS_DATARECORD_H_
#define _MCU_DRIVERS_DATARECORD_H_
 
#include "015_Common.h" 
#include "017_InnerFlash.h"
 
void FillLagrePage(u16 pageIndex);
void EraseLargePage(u16 pageIndex, u32 offsetaddr);
void EraseSomePage(u16 highAddr, u16 lowAddr, u16 pageNum);
void DataRecord_ErasePage(u16 DATA_RECORD_START_PAGE,u16 DATA_RECORD_START_ADDR );
u16 DataRecord_ReadData(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 len, u8 *pdata); 
u16 DataRecord_WriteData(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, volatile OneInstruction_t data); 
u16 DataRecord_WriteDataArray(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, volatile OneInstruction_t *data, u16 length);
u16 DataRecord_WriteBytesArray(u16 DATA_RECORD_START_PAGE, u16 DATA_RECORD_START_ADDR, u16 index, u8 *data, u16 length);
void DataRecord_WriteReadTest(void);

#endif