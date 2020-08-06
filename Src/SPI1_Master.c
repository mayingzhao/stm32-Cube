/*
+-----------------------------------------------------------------------------+
| ***************************** SPI1_Master.c ******************************  |
+-----------------------------------------------------------------------------+
|                  Copyright by PHOENIX CONTACT Software GmbH                 |
|                                                                             |
|                       PHOENIX CONTACT Software GmbH                         |
|                       Langenbruch 6                                         |
|                       D-32657 Lemgo                                         |
|                       Germany                                               |
|                                                                             |
+-----------------------------------------------------------------------------+
| Description:                                                                |
+-----------------------------------------------------------------------------+
| The use of this Runtime Software Toolkit including but not limited to its   |
| Source Code is subject to restrictions as agreed in the GENERAL TERMS AND   |
| CONDITIONS and ENDUSER LICENCE AGREEMENT (EULA)/SOFTWARE LICENCE AGREEMENT  |
| of Phoenix Contact Software GmbH.                                           |
|                                                                             |
| Copying, distribution or modification of this Source Code is not allowed    |
| unless expressly permitted in written form by PHOENIX CONTACT Software GmbH.|
|                                                                             |
| If you happen to find any faults in it, contact the technical support of    |
| Phoenix Contact Software GmbH (support-pcs@phoenixcontact.com).             |
+-----------------------------------------------------------------------------+
*/

/*! \file SPI1_Master.c
 *  \brief platform depended read/write functions for host parallel/serial communication
 */ 
 
/*===========================================================================*/
/* Includes                                                                  */
/*===========================================================================*/
#include <TPS_1_API.h>
#include "main.h"
#include "stm32f1xx_hal.h"

#ifndef USE_INT_APP
//    #include <low_level_initialization.h>
#endif
/* Write buffer incluxsive the comannd!                                      */
/*---------------------------------------------------------------------------*/
#ifdef SPI_INTERFACE
static USIGN8 g_byMEMDirect[MAX_BUFFER_LEN_SPI_DATA];
#endif

#if !defined(PARALLEL_INTERFACE) && !defined(SPI_INTERFACE)
#error Please define TPS access mode: PARALLEL_INTERFACE or SPI_INTERFACE.
#endif
#if defined(PARALLEL_INTERFACE) && defined(SPI_INTERFACE)
#error Please define only one TPS access mode: PARALLEL_INTERFACE or SPI_INTERFACE.
#endif

/* Defines for 4 kB page size mode.                                          */
/*---------------------------------------------------------------------------*/
#ifdef USE_4KB_PAGES
static USIGN32 locCalculate4kPageAddress(USIGN8* pbyAddress, USIGN8** ppbyCorrectedAddress);

#define PAGE_SELECT_POS  12
#define PAGE_0           (0x00 << PAGE_SELECT_POS)
#define PAGE_1           (0x01 << PAGE_SELECT_POS)
#define PAGE_2           (0x02 << PAGE_SELECT_POS)
#define PAGE_3           (0x03 << PAGE_SELECT_POS)
#define ADDRESS_MASK     0x0FFF
#endif

/* Transmit and receive buffer for TPS_ReadData() and TPS_WriteData()        */
/*---------------------------------------------------------------------------*/
#ifdef SPI_INTERFACE
static USIGN8 g_byRxTxBuffer[MAX_BUFFER_LEN_SPI_DATA];
#endif

extern SPI_HandleTypeDef hspi1;

