/******************************************************************************
*
*  Copyright PHOENIX CONTACT Software GmbH
*
*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "TPS_1_API.h"

#include "AssetMgm.h"

#define GET_BIT(p, n) ((((USIGN8*)p)[n/8]>>(n%8))&0x01)

/*******************************************************************/

VOID        FillGUID(T_UUID *poGUID, const USIGN8 *pbyData, USIGN32 *pdwIndex);
USIGN32     GetUInt(const USIGN8 *pbyData, USIGN32 dwQuantity, USIGN32 *pdwIndex);
VOID        SkipPadding32(USIGN32* pdwIndex);
VOID        BlockHeaderDecode(T_BLOCKHEADER *poBlockHeader, const USIGN8 *pbyData, USIGN32 *pdwIndex);

VOID SetBlockHeader(USIGN8 *pbyDataBuffer, USIGN16 *pwDataIndex, USIGN16 wBlockType, USIGN8 bBlockVersionHigh, USIGN8 bBlockVersionLow)
{
    USIGN16 wBlkType = TPS_htons(wBlockType);
    memcpy((pbyDataBuffer + *pwDataIndex), &wBlkType, 2);
    *pwDataIndex += 2;
    /* BlockLength - fill it later */
    *pwDataIndex += 2;
    /* BlockVersion */
    *(pbyDataBuffer + *pwDataIndex) = bBlockVersionHigh;
    *pwDataIndex += 1;
    *(pbyDataBuffer + *pwDataIndex) = bBlockVersionLow;
    *pwDataIndex += 1;
}

VOID SetBlockLength(USIGN8 *pbyStartOfBlockData, USIGN16 wBlockLength)
{
    USIGN16 wDataBlock = TPS_htons(wBlockLength - 2 /* BlockType */ - 2 /* BlockLength */);
    memcpy((pbyStartOfBlockData + 2), &wDataBlock, 2);
}

VOID FillGUID(T_UUID *poGUID, const USIGN8 *pbyData, USIGN32 *pdwIndex)
{
    poGUID->dwData1 = GetUInt(pbyData, 4, pdwIndex);

    poGUID->wData2 = (USIGN16)GetUInt(pbyData, 2, pdwIndex);

    poGUID->wData3 = (USIGN16)GetUInt(pbyData, 2, pdwIndex);

    memcpy(poGUID->pbyData4, &pbyData[*pdwIndex], 8);
    *pdwIndex += 8;
}

USIGN32 SetUUID(USIGN8 *pbyData, T_UUID *poGUID)
{
    USIGN32 dwIndex = 0;
    USIGN32 dwDataDoubleWord;
    USIGN16 wDataWord;

    if((poGUID == NULL) || (pbyData == NULL))
    {
        return 0;
    }
    dwDataDoubleWord = TPS_ntohl(poGUID->dwData1);
    memcpy((pbyData + dwIndex), &dwDataDoubleWord, 4);
    dwIndex += 4;

    wDataWord = TPS_ntohl(poGUID->wData2);
    memcpy((pbyData + dwIndex), &wDataWord, 2);
    dwIndex += 2;

    wDataWord = TPS_ntohl(poGUID->wData3);
    memcpy((pbyData + dwIndex), &wDataWord, 2);
    dwIndex += 2;

    memcpy((pbyData + dwIndex), &(poGUID->pbyData4), 8);
    dwIndex += 8;

    return dwIndex;
}

USIGN32 GetUInt(const USIGN8 *pbyData, USIGN32 dwQuantity, USIGN32 *pdwIndex)
{
    USIGN32 dwResult = 0;

    if(dwQuantity == 2)
    {
        dwResult = TPS_ntohl(*((USIGN16*)&pbyData[*pdwIndex]));
        (*pdwIndex) += 2;
    } else if(dwQuantity == 4)
    {
        dwResult = TPS_ntohl(*((USIGN32*)&pbyData[*pdwIndex]));
        (*pdwIndex) += 4;
    }

    return dwResult;
}

VOID SkipPadding32(USIGN32* pdwIndex)
{
    while(*pdwIndex % 4 != 0)
    {
        (*pdwIndex)++;
    }
}

VOID BlockHeaderDecode(T_BLOCKHEADER *poBlockHeader, const USIGN8 *pbyData, USIGN32 *pdwIndex)
{
    poBlockHeader->oBlockType = (T_BLOCKTYPE)GetUInt(pbyData, 2, pdwIndex);

    poBlockHeader->wBlockLength = (USIGN16)GetUInt(pbyData, 2, pdwIndex);

    poBlockHeader->byBlockVersionHigh = pbyData[*pdwIndex];
    (*pdwIndex)++;

    poBlockHeader->byBlockVersionLow = pbyData[*pdwIndex];
    (*pdwIndex)++;
}

/*******************************************************************/

/*! \addtogroup asset Asset Management Interface 
 *@{
*/

VOID switchAMLocation(USIGN8 *pbyAMLocation)
{
    USIGN16 i;
    USIGN8  b;

    for(i = 0; i < AM_LOCATION_SIZE/2; i++)
    {
        b = pbyAMLocation[(AM_LOCATION_SIZE - 1) - i];
        pbyAMLocation[(AM_LOCATION_SIZE - 1) - i] = pbyAMLocation[i];
        pbyAMLocation[i] = b;
    }
}

/*!
 * \brief       Helper function to free memory depending of the block type information of an asset management block	
 * \param[in]   poAMBlock pointer to the asset management block that has to be freed
 * \retval      VOID
 */
