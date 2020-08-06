/******************************************************************************
*
*  Copyright PHOENIX CONTACT Software GmbH
*
*****************************************************************************/

#ifndef H_ASSET_MGM
#define H_ASSET_MGM

#define AM_LOCATION_SIZE                16
#define IM_ANNOTATION_SIZE              64
#define IM_ORDERID_SIZE                 64
#define AM_SOFTWARE_REVISION_SIZE       64
#define AM_HARDWARE_REVISION_SIZE       64
#define IM_SERIALNUMBER_SIZE            16

#define AM_STRUCTURE_TREE_FORMAT        1
#define AM_STRUCTURE_SLOT_FORMAT        2
#define AM_LOCATION_LEVEL_STRUCTURE     0
#define AM_LOCATION_LEVEL0              8
#define AM_LOCATION_LEVEL1              18
#define AM_LOCATION_LEVEL2              28
#define AM_LOCATION_LEVEL3              38
#define AM_LOCATION_LEVEL4              48
#define AM_LOCATION_LEVEL5              58
#define AM_LOCATION_LEVEL6              68
#define AM_LOCATION_LEVEL7              78
#define AM_LOCATION_LEVEL8              88
#define AM_LOCATION_LEVEL9              98
#define AM_LOCATION_LEVEL10             108
#define AM_LOCATION_LEVEL11             118


typedef enum t_blocktype
{
    IM5                         = 0x0025,
    IM5Data                     = 0x0034,
    AssetManagementData         = 0x0035,
    AM_FullInformation          = 0x0036,
    AM_OnlyHardwareInformation  = 0x0037,
    AM_OnlyFirmwareInformation  = 0x0038
}T_BLOCKTYPE;


/*! This structure stores a universal unique identifier (UUID). UUIDs uniquely identify objects such as interfaces, manager entry-point vectors and class objects. */
typedef struct t_uuid
{
    USIGN32  dwData1;        /*!< The field contains the first eight hexadecimal digits. */
    USIGN16 wData2;         /*!< The field contains the first group of four hexadecimal digits. */
    USIGN16 wData3;         /*!< The field contains the second group of four hexadecimal digits. */
    USIGN8  pbyData4[8];    /*!< This field contains an array of eight bytes.
                                 The first two bytes contain the third group of four hexadecimal digits. The remaining six bytes contain the final 12 hexadecimal digits.
                             */
} T_UUID;

/*! This substitution contains general information about the block. */
typedef struct t_blockheader
{
    T_BLOCKTYPE    oBlockType;       /*! This enumeration lists the blocks with their appropriate block type numbers. */
    USIGN16 wBlockLength;            /*!< This field contains the number of octets of the Record Response without counting the fields BlockType and BlockLength. */
    USIGN8  byBlockVersionHigh;      /*!< This field contains the first number of the block version.
                                        Allowed values:
                                        Value   |   Meaning
                                        -|-
                                        0x00        |   Reserved
                                        0x01        |   Version 1
                                        0x02 - 0xFF |   Reserved
                                    */
    USIGN8 byBlockVersionLow;       /*!< This field contains the last number of the block version.
                                        Allowed values:
                                        Value   |   Meaning
                                        -|-
                                        0x00        |   Version 0
                                        0x01        |   Version 1
                                        0x02 - 0xFF |   Version 2 to version 255
                                    */
}T_BLOCKHEADER;

typedef struct t_am_location_slot_format
{
    USIGN16 wStructure;             /*!< the lower 8bits represents AM_Location.Structure. The 8 high bits are reserved! */
    USIGN16 wBeginSlotNumber;       /*!< the slot number at which the reported asset starts */
    USIGN16 wBeginSubSlotNumber;    /*!< the sub slot number at which the reported asset starts*/
    USIGN16 wEndSlotNumber;         /*!< the slot number at which the reported asset ends */
    USIGN16 wEndSubSlotNumber;      /*!< the sub slot number at which the reported asset ends*/
    USIGN16 wReserved2;
    USIGN16 wReserved3;
    USIGN16 wReserved4;
}T_AM_LOCATION_SLOT_FORMAT;

