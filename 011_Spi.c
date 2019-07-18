//       10        20        30        40        50        60        70
//--*----*----*----*----*----*----*----*----*----*----*----*----*----*----*----*

/******************************************************************************
 * A library for PIC24F SPI2(for CLRC663)
 * This file is about the SPI2 API of PIC24
 *
 * Copyright (c) 2019 Mobit technology inc.
 * @Author       : Damon
 * @Create time  : 03/11/2019
******************************************************************************/

#include "011_Spi.h"

/**
 Section: File specific functions
*/

/**
  SPI2 Transfer Mode Enumeration

  @Summary
    Defines the Transfer Mode enumeration for SPI2.

  @Description
    This defines the Transfer Mode enumeration for SPI2.
 */
typedef enum {
    SPI2_TRANSFER_MODE_32BIT  = 2,
    SPI2_TRANSFER_MODE_16BIT = 1,
    SPI2_TRANSFER_MODE_8BIT = 0
} SPI2_TRANSFER_MODE;

inline __attribute__((__always_inline__)) SPI2_TRANSFER_MODE SPI2_TransferModeGet(void);
void SPI2_Exchange( uint8_t *pTransmitData, uint8_t *pReceiveData );
uint16_t SPI2_ExchangeBuffer(uint8_t *pTransmitData, uint16_t byteCount, uint8_t *pReceiveData);

/**
 Section: Driver Interface Function Definitions
*/

void SPI2_Initialize (void)
{
    RPINR22bits.SDI2R = 0x0015;    // RG6->SPI2:SDI2
    RPOR9bits.RP19R = 0x000B;      // RG8->SPI2:SCK2OUT
    RPOR13bits.RP26R = 0x000A;     // RG7->SPI2:SDO2
    RPINR22bits.SCK2R = 0x0013;    // RG8->SPI2:SCK2IN

    LATE = 0x0000;
    LATG = 0x0000;

    TRISE = 0x007F;
    TRISG = 0x024C;

    IOCPDE = 0x0000;
    IOCPDG = 0x0000;
    IOCPUE = 0x0000;
    IOCPUG = 0x0000;

    ODCE = 0x0000;
    ODCG = 0x0000;

    ANSE = 0x0010;
    ANSG = 0x0000;

    // AUDEN disabled; FRMEN disabled; AUDMOD I2S; FRMSYPW One clock wide; AUDMONO stereo; FRMCNT 0; MSSEN disabled; FRMPOL disabled; IGNROV disabled; SPISGNEXT not sign-extended; FRMSYNC disabled; URDTEN disabled; IGNTUR disabled;
    SPI2CON1H = 0x00;
    // WLENGTH 0;
    SPI2CON2L = 0x00;
    // SPIROV disabled; FRMERR disabled;
    SPI2STATL = 0x00;
    // SPI2BRGL 31;
    SPI2BRGL = 0x1F;
    // SPITBFEN disabled; SPITUREN disabled; FRMERREN disabled; SRMTEN disabled; SPIRBEN disabled; BUSYEN disabled; SPITBEN disabled; SPIROVEN disabled; SPIRBFEN disabled;
    SPI2IMSKL = 0x00;
    // RXMSK 0; TXWIEN disabled; TXMSK 0; RXWIEN disabled;
    SPI2IMSKH = 0x00;
    // SPI2URDTL 0;
    SPI2URDTL = 0x00;
    // SPI2URDTH 0;
    SPI2URDTH = 0x00;
    // SPIEN enabled; DISSDO disabled; MCLKEN FOSC/2; CKP Idle:Low, Active:High; SSEN disabled; MSTEN Master; MODE16 disabled; SMP Middle; DISSCK disabled; SPIFE Frame Sync pulse precedes; CKE Active to Idle; MODE32 disabled; SPISIDL disabled; ENHBUF enabled; DISSDI disabled;
    SPI2CON1L = 0x8121;
}

void SPI2_Exchange( uint8_t *pTransmitData, uint8_t *pReceiveData )
{
    while( SPI2STATLbits.SPITBF == true )
    {
    }

    SPI2BUFL = *((uint8_t*)pTransmitData);

    while ( SPI2STATLbits.SPIRBE == true)
    {
    }

    *((uint8_t*)pReceiveData) = SPI2BUFL;
}

uint16_t SPI2_ExchangeBuffer(uint8_t *pTransmitData, uint16_t byteCount, uint8_t *pReceiveData)
{

    uint16_t dataSentCount = 0;
    uint16_t dataReceivedCount = 0;
    uint16_t dummyDataReceived = 0;
    uint16_t dummyDataTransmit = SPI2_DUMMY_DATA;

    uint8_t  *pSend, *pReceived;
    uint16_t addressIncrement;
    uint16_t receiveAddressIncrement, sendAddressIncrement;

    addressIncrement = 1;

    // set the pointers and increment delta
    // for transmit and receive operations
    if (pTransmitData == NULL)
    {
        sendAddressIncrement = 0;
        pSend = (uint8_t*)&dummyDataTransmit;
    }
    else
    {
        sendAddressIncrement = addressIncrement;
        pSend = (uint8_t*)pTransmitData;
    }

    if (pReceiveData == NULL)
    {
       receiveAddressIncrement = 0;
       pReceived = (uint8_t*)&dummyDataReceived;
    }
    else
    {
       receiveAddressIncrement = addressIncrement;
       pReceived = (uint8_t*)pReceiveData;
    }


    while( SPI2STATLbits.SPITBF == true )
    {
    }

    while (dataSentCount < byteCount)
    {
        if ( SPI2STATLbits.SPITBF != true )
        {
            SPI2BUFL = *pSend;

            pSend += sendAddressIncrement;
            dataSentCount++;
        }

        if (SPI2STATLbits.SPIRBE == false)
        {
            *pReceived = SPI2BUFL;

            pReceived += receiveAddressIncrement;
            dataReceivedCount++;
        }

    }

    while (dataReceivedCount < byteCount)
    {
        if (SPI2STATLbits.SPIRBE == false)
        {
            *pReceived = SPI2BUFL;

            pReceived += receiveAddressIncrement;
            dataReceivedCount++;
        }
    }

    return dataSentCount;
}

uint8_t SPI2_Exchange8bit( uint8_t data )
{
    uint8_t receiveData;

    SPI2_Exchange(&data, &receiveData);

    return (receiveData);
}


uint16_t SPI2_Exchange8bitBuffer(uint8_t *dataTransmitted, uint16_t byteCount, uint8_t *dataReceived)
{
    return (SPI2_ExchangeBuffer(dataTransmitted, byteCount, dataReceived));
}

inline __attribute__((__always_inline__)) SPI2_TRANSFER_MODE SPI2_TransferModeGet(void)
{
    if (SPI2CON1Lbits.MODE32 == 1)
        return SPI2_TRANSFER_MODE_32BIT;
    else if (SPI2CON1Lbits.MODE16 == 1)
        return SPI2_TRANSFER_MODE_16BIT;
    else
        return SPI2_TRANSFER_MODE_8BIT;
}

SPI2_STATUS SPI2_StatusGet()
{
    return(SPI2STATL);
}

/**
 End of File
*/