VOID AM_AssetBlockFree(T_ASSET_MANAGEMENT_BLOCK *poAMBlock)
{
    if(poAMBlock == NULL)
    {
        return;
    }

    if(poAMBlock->oBlockHeader.oBlockType == AM_FullInformation)
    {
        free(poAMBlock->oAMInfo.poAmFullInformation);
        poAMBlock->oAMInfo.poAmFullInformation = NULL;
    }
    else if(poAMBlock->oBlockHeader.oBlockType == AM_OnlyFirmwareInformation)
    {
        free(poAMBlock->oAMInfo.poAmFirmwareOnlyInformation);
        poAMBlock->oAMInfo.poAmFirmwareOnlyInformation = NULL;
    }
    else if(poAMBlock->oBlockHeader.oBlockType == AM_OnlyHardwareInformation)
    {
        free(poAMBlock->oAMInfo.poAmHardwareOnlyInformation);
        poAMBlock->oAMInfo.poAmHardwareOnlyInformation = NULL;
    }
}

/*!
 * \brief       This function converts a byte stream into the T_ASSET_MANAGEMENT_BLOCK structure.
 *              The block type of the oBlockHeader tells the function which information of the structure have to be saved.
 * \param[in]   pbyBuffer pointer to a byte buffer with the data stream.
 * \param[in]   poAMBlock pointer to the T_ASSET_MANAGEMENT_BLOCK structure the byte information is stored to.
 * \retval      USIGN32 size in bytes of the converted data
 */