/*****************************************************************************
**
** FUNCTION NAME: TPS_SetValue8()
**
** DESCRIPTION:   The function writes a byte to a memory location
**                that is described by a pointer into the DPRAM.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pbyMemory
**                USIGN8  byValue (value to be set)
**
**
*******************************************************************************
*/
USIGN32 TPS_SetValue8(USIGN8* pbyMemory, USIGN8 byValue)
{
#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    *(USIGN8*)pbyMemory = byValue;
#endif

#ifdef SPI_INTERFACE
    USIGN8  byMEMAddress[4] = { 0x00, 0x00, 0x00, 0x00 };
    USIGN8  byMEMDirectW[]  = { 0x41, 0x00, 0x00, 0x00};
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    /*----------------------------------------------------------------------*/
    /*                        |      |     |    |-----|-> data              */
    /*                        |      |     |                                */
    /*                        |      |-----|-> address (16-bit)             */
    /*                        |-> Write direct MEM, 2 byte                  */
    /*----------------------------------------------------------------------*/

    /* Extract the memory address!                                          */
    /*----------------------------------------------------------------------*/
    memcpy(&byMEMAddress[0], &pbyMemory, 4);
    memcpy(&byMEMDirectW[1], &byMEMAddress[0], 1);
    memcpy(&byMEMDirectW[2], &byMEMAddress[1], 1);

    /* Copy value into the variable!                                        */
    /*----------------------------------------------------------------------*/
    memcpy(&byMEMDirectW[3], &byValue, 1);

    /* Send write command!                                                  */
    /*----------------------------------------------------------------------*/
    dwErrorCode = TPS_SPI_WriteData(byMEMDirectW, 4);
    if (dwErrorCode != TPS_ACTION_OK)
    {
        return (SPI_INTERFACE_WRITE_FAULT);
    }
#endif

    return(TPS_ACTION_OK);
}

/*****************************************************************************
**
** FUNCTION NAME: TPS_SetValue16()
**
** DESCRIPTION:   The function writes a word to a memory location
**                that is described by a pointer into the DPRAM.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**                                 SPI_INTERFACE_WRITE_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pbyMemory
**                USIGN8  wValue (value to be set)
**
**
*******************************************************************************
*/
USIGN32 TPS_SetValue16(USIGN8* pbyMemory, USIGN16 wValue)
{
#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef USE_INT_APP
    ((U16P*)(pbyMemory))->value = wValue;
    return TPS_ACTION_OK;
#endif

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    *(USIGN16*)pbyMemory = wValue;
#endif

#ifdef SPI_INTERFACE
    USIGN8  byMEMAddress[4] = { 0x00, 0x00, 0x00, 0x00 };
    USIGN8  byMEMDirectW[] = { 0x42, 0x00, 0x00, 0x00, 0x00 };
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    /*----------------------------------------------------------------------*/
    /*                        |      |     |    |-----|-> data              */
    /*                        |      |     |                                */
    /*                        |      |-----|-> address (16-bit)             */
    /*                        |-> Write direct MEM, 2 byte                  */
    /*----------------------------------------------------------------------*/

    /* Extract the memory address!                                          */
    /*----------------------------------------------------------------------*/
    memcpy(&byMEMAddress[0], &pbyMemory, 4);
    byMEMDirectW[1] = byMEMAddress[0];
    byMEMDirectW[2] = byMEMAddress[1];

    /* Copy value into the variable!                                        */
    /*----------------------------------------------------------------------*/
    memcpy(&byMEMDirectW[3], &wValue, 2);

    /* Send read command!                                                   */
    /*----------------------------------------------------------------------*/
    dwErrorCode = TPS_SPI_ReadData(byMEMDirectW, 5);
    if (dwErrorCode != TPS_ACTION_OK)
    {
        return (SPI_INTERFACE_WRITE_FAULT);
    }
#endif

    return(TPS_ACTION_OK);
}

