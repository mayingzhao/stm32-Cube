/*
+-----------------------------------------------------------------------------+
| ****************************** SPI1_Master.h *****************************  |
+-----------------------------------------------------------------------------+
|              Copyright 2015 by Phoenix Contact Software GmbH                |
|                                                                             |
|                       Phoenix Contact Software GmbH                         |
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
| unless expressly permitted in written form by Phoenix Contact Software GmbH.|
|                                                                             |
| If you happen to find any faults in it, contact the technical support of    |
| Phoenix Contact Software GmbH (support-pcs@phoenixcontact.com).             |
+-----------------------------------------------------------------------------+
*/

/*! \file SPI1_Master.h
 *  \brief header defintion for SPI1_Master.c
 */
 
#ifndef _SPI_MASTER_H_
#define _SPI_MASTER_H_

#include <TPS_1_user.h>

#define CMD_MEM_LEN         5       /* length of SPI command                 */
#define BUFFER_SIZE         340
#define NRT_AREA_SIZE       0x8000  /* Maximum size for read and write!      */

/* Max. Mailbox Size + Command                                               */
/*---------------------------------------------------------------------------*/
#define MAX_BUFFER_LEN_SPI_DATA    (MAX_LEN_ETHERNET_FRAME + CMD_MEM_LEN)

#define COMMAND_LEN          0x01
#define ADDRESS_LEN          0x02
#define EXCHANGE_COMMAND_LEN 0x03

#define BYTE_LEN             0x01
#define WORD_LEN             0x02
#define DWORD_LEN            0x04

USIGN32   TPS_SPI_ReadData(USIGN8* pbyReadBuffer, USIGN32 dwBufferLength);
USIGN32   TPS_SPI_WriteData(USIGN8* pbyWriteBuffer, USIGN32 dwBufferLength);

USIGN32   TPS_SetValue8(USIGN8* pbyMemory, USIGN8 byValue);
USIGN32   TPS_SetValue16(USIGN8* pbyMemory, USIGN16 wValue);
USIGN32   TPS_SetValue32(USIGN8* pbyMemory, USIGN32 dwValue);
USIGN32   TPS_SetValueData(USIGN8* pbyMemory, USIGN8* pbySourceMemory, USIGN32 dwBufferLength);

USIGN32   TPS_GetValue8(USIGN8* pbyMemory, USIGN8* pbyValue);
USIGN32   TPS_GetValue16(USIGN8* pbyMemory, USIGN16* pwValue);
USIGN32   TPS_GetValue32(USIGN8* pbyMemory, USIGN32* pdwValue);
USIGN32   TPS_GetValueData(USIGN8* pbyMemory, USIGN8* pbyDestMemory, USIGN32 dwBufferLength);
USIGN32   TPS_GetIOAddress(USIGN8* pbyMemory, USIGN8** pdwValue);

USIGN32   TPS_MemSet(USIGN8* pbyTarget, USIGN8 byValue, USIGN16 wLength);

#endif /* #ifndef _SPI_MASTER_H_ */