USIGN32 AM_BlockDecode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_BLOCK *poAMBlock)
{
    USIGN32 dwIndex = 0;

    if((pbyBuffer == NULL) || (poAMBlock == NULL))
    {
        return 0;
    }
    BlockHeaderDecode(&poAMBlock->oBlockHeader, pbyBuffer, &dwIndex);
	SkipPadding32(&dwIndex);

    switch(poAMBlock->oBlockHeader.oBlockType)
    {
        case AM_FullInformation:
        {
            T_AM_FULLINFORMATION *poFullInfo;
            poAMBlock->oAMInfo.poAmFullInformation = (T_AM_FULLINFORMATION *)calloc(1, sizeof(T_AM_FULLINFORMATION));
            poFullInfo = poAMBlock->oAMInfo.poAmFullInformation;
            if(poFullInfo == NULL)
            {
                return 0;
            }
            FillGUID(&poFullInfo->oIMUniqueIdentifier, pbyBuffer + dwIndex, &dwIndex);

            memcpy(poFullInfo->pbyAMLocation, pbyBuffer + dwIndex, AM_LOCATION_SIZE);
            dwIndex += AM_LOCATION_SIZE;

            memcpy(poFullInfo->pbyIMAnnotation, pbyBuffer + dwIndex, IM_ANNOTATION_SIZE);
            dwIndex += IM_ANNOTATION_SIZE;

            memcpy(poFullInfo->pbyIMOrderID, pbyBuffer + dwIndex, IM_ORDERID_SIZE);
            dwIndex += IM_ORDERID_SIZE;

            memcpy(poFullInfo->pbyAMSoftwareRevision, pbyBuffer + dwIndex, AM_SOFTWARE_REVISION_SIZE);
            dwIndex += AM_SOFTWARE_REVISION_SIZE;

            memcpy(poFullInfo->pbyAMHardwareRevision, pbyBuffer + dwIndex, AM_HARDWARE_REVISION_SIZE);
            dwIndex += AM_HARDWARE_REVISION_SIZE;

            memcpy(poFullInfo->pbyIMSerialNumber, pbyBuffer + dwIndex, IM_SERIALNUMBER_SIZE);
            dwIndex += IM_SERIALNUMBER_SIZE;

            memcpy(&poFullInfo->oIMSWRevision, pbyBuffer + dwIndex, sizeof(T_IM_SW_REVISION));
            dwIndex += sizeof(T_IM_SW_REVISION);

            poFullInfo->oAMDeviceIdentification.wDeviceSubID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poFullInfo->oAMDeviceIdentification.wDeviceID= (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poFullInfo->oAMDeviceIdentification.wVendorID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poFullInfo->oAMDeviceIdentification.wOrganization = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            poFullInfo->wAMTypeIdentification = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            poFullInfo->wIMHardwareRevision = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            break;
        }
        case AM_OnlyFirmwareInformation:
        {
            T_AM_FIRMWAREONLYINFORMATION *poFwInfo;
            poAMBlock->oAMInfo.poAmFirmwareOnlyInformation = (T_AM_FIRMWAREONLYINFORMATION *)calloc(1, sizeof(T_AM_FIRMWAREONLYINFORMATION));
            poFwInfo = poAMBlock->oAMInfo.poAmFirmwareOnlyInformation;
            if(poFwInfo == NULL)
            {
                return 0;
            }
            FillGUID(&poFwInfo->oIMUniqueIdentifier, pbyBuffer + dwIndex, &dwIndex);

            memcpy(poFwInfo->pbyAMLocation, pbyBuffer + dwIndex, AM_LOCATION_SIZE);
            dwIndex += AM_LOCATION_SIZE;

            memcpy(poFwInfo->pbyIMAnnotation, pbyBuffer + dwIndex, IM_ANNOTATION_SIZE);
            dwIndex += IM_ANNOTATION_SIZE;

            memcpy(poFwInfo->pbyIMOrderID, pbyBuffer + dwIndex, IM_ORDERID_SIZE);
            dwIndex += IM_ORDERID_SIZE;

            memcpy(poFwInfo->pbyAMSoftwareRevision, pbyBuffer + dwIndex, AM_SOFTWARE_REVISION_SIZE);
            dwIndex += AM_SOFTWARE_REVISION_SIZE;

            memcpy(poFwInfo->pbyIMSerialNumber, pbyBuffer + dwIndex, IM_SERIALNUMBER_SIZE);
            dwIndex += IM_SERIALNUMBER_SIZE;

            memcpy(&poFwInfo->oIMSWRevision, pbyBuffer + dwIndex, sizeof(T_IM_SW_REVISION));
            dwIndex += sizeof(T_IM_SW_REVISION);

            poFwInfo->oAMDeviceIdentification.wDeviceSubID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poFwInfo->oAMDeviceIdentification.wDeviceID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poFwInfo->oAMDeviceIdentification.wVendorID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poFwInfo->oAMDeviceIdentification.wOrganization = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            poFwInfo->wAMTypeIdentification = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            break;
        }
        case AM_OnlyHardwareInformation:
        {
            T_AM_HARDWAREONLYINFORMATION *poHwInfo;
            poAMBlock->oAMInfo.poAmHardwareOnlyInformation = (T_AM_HARDWAREONLYINFORMATION *)calloc(1, sizeof(T_AM_HARDWAREONLYINFORMATION));
            poHwInfo = poAMBlock->oAMInfo.poAmHardwareOnlyInformation;
            if(poHwInfo == NULL)
            {
                return 0;
            }

            FillGUID(&poHwInfo->oIMUniqueIdentifier, pbyBuffer + dwIndex, &dwIndex);

            memcpy(poHwInfo->pbyAMLocation, pbyBuffer + dwIndex, AM_LOCATION_SIZE);
            dwIndex += AM_LOCATION_SIZE;

            memcpy(poHwInfo->pbyIMAnnotation, pbyBuffer + dwIndex, IM_ANNOTATION_SIZE);
            dwIndex += IM_ANNOTATION_SIZE;

            memcpy(poHwInfo->pbyIMOrderID, pbyBuffer + dwIndex, IM_ORDERID_SIZE);
            dwIndex += IM_ORDERID_SIZE;

            memcpy(poHwInfo->pbyAMHardwareRevision, pbyBuffer + dwIndex, AM_HARDWARE_REVISION_SIZE);
            dwIndex += AM_HARDWARE_REVISION_SIZE;

            memcpy(poHwInfo->pbyIMSerialNumber, pbyBuffer + dwIndex, IM_SERIALNUMBER_SIZE);
            dwIndex += IM_SERIALNUMBER_SIZE;

            poHwInfo->oAMDeviceIdentification.wDeviceSubID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poHwInfo->oAMDeviceIdentification.wDeviceID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poHwInfo->oAMDeviceIdentification.wVendorID = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
            poHwInfo->oAMDeviceIdentification.wOrganization = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            poHwInfo->wAMTypeIdentification = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            poHwInfo->wIMHardwareRevision = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

            break;
        }
        default:
        {
            return 0;
        }
    }
	SkipPadding32(&dwIndex);

    return dwIndex;
}

/*!
 * \brief           This function converts a byte stream into the T_ASSET_MANAGEMENT_DATA structure.
 * \param[in]       pbyBuffer pointer to a byte buffer where the structure is saved to
 * \param[in,out]   poData pointer to the T_ASSET_MANAGEMENT_DATA structure. 
 * \retval          USIGN32 size in bytes of the converted data
 */
USIGN32 AM_AssetDataDecode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_DATA *poData)
{
    USIGN32 dwIndex = 0;
    USIGN32 i;
    T_ASSET_MANAGEMENT_INFO *poAMInfo;

    if((pbyBuffer == NULL) || (poData==NULL))
    {
        return 0;
    }

    BlockHeaderDecode(&poData->oBlockHeader, pbyBuffer, &dwIndex);

    poAMInfo = &poData->oAssetManagementInfo;
    poAMInfo->wNumberOfEntries = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);
    poAMInfo->poAssetManagementBlocks = (T_ASSET_MANAGEMENT_BLOCK *)calloc(1, sizeof(T_ASSET_MANAGEMENT_BLOCK)*poAMInfo->wNumberOfEntries);
    if(poAMInfo->poAssetManagementBlocks == NULL)
    {
        return 0;
    }

    for(i = 0; i < poAMInfo->wNumberOfEntries; i++)
    {
        dwIndex += AM_BlockDecode(pbyBuffer + dwIndex, &poAMInfo->poAssetManagementBlocks[i]);
    }
    return dwIndex;
}

/*!
 * \brief           This function converts information of the T_ASSET_MANAGEMENT_BLOCK into a byte stream.	
 * \param[in,out]   pbyBuffer a pointer to a byte buffer that is big enough to hold the data of the T_ASSET_MANAGEMENT_BLOCK structure.
 * \param[in]       poAMBlock pointer to the T_ASSET_MANAGEMENT_BLOCK structure.
 * \retval          USIGN32 size in bytes of the converted data
 */