/*****************************************************************************
**
** FUNCTION NAME: TPS_SetValue32()
**
** DESCRIPTION:   The function writes a "double word" to a memory location
**                that is described by a pointer into the DPRAM.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**                                 SPI_INTERFACE_WRITE_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pbyMemory
**                USIGN32 dwValue (value to be set)
**
**
*******************************************************************************
*/
USIGN32 TPS_SetValue32(USIGN8* pbyMemory, USIGN32 dwValue)
{
#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef USE_INT_APP
    ((U32P*)(pbyMemory))->value = dwValue;
    return TPS_ACTION_OK;
#endif

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    *(USIGN32*)pbyMemory = dwValue;
#endif

#ifdef SPI_INTERFACE
    USIGN8  byPointer[4] = { 0x00, 0x00, 0x00, 0x00 };
    USIGN8  byMEMDirectW[] = { 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    /*----------------------------------------------------------------------*/
    /*                        |      |     |    |-----|-----|-----|-> data  */
    /*                        |      |     |                                */
    /*                        |      |-----|-> address (16-bit)             */
    /*                        |-> Write direct MEM, 4 byte                  */
    /*----------------------------------------------------------------------*/

    /* Extract the memory address!                                          */
    /*----------------------------------------------------------------------*/
    memcpy(&byPointer[0], &pbyMemory, 4);
    byMEMDirectW[1] = byPointer[0];
    byMEMDirectW[2] = byPointer[1];

    memcpy(&byMEMDirectW[3], &dwValue, 4);

    /* Send read command!                                                   */
    /*----------------------------------------------------------------------*/
    dwErrorCode = TPS_SPI_ReadData(byMEMDirectW, 7);
    if (dwErrorCode != TPS_ACTION_OK)
    {
        return (SPI_INTERFACE_WRITE_FAULT);
    }
#endif

    return(TPS_ACTION_OK);
}

/*****************************************************************************
**
** FUNCTION NAME: TPS_SetValueData()
**
** DESCRIPTION:   The function writes a data buffer (byte)
**                to a memory location that is described by a pointer.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pbyMemory (pointer into the DPRAM, target)
**                USIGN8* pbySourceMemory (pointer to the source buffer)
**                USIGN32 dwBufferLength (value to be set)
**
**
*******************************************************************************
*/
USIGN32 TPS_SetValueData(USIGN8* pbyMemory, USIGN8* pbySourceMemory, USIGN32 dwBufferLength)
{
#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef SPI_INTERFACE
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    USIGN8 *pbyBufferLen = NULL;
    USIGN8 byPointer[4]  = { 0x00, 0x00, 0x00, 0x00 };
#endif

    /* If 0, no data to be transfered!                                      */
    /*----------------------------------------------------------------------*/
    if(dwBufferLength == 0)
    {
        return TPS_ACTION_OK;
    }

    if (dwBufferLength > (MAX_BUFFER_LEN_SPI_DATA - CMD_MEM_LEN) )
    {
        return (SPI_INTERFACE_SET_BUFFER_FAULT);
    }

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    memcpy(pbyMemory, pbySourceMemory, dwBufferLength);
#endif

#ifdef SPI_INTERFACE
    /* Delete send-buffer.                                                  */
    /*----------------------------------------------------------------------*/
    memset(&g_byMEMDirect[0], 0x00, MAX_BUFFER_LEN_SPI_DATA);

    /* Set SPI Command (Write MEM!)                                         */
    /*----------------------------------------------------------------------*/
    g_byMEMDirect[0] = 0x40;

    /* Extract the memory address and copy it to the Send Buffer!           */
    /*----------------------------------------------------------------------*/
    memcpy(&byPointer[0], &pbyMemory, 4);
    g_byMEMDirect[1] = byPointer[0];
    g_byMEMDirect[2] = byPointer[1];

    /* Copy the data length                                                 */
    /*----------------------------------------------------------------------*/
    pbyBufferLen = (USIGN8*)&dwBufferLength;
    g_byMEMDirect[3] = *pbyBufferLen;
    pbyBufferLen++;
    g_byMEMDirect[4] = *pbyBufferLen;

    /* Copy Source Buffer into the "send buffer".                           */
    /*----------------------------------------------------------------------*/
    memcpy(&g_byMEMDirect[CMD_MEM_LEN], pbySourceMemory, dwBufferLength);

    /* Send SPI write command (Buffer length + command length)              */
    /*----------------------------------------------------------------------*/
    dwErrorCode = TPS_SPI_WriteData(g_byMEMDirect, dwBufferLength + CMD_MEM_LEN);
    if (dwErrorCode != TPS_ACTION_OK)
    {
        return (SPI_INTERFACE_WRITE_FAULT);
    }
#endif

    return(TPS_ACTION_OK);
}


/*****************************************************************************
**
** FUNCTION NAME: TPS_GetValue8()
**
** DESCRIPTION:   This function reads a byte from a memory
**                location (DPRAM) and writes it into a variable.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pbyMemory
**                USIGN8* pbyValue (pointer to variable)
**
**
*******************************************************************************
*/
USIGN32 TPS_GetValue8(USIGN8 *pbyMemory, USIGN8 *pbyValue)
{
#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    *pbyValue = *(USIGN8*)pbyMemory;
#endif

#ifdef SPI_INTERFACE
    USIGN8 byMEMAddress[4] = { 0x00, 0x00, 0x00, 0x00 };
    USIGN8 byMEMDirectR[] = { 0x81, 0x00, 0x00, 0x00};
    /*----------------------------------------------------------------------*/
    /*                        |      |     |    |-----|-> data              */
    /*                        |      |     |                                */
    /*                        |      |-----|-> address (16-bit)             */
    /*                        |-> Read direct MEM, 2 byte                   */
    /*----------------------------------------------------------------------*/

    /* Extract the memory address!                                          */
    /*----------------------------------------------------------------------*/
    memcpy(&byMEMAddress[0],&pbyMemory,4);
    byMEMDirectR[1] = byMEMAddress[0];
    byMEMDirectR[2] = byMEMAddress[1];

    /* Send read command!                                                   */
    /*----------------------------------------------------------------------*/
    TPS_SPI_ReadData(byMEMDirectR,4);

    /* Copy value into the variable!                                        */
    /*----------------------------------------------------------------------*/
    memcpy(pbyValue,&byMEMDirectR[3],1);
#endif

    return(TPS_ACTION_OK);
}

/*****************************************************************************
**
** FUNCTION NAME: TPS_GetValue16()
**
** DESCRIPTION:   This function reads a word (USIGN16) from a memory
**                location (DPRAM) and writes it into a variable.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8*  pbyMemory
**                USIGN16* pwValue (pointer to variable)
**
**
*******************************************************************************
*/
USIGN32 TPS_GetValue16(USIGN8* pbyMemory, USIGN16* pwValue)
{
#ifdef USE_INT_APP
    *pwValue = ((U16P*)(pbyMemory))->value;
    return TPS_ACTION_OK ;
#endif

#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    *pwValue = *(USIGN16*)pbyMemory;
#endif

#ifdef SPI_INTERFACE
    USIGN8 byMEMAddress[4] = { 0x00, 0x00, 0x00, 0x00 };
    USIGN8 byMEMDirectR[] = { 0x82, 0x00, 0x00, 0x00, 0x00};
    /*----------------------------------------------------------------------*/
    /*                        |      |     |    |-----|-> data              */
    /*                        |      |     |                                */
    /*                        |      |-----|-> address (16-bit)             */
    /*                        |-> Read direct MEM, 2 byte                   */
    /*----------------------------------------------------------------------*/

    /* Extract the memory address!                                          */
    /*----------------------------------------------------------------------*/
    memcpy(&byMEMAddress[0], &pbyMemory, 4);
    byMEMDirectR[1] = byMEMAddress[0];
    byMEMDirectR[2] = byMEMAddress[1];

    /* Send read command!                                                   */
    /*----------------------------------------------------------------------*/
    TPS_SPI_ReadData(byMEMDirectR, 5);

    /* Copy value into the variable!                                        */
    /*----------------------------------------------------------------------*/
    memcpy(pwValue, &byMEMDirectR[3], 2);
#endif

    return(TPS_ACTION_OK);
}

/*****************************************************************************
**
** FUNCTION NAME: TPS_GetValue32()
**
** DESCRIPTION:   This function reads a doubpe word (USIGN32) from a memory
**                location (DPRAM) and writes it into a variable.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8*  pbyMemory
**                USIGN32* pdwValue
**
**
**
*******************************************************************************
*/
USIGN32 TPS_GetValue32(USIGN8* pbyMemory, USIGN32* pdwValue)
{
#ifdef USE_INT_APP
    *pdwValue = ((U32P*)(pbyMemory))->value;
    return(TPS_ACTION_OK);
#endif

#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    *pdwValue = *(USIGN32*)pbyMemory;
#endif

#ifdef SPI_INTERFACE
    USIGN8 byPointer[4] = { 0x00, 0x00, 0x00, 0x00 };
    USIGN8 byMEMDirectR[] = { 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    USIGN32 dwErrorCode = TPS_ACTION_OK;

    /*----------------------------------------------------------------------*/
    /*                        |      |     |    |-----|-----|-----|-> data  */
    /*                        |      |     |                                */
    /*                        |      |-----|-> address (16-bit)             */
    /*                        |-> Read direct MEM, 4 byte                   */
    /*----------------------------------------------------------------------*/

    /* Extract the memory address!                                          */
    /*----------------------------------------------------------------------*/
    memcpy(&byPointer[0], &pbyMemory, 4);
    byMEMDirectR[1] = byPointer[0];
    byMEMDirectR[2] = byPointer[1];

    /* Send read command!                                                   */
    /*----------------------------------------------------------------------*/
    dwErrorCode = TPS_SPI_ReadData(byMEMDirectR, 7);
    if ( dwErrorCode != TPS_ACTION_OK)
    {
        return (dwErrorCode);
    }
    /* Copy value into the variable!                                        */
    /*----------------------------------------------------------------------*/
    memcpy(pdwValue, &byMEMDirectR[3], 4);
#endif

     return(TPS_ACTION_OK);
}


/*****************************************************************************
**
** FUNCTION NAME: TPS_GetIOAddress()
**
** DESCRIPTION:   This function reads the start address of the Input or
**                Output IO DPRAM from the DPRAM writes it into a variable.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8** pbyMemory
**                USIGN32* pdwValue
**
**
**
*******************************************************************************
*/
USIGN32 TPS_GetIOAddress(USIGN8* pbyMemory, USIGN8** pdwValue)
{
#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

#ifdef PARALLEL_INTERFACE
    pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
    *pdwValue = (USIGN8*)( *(USIGN16*)pbyMemory );
#endif

#ifdef SPI_INTERFACE
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    USIGN8 byPointer[4] = { 0x00, 0x00, 0x00, 0x00 };
    USIGN8 byMEMDirectR[] = { 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    /*----------------------------------------------------------------------*/
    /*                        |      |     |    |-----|-----|-----|-> data  */
    /*                        |      |     |                                */
    /*                        |      |-----|-> address (16-bit)             */
    /*                        |-> Read direct MEM, 4 byte                   */
    /*----------------------------------------------------------------------*/

    /* Extract the memory address!                                          */
    /*----------------------------------------------------------------------*/
    memcpy(&byPointer[0], &pbyMemory, 4);
    byMEMDirectR[1] = byPointer[0];
    byMEMDirectR[2] = byPointer[1];

    /* Send read command!                                                   */
    /*----------------------------------------------------------------------*/
    dwErrorCode = TPS_SPI_ReadData(byMEMDirectR, 7);
    if ( dwErrorCode != TPS_ACTION_OK)
    {
        return (dwErrorCode);
    }
    /* Copy value into the variable!                                        */
    /*----------------------------------------------------------------------*/
    memcpy(pdwValue, &byMEMDirectR[3], 2);
#endif

    return(TPS_ACTION_OK);
}


/*****************************************************************************
**
** FUNCTION NAME: TPS_GetValueData()
**
** DESCRIPTION:   The function read a data buffer (byte)
**                to a memory location that is described by a pointer.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_GET_BUFFER_FAULT
**                ErrorCode        SPI_INTERFACE_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pbyMemory (pointer into the DPRAM)
**                USIGN8* pbyDestMemory (pointer to the program buffer)
**                USIGN32 dwBufferLength (value to be set)
**
**
*******************************************************************************
*/
USIGN32 TPS_GetValueData(USIGN8* pbyMemory, USIGN8* pbyDestMemory, USIGN32 dwBufferLength)
{
#ifdef USE_4KB_PAGES
  /* Calculate the correct address if 4 kB Pages are used. */
  USIGN32 dwErrorCode = TPS_ACTION_OK;
  dwErrorCode = locCalculate4kPageAddress(pbyMemory, &pbyMemory);

  if(dwErrorCode != TPS_ACTION_OK)
  {
    return dwErrorCode;
  }
#endif

#ifdef SPI_INTERFACE
  USIGN8 byPointer[4] = { 0x00, 0x00, 0x00, 0x00 };
  USIGN32 dwErrorCode = TPS_ACTION_OK;
#endif

  if (pbyDestMemory == NULL)
  {
    return (SPI_INTERFACE_PARAM_FAULT);
  }

  /* If 0, no data to be transfered or too much!                          */
  /*----------------------------------------------------------------------*/
  if ((dwBufferLength == 0) || (dwBufferLength > (MAX_BUFFER_LEN_SPI_DATA - CMD_MEM_LEN)))
  {
    return(SPI_INTERFACE_GET_BUFFER_FAULT);
  }

#ifdef PARALLEL_INTERFACE

  pbyMemory += BASE_ADDRESS_OFFSET_DPRAM;
  
  memcpy(pbyDestMemory, pbyMemory, dwBufferLength);

#endif

#ifdef SPI_INTERFACE

  /* Initialize send buffer.                                               */
  /*-----------------------------------------------------------------------*/
  memset(&g_byMEMDirect[0], 0x00, MAX_BUFFER_LEN_SPI_DATA);
  g_byMEMDirect[0] = 0x80;              /* MEM_READ command.               */

  /* Extract the memory address!                                     -     */
  /*-----------------------------------------------------------------------*/
  memcpy(&byPointer[0], &pbyMemory, 4);
  g_byMEMDirect[1] = byPointer[0];
  g_byMEMDirect[2] = byPointer[1];
  /* Copy the data length                                                  */
  /*-----------------------------------------------------------------------*/
  g_byMEMDirect[3] =(USIGN8)(dwBufferLength & 0xFF);
  g_byMEMDirect[4] =(USIGN8)(dwBufferLength >> 0x08);

  /* Send read command!                                                    */
  /*-----------------------------------------------------------------------*/
  dwErrorCode = TPS_SPI_ReadData(g_byMEMDirect, dwBufferLength + CMD_MEM_LEN);
  if ( dwErrorCode != TPS_ACTION_OK)
  {
    return (dwErrorCode);
  }
  /* Copy value into the variable!                                         */
  /*-----------------------------------------------------------------------*/
  memcpy(pbyDestMemory, &g_byMEMDirect[CMD_MEM_LEN], dwBufferLength);
#endif

  return(TPS_ACTION_OK);
}

#ifdef SPI_INTERFACE
/*****************************************************************************
**
** FUNCTION NAME: TPS_SPI_ReadData()
**
** DESCRIPTION:   Data out of the DPRAM is read and copied into a buffer
**                of the host CPU.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_READ_FAULT
**                                 SPI_INTERFACE_READ_PARAM_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pReadBuffer
**                USIGN32 dwBufferLength
**
** This function is CPU dependent!
**
*******************************************************************************
*/
USIGN32 TPS_SPI_ReadData(USIGN8* pbyReadBuffer, USIGN32 dwBufferLength)
{
    USIGN32 idx;
    HAL_StatusTypeDef dwErrorCode = HAL_OK;

    /* Check length of data buffer. If 0, no data to be transfered!          */
    /*-----------------------------------------------------------------------*/
    if( (dwBufferLength == 0) || (dwBufferLength > MAX_BUFFER_LEN_SPI_DATA) )
    {
        return (SPI_INTERFACE_READ_PARAM_FAULT);
    }

    /* Clear buffer                                                          */
    /*-----------------------------------------------------------------------*/
    //memset(g_byRxTxBuffer, 0x00, dwBufferLength);

    /* send buffer, buffer length, receive buffer                            */
    /*-----------------------------------------------------------------------*/
    //dwErrorCode = HAL_SPI_TransmitReceive(&hspi1,pbyReadBuffer,g_byRxTxBuffer,dwBufferLength,100);
    for(idx = 0 ; idx < dwBufferLength ; idx ++)
    {
      HAL_GPIO_WritePin(HOST_SFRN_GPIO_Port,HOST_SFRN_Pin,GPIO_PIN_RESET);
      hspi1.Instance->DR = pbyReadBuffer[idx];
      while(1)
      {
        if(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_RXNE))break;
      }
      pbyReadBuffer[idx] = hspi1.Instance->DR;
      HAL_GPIO_WritePin(HOST_SFRN_GPIO_Port,HOST_SFRN_Pin,GPIO_PIN_SET);
      for(int delay_cnt = 0 ; delay_cnt < 4 ; delay_cnt ++){ __nop();}
    }
    if ( dwErrorCode != HAL_OK )
    {
        return (SPI_INTERFACE_READ_FAULT);
    }

    //memcpy(pbyReadBuffer, g_byRxTxBuffer, dwBufferLength);

    return(dwErrorCode);
}

/*****************************************************************************
**
** FUNCTION NAME: TPS_SPI_WriteData()
**
** DESCRIPTION:   This function writes data into the DPRAM.
**
** RETURN:        TPS_ACTION_OK
**                ErrorCode        SPI_INTERFACE_WRITE_FAULT
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pWriteBuffer
**                USIGN32 dwBufferLength
**
** This function is CPU dependent!
**
*******************************************************************************
*/
USIGN32 TPS_SPI_WriteData(USIGN8* pbyWriteBuffer, USIGN32 dwBufferLength)
{
    USIGN32 idx;
    HAL_StatusTypeDef dwErrorCode = HAL_OK;

    /* If 0, no data to be transfered!                                       */
    /*-----------------------------------------------------------------------*/
    if ( (dwBufferLength == 0) || (dwBufferLength > MAX_BUFFER_LEN_SPI_DATA) )
    {
        return(SPI_INTERFACE_WRITE_FAULT);
    }

    /* Initialize the receive Buffer                                        */
    /*-----------------------------------------------------------------------*/
    memset(g_byRxTxBuffer, 0x00, dwBufferLength);
    
    for(idx = 0 ; idx < dwBufferLength ; idx ++)
    {
      HAL_GPIO_WritePin(HOST_SFRN_GPIO_Port,HOST_SFRN_Pin,GPIO_PIN_RESET);
      hspi1.Instance->DR = pbyWriteBuffer[idx];
      while(1)
      {
        if(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_RXNE))break;
      }
      g_byRxTxBuffer[idx] = hspi1.Instance->DR;
      HAL_GPIO_WritePin(HOST_SFRN_GPIO_Port,HOST_SFRN_Pin,GPIO_PIN_SET);
      for(int delay_cnt = 0 ; delay_cnt < 4 ; delay_cnt ++){ __nop();}
    }

    /* Start transmission!                                                   */
    /*-----------------------------------------------------------------------*/
    //dwErrorCode = HAL_SPI_TransmitReceive(&hspi1,pbyWriteBuffer, g_byRxTxBuffer,dwBufferLength,100);
    if (dwErrorCode != HAL_OK)
    {
       return(SPI_INTERFACE_WRITE_FAULT);
    }

    return(TPS_ACTION_OK);
}

#endif
/*****************************************************************************
**
** FUNCTION NAME: TPS_MemSet()
**
** DESCRIPTION:   Initialize memory with special values.
**                This function uses no data buffer to save memory Space.
**                If it is possible TPS_SetValueData() can also be used.
**
** RETURN:        TPS_ACTION_OK
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN8* pbyTarget
**                USIGN8  byValue - initial value
**                USIGN16 wLength - init area size
**
** Note:          A pointer can have the value "0" (start address of the DPRAM).
**
*******************************************************************************
*/
USIGN32 TPS_MemSet(USIGN8* pbyTarget, USIGN8 byValue, USIGN16 wLength)
{
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    USIGN16 wCount;

#ifdef USE_4KB_PAGES
    /* Calculate the correct address if 4 kB Pages are used. */
    dwErrorCode = locCalculate4kPageAddress(pbyTarget, &pbyTarget);

    if(dwErrorCode != TPS_ACTION_OK)
    {
        return dwErrorCode;
    }
#endif

    /* No data to be transfered!                                             */
    /*-----------------------------------------------------------------------*/
    if (wLength == 0)
    {
        return (SPI_INTERFACE_PARAM_FAULT);
    }

    for(wCount = 0; wCount < wLength; wCount++)
    {
        dwErrorCode = TPS_SetValue8(pbyTarget, byValue);
        if (dwErrorCode != TPS_ACTION_OK)
        {
            return(SPI_INTERFACE_WRITE_FAULT);
        }
        pbyTarget++;
    }

    return(dwErrorCode);
}


#ifdef USE_4KB_PAGES
/*****************************************************************************
 **
 ** FUNCTION NAME:   locCalculate4kPageAddress
 **
 ** DESCRIPTION:     Calculates the correct address for the 4 kB Page Mode of the TPS-1.
 **
 ** RETURN:          TPS_ACTION_OK     0
 **                  Action failed     ErrorCode
 **
 ** PARAMETER:       USIGN8*   pbyAddress - The original address.
 **                  USIGN8**  ppbyCorrectedAddress - In this variable the corrected address is stored.
 ********************************************************************************/
static USIGN32 locCalculate4kPageAddress(USIGN8* pbyAddress, USIGN8** ppbyCorrectedAddress)
{
    USIGN32 dwAddress = (USIGN32)pbyAddress;
    USIGN32 dwCorrectedAddress;
    USIGN32 dwRetval = TPS_ACTION_OK;

    if(/*(dwAddress >= 0x0000) && */
       (dwAddress <  0x1000))
    {
        /* Page 0 */
        dwCorrectedAddress = (dwAddress & ADDRESS_MASK) + PAGE_0;
    }
    else if((dwAddress >= 0x2000) &&
            (dwAddress <  0x3000))
    {
        /* Page 1 */
        dwCorrectedAddress = (dwAddress & ADDRESS_MASK) + PAGE_1;
    }
    else if((dwAddress >= 0x8000) &&
            (dwAddress <  0x9000))
    {
        /* Page 2 */
        dwCorrectedAddress = (dwAddress & ADDRESS_MASK) + PAGE_2;
    }
    else if((dwAddress >= 0x9000) &&
            (dwAddress <  0xA000))
    {
        /* Page 3 */
        dwCorrectedAddress = (dwAddress & ADDRESS_MASK) + PAGE_3;
    }
    else
    {
        dwRetval = SPI_INVALID_4KB_ADDRES;
    }

    if(dwRetval == TPS_ACTION_OK)
    {
        *ppbyCorrectedAddress = (USIGN8*)dwCorrectedAddress;
    }

    return dwRetval;
}
#endif