typedef struct t_im_sw_revision
{
    USIGN8  bySWRevisionPrefix;     /*!< the value shall be set manufacturer specific using the character:
                                        - 'V' for an officially released version
                                        - 'R_ for Revision
                                        - 'P' for Prototype
                                        - 'U' for Under Test (Field Test)
                                        - 'T' for Test Device*/
    USIGN8  bySWRevisionFunctionalEnhancement;
    USIGN8  bySWRevisionBugFix;
    USIGN8  bySWRevisionInternalChange;
}T_IM_SW_REVISION;

typedef struct t_am_device_identification
{
    USIGN16     wDeviceSubID;       /*!< allowed values 0x0001 - 0xFFFF */
    USIGN16     wDeviceID;          /*!< allowed values 0x0000 - 0xFFFF */
    USIGN16     wVendorID;          /*!< allowed values 0x0000 - 0xFFFF */
    USIGN16     wOrganization;      /*!< allowed values:
                                        - 0x0000            : for this document
                                        - 0x0001 - 0x00FF   : reserved
                                        - 0x0100            : IEC 61131-9
                                        - 0x0101            : IEC 61158 Type 3
                                        - 0x0102            : IEC 61158 Type 6
                                        - 0x0103            : IEC 62026-2
                                        - 0x0104            : IEC 61158 Type 9
                                        - 0x0105            : IEC 61158 Type 2
                                        - 0x0106            : IEC 61158 Type 1
                                        - 0x0107            : EN 50325-4
                                        - 0x0108            : IEC 61158 Type 8
                                        - 0x0109 - 0xFFFF   : reserved */
}T_AM_DEVICE_IDENTIFICATION;

/*!< This structure hold all information hardware and software. T_AM_FIRMWAREONLYINFORMATION and T_AM_HARDWAREONLYINFORMATION are subsets of this structure.*/
typedef struct t_am_fullinformation
{
    T_UUID                      oIMUniqueIdentifier;                                /*!< manufacturer defined UUID. 0=reserved */
    USIGN8                      pbyAMLocation[AM_LOCATION_SIZE];                    /*!< 16byte value holding location information.
                                                                                         the byte0 defines the structure format: 1 = twelve level tree format
                                                                                                                                 2 = slot-/subslot number format
                                                                                         Twelve level tree format: byte 1-15 are 10bit coded and hold the level0-11 information
                                                                                         slot-/subslot number format: cast the T_AM_LOCATION_SLOT_FORMAT structure over this 16byte buffer.*/
    USIGN8                      pbyIMAnnotation[IM_ANNOTATION_SIZE];                /*!< manufacturer defined. All data shall be left justified. If the text is shorter than the defined 
                                                                                         string length, the gap shall be filled with blanks.*/
    USIGN8                      pbyIMOrderID[IM_ORDERID_SIZE];                      /*!< manufacturer defined. All data shall be left justified. If the text is shorter than the defined 
                                                                                         string length, the gap shall be filled with blanks.*/
    USIGN8                      pbyAMSoftwareRevision[AM_SOFTWARE_REVISION_SIZE];   /*!< manufacturer defined software revision. 
                                                                                         The support of T_IM_SW_REVISION is preferred 
                                                                                         and if supported then this field shall be filled with blanks.*/
    USIGN8                      pbyAMHardwareRevision[AM_HARDWARE_REVISION_SIZE];   /*!< manufacturer defined hardware revision.
                                                                                         The support of wIMHardwareRevision is preferred
                                                                                         and if supported then this field shall be filled with blanks.*/
    USIGN8                      pbyIMSerialNumber[IM_SERIALNUMBER_SIZE];            /*!< manufacturer defined serial number */
    T_IM_SW_REVISION            oIMSWRevision;                                      /*!< manufacturer defined software revision */
    T_AM_DEVICE_IDENTIFICATION  oAMDeviceIdentification;                            /*!< see T_AM_DEVICE_IDENTIFICATION structure*/
    USIGN16                     wAMTypeIdentification;                              /*!< defintion is like IM_Profile_Specific_Type :
                                                                                        - 0x0000            : unspecified
                                                                                        - 0x0001            : standard controller(PLC)
                                                                                        - 0x0002            : PC based station
                                                                                        - 0x0003            : IO module/submodule
                                                                                        - 0x0004            : communication module/submodule
                                                                                        - 0x0005            : interface module/submodule
                                                                                        - 0x0006            : active network infrastructure component
                                                                                        - 0x0007            : media attachment unit
                                                                                        - 0x0100 - 0x7FFF   : shall be defined by the manufacturer of the reporting unit
                                                                                        - other             : reserved */
    USIGN16                     wIMHardwareRevision;                                /*!< allowed values 0x0000 - 0xFFFF */
}T_AM_FULLINFORMATION;