USIGN32 AM_AssetBlockEncode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_BLOCK *poAMBlock)
{
    USIGN32 dwIndex = 0;
    USIGN16 wDataWord;
    USIGN8  byPadding;

    if((poAMBlock == NULL) || (pbyBuffer == NULL))
    {
        return 0;
    }
    SetBlockHeader(pbyBuffer, (USIGN16*)&dwIndex, poAMBlock->oBlockHeader.oBlockType, 1, 0);

    byPadding = (sizeof(USIGN32) - (dwIndex % sizeof(USIGN32))) % sizeof(USIGN32);
    memset((pbyBuffer + dwIndex), 0, byPadding);
    dwIndex += byPadding;

    switch(poAMBlock->oBlockHeader.oBlockType)
    {
        case AM_FullInformation:
        {
            T_AM_FULLINFORMATION *poFullInfo = poAMBlock->oAMInfo.poAmFullInformation;
            if(poFullInfo == NULL)
            {
                return 0;
            }
            dwIndex += (USIGN16)SetUUID(pbyBuffer + dwIndex, &poFullInfo->oIMUniqueIdentifier);

            memcpy(pbyBuffer + dwIndex, poFullInfo->pbyAMLocation, AM_LOCATION_SIZE);
            dwIndex += AM_LOCATION_SIZE;

            memcpy(pbyBuffer + dwIndex, poFullInfo->pbyIMAnnotation, IM_ANNOTATION_SIZE);
            dwIndex += IM_ANNOTATION_SIZE;

            memcpy(pbyBuffer + dwIndex, poFullInfo->pbyIMOrderID, IM_ORDERID_SIZE);
            dwIndex += IM_ORDERID_SIZE;

            memcpy(pbyBuffer + dwIndex, poFullInfo->pbyAMSoftwareRevision, AM_SOFTWARE_REVISION_SIZE);
            dwIndex += AM_SOFTWARE_REVISION_SIZE;

            memcpy(pbyBuffer + dwIndex, poFullInfo->pbyAMHardwareRevision, AM_HARDWARE_REVISION_SIZE);
            dwIndex += AM_HARDWARE_REVISION_SIZE;

            memcpy(pbyBuffer + dwIndex, poFullInfo->pbyIMSerialNumber, IM_SERIALNUMBER_SIZE);
            dwIndex += IM_SERIALNUMBER_SIZE;

            memcpy(pbyBuffer + dwIndex, &poFullInfo->oIMSWRevision, sizeof(T_IM_SW_REVISION));
            dwIndex += sizeof(T_IM_SW_REVISION);

            wDataWord = TPS_htons(poFullInfo->oAMDeviceIdentification.wDeviceSubID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poFullInfo->oAMDeviceIdentification.wDeviceID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poFullInfo->oAMDeviceIdentification.wVendorID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poFullInfo->oAMDeviceIdentification.wOrganization);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;

            wDataWord = TPS_htons(poFullInfo->wAMTypeIdentification);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;

            wDataWord = TPS_htons(poFullInfo->wIMHardwareRevision);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            break;
        }
        case AM_OnlyFirmwareInformation:
        {
            T_AM_FIRMWAREONLYINFORMATION *poFwInfo = poAMBlock->oAMInfo.poAmFirmwareOnlyInformation;
            if(poFwInfo == NULL)
            {
                return 0;
            }
            dwIndex += (USIGN16)SetUUID(pbyBuffer + dwIndex, &poFwInfo->oIMUniqueIdentifier);

            memcpy(pbyBuffer + dwIndex, poFwInfo->pbyAMLocation, AM_LOCATION_SIZE);
            dwIndex += AM_LOCATION_SIZE;

            memcpy(pbyBuffer + dwIndex, poFwInfo->pbyIMAnnotation, IM_ANNOTATION_SIZE);
            dwIndex += IM_ANNOTATION_SIZE;

            memcpy(pbyBuffer + dwIndex, poFwInfo->pbyIMOrderID, IM_ORDERID_SIZE);
            dwIndex += IM_ORDERID_SIZE;

            memcpy(pbyBuffer + dwIndex, poFwInfo->pbyAMSoftwareRevision, AM_SOFTWARE_REVISION_SIZE);
            dwIndex += AM_SOFTWARE_REVISION_SIZE;

            memcpy(pbyBuffer + dwIndex, poFwInfo->pbyIMSerialNumber, IM_SERIALNUMBER_SIZE);
            dwIndex += IM_SERIALNUMBER_SIZE;

            memcpy(pbyBuffer + dwIndex, &poFwInfo->oIMSWRevision, sizeof(T_IM_SW_REVISION));
            dwIndex += sizeof(T_IM_SW_REVISION);

            wDataWord = TPS_htons(poFwInfo->oAMDeviceIdentification.wDeviceSubID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poFwInfo->oAMDeviceIdentification.wDeviceID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poFwInfo->oAMDeviceIdentification.wVendorID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poFwInfo->oAMDeviceIdentification.wOrganization);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;

            wDataWord = TPS_htons(poFwInfo->wAMTypeIdentification);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;

            break;
        }
        case AM_OnlyHardwareInformation:
        {
            T_AM_HARDWAREONLYINFORMATION *poHwInfo = poAMBlock->oAMInfo.poAmHardwareOnlyInformation;
            if(poHwInfo == NULL)
            {
                return 0;
            }
            dwIndex += (USIGN16)SetUUID(pbyBuffer + dwIndex, &poHwInfo->oIMUniqueIdentifier);

            memcpy(pbyBuffer + dwIndex, poHwInfo->pbyAMLocation, AM_LOCATION_SIZE);
            dwIndex += AM_LOCATION_SIZE;

            memcpy(pbyBuffer + dwIndex, poHwInfo->pbyIMAnnotation, IM_ANNOTATION_SIZE);
            dwIndex += IM_ANNOTATION_SIZE;

            memcpy(pbyBuffer + dwIndex, poHwInfo->pbyIMOrderID, IM_ORDERID_SIZE);
            dwIndex += IM_ORDERID_SIZE;

            memcpy(pbyBuffer + dwIndex, poHwInfo->pbyAMHardwareRevision, AM_HARDWARE_REVISION_SIZE);
            dwIndex += AM_HARDWARE_REVISION_SIZE;

            memcpy(pbyBuffer + dwIndex, poHwInfo->pbyIMSerialNumber, IM_SERIALNUMBER_SIZE);
            dwIndex += IM_SERIALNUMBER_SIZE;

            wDataWord = TPS_htons(poHwInfo->oAMDeviceIdentification.wDeviceSubID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poHwInfo->oAMDeviceIdentification.wDeviceID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poHwInfo->oAMDeviceIdentification.wVendorID);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            wDataWord = TPS_htons(poHwInfo->oAMDeviceIdentification.wOrganization);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;

            wDataWord = TPS_htons(poHwInfo->wAMTypeIdentification);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;

            wDataWord = TPS_htons(poHwInfo->wIMHardwareRevision);
            memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
            dwIndex += 2;
            break;
        }
        default:
        {
            return 0;
        }
    }

    byPadding = 1;
    while(byPadding)
    {
        byPadding = (sizeof(USIGN32) - (dwIndex % sizeof(USIGN32))) % sizeof(USIGN32);
        memset((pbyBuffer + dwIndex), 0, byPadding);
        dwIndex += byPadding;
    }

    SetBlockLength(pbyBuffer, (USIGN16)dwIndex);
    return dwIndex;
}

/*!
 * \brief           This function converts information of the T_ASSET_MANAGEMENT_DATA into a byte stream.
 * \param[in,out]   pbyBuffer a pointer to a byte buffer that is big enough to hold the data of the T_ASSET_MANAGEMENT_BLOCK structure.
 *                  The function AM_GetAssetDataSize() can be used to determine the size of the buffer without padding.
 * \param[in]       poAMData pointer to the T_ASSET_MANAGEMENT_Data structure. 
 * \retval          USIGN32 size in bytes of the converted data
 */
USIGN32 AM_AssetDataEncode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_DATA *poAMData)
{
    USIGN32 dwIndex = 0;
    USIGN16 i;
    USIGN16 wDataWord;

    if((poAMData == NULL) || (pbyBuffer == NULL))
    {
        return 0;
    }

    SetBlockHeader(pbyBuffer, (USIGN16*)&dwIndex, AssetManagementData, 1, 0);

    wDataWord = TPS_htons(poAMData->oAssetManagementInfo.wNumberOfEntries);
    memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
    dwIndex += 2;

    for(i = 0; i < poAMData->oAssetManagementInfo.wNumberOfEntries; i++)
    {
        T_ASSET_MANAGEMENT_BLOCK *poAMBlock = &poAMData->oAssetManagementInfo.poAssetManagementBlocks[i];
        dwIndex += AM_AssetBlockEncode(pbyBuffer + dwIndex, poAMBlock);
    }

    SetBlockLength(pbyBuffer, (USIGN16)dwIndex);

    return dwIndex;
}


/*!
 * \brief       This function returns the AM_LOCATION.structure out of the pbyAMLocation 16byte buffer.	
 * \param[in]   pbyAMLocation pointer to a 16byte sized buffer
 * \retval      USIGN8  AM_LOCATION structure format: 1 = twelve level tree format, 2 = slot-/subslot number format
 */
USIGN8 AM_GetAMLocationStructure(const USIGN8 *pbyAMLocation)
{
    USIGN8 byFormat = 0;
    switchAMLocation((USIGN8*)pbyAMLocation);
    byFormat = pbyAMLocation[AM_LOCATION_LEVEL_STRUCTURE];
    switchAMLocation((USIGN8*)pbyAMLocation);
    return byFormat;
}

/*!
* \brief       This function sets the AM_LOCATION.structure in the pbyAMLocation 16byte buffer.
* \param[in]   pbyAMLocation pointer to a 16byte sized buffer
* \param[in]   byStructure new structure format. Also see AM_GetAMLocationStructure().
* \retval      VOID
*/
VOID AM_SetAMLocationStructure(USIGN8 *pbyAMLocation, USIGN8 byStructure)
{
   switchAMLocation(pbyAMLocation);
   pbyAMLocation[AM_LOCATION_LEVEL_STRUCTURE] = byStructure;
   switchAMLocation(pbyAMLocation);
}

/*!
 * \brief This function returns the value specified by the location level in the 16 byte buffer of AM_Location	
 * \param[in]  pbyAMLocation Pointer to the 16 byte buffer holding all bits
 * \param[in]  byLevel The location level (bit position) where the value has to be read of
 * \retval     USIGN16 16 bit value for each level.
 */
USIGN16 AM_GetAMLocationLevel(const USIGN8 *pbyAMLocation, const USIGN8 byLevel)
{
    USIGN8 i,fact=0;
    USIGN16 wValue=0;
    USIGN8 bitField[128];
    USIGN8 byFormat;

    switchAMLocation((USIGN8*)pbyAMLocation);
    if(byLevel == AM_LOCATION_LEVEL_STRUCTURE)
    {
        byFormat = pbyAMLocation[AM_LOCATION_LEVEL_STRUCTURE];
        switchAMLocation((USIGN8*)pbyAMLocation);
        return byFormat;
    }

    if(byLevel > AM_LOCATION_LEVEL11)
    {
        return 0;
    }
    for(i = 0; i < 16 * 8; i++)
    {
        bitField[i] = GET_BIT(pbyAMLocation, i);
    }
    for(i = byLevel; i < byLevel + 10; i++)
    {
        wValue += bitField[i] << fact;
        fact++;
    }
    switchAMLocation((USIGN8*)pbyAMLocation);
    return wValue;
}