/*!< This structure hold only software relevant information. See T_AM_FULLINFORMATION for more details.*/
typedef struct t_am_firmwareonlyinformation
{
    T_UUID                      oIMUniqueIdentifier;                                /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyAMLocation[AM_LOCATION_SIZE];                    /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyIMAnnotation[IM_ANNOTATION_SIZE];                /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyIMOrderID[IM_ORDERID_SIZE];                      /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyAMSoftwareRevision[AM_SOFTWARE_REVISION_SIZE];   /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyIMSerialNumber[IM_SERIALNUMBER_SIZE];            /*!< see T_AM_FULLINFORMATION */
    T_IM_SW_REVISION            oIMSWRevision;                                      /*!< see T_AM_FULLINFORMATION */
    T_AM_DEVICE_IDENTIFICATION  oAMDeviceIdentification;                            /*!< see T_AM_FULLINFORMATION */
    USIGN16                     wAMTypeIdentification;                              /*!< see T_AM_FULLINFORMATION */
}T_AM_FIRMWAREONLYINFORMATION;

/*!< This structure hold only hardware relevant information. See T_AM_FULLINFORMATION for more details.*/
typedef struct t_am_hardwareonlyinformation
{
    T_UUID                      oIMUniqueIdentifier;                                /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyAMLocation[AM_LOCATION_SIZE];                    /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyIMAnnotation[IM_ANNOTATION_SIZE];                /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyIMOrderID[IM_ORDERID_SIZE];                      /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyAMHardwareRevision[AM_HARDWARE_REVISION_SIZE];   /*!< see T_AM_FULLINFORMATION */
    USIGN8                      pbyIMSerialNumber[IM_SERIALNUMBER_SIZE];            /*!< see T_AM_FULLINFORMATION */
    T_AM_DEVICE_IDENTIFICATION  oAMDeviceIdentification;                            /*!< see T_AM_FULLINFORMATION */
    USIGN16                     wAMTypeIdentification;                              /*!< see T_AM_FULLINFORMATION */
    USIGN16                     wIMHardwareRevision;                                /*!< see T_AM_FULLINFORMATION */
}T_AM_HARDWAREONLYINFORMATION;

/*!< This structure combines three possible information blocks where just one shall be filled with valid data and the rest shall be set to NULL. */
typedef struct t_am_info
{
    T_AM_FULLINFORMATION         *poAmFullInformation;
    T_AM_FIRMWAREONLYINFORMATION *poAmFirmwareOnlyInformation;
    T_AM_HARDWAREONLYINFORMATION *poAmHardwareOnlyInformation;
}T_AM_INFO;

typedef struct t_asset_management_block
{
    T_BLOCKHEADER   oBlockHeader;   /*< the block type defines which pointer in T_AM_INFO is used. The rest shall be set to NULL.*/
    T_AM_INFO       oAMInfo;
}T_ASSET_MANAGEMENT_BLOCK;

typedef struct t_asset_management_info
{
    USIGN16            wNumberOfEntries;                /*!< number of asset management blocks in this package.*/
    T_ASSET_MANAGEMENT_BLOCK *poAssetManagementBlocks;
}T_ASSET_MANAGEMENT_INFO;

typedef struct t_asset_management_data
{
    T_BLOCKHEADER   oBlockHeader;
    T_ASSET_MANAGEMENT_INFO oAssetManagementInfo;
}T_ASSET_MANAGEMENT_DATA;

typedef struct t_im5_data
{
    T_BLOCKHEADER       oBlockHeader;
    USIGN8              pbyIMAnnotation[IM_ANNOTATION_SIZE];
    USIGN8              pbyIMOrderID[IM_ORDERID_SIZE];
    USIGN8              byVendorIDHigh;
    USIGN8              byVendorIDLow;
    USIGN8              pbyIMSerialNumber[IM_SERIALNUMBER_SIZE];
    USIGN16             wIMHardwareRevision;
    T_IM_SW_REVISION    oIMSWRevision;
}T_IM5_DATA;

/*!< This structure combines two possible substructures where just one shall be filled with data and the rest shall be set to NULL*/
typedef struct t_im5_block
{
    T_IM5_DATA               *poIM5Data; 
    T_ASSET_MANAGEMENT_BLOCK *poAMBlock;    /*!< if an asset management block is used the T_AM_LOCATION substructure shall be filled with 0.*/
}T_IM5_BLOCK;

typedef struct t_im5
{
    T_BLOCKHEADER      oBlockHeader;
    USIGN16            wNumberOfEntries;
    T_IM5_BLOCK       *poIM5Block;
}T_IM5;

USIGN8    AM_GetAMLocationStructure(const USIGN8 *pbyAMLocation); 
VOID      AM_SetAMLocationStructure(USIGN8 *pbyAMLocation, USIGN8 byStructure);

VOID      AM_SetAMLocationLevel(USIGN8 *pbyAMLocation, const USIGN8 byLevel, USIGN16 wValue);
USIGN16   AM_GetAMLocationLevel(const USIGN8 *pbyAMLocation, const USIGN8 byLevel);

VOID      AM_SetAMLocationSlotFormat(USIGN8 *pbyAMLocation, T_AM_LOCATION_SLOT_FORMAT *poSlotFormat);
VOID      AM_GetAMLocationSlotFormat(USIGN8 *pbyAMLocation, T_AM_LOCATION_SLOT_FORMAT *poSlotFormat);

USIGN32   AM_BlockDecode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_BLOCK *poAMBlock);
USIGN32   AM_AssetDataDecode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_DATA *poAMData);

USIGN32   AM_AssetBlockEncode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_BLOCK *poAMBlock);
USIGN32   AM_AssetDataEncode(USIGN8 *pbyBuffer, T_ASSET_MANAGEMENT_DATA *poAMData);

USIGN32   AM_GetAssetDataSize(T_ASSET_MANAGEMENT_DATA *poData);
VOID      AM_AssetBlockFree(T_ASSET_MANAGEMENT_BLOCK *poAMBlock);
VOID      AM_AssetDataFree(T_ASSET_MANAGEMENT_DATA *poData);

USIGN32   AM_IM5DataDecode(USIGN8 *pbyBuffer, T_IM5_DATA *poIMData);
USIGN32   AM_IM5DataEncode(USIGN8 *pbyBuffer, T_IM5_DATA *poIM5Data);

USIGN32   AM_IM5Decode(USIGN8 *pbyBuffer, T_IM5 *poIM5);
USIGN32   AM_IM5Encode(USIGN8 *pbyBuffer, T_IM5 *poIM5);

USIGN32   AM_GetIM5DataSize(T_IM5 *poIM5);
VOID      AM_IM5DataFree(T_IM5 *poIM5);
#endif