/*!
 * \brief This function sets the desired bits in the AM_Location 16 byte buffer.	
 * \param[in]  pbyAMLocation Pointer to the 16 byte buffer holding all bits
 * \param[in]  byLevel       The location level where the new value has to be saved
 * \param[in]  wValue        The new value. Only the first 10 bits are saved
 * \retval     VOID
 */
VOID AM_SetAMLocationLevel(USIGN8 *pbyAMLocation, const USIGN8 byLevel, USIGN16 wValue)
{
    USIGN8 i,t,o;
    USIGN8 bitField[128];
    USIGN8 valueBits[10];

    switchAMLocation(pbyAMLocation);
    if(byLevel == AM_LOCATION_LEVEL_STRUCTURE)
    {
        pbyAMLocation[AM_LOCATION_LEVEL_STRUCTURE] = (USIGN8)(wValue & 0xFF);
        switchAMLocation(pbyAMLocation);
        return;
    }
    if(byLevel > AM_LOCATION_LEVEL11)
    {
        switchAMLocation(pbyAMLocation);
        return;
    }
    pbyAMLocation[AM_LOCATION_LEVEL_STRUCTURE] = AM_STRUCTURE_TREE_FORMAT;

    for(i = 0; i < 16 * 8; i++)
    {
        bitField[i] = GET_BIT(pbyAMLocation, i);
    }
    o = 0;
    for(i = 0; i<10; i++)
    {
        valueBits[o] = GET_BIT(&wValue, i);
        o++;
    }

    o = 0;
    for(i = byLevel; i < byLevel + 10; i++)
    {
        bitField[i] = valueBits[o];
        o++;
    }

    o = 0;
    for (i=0;i<16;i++)
    {
        pbyAMLocation[i] = 0;
        for(t = 0; t < 8; t++)
        {
            pbyAMLocation[i] += bitField[o] << t;
            o++;
        }
    }
    switchAMLocation(pbyAMLocation);
}

VOID AM_SetAMLocationSlotFormat(USIGN8 *pbyAMLocation, T_AM_LOCATION_SLOT_FORMAT *poSlotFormat)
{
    if((pbyAMLocation == NULL) || (poSlotFormat == NULL))
    {
        return;
    }
    memcpy(pbyAMLocation, poSlotFormat, AM_LOCATION_SIZE);
    pbyAMLocation[AM_LOCATION_LEVEL_STRUCTURE] = AM_STRUCTURE_SLOT_FORMAT;
    switchAMLocation(pbyAMLocation);
}

VOID AM_GetAMLocationSlotFormat(USIGN8 *pbyAMLocation, T_AM_LOCATION_SLOT_FORMAT *poSlotFormat)
{
    USIGN8 *pbyBuf;

    if((pbyAMLocation == NULL) || (poSlotFormat == NULL))
    {
        return;
    }
    switchAMLocation(pbyAMLocation);
    pbyBuf = (USIGN8 *)poSlotFormat;
    memcpy(pbyBuf, pbyAMLocation, AM_LOCATION_SIZE);
    switchAMLocation(pbyAMLocation);
}

/*!
 * \brief       This function returns the actual data size of the asset management data with all asset management blocks. But without padding information!
 *              Use this function to get the minimum amount of data for the asset management record and add additional space for padding information
 * \param[in]   poData Pointer to the asset management data
 * \retval	    USIGN32 size of the complete structure without padding.
 */
USIGN32 AM_GetAssetDataSize(T_ASSET_MANAGEMENT_DATA *poData)
{
	USIGN16 i;
	USIGN32 dwSize;

	dwSize = sizeof(T_BLOCKHEADER);
    dwSize += 2;  /* wNumberOfEntries */

	for(i = 0; i < poData->oAssetManagementInfo.wNumberOfEntries; i++)
	{
        dwSize += sizeof(T_BLOCKHEADER);
		switch(poData->oAssetManagementInfo.poAssetManagementBlocks[i].oBlockHeader.oBlockType)
		{
			case AM_FullInformation:
			{
				dwSize += sizeof(T_AM_FULLINFORMATION);
				break;
			}
			case AM_OnlyFirmwareInformation:
			{
				dwSize += sizeof(T_AM_FIRMWAREONLYINFORMATION);
				break;
			}
			case AM_OnlyHardwareInformation:
			{
				dwSize += sizeof(T_AM_HARDWAREONLYINFORMATION);
				break;
			}
			default:
			break;
		}
	}
	return dwSize;
}

/*!
 * \brief       This function returns the actual data size of the I&M5 data with all sub blocks. 
 *              Use this function to get the minimum amount of data for the I&M5 record.
 * \param[in]   poIM5 Pointer to the T_IM5 structure
 * \retval	    USIGN32 size of the complete structure.
 */
USIGN32 AM_GetIM5DataSize(T_IM5 *poIM5)
{
    USIGN16 i;
    USIGN32 dwSize;

    dwSize = sizeof(T_IM5);
    for(i = 0; i < poIM5->wNumberOfEntries; i++)
    {
        if(poIM5->poIM5Block[i].poIM5Data != NULL)
        {
            dwSize += sizeof(T_IM5_DATA);
        }
        else if(poIM5->poIM5Block[i].poAMBlock != NULL)
        {
            T_ASSET_MANAGEMENT_BLOCK *poAMBlock = poIM5->poIM5Block[i].poAMBlock;
            dwSize += sizeof(T_BLOCKHEADER);
            switch(poAMBlock->oBlockHeader.oBlockType)
            {
                case AM_FullInformation:
                {
                    dwSize += sizeof(T_AM_FULLINFORMATION);
                    break;
                }
                case AM_OnlyFirmwareInformation:
                {
                    dwSize += sizeof(T_AM_FIRMWAREONLYINFORMATION);
                    break;
                }
                case AM_OnlyHardwareInformation:
                {
                    dwSize += sizeof(T_AM_HARDWAREONLYINFORMATION);
                    break;
                }
                default:
                break;
            }
        }
    }
    return dwSize;
}

/*!
 * \brief      This function frees all memory blocks in the asset management data structure.
 * \param[in]  pointer to the asset management data structure
 * \retval     VOID
 */
VOID AM_AssetDataFree(T_ASSET_MANAGEMENT_DATA *poData)
{
    USIGN16 i;

    if(poData==NULL)
    {
        return;
    }
    for(i = 0; i < poData->oAssetManagementInfo.wNumberOfEntries; i++)
    {
        AM_AssetBlockFree(&poData->oAssetManagementInfo.poAssetManagementBlocks[i]);
    }
    free(poData->oAssetManagementInfo.poAssetManagementBlocks);
    poData->oAssetManagementInfo.poAssetManagementBlocks = NULL;

    poData->oAssetManagementInfo.wNumberOfEntries = 0;
}


/*!
 * \brief      This function frees all memory blocks in the T_IM5 structure.
 * \param[in]  poIM5 pointer to the I&M5 structure.
 * \retval     VOID
 */
VOID AM_IM5DataFree(T_IM5 *poIM5)
{
    USIGN16 i;

    if(poIM5 == NULL)
    {
        return;
    }
    for (i=0;i<poIM5->wNumberOfEntries;i++)
    {
        if(poIM5->poIM5Block[i].poIM5Data != NULL)
        {
            free(poIM5->poIM5Block[i].poIM5Data);
            poIM5->poIM5Block[i].poIM5Data = NULL;
        }
        else if(poIM5->poIM5Block[i].poAMBlock != NULL)
        {
            AM_AssetBlockFree(poIM5->poIM5Block[i].poAMBlock);
            poIM5->poIM5Block[i].poAMBlock = NULL;
        }
    }
    poIM5->wNumberOfEntries = 0;
}


/*!
 * \brief           This function converts a byte stream into the T_IM5_DATA structure.
 * \param[in]       pbyBuffer pointer to a byte buffer where the structure is saved to
 * \param[in,out]   poIMData pointer to the T_IM5_DATA structure.
 * \retval          USIGN32 size in bytes of the converted data. In case of an error this function returns 0.
 */
USIGN32 AM_IM5DataDecode(USIGN8 *pbyBuffer, T_IM5_DATA *poIMData)
{
    USIGN32 dwIndex = 0;

    if((pbyBuffer == NULL) || (poIMData==NULL))
    {
        return 0;
    }

    BlockHeaderDecode(&poIMData->oBlockHeader, pbyBuffer, &dwIndex);

    if(poIMData->oBlockHeader.oBlockType != IM5Data)
    {
        return 0;
    }

    memcpy(poIMData->pbyIMAnnotation, pbyBuffer + dwIndex, IM_ANNOTATION_SIZE);
    dwIndex += IM_ANNOTATION_SIZE;

    memcpy(poIMData->pbyIMOrderID, pbyBuffer + dwIndex, IM_ORDERID_SIZE);
    dwIndex += IM_ORDERID_SIZE;

    poIMData->byVendorIDHigh = pbyBuffer[dwIndex];
    dwIndex++;
    poIMData->byVendorIDLow = pbyBuffer[dwIndex];
    dwIndex++;

    memcpy(poIMData->pbyIMSerialNumber, pbyBuffer + dwIndex, IM_SERIALNUMBER_SIZE);
    dwIndex += IM_SERIALNUMBER_SIZE;

    poIMData->wIMHardwareRevision = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

    memcpy(&poIMData->oIMSWRevision, pbyBuffer + dwIndex, sizeof(T_IM_SW_REVISION));
    dwIndex += sizeof(T_IM_SW_REVISION);

    return dwIndex;
}

/*!
 * \brief           This function converts a byte stream into the T_IM5 structure.
 * \param[in]       pbyBuffer pointer to a byte buffer where the structure is saved to
 * \param[in,out]   poIM5 pointer to the T_IM5 structure. 
 * \retval          USIGN32 size in bytes of the converted data. In case of an error this function returns 0.
 */
USIGN32 AM_IM5Decode(USIGN8 *pbyBuffer, T_IM5 *poIM5)
{
    USIGN32 dwIndex = 0;
    USIGN16 i;

    if((pbyBuffer == NULL) || (poIM5 == NULL))
    {
        return 0;
    }

    BlockHeaderDecode(&poIM5->oBlockHeader, pbyBuffer, &dwIndex);
    poIM5->wNumberOfEntries = (USIGN16)GetUInt(pbyBuffer, 2, &dwIndex);

    poIM5->poIM5Block = (T_IM5_BLOCK*)calloc(1, sizeof(T_IM5_BLOCK)*poIM5->wNumberOfEntries);
    if(poIM5->poIM5Block == NULL)
    {
        return 0;
    }

    for (i=0;i<poIM5->wNumberOfEntries;i++)
    {
        USIGN32 idx = 0;
        poIM5->poIM5Block[i].poIM5Data = (T_IM5_DATA *)calloc(1, sizeof(T_IM5_DATA));
        if(poIM5->poIM5Block[i].poIM5Data == NULL)
        {
            return 0;
        }

        idx = AM_IM5DataDecode(pbyBuffer + dwIndex, poIM5->poIM5Block[i].poIM5Data);
        if(idx == 0)
        {
            free(poIM5->poIM5Block->poIM5Data);
            poIM5->poIM5Block->poIM5Data = NULL;

            poIM5->poIM5Block[i].poAMBlock = (T_ASSET_MANAGEMENT_BLOCK *)calloc(1, sizeof(T_ASSET_MANAGEMENT_BLOCK));
            idx = AM_BlockDecode(pbyBuffer + dwIndex, poIM5->poIM5Block[i].poAMBlock);
            if(idx == 0)
            {
                AM_AssetBlockFree(poIM5->poIM5Block[i].poAMBlock);
                poIM5->poIM5Block[i].poAMBlock = NULL;
            }
        }
        dwIndex += idx;
    }
    return dwIndex;
}

/*!
 * \brief           This function converts information of the T_IM5_DATA into a byte stream.
 * \param[in,out]   pbyBuffer a pointer to a byte buffer that is big enough to hold the data of the T_IM5_DATA structure.
 *                  The function AM_GetIM5DataSize() can be used to determine the size of the buffer without padding.
 * \param[in]       poIM5Data pointer to the T_IM5_DATA structure.
 * \retval          USIGN32 size in bytes of the converted data. In case of an error this function returns 0.
 */
USIGN32 AM_IM5DataEncode(USIGN8 *pbyBuffer, T_IM5_DATA *poIM5Data)
{
    USIGN32 dwIndex = 0;
    USIGN16 wDataWord;

    if((pbyBuffer == NULL) || (poIM5Data == NULL))
    {
        return 0;
    }

    SetBlockHeader(pbyBuffer, (USIGN16*)&dwIndex, IM5Data, 1, 0);

    memcpy(pbyBuffer + dwIndex, poIM5Data->pbyIMAnnotation, IM_ANNOTATION_SIZE);
    dwIndex += IM_ANNOTATION_SIZE;

    memcpy(pbyBuffer + dwIndex, poIM5Data->pbyIMOrderID, IM_ORDERID_SIZE);
    dwIndex += IM_ORDERID_SIZE;

    pbyBuffer[dwIndex] = poIM5Data->byVendorIDHigh;
    dwIndex++;
    pbyBuffer[dwIndex] = poIM5Data->byVendorIDLow;
    dwIndex++;

    memcpy(pbyBuffer + dwIndex, poIM5Data->pbyIMSerialNumber, IM_SERIALNUMBER_SIZE);
    dwIndex += IM_SERIALNUMBER_SIZE;

    wDataWord = TPS_htons(poIM5Data->wIMHardwareRevision);
    memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
    dwIndex += 2;

    memcpy(pbyBuffer + dwIndex, &poIM5Data->oIMSWRevision, sizeof(T_IM_SW_REVISION));
    dwIndex += sizeof(T_IM_SW_REVISION);

    SetBlockLength(pbyBuffer, (USIGN16)dwIndex);

    return dwIndex;
}

/*!
 * \brief           This function converts information of the T_IM5_BLOCK into a byte stream.
 * \param[in,out]   pbyBuffer a pointer to a byte buffer that is big enough to hold the data of the T_IM5_BLOCK structure.
 * \param[in]       poIM5Block pointer to the T_IM5_BLOCK structure.
 * \retval          USIGN32 size in bytes of the converted data. In case of an error this function returns 0.
 */
USIGN32 AM_EncodeIM5Block(USIGN8 *pbyBuffer, T_IM5_BLOCK *poIM5Block)
{
    if((pbyBuffer == NULL) || (poIM5Block == NULL))
    {
        return 0;
    }

    if(poIM5Block->poIM5Data != NULL)
    {
        return AM_IM5DataEncode(pbyBuffer, poIM5Block->poIM5Data);
    }
    if(poIM5Block->poAMBlock != NULL)
    {
        return AM_AssetBlockEncode(pbyBuffer, poIM5Block->poAMBlock);
    }

    return 0;
}

/*!
 * \brief           This function converts information of the T_IM5 into a byte stream.
 * \param[in,out]   pbyBuffer a pointer to a byte buffer that is big enough to hold the data of the T_IM5 structure.
 * \param[in]       poIM5 pointer to the T_IM5 structure.
 * \retval          USIGN32 size in bytes of the converted data. In case of an error this function returns 0.
 */
USIGN32 AM_IM5Encode(USIGN8 *pbyBuffer, T_IM5 *poIM5)
{
    USIGN32 dwIndex = 0;
    USIGN16 wDataWord;
    USIGN16 i;

    if((poIM5 == NULL) || (pbyBuffer == NULL))
    {
        return 0;
    }

    SetBlockHeader(pbyBuffer, (USIGN16*)&dwIndex, IM5, 1, 0);

    wDataWord = TPS_htons(poIM5->wNumberOfEntries);
    memcpy(pbyBuffer + dwIndex, &wDataWord, 2);
    dwIndex += 2;

    for(i = 0; i < poIM5->wNumberOfEntries; i++)
    {
        dwIndex += AM_EncodeIM5Block(pbyBuffer + dwIndex, &poIM5->poIM5Block[i]);
    }

    SetBlockLength(pbyBuffer, (USIGN16)dwIndex);

    return dwIndex;
}

/*!@} Asset Management Interface*/
