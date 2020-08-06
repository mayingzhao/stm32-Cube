/*
+-----------------------------------------------------------------------------+
| ****************************** TPS_1_API.c *******************************  |
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

/*! \file TPS_1_API.c
 *  \brief driver implementation to communicate with the TPS-1 firmware.
 */

/*---------------------------------------------------------------------------*/
/* Application Includes                                                      */
/*---------------------------------------------------------------------------*/
#include <TPS_1_API.h>

/*---------------------------------------------------------------------------*/
/* Local defines                                                             */
/*---------------------------------------------------------------------------*/
#define OP_MODE_GO_PULL                        0x01
#define OP_MODE_GO_RETURN                      0x02

#define BLOCK_VERSION_HIGH                     0x01
#define BLOCK_VERSION_LOW                      0x00

#define BLOCK_TYPE_SUBSTITUTE_VALUE            0x0014
#define BLOCK_TYPE_IM0                         0x0020
#define BLOCK_TYPE_IM1                         0x0021
#define BLOCK_TYPE_IM2                         0x0022
#define BLOCK_TYPE_IM3                         0x0023
#define BLOCK_TYPE_IM4                         0x0024
#define BLOCK_TYPE_RECORD_INPUT_DATA_OBJECT    0x0015
#define BLOCK_TYPE_RECORD_OUTPUT_DATA_OBJECT   0x0016

#define SUBSTITUTION_MODE_ZERO                 0x0000

#define RECORD_ERROR_INVALID_INDEX             -1

/*---------------------------------------------------------------------------*/
/* Local functions                                                           */
/*---------------------------------------------------------------------------*/
static SLOT*    AppGetSlotHandle(USIGN32 dwApiNumber, USIGN16 wSlotNumber);
static SUBSLOT* AppGetSubslotHandle(USIGN32 dwApiNumberApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber);
static VOID     AppOnConnect(USIGN32 dwARNumber);
static VOID     AppOnConnect_Done(USIGN32 dwARNumber);
static VOID     AppOnPRMENDDone(USIGN32 dwARNumber);
static USIGN32  AppSetEventAppRdy(USIGN32 dwARNumber);
static VOID     AppOnReadRecord(VOID);
static USIGN32  AppCreateAlarmMailBox(USIGN32 dwBoxNumber);
static VOID     AppOnWriteRecord(VOID);
static VOID     AppOnAlarmAck(VOID);
static VOID     AppOnDiagAckn(VOID);
static VOID     AppOnAbort(USIGN32 dwARNumber);
static VOID     AppOnSetStationName(T_DCP_SET_MODE zMode);
static VOID     AppOnFSUParameterChange(VOID);
static VOID     AppOnSetStationIpAddr(T_DCP_SET_MODE zMode);
static VOID     AppOnSetStationDcpSignal(VOID);
static VOID     AppOnResetFactorySettings(VOID);
static USIGN32  AppSetEventAlarmReq(USIGN32 dwArNumber);
static VOID     AppClearTypeOfStation(USIGN8* pbyData);
static USIGN32  APP_AddDevice (USIGN16 wVendorID, USIGN16 wDeviceID, CHAR *pszDeviceType,
                               T_DEVICE_SOFTWARE_VERSION* pzSoftwareVersion, USIGN16 wHWVersion);
#ifdef USE_ETHERNET_INTERFACE
static VOID     AppOnEthernetReceive(VOID);
#endif
static VOID     AppOnTPSMessageReceive(VOID);
static VOID     AppOnTPSReset( VOID );

static VOID     AppSetLastError(USIGN32 dwErrorCode);
static USIGN32  AppSetEventRegApp(USIGN32 dwEventBit);
static USIGN32  AppSetEventRegAppAckn(USIGN32 dwEventBit);
static USIGN32  AppSetEventOnConnectOK(USIGN32 dwARNumber);
static SIGN32   AppReadRecordDataObject(T_RECORD_DATA_OBJECT_TYPE zObjectToRead, USIGN32 dwArNumber, USIGN32 dwApiNumber,
                                        USIGN16 wSlotNumber, USIGN16 wSubslotNumber, USIGN8* pbyArrMailboxData);
static USIGN32  AppGetSlotInfoFromHandle(SUBSLOT* poSubslot,
                                         USIGN32* pdwApiNumber, USIGN16* pwSlotNumber, USIGN16* pwSubslotNumber);

/* I&M functions */
static SIGN32  AppReadIM0(SUBSLOT* poSubslot, USIGN8* pbyResponse);
static SIGN32  AppReadIM1(SUBSLOT* poSubslot, USIGN8* pbyResponse);
static SIGN32  AppReadIM2(SUBSLOT* poSubslot, USIGN8* pbyResponse);
static SIGN32  AppReadIM3(SUBSLOT* poSubslot, USIGN8* pbyResponse);
static SIGN32  AppReadIM4(SUBSLOT* poSubslot, USIGN8* pbyResponse);
static SIGN32  AppWriteIMData(USIGN8 byIMSupportedFlag, USIGN32 dwApi, USIGN16 wSlotNr, USIGN16 wSubslotNr, USIGN8* pbyResponse);
static SIGN32  AppReadIMData(USIGN32 dwApi, USIGN16 wSlotNr, USIGN16 wSubslotNr, USIGN16 wIndex, USIGN8* pbyResponse);
static USIGN32 AppFactoryResetForIM1_4(VOID);
static VOID    AppOnTPSLedChanged(VOID);

#ifdef PLUG_RETURN_SUBMODULE_ENABLE
static USIGN32 AppPullPlugSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber, USIGN16 wOpMode);
static USIGN32 AppPullSubModule(SUBSLOT* pzSubslot, USIGN8 bySubmoduleIOXS);
static USIGN32 AppRePlugSubmodule(SUBSLOT *pzSubslot);
#endif
static USIGN32 AppReactivateSubmodule(SUBSLOT *pzSubslot);
USIGN32 App_GetAPDUOffsetForAR(USIGN8 byArNr);

/*---------------------------------------------------------------------------*/
/* Global variables.                                                         */
/*---------------------------------------------------------------------------*/
static USIGN32 g_dwLastErrorCode = TPS_ACTION_OK; /*!< Last error code of an operation */

/*---------------------------------------------------------------------------*/
/* Pointer is used to step through the configuration memory.                 */
/*---------------------------------------------------------------------------*/
static USIGN8   *g_pbyConfigNRTMem = NULL;   /*!< Base address NRT area                */
static USIGN32  g_dwConfigNRTMemSize = 0;    /*!< Size of the configuration memory     */
static USIGN8   *g_pbyCurrentPointer = NULL; /*!< Pointer to the current memory point  */

/*---------------------------------------------------------------------------*/
/* Global variables                                                          */
/*---------------------------------------------------------------------------*/
static USIGN8  g_byApiState = STATE_INITIAL;

static USIGN16 *g_pwNumberOfApis       = NULL; /*!< Pointer into the NRT area of the number of registered APIs */
static USIGN16 *g_pwNumberOfSlots      = NULL; /*!< Pointer into the NRT area of the number of registered slots */

static USIGN32 g_dwArEstablished_0     = AR_NOT_IN_OPERATION; /*!< current AR status of AR_1 */
static USIGN32 g_dwArEstablished_1     = AR_NOT_IN_OPERATION; /*!< current AR status of AR_2 */
static USIGN32 g_dwArEstablished_IOSR  = AR_NOT_IN_OPERATION; /*!< current AR status of supervisor-AR */
#ifdef FW_UPDATE_OVER_HOST
    static USIGN32 g_dwUpdaterStartTimeout = 500000; /*!< Timeout after which the update process will report an error */
#endif

/*---------------------------------------------------------------------------*/
    static API_AR_CTX          g_zApiARContext         = {0};
static API_DEV_CTX         g_zApiDeviceContext     = {0};
#ifdef USE_ETHERNET_INTERFACE
static T_API_ETHERNET_CTX  g_zApiEthernetContext   = {0};
#endif

#ifdef DIAGNOSIS_ENABLE
static USIGN16 g_wNumberOfDiagEntries = 0; /*!< Number of diagnosis entries available */
#endif

static NRT_APP_CONFIG_HEAD *g_pzNrtConfigHeader = NULL; /*!< Pointer to the NRT area configuration header */
static ALARM_MB            g_zAlarmMailbox[NR_ALARM_MAILBOXES] = {{0}}; /*!< Alarm mailbox array */

/* Transmit Buffer for transferring MailBox data.                            */
/*---------------------------------------------------------------------------*/
static USIGN8 g_byTransmitBuffer[SIZE_RECORD_MB0] = {0};

/* Length of Output Data Buffer for AR                                       */
/*---------------------------------------------------------------------------*/
static USIGN8* g_pbyApduAddr[MAX_NUMBER_IOAR] = {0};

#ifdef USE_TPS_COMMUNICATION_CHANNEL
/* Variables for the internal communication interface.                       */
/*---------------------------------------------------------------------------*/
static T_ETHERNET_MAILBOX* g_poTPSIntRXMailbox = NULL; /*!< internal mailbox stack --> app */
static T_ETHERNET_MAILBOX* g_poTPSIntTXMailbox = NULL; /*!< internal mailbox app --> stack */
static T_API_TPS_MSG_CTX   g_oApiTpsMsgContext = {0}; /* The callback function. */
#endif

#ifdef USE_ETHERNET_INTERFACE
/* Buffer for the ethernet interface.                                        */
/*---------------------------------------------------------------------------*/
static T_ETHERNET_MAILBOX* g_poEthernetRXMailbox = NULL; /*!< ethernet mailbox stack --> app */
static T_ETHERNET_MAILBOX* g_poEthernetTXMailbox = NULL; /*!< ethernet mailbox app --> stack */
#endif

#if defined(USE_TPS_COMMUNICATION_CHANNEL) && !defined(USE_ETHERNET_INTERFACE)
#error "For the TPS Communication channel the Ethernet Interface has to be defined too!"
#endif

/* Checks for compiler options.                                              */
/*---------------------------------------------------------------------------*/
#if USED_NUMBER_OF_API > MAX_NUMBER_OF_API
#error "Invalid number of API!"
#endif
#if USED_NUMBER_SLOT > MAX_NUMBER_SLOT
#error "Invalid number of slots!"
#endif
#if USED_NUMBER_SUBSLOT > MAX_NUMBER_SUBSLOT
#error "Invalid number of subslots!"
#endif

/* Local malloc function; should be replaced by a library function           */
/* if necessary.                                                            */
/*---------------------------------------------------------------------------*/
#ifdef PRIVATE_MALLOC

static USIGN16     g_wCurrentApi    = 0;
static USIGN16     g_wCurrentSlot   = 0;
static USIGN16     g_wCurrentSubslot  = 0;

USIGN32* lib_malloc(USIGN32 dwSize)
{
    static API_LIST    zApiList[USED_NUMBER_OF_API]       = {{0}};
    static SLOT        zSlotList[USED_NUMBER_SLOT]        = {{0}};
    static SUBSLOT     zSubslotList[USED_NUMBER_SUBSLOT]  = {{0}};

    USIGN32 *pdwRetval = NULL;

    switch(dwSize)
    {
        case (sizeof(API_LIST)):
            if(g_wCurrentApi < USED_NUMBER_OF_API)
            {
                pdwRetval = (USIGN32*)&zApiList[g_wCurrentApi];
                g_wCurrentApi++;
            }
            break;

        case (sizeof(SLOT)):
            if(g_wCurrentSlot < USED_NUMBER_SLOT)
            {
                pdwRetval = (USIGN32*)&zSlotList[g_wCurrentSlot];
                g_wCurrentSlot++;
            }
            break;

        case (sizeof(SUBSLOT)):
            if(g_wCurrentSubslot < USED_NUMBER_SUBSLOT)
            {
                pdwRetval = (USIGN32*)&zSubslotList[g_wCurrentSubslot];
                g_wCurrentSubslot++;
            }
            break;

        default:
            break;
    }

    return pdwRetval;
}

VOID lib_reset_malloc(VOID)
{
    g_wCurrentApi    = 0;
    g_wCurrentSlot   = 0;
    g_wCurrentSubslot  = 0;
}

#endif /* end of define PRIVATE_MALLOC */

/*---------------------------------------------------------------------------*/
/* Functions                                                                 */
/*---------------------------------------------------------------------------*/

/*! \addtogroup devconf Device Configuration Interface
 *@{
*/

/*!
 * \brief       Function initializes the communication interface between the
 *              TPS and the application.
 *
 * \param[in]   VOID
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 */
USIGN32 TPS_InitApplicationInterface (VOID)
{
    /* Check the correct state!                                              */
    /*-----------------------------------------------------------------------*/
    if(g_byApiState != STATE_INITIAL)
    {
        return (TPS_ERROR_WRONG_API_STATE);
    }

    /* Set pointer to the first Byte of config memory                        */
    /* (starting with VendorID)                                              */
    /*-----------------------------------------------------------------------*/
    g_pbyConfigNRTMem = (USIGN8*)(BASE_ADDRESS_NRT_AREA);
    g_pbyCurrentPointer = g_pbyConfigNRTMem;
    g_dwConfigNRTMemSize = BASE_NRT_AREA_SIZE;

    #ifdef DEBUG_API_TEST
         printf("DEBUG_API > API: Address of the shared memory 0x%X (0x%X)\n",
                (USIGN32)g_pbyCurrentPointer, (*(USIGN32*)BASE_ADDRESS_NRT_AREA));
         printf("DEBUG_API > API: Size of the shared memory block %d\n",
                g_dwConfigNRTMemSize);
    #endif

#ifdef DIAGNOSIS_ENABLE
     g_wNumberOfDiagEntries = 0;
#endif

    /* Init the context management -> api list                               */
    /*-----------------------------------------------------------------------*/
    g_zApiARContext.api_list = NULL;
    g_byApiState = STATE_API_INIT_RDY;

    /* Init internal values.                                                 */
    /*-----------------------------------------------------------------------*/
    g_dwArEstablished_0 = AR_NOT_IN_OPERATION;
    g_dwArEstablished_1 = AR_NOT_IN_OPERATION;
    g_dwArEstablished_IOSR = AR_NOT_IN_OPERATION;

    return (TPS_ACTION_OK);
}


/*!
 *              Creates the device specific data during the startup of the host application.
 *              With these parameters the device gets its identification.
 *
 * \param[in]   wVendorID   Vendor ID of the device. Use the same as configured with the TPS Configurator.
 * \param[in]   wDeviceID   Device ID of the device. Use the same as configured with the TPS Configurator.
 * \param[in]   pszDeviceType device type string (zero terminated string). Use the same as configured with the TPS Configurator.
 * \param[in]   pzSoftwareVersion The software version of this device. This must be the same as in the I&M0 data
 * \param[in]   wHWVersion The Hardware Revision of the device. If not 0x0000 overwrites the hardware revision set with the TPS Configurator. If 0 the configured HW revision is used.
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - SOFTWARE_VERSION_IS_NULL
 *              - DEVICE_TYPE_TO_LONG
 *              - CONFIG_MEMORY_TO_SHORT
 */

USIGN32 TPS_AddDeviceAdvanced (USIGN16 wVendorID, USIGN16 wDeviceID, CHAR *pszDeviceType, T_DEVICE_SOFTWARE_VERSION* pzSoftwareVersion, USIGN16 wHWVersion)
{
   return APP_AddDevice ( wVendorID, wDeviceID, pszDeviceType, pzSoftwareVersion, wHWVersion);
}

/*!
 * \brief       <b> legacy function </b> Use TPS_AddDeviceAdvanced instead.<br>
 *              Creates the device specific data during the startup of the host application.
 *              With these parameters the device gets its identification.
 *
 * \param[in]   wVendorID   Vendor ID of the device. Use the same as configured with the TPS Configurator.
 * \param[in]   wDeviceID   Device ID of the device. Use the same as configured with the TPS Configurator.
 * \param[in]   pszDeviceType device type string (zero terminated string). Use the same as configured with the TPS Configurator.
 * \param[in]   pzSoftwareVersion The software version of this device. This must be the same as in the I&M0 data
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - SOFTWARE_VERSION_IS_NULL
 *              - DEVICE_TYPE_TO_LONG
 *              - CONFIG_MEMORY_TO_SHORT
 */

USIGN32 TPS_AddDevice (USIGN16 wVendorID, USIGN16 wDeviceID, CHAR *pszDeviceType, T_DEVICE_SOFTWARE_VERSION* pzSoftwareVersion)
{
   return APP_AddDevice ( wVendorID, wDeviceID, pszDeviceType, pzSoftwareVersion, 0);
}

USIGN32 APP_AddDevice (USIGN16 wVendorID, USIGN16 wDeviceID, CHAR *pszDeviceType, T_DEVICE_SOFTWARE_VERSION* pzSoftwareVersion, USIGN16 wHWVersion)
{
    USIGN32   dwDeviceMemorySize = 0;
    USIGN8    byStringLength     = 0;
    USIGN32   dwReturnCode       = 0;
    USIGN32   dwAr               = 0;
    USIGN32   dwIndex            = 0;

    /* Check the correct state!                                             */
    /*----------------------------------------------------------------------*/
    if(g_byApiState != STATE_API_INIT_RDY)
    {
        return (TPS_ERROR_WRONG_API_STATE);
    }

    if(pzSoftwareVersion == NULL)
    {
        return SOFTWARE_VERSION_IS_NULL;
    }

    /* Check the length of the DeviceType (Device Vendor Type).             */
    /*----------------------------------------------------------------------*/
    byStringLength = (USIGN8)(strlen((char*)pszDeviceType));
    if (byStringLength > TYPE_OF_STATION_STRING_LEN)
    {
         return (DEVICE_TYPE_TO_LONG);
    }

    #ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_AddDevice() > DeviceType length: %d character.\n",byStringLength);
    #endif

    /* Check if there is enough memory for the data                         */
    /*----------------------------------------------------------------------*/
    dwDeviceMemorySize = sizeof(NRT_APP_CONFIG_HEAD) +
                         SIZE_RECORD_MB0  +
                         SIZE_RECORD_MB1  +
                         SIZE_RECORD_MB2  +
                         SIZE_RECORD_MB3  +
                         (MAX_NUMBER_IOAR*SIZE_ALARM_MB*2);

    #ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_AddDevice() > DeviceMemorySize:   %x Byte\n",dwDeviceMemorySize);
    #endif

    if (dwDeviceMemorySize > g_dwConfigNRTMemSize)
    {
        return (CONFIG_MEMORY_TO_SHORT);
    }

    /* Set address for header!                                              */
    /*----------------------------------------------------------------------*/
    g_pzNrtConfigHeader = (NRT_APP_CONFIG_HEAD *)g_pbyConfigNRTMem;

    /* Copy the VendorID, deviceID, DeviceTypeLength and the DeviceType     */
    /* into the NRT-Area!                                                   */
    /*----------------------------------------------------------------------*/
#ifndef USE_INT_APP
    TPS_SetValue16((USIGN8*)&(g_pzNrtConfigHeader->wVendorID), wVendorID);
    TPS_SetValue16((USIGN8*)&(g_pzNrtConfigHeader->wDeviceID), wDeviceID);
    TPS_SetValue16((USIGN8*)&(g_pzNrtConfigHeader->wDeviceVendorTypeLength), byStringLength);

    if(wHWVersion != 0)
    {
       TPS_SetValue16((USIGN8*)&(g_pzNrtConfigHeader->wHWVersion), wHWVersion);
    }

    AppClearTypeOfStation(g_pzNrtConfigHeader->byDeviceVendorType);

    /* Write DeviceVendorType                                               */
    /*----------------------------------------------------------------------*/
    TPS_SetValueData(g_pzNrtConfigHeader->byDeviceVendorType, (USIGN8*)pszDeviceType, byStringLength);
#endif
    /* Write Device Software Version                                        */
    /*----------------------------------------------------------------------*/
    TPS_SetValue8((USIGN8*)&(g_pzNrtConfigHeader->byRevisionPrefix), pzSoftwareVersion->byRevisionPrefix);
    TPS_SetValue8((USIGN8*)&(g_pzNrtConfigHeader->byRevisionFunctionalEnhancement),
                             pzSoftwareVersion->byRevisionFunctionalEnhancement);
    TPS_SetValue8((USIGN8*)&(g_pzNrtConfigHeader->byBugFix), pzSoftwareVersion->byBugFix);
    TPS_SetValue8((USIGN8*)&(g_pzNrtConfigHeader->byInternalChange), pzSoftwareVersion->byInternalChange);

#ifdef API_VERSION
    TPS_SetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwAPIVersion), API_VERSION);
#endif

    g_pbyCurrentPointer += sizeof(NRT_APP_CONFIG_HEAD);
    DWORD_ALIGN(g_pbyCurrentPointer);

    TPS_SetValue16(g_pbyCurrentPointer, MAX_NUMBER_IOAR);

    g_pbyCurrentPointer += 2;

    /* Create the mailboxes for the allowed two IO-ARs.                     */
    /*----------------------------------------------------------------------*/
    for(dwAr = 0; (dwAr < MAX_NUMBER_IOAR) && (dwReturnCode == TPS_ACTION_OK); dwAr++)
    {
        /* Initialize the mailbox memory (context management)              */
        /*-----------------------------------------------------------------*/
        g_zApiARContext.record_mb[dwAr].dwSizeRecordMailbox = SIZE_RECORD_MB0;

        /* Write the mailbox record size into the Dual-Ported-RAM.         */
        /*-----------------------------------------------------------------*/
        TPS_SetValue32(g_pbyCurrentPointer, SIZE_RECORD_MB0);

        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[dwAr].pt_flags      = g_pbyCurrentPointer;
        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[dwAr].pt_errorcode1 = g_pbyCurrentPointer;
        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[dwAr].pt_req_header = g_pbyCurrentPointer;
        g_pbyCurrentPointer += SIZE_OF_REQ_HEADER;
        g_zApiARContext.record_mb[dwAr].pt_data       = g_pbyCurrentPointer;
        g_pbyCurrentPointer += SIZE_RECORD_MB0;

        /* Initialize the mailbox memory                                  */
        /*----------------------------------------------------------------*/
        memset(&g_byTransmitBuffer[0], 0x00, g_zApiARContext.record_mb[dwAr].dwSizeRecordMailbox);
        TPS_SetValueData(g_zApiARContext.record_mb[dwAr].pt_flags,
                         &g_byTransmitBuffer[0],
                         g_zApiARContext.record_mb[dwAr].dwSizeRecordMailbox);

        /* Add the alarm mailboxes to the record mailbox (low priority alarm  */
        /* alarm and high priority alarm.                                     */
        /*--------------------------------------------------------------------*/
        for(dwIndex = 0; (dwIndex < 2) && (dwReturnCode == TPS_ACTION_OK); dwIndex++)
        {
            dwReturnCode = AppCreateAlarmMailBox(dwIndex + (2*dwAr));
        }
    } /* End for create Record-Mailboxes - IO-AR */

    if(dwReturnCode == TPS_ACTION_OK)
    {
        /* Create mailbox 2 for the DeviceAccess and Supervisor AR.           */
        /*--------------------------------------------------------------------*/
        g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].dwSizeRecordMailbox = SIZE_RECORD_MB2;
        TPS_SetValue32(g_pbyCurrentPointer, SIZE_RECORD_MB2);

        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].pt_flags      = g_pbyCurrentPointer;
        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].pt_errorcode1 = g_pbyCurrentPointer;
        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].pt_req_header = g_pbyCurrentPointer;
        g_pbyCurrentPointer += SIZE_OF_REQ_HEADER;
        g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].pt_data       = g_pbyCurrentPointer;
        g_pbyCurrentPointer += SIZE_RECORD_MB2;

        memset(&g_byTransmitBuffer[0], 0x00, g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].dwSizeRecordMailbox);
        TPS_SetValueData(g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].pt_flags,
                         &g_byTransmitBuffer[0],
                         g_zApiARContext.record_mb[SUPERVISOR_MB_NUM].dwSizeRecordMailbox);

        /* Create mailbox 3 for the Implicit AR.                                */
        /*----------------------------------------------------------------------*/
        g_zApiARContext.record_mb[IMPLICITE_MB_NUM].dwSizeRecordMailbox = SIZE_RECORD_MB3;
        TPS_SetValue32(g_pbyCurrentPointer, SIZE_RECORD_MB3);

        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[IMPLICITE_MB_NUM].pt_flags      = g_pbyCurrentPointer;
        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[IMPLICITE_MB_NUM].pt_errorcode1 = g_pbyCurrentPointer;
        g_pbyCurrentPointer += 4;
        g_zApiARContext.record_mb[IMPLICITE_MB_NUM].pt_req_header = g_pbyCurrentPointer;
        g_pbyCurrentPointer += SIZE_OF_REQ_HEADER;
        g_zApiARContext.record_mb[IMPLICITE_MB_NUM].pt_data       = g_pbyCurrentPointer;
        g_pbyCurrentPointer += SIZE_RECORD_MB3;

        memset(&g_byTransmitBuffer[0], 0x00, SIZE_RECORD_MB0);
        TPS_SetValueData(g_zApiARContext.record_mb[IMPLICITE_MB_NUM].pt_flags,
                         &g_byTransmitBuffer[0],
                         g_zApiARContext.record_mb[IMPLICITE_MB_NUM].dwSizeRecordMailbox);

        /* Reserve space for the variable "Number_of_APIs" and initialize     */
        /* the value to zero.                                                 */
        /*--------------------------------------------------------------------*/
        g_pwNumberOfApis = (USIGN16*)g_pbyCurrentPointer;

        TPS_SetValue16((USIGN8*)g_pwNumberOfApis, 0);

        g_pbyCurrentPointer += 2;           /* set current pointer new        */

    #ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_AddDevice() > Device added\n");
        printf("DEBUG_API > API: TPS_AddDevice() > Available Config Memory: %x Byte\n", g_dwConfigNRTMemSize, g_pbyCurrentPointer - g_pbyConfigNRTMem);
    #endif

        g_byApiState = STATE_DEVICE_RDY;
    }

    return (dwReturnCode);
}

/*!
 * \brief       This function configures an application process
 *              identifier (API) to a PROFINET IO Device. The TPS-1 supports
 *              a maximum of two APIs.
 *
 * \param[in]   dwApi application process identifier
 * \retval      API_LIST* A handle to the created API or NULL if an error occurred. The error code can be obtained by TPS_GetLastError()
 */
API_LIST* TPS_AddAPI(USIGN32 dwApi)
{
    USIGN16  wReturnValue = TPS_ACTION_OK;
    API_LIST *pzApiList = NULL;

    /* Check the correct state!                                              */
    /*-----------------------------------------------------------------------*/
    if(((g_byApiState != STATE_DEVICE_RDY) && (dwApi == 0x00)) || /* First API: State has to be STATE_DEVICE_RDY. */
       ((g_byApiState != STATE_SUBSLOT_RDY) && (dwApi > 0x00)))   /* All further APIs: state has to be STATE_SUBSLOT_RDY. */
    {
        AppSetLastError(TPS_ERROR_WRONG_API_STATE);
        return NULL;
    }

    /* Check rest of config memory. Number of Slots = USIGN16.               */
    /*-----------------------------------------------------------------------*/
    if ((USIGN32)((g_pbyCurrentPointer + sizeof(dwApi) + sizeof(USIGN16)) - g_pbyConfigNRTMem) > g_dwConfigNRTMemSize )
    {
        #ifdef DEBUG_API_TEST
            printf("DEBUG_API > API: TPS_AddAPI() > Not enough memory left\n");
        #endif
        AppSetLastError(ADD_API_OUT_OF_MEMORY);
        return NULL;
    }

    /* Increase the "Number_of_APIs" in the DPRAM.                           */
    /*-----------------------------------------------------------------------*/
    TPS_GetValue16((USIGN8*)g_pwNumberOfApis, &wReturnValue);
    wReturnValue++;
    TPS_SetValue16((USIGN8*)g_pwNumberOfApis, wReturnValue);

    /* Write the API Number to the DPRAM.                                   */
    /*-----------------------------------------------------------------------*/
    TPS_SetValue32(g_pbyCurrentPointer, dwApi);
    g_pbyCurrentPointer += sizeof(dwApi);

    /* Set the "Number_of_Slots" in the DPRAM.                               */
    /*-----------------------------------------------------------------------*/
    g_pwNumberOfSlots = (USIGN16*)(g_pbyCurrentPointer);
    TPS_SetValue16((USIGN8*)g_pwNumberOfSlots, 0x0000);
    g_pbyCurrentPointer += sizeof(USIGN16);

    /* Initialize the "api_list" structure in "API_AR_CTX"-structure.        */
    /*-----------------------------------------------------------------------*/
    if(g_zApiARContext.api_list == NULL)
    {
        /* Create the "api_list" structure.                                */
        /*-----------------------------------------------------------------*/
        g_zApiARContext.api_list = (API_LIST*)(t_malloc(sizeof(API_LIST)));

        if(g_zApiARContext.api_list == NULL)
        {
            #ifdef DEBUG_API_TEST
                printf("DEBUG_API > API: TPS_AddAPI()> Error: Could not create API_LIST Entry.\n");
            #endif
            AppSetLastError(ADD_API_OUT_OF_MEMORY);
            return NULL;
        }
        pzApiList = g_zApiARContext.api_list;
    }
    else
    {
        /* "api_list" structure available, pointer set                     */
        /*-----------------------------------------------------------------*/
        for(pzApiList = g_zApiARContext.api_list; pzApiList->next != NULL; pzApiList = pzApiList->next)
        {
        /* Jump to the end of the list!                                    */
        /*-----------------------------------------------------------------*/
        };
        pzApiList->next = (API_LIST*)(t_malloc(sizeof(API_LIST)));
        pzApiList = pzApiList->next;
    }

    if(pzApiList == NULL)
    {
        #ifdef DEBUG_API_TEST
            printf("DEBUG_API > API: TPS_AddAPI() > Error: Could not create API_LIST Entry.\n");
        #endif
        AppSetLastError(ADD_API_OUT_OF_MEMORY);
        return NULL;
    }

    /* Initialize "api_list" structure (-> api_ar_ctx.api_list).            */
    /*----------------------------------------------------------------------*/
    pzApiList->index     = dwApi;
    pzApiList->firstslot = NULL;
    pzApiList->next      = NULL;

    g_byApiState = STATE_API_RDY;

    return pzApiList;
}

/*!
 * \brief       This function activates a declared configuration by
 *              setting the event "CONFIG_FINISHED"
 *              (AppEventRegister bit 0 -> CONFIG_FINISHED)
 *
 * \param[in]   VOID
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - TPS_ERROR_STACK_START_FAILED
 */
USIGN32 TPS_StartDevice (VOID)
{
    USIGN32 dwValue = 0;
    USIGN8 byCorrectApiState = STATE_SUBSLOT_RDY;

    #ifdef USE_TPS_COMMUNICATION_CHANNEL
        byCorrectApiState = STATE_TPS_CHANNEL_RDY;
    #endif

    /* Check the State of the Driver. Start is only allowed if at least one   */
    /* slot with one subslot was added.                                                 */
    if((g_byApiState != byCorrectApiState) &&
       (g_byApiState != STATE_DEVICE_START_INITIATED) )
    {
        return TPS_ERROR_WRONG_API_STATE;
    }

    /* Set NrtMemSize to zero -> this indicates to the TPS-1 firmware that    */
    /* the application finished the device configuration. The firmware        */
    /* confirms this by resetting the value to 0x8000 (BASE_NRT_AREA_SIZE)    */
    /*------------------------------------------------------------------------*/
    if(g_byApiState == byCorrectApiState)
    {
        TPS_SetValue32((USIGN8*)(&g_pzNrtConfigHeader->dwNrtMemSize), 0x00000000);
        AppSetEventRegApp(APP_EVENT_CONFIG_FINISHED);

        /* Device Start was initiated by overwriting dwNrtMemSize with zero */
        g_byApiState = STATE_DEVICE_START_INITIATED;
    }

    /* Now check if the stack is completely started. The stack will indicate
     * this by writing the NrtMemSize back to the correct value.            */
    /*----------------------------------------------------------------------*/
#ifndef USE_INT_APP

    TPS_GetValue32((USIGN8*)(&g_pzNrtConfigHeader->dwNrtMemSize), &dwValue);

    if(dwValue != BASE_NRT_AREA_SIZE)
    {
        /* Nrt mem size was not reset by TPS firmware */
        return (TPS_ERROR_STACK_START_FAILED);
    }
    #ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_StartDevice() reached!\n");
    #endif
#endif

    g_byApiState = STATE_DEVICE_STARTED;

    return (TPS_ACTION_OK);
}

/*!
 * \brief       This function adds a new module (slot) to the configuration.
 *
 * \param[in]   pzApi handle to the API of this module. The handle is returned by TPS_AddApi()
 * \param[in]   wSlotNumber the slot number to add
 * \param[in]   dwModuleIdentNumber the module ID of the slot
 * \retval      SLOT* a valid slot handle of the new plugged module or NULL in case of an error
 * \note        Do not change this function!
*/
SLOT* TPS_PlugModule (API_LIST* pzApi, USIGN16 wSlotNumber, USIGN32 dwModuleIdentNumber)
{
    SLOT*   pzSlot = NULL;
    USIGN32 dwDataLength = 0;
    USIGN16 wReturnData  = 0;

    /* Check the correct state!                                              */
    /*-----------------------------------------------------------------------*/
    if((g_byApiState != STATE_API_RDY) && (g_byApiState != STATE_SUBSLOT_RDY) && (g_byApiState != STATE_SLOT_RDY))
    {
        AppSetLastError(TPS_ERROR_WRONG_API_STATE);
        return NULL;
    }

    /* Check validity of dwModuleIdentNumber                                 */
    /* ModuleID 0x00 is reserved for autoconfiguration.                      */
    /*-----------------------------------------------------------------------*/
    if(dwModuleIdentNumber == AUTOCONF_MODULE)
    {
        #ifndef USE_AUTOCONF_MODULE
            AppSetLastError(PLUGMODULE_INVALID_MODULE_ID);
            return NULL;
        #endif

        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugModule() > Configuring slot 0x%X for autoconfiguration.\n", wSlotNumber);
        #endif
    }

    /* Check parameter!                                                      */
    /*-----------------------------------------------------------------------*/
    if(pzApi == NULL)
    {
        AppSetLastError(TPS_ERROR_NULL_POINTER);
        return NULL;
    }

    /* Check if this slot number doesn't exist already.                      */
    /*-----------------------------------------------------------------------*/
    pzSlot = AppGetSlotHandle(pzApi->index, wSlotNumber);
    #ifdef DEBUG_API_PLUG_MODULE
        printf("DEBUG_API > API: TPS_PlugModule() Checking if a module with the same number exists (Should not happen).\n");
    #endif
    if(pzSlot != NULL)
    {
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugModule() > A module with the same number exists.\n");
        #endif
        AppSetLastError(PLUGMODULE_SLOT_ALREADY_EXISTS);
        return (NULL);
    }


    /* Check memory space before adding a slot.                              */
    /*-----------------------------------------------------------------------*/
    dwDataLength = sizeof(wSlotNumber) +
                   sizeof(dwModuleIdentNumber) +
                   sizeof(USIGN16);

    if ( (USIGN32)(g_pbyCurrentPointer + dwDataLength - g_pbyConfigNRTMem) > g_dwConfigNRTMemSize )
    {
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugModule() > Not enough space for data!\n");
        #endif
        AppSetLastError(PLUGMODULE_OUT_OF_MEMORY);
        return (NULL);
    }

    /* Add the slot to the API_AR_CTX structure (ar context)                 */
    /*-----------------------------------------------------------------------*/
    if(pzApi->firstslot == NULL)
    {
        pzApi->firstslot = (SLOT*)(t_malloc(sizeof(SLOT)));
        pzSlot = pzApi->firstslot;
    }
    else
    {
        for(pzSlot = pzApi->firstslot;
            pzSlot->pNextSlot != NULL;
            pzSlot = pzSlot->pNextSlot)
        {
            /* Jump to the end of the list                                   */
            /*---------------------------------------------------------------*/
        };
        pzSlot->pNextSlot = (SLOT*)(t_malloc(sizeof(SLOT)));
        pzSlot = pzSlot->pNextSlot;
    }

    /* Check the generated slot handle                                      */
    /*----------------------------------------------------------------------*/
    if(pzSlot == NULL)
    {
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugModule() > Can not allocate Slot_Handle!\n");
        #endif
        AppSetLastError(PLUGMODULE_INVALID_SLOT_HANDLE);
        return (NULL);
    }

    /* Configure the slot data                                              */
    /*----------------------------------------------------------------------*/
    pzSlot->poApi = pzApi;
    pzSlot->pNextSlot = NULL;
    pzSlot->pSubslot  = NULL;
    pzSlot->wProperties = MODULE_NOT_PULLED;

/*
      Structure inside the DPRAM :
      +--------------------------+
      +    Slot_Number (16)      +
      +--------------------------+
      +  ModuleIdent_Number (32) +
      +--------------------------+
      +    Number_of_Submodules (16)  +
      +--------------------------+
*/
    /* Set the slot number  .                                                */
    /*-----------------------------------------------------------------------*/
    TPS_SetValue16(g_pbyCurrentPointer, wSlotNumber);
    pzSlot->pt_slot_number = (USIGN16*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += sizeof(wSlotNumber);

    /* Set the module ID                                                     */
    /*-----------------------------------------------------------------------*/
    TPS_SetValue32(g_pbyCurrentPointer, dwModuleIdentNumber);
    pzSlot->pt_ident_number = (USIGN32*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += sizeof(dwModuleIdentNumber);

    /* Increment g_pwNumberOfSlots (global slot counter)                     */
    /*-----------------------------------------------------------------------*/
    TPS_GetValue16((USIGN8*)g_pwNumberOfSlots, &wReturnData);
    wReturnData++;
    TPS_SetValue16((USIGN8*)g_pwNumberOfSlots, wReturnData);

    /* initialize number of subslots */
    TPS_SetValue16(g_pbyCurrentPointer, 0x0000);
    pzSlot->pt_number_of_subslot = (USIGN16*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += sizeof(USIGN16);

    /* Initialize the "Module State"  (Slot data)                            */
    /*-----------------------------------------------------------------------*/
    TPS_SetValue16(g_pbyCurrentPointer, PROPER_MODULE);
    pzSlot->pt_module_state = (USIGN16*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += sizeof(USIGN16);

    #ifdef DEBUG_API_PLUG_MODULE
        printf("DEBUG_API > API: TPS_PlugModule() > Module %x plugged!\n",wSlotNumber);
        TPS_GetValue16((USIGN8*)g_pwNumberOfSlots,&wReturnData);
        printf("DEBUG_API > API: TPS_PlugModule() > Number of Modules : 0x%X \n", (USIGN32)wReturnData);
        printf("DEBUG_API > API: TPS_PlugModule() > Current Memory Pointer: 0x%X\n", (USIGN32)g_pbyCurrentPointer);
    #endif

    g_byApiState = STATE_SLOT_RDY;
    return (pzSlot);
}

/*!
* \brief       This function adds a submodule (subslot) to a slot. The
*              parameter pzSlotHandle points to the associated slot.
*
* \param[in]   pzSlotHandle handle of the slot
* \param[in]   wSubslotNumber the subslot number to add
* \param[in]   wSubmoduleIdentNumber the submodule ID. Use AUTOCONF_MODULE if this submodule should be later adapted to the expected configuration.
* \param[in]   wInitParameterSize <b>Legacy:</b> the maximum size of initial parameters that can be received for this subslot
*              <br>the size is calculated as follows:
*                  size for Index (2 bytes) + size for the data length (4 bytes) + size of the record data.<br>
*              <b>New Parameterization Handling:</b> Set this parameter 0. This saves memory in the NRT area and streamlines
*              the record write handling but might slow down the connect time a bit.<br>
*               Each Parameter that is sent on connect time triggers a record write event
*              that calls the registered onRecordWrite callback in your application.
* \param[in]   wNumberOfChannelDiag maximum number of diagnostic channels that the application can register for this submodule.
* \param[in]   wSizeOfInputData Size of the cyclic input data of this submodule.
<br>If wSubmoduleIdentNumber = AUTOCONF_MODULE this value is not used but later defined by the expected configuration.
* \param[in]   wSizeOfOutputData Size of the cyclic output data of this submodule.
<br>If wSubmoduleIdentNumber = AUTOCONF_MODULE this value is not used but later defined by the expected configuration.
* \param[in]   pzIM0Data pointer to the I&M0 data structure for this subslot. Each subslot must have at least I&M0 data.
* \param[in]   bIM0Carrier flag if this subslot is a I&M carrier (TPS_TRUE) or not (TPS_FALSE).
*              Slot 0 / subslot 1 (DAP) must be a carrier. Each new carrier should contain different I&M data than the DAP.
* \retval      SUBSLOT* a valid subslot handle of the new plugged subslot or NULL in case of an error
*/
SUBSLOT* TPS_PlugSubmodule(SLOT*       pzSlotHandle,
                           USIGN16     wSubslotNumber,
                           USIGN32     dwSubmoduleIdentNumber,
                           USIGN16     wInitParameterSize,
                           USIGN16     wNumberOfChannelDiag,
                           USIGN16     wSizeOfInputData,
                           USIGN16     wSizeOfOutputData,
                           T_IM0_DATA* pzIM0Data,
                           BOOL        bIM0Carrier)
{
    API_LIST* pzApi = NULL;
    SLOT*     pzTempSlot = NULL;
    SUBSLOT*  pzSubslot = NULL;

    USIGN32   dwMemSizeSubModule = 0;
    USIGN32   dwApiNr = 0;
    USIGN16   wSlotNumber = 0;
    USIGN16   wNumberOfSubslots = 0;
    USIGN16*  pwSubslotNumberLocal = NULL;
    USIGN32   dwReg = 0;
    BOOL      bApiFound = TPS_FALSE;

    /* Check the correct state!                                            */
    /*---------------------------------------------------------------------*/
    if((g_byApiState != STATE_SLOT_RDY) && (g_byApiState != STATE_SUBSLOT_RDY))
    {
        AppSetLastError(TPS_ERROR_WRONG_API_STATE);
        return NULL;
    }

    /* Check the for nullpointer!                                          */
    /*---------------------------------------------------------------------*/
    if(pzSlotHandle == NULL)
    {
        AppSetLastError(TPS_ERROR_NULL_POINTER);
        return NULL;
    }

    /* Check the correct ident number!                                       */
    /* SubmoduleID 0x00 is reserved for autoconfiguration.                      */
    /*-----------------------------------------------------------------------*/
    if(dwSubmoduleIdentNumber == AUTOCONF_MODULE)
    {
        #ifndef USE_AUTOCONF_MODULE
            AppSetLastError(PLUG_SUB_INVALID_SUBMODULE_ID);
            return NULL;
        #endif

        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubodule() > Configuring subslot 0x%X for autoconfiguration.\n", wSubslotNumber);
        #endif
    }

    /* Check if this subslot number doesn't already exists.                   */
    /*-----------------------------------------------------------------------*/
    /* Look for the API in which this slot is plugged. */
    if(g_zApiARContext.api_list != NULL)
    {
        for(pzApi = g_zApiARContext.api_list; pzApi != NULL; pzApi = pzApi->next)
        {
            /* Search the API List for the given Slot to find the correct API Number. */
            for(pzTempSlot = pzApi->firstslot;
                pzTempSlot != NULL;
                pzTempSlot = pzTempSlot->pNextSlot)
            {
                if(pzTempSlot == pzSlotHandle)
                {
                    /* Slot found, store API Number. */
                    dwApiNr = pzApi->index;
                    bApiFound = TPS_TRUE;
                }
            }
        }
    }

    if(bApiFound == TPS_FALSE)
    {
        AppSetLastError(PLUG_SUB_SLOT_NOT_FOUND);
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubodule() > Given Slothandle not found in List of API\n");
        #endif

        return NULL;
    }

    TPS_GetValue16((USIGN8*)pzSlotHandle->pt_slot_number, &wSlotNumber);
    if((wSlotNumber == DAP_MODULE) && (wSubslotNumber == DAP_SUBMODULE) && (pzIM0Data == NULL))
    {
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubmodule() > DAP submodule have no I&M0 Data.\n");
        #endif
        AppSetLastError(PLUG_SUB_MISSING_IM0_DATA_FOR_DAP);
        return NULL;
    }
    if((wSlotNumber == DAP_MODULE) && (wSubslotNumber == DAP_SUBMODULE) && (pzIM0Data != NULL) && (bIM0Carrier==TPS_FALSE))
    {
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubmodule() > DAP submodule have I&M0 carrier.\n");
        #endif
        AppSetLastError(PLUG_SUB_MISSING_IM0_CARRIER_FOR_DAP);
        return NULL;
    }


    if(pzIM0Data == NULL )
    {
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubmodule() > Every submodule needs I&M0 data.\n");
        #endif
        AppSetLastError(PLUG_SUB_MISSING_IM0_DATA);
        return NULL;
    }
    #ifdef DEBUG_API_PLUG_MODULE
        printf("DEBUG_API > API: TPS_PlugSubmodule() Checking if a submodule with the same number exists (Should not happen).\n");
    #endif
    pzSubslot = AppGetSubslotHandle(dwApiNr, wSlotNumber, wSubslotNumber);
    if(pzSubslot != NULL)
    {
        #ifdef DEBUG_API_PLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubmodule() > A module with the same number exists.\n");
        #endif
        AppSetLastError(PLUG_SUB_SLOT_ALREADY_EXISTS);
        return NULL;
    }

    /* Check if sufficient memory is available in NRT area for a new subslot  */
    /*------------------------------------------------------------------------*/
    dwMemSizeSubModule = wInitParameterSize +
                         (wNumberOfChannelDiag * sizeof(DPR_DIAG_ENTRY)) +
                         74 + /* Data accumulated (without ISOCHRON_PARAM)    */
                         36;  /* 9x DWORD_ALIGN   (4 Byte max each)           */
       /* MPr see below  (g_pbyCurrentPointer-g_pbyConfigNRTMem);  * for calc */

    /* Check if there is enough memory available.                           */
    /*----------------------------------------------------------------------*/

    if ( (USIGN32)((g_pbyCurrentPointer + dwMemSizeSubModule) - g_pbyConfigNRTMem) > g_dwConfigNRTMemSize )
    {
    #ifdef DEBUG_API_SUBPLUG_MODULE
        printf("DEBUG_API > API: TPS_PlugSubModule() > Not enough space for data!\n");
    #endif
         AppSetLastError(PLUG_SUB_OUT_OF_MEMORY);
         return NULL;
    }

    /* Check if the slot handle is not NULL                                 */
    /*----------------------------------------------------------------------*/
    if(pzSlotHandle == NULL)
    {
        #ifdef DEBUG_API_SUBPLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubModule() > Slot Handle not correct!\n");
        #endif
        AppSetLastError(PLUG_SUB_INVALID_SLOT_HANDLE);
        return NULL;
    }

    /* Check if the API List is set before!                                 */
    /*----------------------------------------------------------------------*/
    if(g_zApiARContext.api_list == NULL)
    {
        #ifdef DEBUG_API_SUBPLUG_MODULE
            printf("DEBUG_API > API: TPS_PlugSubModule() > Insert an API first!\n");
        #endif
        AppSetLastError(PLUG_SUB_MISSING_API);
        return NULL;
    }

    /* Reserve memory for the subslot and initialize the structure!         */
    /*----------------------------------------------------------------------*/
    if(pzSlotHandle->pSubslot == NULL)
    {
        pzSlotHandle->pSubslot = (SUBSLOT*)(t_malloc(sizeof(SUBSLOT)));
        pzSubslot = pzSlotHandle->pSubslot;
    }
    else
    {
        for(pzSubslot=((SLOT*)pzSlotHandle)->pSubslot ;
            pzSubslot->poNextSubslot!=0 ;
            pzSubslot=pzSubslot->poNextSubslot)
        {
            /* Iterate to the end of the slot list */
        }

        pzSubslot->poNextSubslot = (SUBSLOT*)(t_malloc(sizeof(SUBSLOT)));
        pzSubslot = pzSubslot->poNextSubslot;
    }

    /* Delete the structures when the initialization failed!                */
    /*----------------------------------------------------------------------*/
    if(pzSubslot == NULL)
    {
        #ifdef DEBUG_API_SUBPLUG_MODULE
          printf("DEBUG_API > API: TPS_PlugSubModule() > Error. Can not allocate SubSlot_Handle\n");
        #endif
        AppSetLastError(PLUG_SUB_INVALID_SUBSLOT_HANDLE);
        return NULL;
    }

    pzSubslot->pzSlot = pzSlotHandle;
    pzSubslot->poNextSubslot = NULL;

    /* Increment the "Number_of_Subslots"!                                  */
    /*----------------------------------------------------------------------*/
    pwSubslotNumberLocal = pzSlotHandle->pt_number_of_subslot;
    #ifdef DEBUG_API_SUBPLUG_MODULE
          printf("DEBUG_API > API: TPS_PlugSubModule() > Address Number_of_Subslots : 0x%X\n", (USIGN32)pwSubslotNumberLocal);
    #endif
    TPS_GetValue16((USIGN8*)pwSubslotNumberLocal, &wNumberOfSubslots);
    wNumberOfSubslots++;
    TPS_SetValue16((USIGN8*)pwSubslotNumberLocal, wNumberOfSubslots);

    #ifdef DEBUG_API_SUBPLUG_MODULE
          printf("DEBUG_API > API: TPS_PlugSubModule() > Number_of_Subslots = 0x%X\n", wNumberOfSubslots);
    #endif


    /* Write Subslot_Number into the DPRAM                                  */
    /*----------------------------------------------------------------------*/
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, wSubslotNumber);
    pzSubslot->pt_subslot_number = (USIGN16*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 2;

    /* Set SubModuleIdnet_Number (SubSlot data)                             */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)g_pbyCurrentPointer, dwSubmoduleIdentNumber);
    pzSubslot->pt_ident_number = (USIGN32*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 4;

    if(dwSubmoduleIdentNumber == AUTOCONF_MODULE)
    {
        pzSubslot->wAdaptiveIos = 1;
    }
    else
    {
        pzSubslot->wAdaptiveIos = 0;
    }

    /* Set I&M0 data, if available.                                         */
    /*----------------------------------------------------------------------*/
    if ( (pzIM0Data != NULL) && (bIM0Carrier == TPS_TRUE))
    {
        TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, IMO_DATA_AVAIL);
        pzSubslot->poIM0DataAvailable = (USIGN16*)g_pbyCurrentPointer;
    }
    g_pbyCurrentPointer += 2;

    /* Set size of init records (Size_Init_Records).                        */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)g_pbyCurrentPointer, wInitParameterSize);
    pzSubslot->pt_size_init_records = (USIGN32*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 4;

    /* Set size of init records used.                                       */
    /*----------------------------------------------------------------------*/
    pzSubslot->pt_size_init_records_used = (USIGN32*)g_pbyCurrentPointer;
    TPS_SetValue32((USIGN8*)pzSubslot->pt_size_init_records_used, 0x00000000);
    g_pbyCurrentPointer += 4;

    /* Set init parameter size                                              */
    /*----------------------------------------------------------------------*/
    pzSubslot->pt_init_records = g_pbyCurrentPointer;
    g_pbyCurrentPointer += wInitParameterSize;

    DWORD_ALIGN(g_pbyCurrentPointer);

    /* Set Size_Channel_Diagnosis.                                          */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)g_pbyCurrentPointer, wNumberOfChannelDiag * sizeof(DPR_DIAG_ENTRY));
    pzSubslot->pt_size_chan_diag = (USIGN32*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 4;

    /* Set Number_Channel_Diagnosis.                                        */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)g_pbyCurrentPointer, wNumberOfChannelDiag);
    pzSubslot->pt_nmbr_chan_diag = (USIGN32*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 4;

    /* Diagnosis buffer                                                     */
    /*----------------------------------------------------------------------*/
    pzSubslot->pt_chan_diag = g_pbyCurrentPointer;
    TPS_GetValue32((USIGN8*)pzSubslot->pt_size_chan_diag, &dwReg);

    memset(&g_byTransmitBuffer[0], 0x00, dwReg);
    TPS_SetValueData((USIGN8*)(pzSubslot->pt_chan_diag),
                     &g_byTransmitBuffer[0],
                     dwReg);
    g_pbyCurrentPointer += wNumberOfChannelDiag * sizeof(DPR_DIAG_ENTRY);

    DWORD_ALIGN(g_pbyCurrentPointer);

    /*** IO Process data configurations **/
    pzSubslot->pt_used_in_cr = (USIGN16*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 2;

    /* Input data size                                                      */
    /*----------------------------------------------------------------------*/
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, wSizeOfInputData);
    pzSubslot->pt_size_input_data = (USIGN16*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 2;

    /* buffer for input data */
    pzSubslot->pt_input_data = (USIGN32*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 4;

    /* This data will be written by stack while establishing the connection */
    pzSubslot->pt_offset_input_data = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_offset_input_iocs = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_input_iocs = (USIGN8*)g_pbyCurrentPointer;
    TPS_SetValue8((USIGN8*)g_pbyCurrentPointer, 0x00);
    g_pbyCurrentPointer += 1;

    DWORD_ALIGN(g_pbyCurrentPointer);

    pzSubslot->pt_offset_input_iops = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_operational_state = (USIGN8*)g_pbyCurrentPointer;
    TPS_SetValue8((USIGN8*)g_pbyCurrentPointer, 0x00);
    g_pbyCurrentPointer += 1;

    DWORD_ALIGN(g_pbyCurrentPointer);

    pzSubslot->pt_offset_input_DataStatus = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_input_DataStatus = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    /* shared input data */
    pzSubslot->pt_offset_indata_shared = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_offset_input_iocs_shared=(USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_input_iocs_shared = (USIGN8*)g_pbyCurrentPointer;
    TPS_SetValue8((USIGN8*)g_pbyCurrentPointer, 0x00);
    g_pbyCurrentPointer += 1;

    DWORD_ALIGN(g_pbyCurrentPointer);

    pzSubslot->pt_offset_input_iops_shared=(USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer,0x0000);
    g_pbyCurrentPointer+=2;

    pzSubslot-> pt_operational_state_shared = (USIGN8*)g_pbyCurrentPointer;
    TPS_SetValue8((USIGN8*)g_pbyCurrentPointer, 0x00);
    g_pbyCurrentPointer += 1;

    DWORD_ALIGN(g_pbyCurrentPointer);

    pzSubslot->pt_offset_input_shared_DataStatus = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_wSubslotOwnedByAr = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    /* Output data */
    /* Size_Output_Data (Context Management) */
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, wSizeOfOutputData);
    pzSubslot->pt_size_output_data = (USIGN16*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_output_data = (USIGN32*)g_pbyCurrentPointer;
    g_pbyCurrentPointer += 4;

    pzSubslot->pt_offset_output_data = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_offset_output_iocs = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_output_iocs = (USIGN8*)g_pbyCurrentPointer;
    TPS_SetValue8((USIGN8*)g_pbyCurrentPointer, 0x00);
    g_pbyCurrentPointer += 1;

    DWORD_ALIGN(g_pbyCurrentPointer);

    pzSubslot->pt_offset_output_iops = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_output_iops = (USIGN8*)g_pbyCurrentPointer;
    TPS_SetValue8((USIGN8*)g_pbyCurrentPointer, 0x00);
    g_pbyCurrentPointer += 1;

    DWORD_ALIGN(g_pbyCurrentPointer);

    pzSubslot->pt_offset_output_DataStatus = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_output_DataStatus = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    pzSubslot->pt_properties = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, 0x0000);
    g_pbyCurrentPointer += 2;

    /* Add Sync Application parameters only to DAP subslot*/
    TPS_GetValue16((USIGN8*)pzSlotHandle->pt_slot_number, &wSlotNumber);
    if((wSlotNumber == DAP_MODULE) && (wSubslotNumber == DAP_SUBMODULE))
    {
        pzSubslot->poSyncAppParam = (T_ISOCHRON_PARAMETERS*)g_pbyCurrentPointer;
        TPS_MemSet(g_pbyCurrentPointer, 0, sizeof(T_ISOCHRON_PARAMETERS));
        g_pbyCurrentPointer += sizeof(T_ISOCHRON_PARAMETERS);
        DWORD_ALIGN(g_pbyCurrentPointer);
    }

    pzSubslot->pt_submodule_state = (USIGN16*)g_pbyCurrentPointer;
    TPS_SetValue16((USIGN8*)g_pbyCurrentPointer, SUBMODULE_OK);
    g_pbyCurrentPointer += 2;

    #ifdef DEBUG_API_SUBPLUG_MODULE
        printf("DEBUG_API > API: TPS_PlugSubModule() > Submodule plugged\n");
        printf("DEBUG_API > API: TPS_PlugSubModule() > Memory used 0x%d\n",
               (USIGN32)(g_pbyCurrentPointer - g_pbyConfigNRTMem));
    #endif

    #ifdef DEBUG_API_SUBPLUG_MODULE_PRINT
        TPS_PrintSlotSubslotConfiguration();
    #endif

    pzSubslot->poIM0 = pzIM0Data;

    g_byApiState = STATE_SUBSLOT_RDY;

    return pzSubslot;
}

/*!
 * \brief       This function resets the device configuration event to the TPS-1
 *              only if the device isn't connected
                Call TPS_CleanApiConf() afterwards to initialize the configuration in the application.
 *
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_AR_STILL_ACTIVE
 */
USIGN32 TPS_ResetDeviceConfig(VOID)
{
    if ((g_dwArEstablished_0 != AR_NOT_IN_OPERATION)
        || (g_dwArEstablished_1 != AR_NOT_IN_OPERATION)
        || (g_dwArEstablished_IOSR != AR_NOT_IN_OPERATION))
    {
        return TPS_ERROR_AR_STILL_ACTIVE;
    }
    return AppSetEventRegApp(APP_EVENT_RESET_STACK_CONFIG);
}

/*!
 * \brief       This function registers the I&M Data for the given Slot.
 *
 * \param[in]   pzSubslot handle to the sub slot structure
 * \param[in]   pzIM0Data pointer to the I&M0 data buffer
 * \param[in]   pzIM1Data pointer to the I&M1 data buffer or NULL
 * \param[in]   pzIM2Data pointer to the I&M2 data buffer or NULL
 * \param[in]   pzIM3Data pointer to the I&M3 data buffer or NULL
 * \param[in]   pzIM4Data pointer to the I&M4 data buffer or NULL
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - REGISTER_IM_SUBSLOT_NOT_FOUND
 */
USIGN32 TPS_RegisterIMDataForSubslot(SUBSLOT* pzSubslot, T_IM0_DATA* pzIM0Data, T_IM1_DATA* pzIM1Data,
    T_IM2_DATA* pzIM2Data, T_IM3_DATA* pzIM3Data, T_IM4_DATA* pzIM4Data)
{
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN32 dwApi = 0x00;
    USIGN16 wSlotNumber = 0x00;
    USIGN16 wSubslotNumber = 0x00;

    /* The registration of I&M Data is only allowed on the DAP. */
    dwRetval = AppGetSlotInfoFromHandle(pzSubslot, &dwApi, &wSlotNumber, &wSubslotNumber);
    if (dwRetval != TPS_ACTION_OK)
    {
        dwRetval = REGISTER_IM_SUBSLOT_NOT_FOUND;
    }

    /* Set the data. */
    if ((dwRetval == TPS_ACTION_OK) &&
        (pzIM0Data != NULL))
    {
        pzIM0Data->IM_Supported = IM_NOT_SUPPORTED;
        pzSubslot->poIM0 = pzIM0Data;

        if (pzIM1Data != NULL)
        {
            pzIM0Data->IM_Supported |= IM1_SUPPORTED;
            pzSubslot->poIM0->p_im1 = pzIM1Data;
        }
        else
        {
            pzSubslot->poIM0->p_im1 = NULL;
        }

        if (pzIM2Data != NULL)
        {
            pzIM0Data->IM_Supported |= IM2_SUPPORTED;
            pzSubslot->poIM0->p_im2 = pzIM2Data;
        }
        else
        {
            pzSubslot->poIM0->p_im2 = NULL;
        }

        if (pzIM3Data != NULL)
        {
            pzIM0Data->IM_Supported |= IM3_SUPPORTED;
            pzSubslot->poIM0->p_im3 = pzIM3Data;
        }
        else
        {
            pzSubslot->poIM0->p_im3 = NULL;
        }

        if (pzIM4Data != NULL)
        {
            pzIM0Data->IM_Supported |= IM4_SUPPORTED;
            pzSubslot->poIM0->p_im4 = pzIM4Data;
        }
        else
        {
            pzSubslot->poIM0->p_im4 = NULL;
        }
    }

    return dwRetval;
}

/*!
 * \brief       This function returns the MAC Addresses of the TPS-1.
 *
 * \param[out]  pbyInterfaceMac pointer to the buffer in which the interface MAC address will be stored.
                <b>Buffer must have at least the size MAC_ADDRESS_SIZE
 * \param[out]  pbyPort1Mac pointer to the buffer in which the MAC address of port 1 will be stored.
                <b>Buffer must have at least the size MAC_ADDRESS_SIZE
 * \param[out]  pbyPort2Mac pointer to the buffer in which the MAC address of port 2 will be stored.
                <b>Buffer must have at least the size MAC_ADDRESS_SIZE
 * \note        The size for all buffers has to be MAC_ADDRESS_SIZE or NULL
 * \retval      TPS_ACTION_OK
 */
USIGN32 TPS_GetMacAddresses(USIGN8* pbyInterfaceMac, USIGN8* pbyPort1Mac, USIGN8* pbyPort2Mac)
{
    if (pbyInterfaceMac != NULL)
    {
        TPS_GetValueData(g_pzNrtConfigHeader->byInterfaceMac, pbyInterfaceMac, MAC_ADDRESS_SIZE);
    }

    if (pbyPort1Mac != NULL)
    {
        TPS_GetValueData(g_pzNrtConfigHeader->byPort1Mac, pbyPort1Mac, MAC_ADDRESS_SIZE);
    }

    if (pbyPort2Mac != NULL)
    {
        TPS_GetValueData(g_pzNrtConfigHeader->byPort2Mac, pbyPort2Mac, MAC_ADDRESS_SIZE);
    }

    return(TPS_ACTION_OK);
}

/*!
 * \brief       This function returns the IP configuration of the TPS-1.
 *
 * \param[out]  pdwIPAddress pointer in which the IP address will be stored
 * \param[out]  pdwSubnetMask pointer in which the subnet mask will be stored
 * \param[out]  pdwGateway pointer in which the default gateway will be stored
 * \retval      USIGN32 TPS_ACTION_OK
 */
USIGN32 TPS_GetIPConfig(USIGN32* pdwIPAddress, USIGN32* pdwSubnetMask, USIGN32* pdwGateway)
{
    if (pdwIPAddress != NULL)
    {
        TPS_GetValueData((USIGN8*)&(g_pzNrtConfigHeader->dwIPAddress), (USIGN8*)pdwIPAddress, IP_ADDRESS_SIZE);
    }

    if (pdwSubnetMask != NULL)
    {
        TPS_GetValueData((USIGN8*)&(g_pzNrtConfigHeader->dwSubnetMask), (USIGN8*)pdwSubnetMask, IP_ADDRESS_SIZE);
    }

    if (pdwGateway != NULL)
    {
        TPS_GetValueData((USIGN8*)&(g_pzNrtConfigHeader->dwGateway), (USIGN8*)pdwGateway, IP_ADDRESS_SIZE);
    }

    return(TPS_ACTION_OK);
}

/*!
 * \brief       This function writes the actual name of station of the TPS-1 into buffer pbyName.
 *
 * \param[out]  pbyName pointer to a buffer in which the name is stored.
 * \param[in]   dwBufferLength length of the buffer. The length must be at least STATION_NAME_LEN+1
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_NULL_POINTER
 *              - TPS_ERROR_BUFFER_TOO_SMALL
 */
USIGN32 TPS_GetNameOfStation(USIGN8* pbyName, USIGN32 dwBufferLength)
{
    if (pbyName == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }

    if (dwBufferLength < (STATION_NAME_LEN + 1))
    {
        return TPS_ERROR_BUFFER_TOO_SMALL;
    }

    TPS_GetValueData((USIGN8*)&(g_pzNrtConfigHeader->byStationName), pbyName, STATION_NAME_LEN);
    pbyName[STATION_NAME_LEN] = 0;

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function writes the serial number of the device into buffer pbySerialnumber.
                The Serialnumber is set with the TPS Configurator and must also be used in the I&M data of this application.
 *
 * \param[out]  pbySerialnumber pointer to a buffer in which the serial number will be stored.
 * \param[in]   dwSerialLength length of the buffer. The length must be at least IM0_SERIALNUMBER_LEN+1
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_NULL_POINTER
 *              - TPS_ERROR_BUFFER_TOO_SMALL
 */
USIGN32 TPS_GetSerialnumber(USIGN8 *pbySerialnumber, USIGN32 dwSerialLength)
{
    if (pbySerialnumber == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }
    if (dwSerialLength < (IM0_SERIALNUMBER_LEN + 1))
    {
        return TPS_ERROR_BUFFER_TOO_SMALL;
    }
    TPS_GetValueData((USIGN8*)&(g_pzNrtConfigHeader->bySerialnumber), pbySerialnumber, IM0_SERIALNUMBER_LEN);
    pbySerialnumber[IM0_SERIALNUMBER_LEN] = 0;

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function returns the order Id of the device.
                The order ID is initially set with the TPS Configurator but can also be changed with TPS_SetOrderId().
                The I&M0 data of this application must have the same order ID as returned in this function.
 *
 * \param[out]  pbyOrderId pointer to a buffer in which the orderId will be stored.
 * \param[in]   dwOrderIdLength length of the buffer. The length must be at least IM0_ORDERID_LEN+1
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_NULL_POINTER
 *              - TPS_ERROR_BUFFER_TOO_SMALL
 */
USIGN32 TPS_GetOrderId(USIGN8 *pbyOrderId, USIGN32 dwOrderIdLength)
{
    if (pbyOrderId == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }
    if (dwOrderIdLength < (IM0_ORDERID_LEN + 1))
    {
        return TPS_ERROR_BUFFER_TOO_SMALL;
    }
    TPS_GetValueData((USIGN8*)&(g_pzNrtConfigHeader->byOrderId), pbyOrderId, IM0_ORDERID_LEN);
    pbyOrderId[IM0_ORDERID_LEN] = 0;

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function sets the orderId of the device. If you use this
 *              function the orderId defined with the TPS-Configurator isn't
 *              valid anymore. Instead this new orderId is used.
 *
 * \param[in]   pbyOrderId pointer to a buffer where the new orderId is defined
 * \param[in]   dwOrderIdLength length of the buffer. The length can be up to IM0_ORDERID_LEN bytes
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_NULL_POINTER
 *              - TPS_ERROR_BUFFER_TOO_BIG
 */
USIGN32 TPS_SetOrderId(USIGN8 *pbyOrderId, USIGN32 dwOrderIdLength)
{
    char tempBuf[IM0_ORDERID_LEN] = {0};

    if (pbyOrderId == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }
    if (dwOrderIdLength > IM0_ORDERID_LEN)
    {
        return TPS_ERROR_BUFFER_TOO_BIG;
    }
    memset(tempBuf, 0x20, IM0_ORDERID_LEN);
    memcpy(tempBuf, pbyOrderId, dwOrderIdLength);
    return TPS_SetValueData((USIGN8*)&(g_pzNrtConfigHeader->byOrderId), (USIGN8*)tempBuf, IM0_ORDERID_LEN);
}

#ifndef USE_INT_APP
/*!
 * \brief       This function changes the name of station. This is the name of the interface that uniquely defines the device in the network.
 *
 * \param[in]   pbyNameBuf pointer to the name of station string buffer
 * \param[in]   dwLenOfName length of the name of station
 * \param[in]   byAttr DCP_SET_PROTECTED: The name of station cannot be changed by dcp.set requests, DCP_SET_ALLOWED: The name of station can be changed by dcp.set requests
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_NULL_POINTER
 *              - TPS_ERROR_WRONG_API_STATE
 *              - API_DEVICE_NAME_TOO_LONG
 *              - API_WRONG_PARAMETER
 */
USIGN32 TPS_SetNameOfStation(USIGN8* pbyNameBuf, USIGN32 dwLenOfName, USIGN8 byAttr)
{
    USIGN8 byAccessMode = 0;
    USIGN8 byNameOfStationBuff[STATION_NAME_LEN];

    if (g_pzNrtConfigHeader == NULL)
    {
        return TPS_ERROR_WRONG_API_STATE;
    }
    else if (dwLenOfName > STATION_NAME_LEN)
    {
        return API_DEVICE_NAME_TOO_LONG;
    }
    else if ((byAttr != DCP_SET_PROTECTED) && (byAttr != DCP_SET_ALLOWED))
    {
        return API_WRONG_PARAMETER;
    }
    else if (pbyNameBuf == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }

    /* get actual value */
    TPS_GetValue8((USIGN8*)&(g_pzNrtConfigHeader->byAppConfMode), &byAccessMode);
    byAccessMode = (byAccessMode & 0xFE) | (byAttr & 0x1);

    /* reset name data */
    memset(&byNameOfStationBuff, 0, STATION_NAME_LEN);
    TPS_SetValueData((USIGN8*)&(g_pzNrtConfigHeader->byStationName), (USIGN8*)&byNameOfStationBuff, STATION_NAME_LEN);

    /* write the name of device */
    TPS_SetValueData((USIGN8*)&(g_pzNrtConfigHeader->byStationName), pbyNameBuf, dwLenOfName);
    TPS_SetValue8((USIGN8*)&(g_pzNrtConfigHeader->byAppConfMode), byAccessMode);


    return TPS_ACTION_OK;
}

/*!
 * \brief       This function temporarily changes the IP configuration of the device. The new IP configuration is not written into flash, so after a reboot
 *              the last known IP Address is used. With the parameter DCP_SET_PROTECTED it's also possible to prevent setting of new IP addresses from external
 *              sources.
 *              <br><br>If you want to change the IP configuration permanently, please refer to TPS_DcpSetDeviceIP().
 *
 * \param[in]   dwIPAddr the IP address
 * \param[in]   dwSubNetMask the subnet mask
 * \param[in]   dwGateway the gateway
 * \param[in]   byAttr DCP_SET_PROTECTED: The IP configuration cannot be changed by dcp.set requests, DCP_SET_ALLOWED: The IP configuration can be changed by dcp.set requests
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - API_WRONG_ATTR_PARAMETER
 */
USIGN32 TPS_SetIPConfig(USIGN32 dwIPAddr, USIGN32 dwSubNetMask, USIGN32 dwGateway, USIGN8 byAttr)
{
    USIGN8 byAccessMode = 0;

    if (g_pzNrtConfigHeader == NULL)
    {
        return TPS_ERROR_WRONG_API_STATE;
    }
    else if ((byAttr != DCP_SET_PROTECTED) && (byAttr != DCP_SET_ALLOWED))
    {
        return API_WRONG_ATTR_PARAMETER;
    }

    TPS_GetValue8((USIGN8*)&(g_pzNrtConfigHeader->byAppConfMode), &byAccessMode);
    byAccessMode = (byAccessMode & 0xFD) | (byAttr << 1);

    TPS_SetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwIPAddress), dwIPAddr);
    TPS_SetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwSubnetMask), dwSubNetMask);
    TPS_SetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwGateway), dwGateway);

    TPS_SetValue8((USIGN8*)&(g_pzNrtConfigHeader->byAppConfMode), byAccessMode);

    return (TPS_ACTION_OK);
}
#endif /*USE_INT_APP*/

/*!
 * \brief       This function deletes all slot/subslot structures.
                It can be used for cleaning up a previously configured API configuration.
                Should always be called after TPS_ResetDeviceConfig()
 *
 * \retval      TPS_ACTION_OK
 */
USIGN32 TPS_CleanApiConf(VOID)
{
    API_LIST* pzApi = g_zApiARContext.api_list;
    API_LIST* pzApiNext = NULL;
    SLOT* pzSlot = NULL;
    SLOT* pzSlotNext = NULL;
    SUBSLOT* pzSubslot = NULL;
    SUBSLOT* pzSubslotNext = NULL;

    while (pzApi != NULL)
    {
        pzApiNext = pzApi->next;

        pzSlot = pzApi->firstslot;

        while (pzSlot != NULL)
        {
            pzSlotNext = pzSlot->pNextSlot;

            pzSubslot = pzSlot->pSubslot;

            while (pzSubslot != NULL)
            {
                pzSubslotNext = pzSubslot->poNextSubslot;

                AppFactoryResetForIM1_4();

                pzSubslot->poNextSubslot = NULL;
                /*free(pzSubslot);*/
                pzSubslot = pzSubslotNext;
            }

            pzSlot->pNextSlot = NULL;
            pzSlot->pSubslot = NULL;
            /*free(pzSlot);*/
            pzSlot = pzSlotNext;
        }

        pzApi->firstslot = NULL;
        pzApi->next = NULL;
        /*free(pzApi);*/
        pzApi = pzApiNext;

    }

    lib_reset_malloc();
    g_zApiARContext.api_list = NULL;
    g_byApiState = STATE_INITIAL;

#ifdef DIAGNOSIS_ENABLE
    g_wNumberOfDiagEntries = 0;
#endif

    return TPS_ACTION_OK;
}

/*!@} Configuration Interface*/

/*! \addtogroup eventCom Event Communication Interface
 *@{
 */

/*!
 * \brief       This function is called from an ISR routine or is used for polling the event register (events from the TPS-1
 *              to the host CPU. The function checks each possible event from the TPS Stack and calls the previously registered
                callback functions.
 *
 * \param[in]   VOID
 * \retval      USIGN32 TPS_ACTION_OK
 */
USIGN32 TPS_CheckEvents(VOID)
{
    USIGN32 dwEventRegValue = 0;

    TPS_GetValue32(((USIGN8*)EVENT_REGISTER_TPS), &dwEventRegValue);

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONCONNECTDONE_IOAR0)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: AppOnConnectDone Event 0\r\n");
#endif
        AppOnConnect_Done(AR_0);
        AppSetEventRegAppAckn(TPS_EVENT_ONCONNECTDONE_IOAR0);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONCONNECTDONE_IOAR1)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: AppOnConnectDone Event 1\r\n");
#endif
        AppOnConnect_Done(AR_1);
        AppSetEventRegAppAckn(TPS_EVENT_ONCONNECTDONE_IOAR1);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONCONNECTDONE_IOSAR)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: AppOnConnectDone Event IOSAR\r\n");
#endif
        AppOnConnect_Done(AR_IOSR);
        AppSetEventRegAppAckn(TPS_EVENT_ONCONNECTDONE_IOSAR);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_PRM_END_DONE_IOAR0)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: AppOnPRMENDDone Event 0\r\n");
#endif
        AppOnPRMENDDone(AR_0);
        AppSetEventRegAppAckn(TPS_EVENT_ON_PRM_END_DONE_IOAR0);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_PRM_END_DONE_IOAR1)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: AppOnPRMENDDone Event 1\r\n");
#endif
        AppOnPRMENDDone(AR_1);
        AppSetEventRegAppAckn(TPS_EVENT_ON_PRM_END_DONE_IOAR1);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_PRM_END_DONE_IOSAR)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: AppOnPRMENDDone Event IOSAR\r\n");
#endif
        AppOnPRMENDDone(AR_IOSR);
        AppSetEventRegAppAckn(TPS_EVENT_ON_PRM_END_DONE_IOSAR);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONABORT_IOAR0)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnAbort Event 0\r\n");
#endif
        AppOnAbort(AR_0);
        AppSetEventRegAppAckn(TPS_EVENT_ONABORT_IOAR0);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONABORT_IOAR1)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnAbort Event 1\r\n");
#endif
        AppOnAbort(AR_1);
        AppSetEventRegAppAckn(TPS_EVENT_ONABORT_IOAR1);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONABORT_IOSAR)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnAbort Event IOSAR\r\n");
#endif
        AppOnAbort(AR_IOSR);
        AppSetEventRegAppAckn(TPS_EVENT_ONABORT_IOSAR);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONREADRECORD)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnReadRecord Event\r\n");
#endif
        AppSetEventRegAppAckn(TPS_EVENT_ONREADRECORD);
        AppOnReadRecord();
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONWRITERECORD)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnWriteRecord Event\r\n");
#endif

        AppSetEventRegAppAckn(TPS_EVENT_ONWRITERECORD);
        AppOnWriteRecord();
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONALARM_ACK_0)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnAlarmAck_0 Event\r\n");
#endif
        AppOnAlarmAck();
        AppSetEventRegAppAckn(TPS_EVENT_ONALARM_ACK_0);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONDIAG_ACK)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnDiagAck Event\r\n");
#endif
        AppOnDiagAckn();
        AppSetEventRegAppAckn(TPS_EVENT_ONDIAG_ACK);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONCONNECT_REQ_REC_0)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnConnect_0 Event\r\n");
#endif
        AppOnConnect(AR_0);
        AppSetEventRegAppAckn(TPS_EVENT_ONCONNECT_REQ_REC_0);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONCONNECT_REQ_REC_1)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnConnect_1 Event\r\n");
#endif
        AppOnConnect(AR_1);
        AppSetEventRegAppAckn(TPS_EVENT_ONCONNECT_REQ_REC_1);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONCONNECT_REQ_REC_2)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnConnect_2 Event\r\n");
#endif
        AppOnConnect(AR_IOSR);
        AppSetEventRegAppAckn(TPS_EVENT_ONCONNECT_REQ_REC_2);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_SET_DEVNAME_TEMP)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnSetStationName Event\r\n");
#endif
        AppOnSetStationName(DCP_SET_TEMPORARY);
        AppSetEventRegAppAckn(TPS_EVENT_ON_SET_DEVNAME_TEMP);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_SET_IP_PERM)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnSetStationIpAddr permanent Event\r\n");
#endif
        AppOnSetStationIpAddr(DCP_SET_PERMANENT);
        AppSetEventRegAppAckn(TPS_EVENT_ON_SET_IP_PERM);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_SET_IP_TEMP)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnSetStationIpAddr temporary Event\r\n");
#endif
        AppOnSetStationIpAddr(DCP_SET_TEMPORARY);
        AppSetEventRegAppAckn(TPS_EVENT_ON_SET_IP_TEMP);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONDCP_BLINK_START)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnSetStationDcpSignal start Event\r\n");
#endif
        AppOnSetStationDcpSignal();
        AppSetEventRegAppAckn(TPS_EVENT_ONDCP_BLINK_START);
    }

    if ((dwEventRegValue & (0x1 <<TPS_EVENT_ONDCP_RESET_TO_FACTORY)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnSetStation factory settings Event\r\n");
#endif
        AppOnResetFactorySettings();
#ifndef USE_TPS1_TO_SAVE_IM_DATA
        AppSetEventRegAppAckn(TPS_EVENT_ONDCP_RESET_TO_FACTORY);
#endif
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ONALARM_ACK_1)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnAlarmAck_1 Event\r\n");
#endif
        AppOnAlarmAck();
        AppSetEventRegAppAckn(TPS_EVENT_ONALARM_ACK_1);
    }

#ifdef USE_ETHERNET_INTERFACE
    if ((dwEventRegValue & (0x1 << TPS_EVENT_ETH_FRAME_REC)) != 0)
    {
#ifdef DEBUG_API_ETH_FRAME
        printf("DEBUG_API > API: OnReceiveEthFrame Event\r\n");
#endif
        AppSetEventRegAppAckn(TPS_EVENT_ETH_FRAME_REC);
        AppOnEthernetReceive();
    }
#endif

    if ((dwEventRegValue & (0x1 << TPS_EVENT_TPS_MESSAGE)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnTPSMessageReceive Event\r\n");
#endif
        AppSetEventRegAppAckn(TPS_EVENT_TPS_MESSAGE);

        AppOnTPSMessageReceive();
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_RESET)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnTPSReset Event\r\n");
#endif
        AppOnTPSReset();

        AppSetEventRegAppAckn(TPS_EVENT_RESET);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_LED_STATE_CHANGE)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnLedStatechange Event\r\n");
#endif


        AppSetEventRegAppAckn(TPS_EVENT_ON_LED_STATE_CHANGE);
        AppOnTPSLedChanged();
    }
    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_SET_DEVNAME_PERM)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnSetStationName Event\r\n");
#endif
        AppOnSetStationName(DCP_SET_PERMANENT);
        AppSetEventRegAppAckn(TPS_EVENT_ON_SET_DEVNAME_PERM);
    }

    if ((dwEventRegValue & (0x1 << TPS_EVENT_ON_FSUDATA_CHANGE)) != 0)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: OnSetStationName Event\r\n");
#endif

        AppOnFSUParameterChange();

        AppSetEventRegAppAckn(TPS_EVENT_ON_FSUDATA_CHANGE);
    }


    return TPS_ACTION_OK;
}

/*!
 * \brief       This function delivers the last error code of an API function.
 *
 * \retval      the last error code
 */
USIGN32 TPS_GetLastError(VOID)
{
    return (g_dwLastErrorCode);
}

/*!
 * \brief       This function checks if the TPS stack has started correctly after
 *              reset or power up.
 *              This function should be called several times until it returns
 *              TPS_ACTION_OK to wait for the TPS stack to start.
 *
 * \param[in]   VOID
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_STACK_VERSION_FAILED
 */
USIGN32 TPS_CheckStackStart(VOID)
{
    USIGN32 dwReturn     = TPS_ACTION_OK;
    USIGN32 dwValue      = 0;   /* Return value for TPS_GetValue32().        */

    /* Set addresses for header!                                             */
    /*-----------------------------------------------------------------------*/
    g_pzNrtConfigHeader = (NRT_APP_CONFIG_HEAD*)BASE_ADDRESS_NRT_AREA;


    /* Wait for the TPS-1 protocol stack.                                    */
    /*-----------------------------------------------------------------------*/
    dwReturn = TPS_GetValue32((USIGN8*)(g_pzNrtConfigHeader), &dwValue);
    if (dwReturn != TPS_ACTION_OK)
    {
        return (dwReturn);
    }

    /* Check if the stack has started.                                       */
    /*-----------------------------------------------------------------------*/
    if ( (dwValue & 0xFFFF0000) == STACK_START_NUMBER)
    {
        /* Check for the right stack version                                 */
        /*-------------------------------------------------------------------*/
        if ( (dwValue & 0x0000FFFF) == STACK_VERSION_NUMBER )
        {
            return (TPS_ACTION_OK);
        }
        else
        {
            return (TPS_ERROR_STACK_VERSION_FAILED);
        }
    }
#ifdef FW_UPDATE_OVER_HOST
    else if((dwValue == UPDATER_LIFE_SIGN) || (dwValue == HOST_LIFE_SIGN))
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: try to reset updater\n");
#endif
        TPS_StartFwStack();
        return TPS_ERROR_STACK_START_FAILED;
    }
#endif
    else
    {
        return (TPS_ERROR_STACK_START_FAILED);
    }
}

/*!
 * \brief       This function registers all callback functions that process the context management and the record interface.
 *
 * \param[in]   which event type ( ONCONNECT_CB, ONCONNECTDONE_CB, ONABORT_CB,
                ONREADRECORD_CB, ONWRITERECORD_CB, ON_PRM_END_DONE_CB,
                ONRESET_CB)
 * \param[in]   pfnFunc pointer to the callback function that shall be linked to the event.
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - REGISTER_FUNCTION_NULL
 *              - REGISTER_RPC_ERROR
 */
USIGN32 TPS_RegisterRpcCallback(USIGN8 which, VOID(*pfnFunc)(USIGN32))
{
    if (pfnFunc == NULL)
    {
        return REGISTER_FUNCTION_NULL;
    }

    switch (which)
    {
    case ONCONNECT_CB:
        g_zApiARContext.OnConnect_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnConnect CB function\n");
#endif
        break;

    case ONCONNECTDONE_CB:
        g_zApiARContext.OnConnectDone_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnConnectDone CB function\n");
#endif
        break;

    case ONABORT_CB:
        g_zApiARContext.OnAbort_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnAbort CB function\n");
#endif
        break;

    case ONREADRECORD_CB:
        g_zApiARContext.OnReadRecord_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnReadRecord CB function\n");
#endif
        break;

    case ONWRITERECORD_CB:
        g_zApiARContext.OnWriteRecord_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnWriteRecord CB function\n");
#endif
        break;

    case ON_PRM_END_DONE_CB:
        g_zApiARContext.OnPRMEND_Done_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered On Parameter End Done CB function\n");
#endif
        break;

    case ONREAD_CB:
        g_zApiARContext.OnRead = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnRead CB function\n");
#endif
        break;

    case ONWRITE_DONE_CB:
        g_zApiARContext.OnWriteDone = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnWrite CB function\n");
#endif
        break;

    case ONRESET_CB:
    {
        g_zApiARContext.OnReset_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnReset CB function\n");
#endif
        break;
    }
    case ONFSUPARAM_CB:
    {
        g_zApiARContext.OnFSUParamChange_CB = pfnFunc;
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: registered OnFsuParameterChange CB function\n");
#endif
        break;
    }
    default:
        return(REGISTER_RPC_ERROR);
    }  /* End of switch */

    return (TPS_ACTION_OK);
}

/*!
 * \brief       This function registers all callback functions that process the alarm interface (ONALARM_CB, ONDIAGN_CB).
 *
 * \param[in]   byWhich event type (ONALARM_CB or ONDIAG_CB)
 * \param[in]   pfnFunction pointer to the callback function that shall be linked to the event.
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - REGISTER_FUNCTION_NULL
 *              - REGISTER_ALARM_ERROR
 */
USIGN32 TPS_RegisterAlarmDiagCallback(USIGN8 byWhich, VOID(*pfnFunction)(USIGN16, USIGN32))
{
    if (pfnFunction == NULL)
    {
        return REGISTER_FUNCTION_NULL;
    }

    switch (byWhich)
    {
     case ONALARM_CB:
       {
          g_zApiARContext.OnAlarm_Ack_CB = pfnFunction;
#ifdef DEBUG_API_TEST
          printf("DEBUG_API > API: registered OnAlarm CB function\n");
#endif
       }
       break;

     case ONDIAG_CB:
       {
       g_zApiARContext.OnDiag_Ack_CB = pfnFunction;
#ifdef DEBUG_API_TEST
       printf("DEBUG_API > API: registered OnDiag CB function\n");
#endif
       }
       break;

     default:
       return REGISTER_ALARM_ERROR;
    }

    return TPS_ACTION_OK;
}

/*!
* \brief       This function registers the DCP callback functions that have one parameter (Set IP, FactoryReset).
*
* \param[in]   byWhich (ONDCP_SET_IP_ADDR_CB or ONDCP_FACT_RESET_CB)
* \param[in]   pfnFunction callback function pointer
* \retval      USIGN32
*              - TPS_ACTION_OK : success
*              - REGISTER_FUNCTION_NULL
*              - REGISTER_DCP_ERROR
*/
USIGN32 TPS_RegisterDcpCallbackPara(USIGN8 byWhich, VOID(*pfnFunction)(USIGN32))
{
   if (pfnFunction == NULL)
   {
      return REGISTER_FUNCTION_NULL;
   }

   switch (byWhich)
   {
    case ONDCP_SET_IP_ADDR_CB:
      {
         g_zApiDeviceContext.OnSetStationIpAddr_CB = pfnFunction;
#ifdef DEBUG_API_TEST
         printf("DEBUG_API > API: registered OnSetStationIpAddr_CB function\n");
#endif
      }
      break;
    case ONDCP_SET_NAME_CB:
      {
         if(g_zApiDeviceContext.OnSetStationName_CB == NULL)
         {
            g_zApiDeviceContext.OnSetStationNameTP_CB = pfnFunction;
         }
         else
         {
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > API: ERROR by registering of OnSetStationName_CB function\n");
#endif
            return (REGISTER_DCP_ERROR);
         }
#ifdef DEBUG_API_TEST
         printf("DEBUG_API > API: registered OnSetStationName_CB function\n");
#endif
      }
      break;
    case ONDCP_FACT_RESET_CB:
      {
         g_zApiDeviceContext.OnResetFactorySettings_CB = pfnFunction;
#ifdef DEBUG_API_TEST
         printf("DEBUG_API > API: registered OnResetFactorySettings_CB function\n");
#endif
      }
      break;

    default:
      return (REGISTER_DCP_ERROR);
   } /* End of switch */

   return TPS_ACTION_OK;
}

/*!
 * \brief       This function registers all callback functions that
 *              processes the DCP interface (Set Name, Set IO, FactoryReset, Flashing).
 *
 * \param[in]   byWhich event type (ONDCP_SET_NAME_CB or ONDCP_BLINK_CB)
 * \param[in]   pfnFunction pointer to the callback function that shall be linked to the event.
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - REGISTER_FUNCTION_NULL
 *              - REGISTER_DCP_ERROR
 */
USIGN32 TPS_RegisterDcpCallback(USIGN8 byWhich, VOID(*pfnFunction)(VOID))
{
   if (pfnFunction == NULL)
   {
      return REGISTER_FUNCTION_NULL;
   }

   switch (byWhich)
   {
    case ONDCP_SET_NAME_CB:
      {
         if(g_zApiDeviceContext.OnSetStationNameTP_CB == NULL)
         {
            g_zApiDeviceContext.OnSetStationName_CB = pfnFunction;
         }
         else
         {
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > API: ERROR by registering of OnSetStationName_CB function\n");
#endif
            return (REGISTER_DCP_ERROR);
         }
#ifdef DEBUG_API_TEST
         printf("DEBUG_API > API: registered OnSetStationName_CB function\n");
#endif
      }
      break;

    case ONDCP_BLINK_CB:
      {
         g_zApiDeviceContext.OnSetStationDcpSignal_CB = pfnFunction;
#ifdef DEBUG_API_TEST
         printf("DEBUG_API > API: registered OnSetStationDcpSignal_CB function\n");
#endif
      }
      break;

    default:
      return (REGISTER_DCP_ERROR);
   } /* End of switch */

   return TPS_ACTION_OK;
}

#ifdef USE_TPS_COMMUNICATION_CHANNEL
/*!
 * \brief       This function registers a callback function that processes the TPS_EVENT_TPS_MESSAGE event.
                See TPS_InitTPSComChannel().
 *
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \param[in]   pfnFunction
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - REGISTER_FUNCTION_NULL
 */
USIGN32 TPS_RegisterTPSMessageCallback(void(*pfnFunction)(T_ETHERNET_MAILBOX*))
{
    if (pfnFunction == NULL)
    {
        return REGISTER_FUNCTION_NULL;
    }

    g_oApiTpsMsgContext.OnTpsMessageRX_CB = pfnFunction;

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > API: registered OnTpsMessageRX_CB function\n");
#endif

    return (TPS_ACTION_OK);
}
#endif
/*!
 * \brief       This function registers a callback function that
 *              processes the TPS_EVENT_ETH_FRAME_REC event.
 *
 * \note        To use this function <b>USE_ETHERNET_INTERFACE</b> in TPS_1_user.h must be defined.
 * \param[in]   pfnFunction callback function pointer
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - REGISTER_FUNCTION_NULL
 */
#ifdef USE_ETHERNET_INTERFACE
USIGN32 TPS_RegisterEthernetReceiveCallback(void(*pfnFunction)(T_ETHERNET_MAILBOX*))
{
    if (pfnFunction == NULL)
    {
        return REGISTER_FUNCTION_NULL;
    }

    g_zApiEthernetContext.OnEthernetFrameRX_CB = pfnFunction;

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > API: registered OnEthernetFrameRX_CB function\n");
#endif

    return (TPS_ACTION_OK);
}
#endif
/*!
* \brief        This function registers the callback function that is called when RecordInputDataObjectElement (index 0x8028)
                RecordOutputDataObjectElement (index 0x8028) for a specific subslot is read.
                These Record Indexes must contain the actual process data of this subslot.
 *
 * \param[in]   pfnFunction pointer to the callback function that shall be linked to the event.
 * \retval      USIGN32
 *               - TPS_ACTION_OK : success
 *               - REGISTER_FUNCTION_NULL
 * \note        These correct responses to these record request are mandatory for the PROFINET certification.
 */
USIGN32 TPS_RegisterRecordDataObjectCallback(USIGN32(*pfnFunction)(T_RECORD_DATA_OBJECT_TYPE zObjectToRead,
    SUBSLOT* pzSubslot,
    USIGN8* pbyIocs, USIGN8* pbyIops,
    USIGN8* pbyData, USIGN16 wDatalength,
    USIGN16* pwSubstituteActiveFlag))
{
    if (pfnFunction == NULL)
    {
        return REGISTER_FUNCTION_NULL;
    }

    g_zApiARContext.OnReadRecordDataObject_CB = pfnFunction;

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function forces the TPS stack to reset (SW reset).
 *
 * \retval      VOID
 */
VOID TPS_SoftReset(VOID)
{
    AppSetEventRegApp(APP_EVENT_RESET_PN_STACK);
#ifdef DEBUG_API_TEST
    printf("DEBUG_API > API: TPS Stack Reset Event was set!\n");
#endif
}

/*!@} Event Communication Interface*/

 /*! \addtogroup  iodata Cyclic Data Interface (IO data)
 *@{
*/

/*!
 * \brief       This function returns the connection state of an application relation (AR).
 *
 * \param[in]   dwARNumber (AR_0, AR_1, AR_IOSR - number of AR)
 * \retval      USIGN32
 *              - AR_ESTABLISHED
 *              - AR_NOT_IN_OPERATION
 */
USIGN32 TPS_GetArEstablished(USIGN32 dwARNumber)
{
    USIGN32 dwConnectState = TPS_ACTION_OK;

    switch (dwARNumber)
    {
    case AR_0:
        dwConnectState = g_dwArEstablished_0;
        break;
    case AR_1:
        dwConnectState = g_dwArEstablished_1;
        break;
    case AR_IOSR:
        dwConnectState = g_dwArEstablished_IOSR;
        break;
    default:
        dwConnectState = 0xFFFF;
        break;
    }

    return(dwConnectState);
}


/*!
 * \brief       This function reads the output data out of the cyclic data frame that is stored in the active Output Buffer in the IO-RAM.
                There are three Output buffers for cyclic frames. The active buffer is changed with TPS_UpdateOutputData().
 *
 * \param[in]   pzSubslot handle to the subslot
 * \param[in]   pbyData pointer to the data buffer where the output data shall be stored
 * \param[in]   wDataLength length of the data buffer ( pbyData )
 * \param[out]  pbyState IOPS of this submodule that is received in the cyclic data frame is written here. The data are only valid if pbyState == IOXS_GOOD.
 * \retval      USIGN32
 *              - on success the function will return the length of the data
 *              - possible errors that can be get with TPS_GetLastError():
                  API_IODATA_NULL_POINTER, API_IODATA_MODULE_HAS_NO_OUTPUTS, API_IODATA_WRONG_BUFFER_LENGTH
 */
USIGN32 TPS_ReadOutputData(SUBSLOT* pzSubslot, USIGN8* pbyData, USIGN16 wDataLength, USIGN8* pbyState)
{
    USIGN8* pbyIOMemory = NULL;
    USIGN16 wOffsetValueData = 0;
    USIGN16 wSizeOutputData = 0;

    /* Check the pointer                                                    */
    /*----------------------------------------------------------------------*/
    if ((pzSubslot == NULL) || (pbyData == NULL))
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_ReadOutputData NULL pointer");
#endif
        AppSetLastError(API_IODATA_NULL_POINTER);
        return 0;
    }

    if (wDataLength == 0x00)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_ReadOutputData no output module.");
#endif
        AppSetLastError(API_IODATA_MODULE_HAS_NO_OUTPUTS);
        return 0;
    }

    /* Get size of data                                                     */
    /*----------------------------------------------------------------------*/
    TPS_GetValue16((USIGN8*)(pzSubslot->pt_size_output_data),
        &wSizeOutputData);

    /* Check the data length                                                  */
    /*----------------------------------------------------------------------*/
    if (wSizeOutputData != wDataLength)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: Wrong buffer length. Given: 0x%X, Expected: 0x%X\n",
            wDataLength, wSizeOutputData);
#endif
        AppSetLastError(API_IODATA_WRONG_BUFFER_LENGTH);
        return 0;
    }

    /* Get the pointer to the output data!                                  */
    /*----------------------------------------------------------------------*/
    TPS_GetIOAddress((USIGN8*)(pzSubslot->pt_output_data), &pbyIOMemory);

    /* Get the offset from beginning IO Area to output data!                */
    /*----------------------------------------------------------------------*/
    TPS_GetValue16((USIGN8*)(pzSubslot->pt_offset_output_data), &wOffsetValueData);
    /* Add offset to memory pointer!                                        */
    /*----------------------------------------------------------------------*/
    pbyIOMemory += wOffsetValueData;

#ifdef DEBUG_API_TEST
    /* printf("DEBUG_API > API: Address of Output Data (IO-Area): 0x%x\n", (USIGN32)pbyIOMemory);
    printf("Size Output Data (IO-Area): %d\n",wSizeOutputData); */
#endif

    /* Get the data out of the DPRAM.                                       */
    /*----------------------------------------------------------------------*/
    TPS_GetValueData(pbyIOMemory, pbyData, wSizeOutputData);

    /* Get State Byte from the DPRAM                                        */
    /*----------------------------------------------------------------------*/
    TPS_GetValue8((USIGN8*)(pbyIOMemory + wSizeOutputData), pbyState);

    return wSizeOutputData; /* Return size of data!                         */
}


/*!
 * \brief       This function initiates a buffer change of the cyclic output data to get the latest output data.
                The function returns after the buffer was successfully changed.
                There are three output buffers for each AR.
 *
 * \param[in]   byARNumber the AR which will be updated (AR_0 or AR_1)
 * \note        Do not change this function!
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_WRONG_AR_NUMBER
 */
USIGN32 TPS_UpdateOutputData(USIGN8 byARNumber)
{
    USIGN32 dwRegValue = 0;
    USIGN32 dwReturnValue = 0;

    if ((byARNumber != AR_0) && (byARNumber != AR_1))
    {
        return API_WRONG_AR_NUMBER;
    }

    /* Set register data for buffer change.                                 */
    /*----------------------------------------------------------------------*/
    dwRegValue = (1 << 6) | (1 << 5) | ((2 * byARNumber) + 2);

    /* Initate the buffer change!                                           */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)BASE_ADDRESS_DPRAM, dwRegValue);

    /* Wait for the acknowledge!                                            */
    /*----------------------------------------------------------------------*/
    do
    {
        TPS_GetValue32((USIGN8*)(BASE_ADDRESS_DPRAM + 4), &dwReturnValue);
    } while ((dwReturnValue & dwRegValue) != (dwRegValue ^ (1 << 5)));

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function reads the APDU state from the output data buffer.
 *
 * \param[in]   byARNumber the number of the AR
 * \retval      possible return values:
 *              - USIGN16 : the APDU state.
 *                          Use the GET_APDU_STATUS macros to decode the APDU state.
 *
 *              - API_WRONG_AR_NUMBER if AR number is not equal AR_0 or AR_1:
 *               \sa
 *               - GET_APDU_STATUS_STATE
 *               - GET_APDU_STATUS_REDUNDANCY
 *               - GET_APDU_STATUS_DATAVALID
 *               - GET_APDU_STATUS_PROVIDERSTATE
 *               - GET_APDU_STATUS_PROBLEMINDICATOR
 *               - GET_APDU_STATUS_IGNORE
 */
USIGN32 TPS_GetAPDUStatusOutCR(USIGN8 byARNumber, APDU_STATUS* ptApduStatus)
{
   USIGN16 wReturnValue = 0;
   USIGN8* pbyIOMemory = NULL;

   if ((byARNumber != AR_0) && (byARNumber != AR_1))
   {
      return API_WRONG_AR_NUMBER;
   }

   pbyIOMemory = g_pbyApduAddr[byARNumber];

   if( pbyIOMemory != NULL )
   {
      TPS_GetValueData(pbyIOMemory, (USIGN8*)ptApduStatus, sizeof(APDU_STATUS));
   }

   return wReturnValue;
}

/*!
 * \brief       This function reads the CR dependent length of the IO data.
 *
 * \param[in]   byARNumber the number of the AR
 * \param[in]   byCrType the Type of CR (input / output)
 * \retval      possible return values:
 *              - USIGN16 : length of data buffer
 *               - API_WRONG_AR_NUMBER if AR number is not equal AR_0 or AR_1:
 */
USIGN16 TPS_GetIOBufLength(USIGN8 byARNumber, USIGN8 byCrType)
{
   USIGN32 dwReturnValue = 0;
   USIGN8  byShift = (byCrType == OUTPUT_USED)?16:0;

   if ((byARNumber != AR_0) && (byARNumber != AR_1))
   {
      return API_WRONG_AR_NUMBER;
   }

   TPS_GetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwIOBufferLenAr[byARNumber] ), &dwReturnValue);

   return (USIGN16)(dwReturnValue >> byShift);

}

/*!
* \brief       This function writes the input data and the IOPS in the active cyclic data frame in the IO RAM for one subslot.
               There are three Input buffers for cyclic frames. The active buffer is changed with TPS_UpdateInputData().
               This function as well as TPS_SetOutputIocs() must be called for each configured subslot before changing the
               buffer with TPS_UpdateInputData() to create valid cyclic data for the application relation.
*
* \param[in]   pzSubslot handle to the sub slot structure
* \param[in]   pbyData pointer to the input data buffer
* \param[in]   wDataLength length of the input data
* \param[in]   byState state of the input data (e.g. IOXS_GOOD)
* \retval      returns the actual size of the input data or 0 on error
* \note        In the error case use TPS_GetLastError() to retrieve the error code.
*              Possible return values:\n
*              - API_IODATA_NULL_POINTER
*              - API_IODATA_NOT_USED_IN_CR
*              - API_IODATA_WRONG_BUFFER_LENGTH
*              - API_IODATA_INPUT_FRAME_BUFFER
*/
USIGN32 TPS_WriteInputData(SUBSLOT *pzSubslot, USIGN8 *pbyData, USIGN16 wDataLength, USIGN8 byState)
{
    USIGN8*  pbyIOMemory = NULL;
    USIGN16  wOffsetInputData = 0x0;
    USIGN16  wOffsetInputIops = 0x0;
    USIGN16  wSizeInputData = 0x0;
    USIGN16  wUsedInCr = 0x0;
#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    USIGN16 wSubslotProperties = 0x0;
#endif

    /* Check the pointers */ /* Allow (pbyData == NULL) if data length is zero */
    if ((pzSubslot == NULL) ||
        ((pbyData == NULL) && (wDataLength > 0)))
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_WriteInputData NULL pointer\n");
#endif
        AppSetLastError(API_IODATA_NULL_POINTER);
        return 0;
    }

    /* check if this subslot is used in a CR */
    TPS_GetValue16((USIGN8*)pzSubslot->pt_used_in_cr, &wUsedInCr);

    if ((wUsedInCr & INPUT_USED) == SUBSLOT_NOT_USED)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_WriteInputData Subslot is not used in a CR\n");
#endif

        AppSetLastError(API_IODATA_NOT_USED_IN_CR);
        return 0;
    }

    /* If the Submodule is pulled, only bad is allowed. */
#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    TPS_GetValue16((USIGN8*)pzSubslot->pt_properties, &wSubslotProperties);
    if ((wSubslotProperties & SUBMODULE_PROPERTIES_MASK) == SUBMODULE_PULLED)
    {
#ifdef DEBUG_API_AUTOCONF
        printf("DEBUG_API > API: TPS_WriteInputData() > Error: Module is pulled, forcing IOPS BAD_BY_SUBSLOT.\n");
#endif
        byState = IOXS_BAD_BY_SUBSLOT;
    }
#endif

    if (byState == IOXS_GOOD)
    {
        /* overwrite IOPS in case of wrong submodule                  */
        TPS_GetValue8((USIGN8*)(pzSubslot->pt_operational_state), &byState);
    }

    /* Get size of input data                                               */
    /*----------------------------------------------------------------------*/
    TPS_GetValue16((USIGN8*)(pzSubslot->pt_size_input_data), &wSizeInputData);

    /* Check the data length                                                */
    /*----------------------------------------------------------------------*/
    if (wSizeInputData != wDataLength)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: Wrong buffer length. Given: 0x%X, Expected: 0x%X\n",
            wDataLength, wSizeInputData);
#endif
        AppSetLastError(API_IODATA_WRONG_BUFFER_LENGTH);
        return 0;
    }

    /* Get the start address of the input data area.                          */
    /*------------------------------------------------------------------------*/
    TPS_GetIOAddress((USIGN8*)(pzSubslot->pt_input_data), &pbyIOMemory);

    /* Get the offset of the input data and iops                              */
    /*------------------------------------------------------------------------*/
    TPS_GetValue16((USIGN8*)(pzSubslot->pt_offset_input_data), &wOffsetInputData);
    TPS_GetValue16((USIGN8*)(pzSubslot->pt_offset_input_iops), &wOffsetInputIops);

    /* Check if wDatalength equals the data length in the input frame         */
    /*------------------------------------------------------------------------*/
    if (wDataLength == (wOffsetInputIops - wOffsetInputData))
    {
        /* Write the data into the DPRAM. */
        TPS_SetValueData((pbyIOMemory + wOffsetInputData), pbyData, wSizeInputData);
        /* write iops into the input area. */
        TPS_SetValue8((USIGN8*)(pbyIOMemory + wOffsetInputIops), byState);
    }
    else
    {
        /* Length of data in the connect request differs from the real        */
        /* configuration. Write iops of this submodule bad and return an error*/
        TPS_SetValue8((USIGN8*)(pbyIOMemory + wOffsetInputIops), IOXS_BAD_BY_SUBSLOT);
        AppSetLastError(API_IODATA_INPUT_FRAME_BUFFER);
        wSizeInputData = 0;
    }


    return wSizeInputData;
}

/*!
* \brief       This function initiates a buffer change of the cyclic input data to send the latest input in the cyclic input data.
               These data will be sent until the next buffer change is initiated.
               The function returns after the buffer was successfully changed.
               There are three input buffers for each AR. Call this function only if all input data and IOCS were written into the
               active input buffer.
*
* \param[in]   byARNumber the AR which will be updated (AR_0 or AR_1)
* \note        Do not change this function!
* \retval      possible return values:
*              - TPS_ACTION_OK : success
*              - API_WRONG_AR_NUMBER
*/
USIGN32 TPS_UpdateInputData(USIGN8 byARNumber)
{
    USIGN32 dwRegValue = 0;
    USIGN32 dwReturnValue = 0;

    if ((byARNumber != AR_0) && (byARNumber != AR_1))
    {
        return API_WRONG_AR_NUMBER;
    }

    /* Set register data for buffer change.                                 */
    /*----------------------------------------------------------------------*/
    dwRegValue = (2 << 6) | (1 << 5) | (2 * byARNumber + 1);

    /* Initiate the buffer change!                                           */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)BASE_ADDRESS_DPRAM, dwRegValue);

    /* Wait for the acknowledge!                                            */
    /*----------------------------------------------------------------------*/
    do
    {
        TPS_GetValue32((USIGN8*)(BASE_ADDRESS_DPRAM + 4), &dwReturnValue);
    } while ((dwReturnValue & dwRegValue) != (dwRegValue ^ (1 << 5)));

    return TPS_ACTION_OK;
}

/*!
 * \brief      The function writes the IOCS (consumer state) of one submodule into the input buffer.
               Usually this is the same as the IOPS (provider state) that is set with TPS_WriteInputData() or TPS_SetInputIops().
 * \ingroup    allfunctions
 * \param[in]  pzSubslot handle to the sub slot structure
 * \param[in]  byState consumer state (IOCS) of this subslot
               IOXS_BAD_BY_SUBSLOT, IOXS_BAD_BY_SLOT, IOXS_BAD_BY_DEVICE, IOXS_BAD_BY_CONTROLLER, IOXS_GOOD
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - SET_IOCS_NULL_POINTER
 *              - API_IODATA_NOT_USED_IN_CR
 */
USIGN32 TPS_SetOutputIocs(SUBSLOT *pzSubslot, USIGN8 byState)
{
    USIGN8*  pbyIOMemory = NULL;
    USIGN16  wOffsetIocs = 0x0;
    USIGN16  wUsedInCr = 0x0;

#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    USIGN16 wSubSlotProperties = 0x00;
#endif

    if (pzSubslot == NULL)
    {
        return SET_IOCS_NULL_POINTER;
    }

    /* check if this subslot is used in a CR */
    TPS_GetValue16((USIGN8*)pzSubslot->pt_used_in_cr, &wUsedInCr);

    if ((wUsedInCr & OUTPUT_USED) == SUBSLOT_NOT_USED)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API: TPS_SetOutputIocs > Subslot is not used in a CR or has no output data\n");
#endif

        return API_IODATA_NOT_USED_IN_CR;
    }

    /* If the Submodule is pulled, only bad is allowed. */
#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    TPS_GetValue16((USIGN8*)pzSubslot->pt_properties, &wSubSlotProperties);
    if ((wSubSlotProperties & SUBMODULE_PROPERTIES_MASK) != SUBMODULE_NOT_PULLED)
    {
#ifdef DEBUG_API_AUTOCONF
        printf("DEBUG_API > API: TPS_SetOutputIocs() > Module is pulled, forcing IOCS BAD_BY_SUBSLOT.\n");
#endif
        byState = IOXS_BAD_BY_SUBSLOT;
    }
#endif

    if (byState == IOXS_GOOD)
    {
        /* overwrite IOPS in case of wrong submodule                  */
        TPS_GetValue8((USIGN8*)(pzSubslot->pt_operational_state), &byState);
    }

    /* Get the pointer to the Input buffer.*/
    TPS_GetIOAddress((USIGN8*)(pzSubslot->pt_input_data), &pbyIOMemory);

    /* Get the offset of the IOCS. */
    TPS_GetValue16((USIGN8*)(pzSubslot->pt_offset_input_iocs), &wOffsetIocs);

    /* Set the IOCS. */
    TPS_SetValue8(pbyIOMemory + wOffsetIocs, byState);

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function sets the IOPS of an input submodule.
                Can be used to overwrite the IOPS that was already written with TPS_WriteInputData() without
                changing the data.
 *
 * \param[in]   pzSubslot handle to the subslot structure
 * \param[in]   byState provider state (IOPS) of the input data
                IOXS_BAD_BY_SUBSLOT, IOXS_BAD_BY_SLOT, IOXS_BAD_BY_DEVICE, IOXS_BAD_BY_CONTROLLER, IOXS_GOOD
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - SET_IOCS_NULL_POINTER
 *              - API_IODATA_NOT_USED_IN_CR
 */
USIGN32 TPS_SetInputIops(SUBSLOT *pzSubslot, USIGN8 byState)
{
    USIGN8*  pbyIOMemory = NULL;
    USIGN16  wOffsetIops = 0x0;
    USIGN16  wUsedInCr = 0x0;

#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    USIGN16 wSubSlotProperties = 0x00;
#endif

    if (pzSubslot == NULL)
    {
        return SET_IOCS_NULL_POINTER;
    }

    /* check if this subslot is used in a CR and has an IOPS */
    TPS_GetValue16((USIGN8*)pzSubslot->pt_used_in_cr, &wUsedInCr);

    if ((wUsedInCr & INPUT_USED) == SUBSLOT_NOT_USED)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: TPS_SetInputIops() > Subslot not used or no input\n");
#endif

        return API_IODATA_NOT_USED_IN_CR;
    }


#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    /* If the Submodule is pulled, IOPS bad is forced. */

    TPS_GetValue16((USIGN8*)pzSubslot->pt_properties, &wSubSlotProperties);
    if ((wSubSlotProperties & SUBMODULE_PROPERTIES_MASK) != SUBMODULE_NOT_PULLED)
    {
#ifdef DEBUG_API_AUTOCONF
        printf("DEBUG_API > TPS_SetInputIops() > Module is pulled, forcing IOPS BAD.\n");
#endif

        byState = IOXS_BAD_BY_SUBSLOT;
    }
#endif

    if (byState == IOXS_GOOD)
    {
        /* If Module Diff was created IOPS bad is forced */
        TPS_GetValue8((USIGN8*)(pzSubslot->pt_operational_state), &byState);
    }

#ifdef DEBUG_API_AUTOCONF
    printf("DEBUG_API > TPS_SetInputIops() > IOPS: 0x%02X\n", byState);
#endif
    /** Write the IOPS into the frame buffer **/
    /* Get the pointer to the Input buffer.*/
    TPS_GetIOAddress((USIGN8*)(pzSubslot->pt_input_data), &pbyIOMemory);

    /* Get the offset of the IOCS. */
    TPS_GetValue16((USIGN8*)(pzSubslot->pt_offset_input_iops), &wOffsetIops);

    /* Write the IOPS. */
    TPS_SetValue8(pbyIOMemory + wOffsetIops, byState);

    return TPS_ACTION_OK;
}

/*!@} Cyclic Data Interface (IO data)*/

/*! \addtogroup  recordhandling Record and Alarm Interface
 *@{
*/

/*!
 * \brief       This function copies the data of the given record mailbox into the data buffer.
                Use this function in your OnRecordWrite callback (ONWRITERECORD_CB) function to get the
                the data out of the record write request.
 *
 * \param[in]   byMBNumber number of the record mailbox
 * \param[in]   pbyData pointer to the data buffer to transfer
 * \param[in]   dwLength length of the data to send
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_READ_MB_INVALID_MAILBOX
 *              - API_READ_MB_WRONG_FLAG
 *              - API_READ_MB_BUFFER_TOO_SMALL
 */
USIGN32 TPS_ReadMailboxData(USIGN8 byMBNumber, USIGN8* pbyData, USIGN32 dwLength)
{
    USIGN32  dwReturnCode = TPS_ACTION_OK;
    USIGN8   byFlags = 0;
    USIGN32  dwRecordDataLength = 0;

    if (byMBNumber >= MAX_NUMBER_RECORDS)
    {
        return API_READ_MB_INVALID_MAILBOX;
    }

    TPS_GetValue32((g_zApiARContext.record_mb[byMBNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER) + 2 + sizeof(UUID_TAG) + 4 + 2 + 2 + 2 + 2), &dwRecordDataLength);

    /* Read Mailbox Flags.                                                   */
    /*-----------------------------------------------------------------------*/
    TPS_GetValue8(g_zApiARContext.record_mb[byMBNumber].pt_flags, &byFlags);

    /* Check if flag is correct!                                             */
    /*-----------------------------------------------------------------------*/
    if (byFlags != RECORD_FLAG_WRITE)
    {
        /* Error                                                           */
        /*-----------------------------------------------------------------*/
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Record Read Mailbox Error -> Wrong flag!\n");
#endif

        return(API_READ_MB_WRONG_FLAG);
    }


    /* Check data length (buffer to small?)                                  */
    /*-----------------------------------------------------------------------*/
    if (dwLength < TPS_htonl(dwRecordDataLength))
    {
        /* Error                                                             */
        /*-------------------------------------------------------------------*/
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Record Read Error -> Buffer too small!\n");
#endif
        return(API_READ_MB_BUFFER_TOO_SMALL);
    }

    dwReturnCode = TPS_GetValueData(g_zApiARContext.record_mb[byMBNumber].pt_data, pbyData, TPS_htonl(dwRecordDataLength));

    return (dwReturnCode);
}


/*!
 * \brief       This function sets the event bit "APP_EVENT_RECORD_DONE" and informs the TPS-1 that the received
                record write request was completely handled so that the record mailbox is freed to receive a new request.
 *              Use this function in your OnRecordWrite callback function after reading the data out of the mailbox
                or to refuse the request with an error code.
 *
 * \param[in]   dwMailboxNumber
 * \param[in]   wErrorCode1 record error code 1
 * \param[in]   wErrorCode2 record error code 2
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_READ_ERRORCODE2_INVALID
 *              - API_READ_MB_WRONG_FLAG
 */
USIGN32 TPS_RecordWriteDone(USIGN32 dwMailboxNumber, USIGN16 wErrorCode1, USIGN16 wErrorCode2)
{
    USIGN8 byFlags = 0;

    if (wErrorCode2 != 0x00)
    {
        return API_RECORD_WRITE_ERRORCODE2_INVALID;
    }

    /* Read the mailbox flag!                                                 */
    /*------------------------------------------------------------------------*/
    TPS_GetValue8(g_zApiARContext.record_mb[dwMailboxNumber].pt_flags, &byFlags);
    if (byFlags == RECORD_FLAG_WRITE)
    {
        byFlags |= RECORD_FLAG_DONE;

        /* Set done so that the application can not change anything          */
        /* in the mailbox. Now the stack has the exclusive right.            */
        /*-------------------------------------------------------------------*/
        TPS_SetValue8(g_zApiARContext.record_mb[dwMailboxNumber].pt_flags, byFlags);
    }
    else
    {
        /* Error                                                             */
        /*-------------------------------------------------------------------*/
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Record Write Done Error -> Wrong flag!\n");
#endif
        return(API_RECORD_WRITE_WRONG_FLAG);
    }

    /* Set ErrorCode1 in mailbox                                               */
    /*------------------------------------------------------------------------*/
    if (wErrorCode1 != 0)
    {
        TPS_SetValue16(g_zApiARContext.record_mb[dwMailboxNumber].pt_errorcode1, wErrorCode1);
    }

    /* Set the record data_len to zero                                      */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header+sizeof(ETH_HEADER)+
                       sizeof(UDP_IP_HEADER)+
                       sizeof(RPC_HEADER)+sizeof(ARGS_REQ)+sizeof(BLOCK_HEADER)+2+sizeof(UUID_TAG)+4+2+2+2+2),
                       0);

    AppSetEventRegApp(APP_EVENT_RECORD_DONE);

#ifdef DEBUG_API_TEST
    /* Get the read flag                                                */
    /*------------------------------------------------------------------*/
    TPS_GetValue8(g_zApiARContext.record_mb[dwMailboxNumber].pt_flags, &byFlags);
    printf("DEBUG_API > Record Write Done set. 0x%x\n", byFlags);
#endif

    return (TPS_ACTION_OK);
}


/*!
 * \brief       This function copies the data out of the given data buffer into the record mailbox.
                Use this function in your OnRecordRead callback (ONREADRECORD_CB) function answer the
                to write the requested data into the mailbox.
 *
 * \param[in]   byMBNumber number of record mailbox
 * \param[in]   pbyData pointer to a data buffer
 * \param[in]   dwLength length of the data buffer
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_WRITE_MB_INVALID_MAILBOX
 *              - API_READ_MB_WRONG_FLAG
 *              - API_WRITE_MB_TOO_MUCH_DATA
 */
USIGN32 TPS_WriteMailboxData(USIGN8 byMBNumber, USIGN8 *pbyData, USIGN32 dwLength)
{
    USIGN8  byFlags = 0;
    USIGN32 dwRecordDataLength = 0;

    if (byMBNumber >= MAX_NUMBER_RECORDS)
    {
        return API_WRITE_MB_INVALID_MAILBOX;
    }

    TPS_GetValue32((g_zApiARContext.record_mb[byMBNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) +
        sizeof(BLOCK_HEADER) + 2 +
        sizeof(UUID_TAG) + 4 + 2 + 2 + 2 + 2), &dwRecordDataLength);

    /* Read mailbox flag.                                                    */
    /*-----------------------------------------------------------------------*/
    TPS_GetValue8(g_zApiARContext.record_mb[byMBNumber].pt_flags, &byFlags);

    /* Check if RECORD_FLAG_READ is set                                      */
    /*-----------------------------------------------------------------------*/
    if (byFlags != RECORD_FLAG_READ)
    {
        /* Error                                                             */
        /*-------------------------------------------------------------------*/
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Record Write Mailbox Error -> Wrong flag!\n");
#endif
        return API_WRITE_MB_WRONG_FLAG;
    }

    /* Check if data fits into requested record data length                  */
    /*-----------------------------------------------------------------------*/
    if (dwLength > TPS_htonl(dwRecordDataLength))
    {
        return API_WRITE_MB_TOO_MUCH_DATA;
    }

    /* Copy data from buffer into record mailbox                             */
    /*-----------------------------------------------------------------------*/
    TPS_SetValueData(g_zApiARContext.record_mb[byMBNumber].pt_data, pbyData, dwLength);

    TPS_SetValue32((g_zApiARContext.record_mb[byMBNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) +
        sizeof(BLOCK_HEADER) + 2 +
        sizeof(UUID_TAG) + 4 + 2 + 2 + 2 + 2), TPS_htonl(dwLength));

    return (TPS_ACTION_OK);
}


/*!
 * \brief       This function sets the event bit "APP_EVENT_RECORD_DONE" and informs the TPS-1 that the received
                record read request was completely handled so that the record mailbox is freed to receive a new request.
 *              The function also writes the error code into the mailbox.
 *              Use this function at the end of your OnRecordRead callback function after writing the requested data into the mailbox
                or to refuse the request with an error code.
 *
 * \param[in]   dwMailboxNumber number of the record mailbox
 * \param[in]   wErrorCode1 record error code 1
 * \param[in]   wErrorCode2 record error code 2. <b>NOT SUPPORTED</b> not supported in this version. Must be 0.
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_READ_ERRORCODE2_INVALID
 *              - API_READ_MB_WRONG_FLAG
 */
USIGN32 TPS_RecordReadDone(USIGN32 dwMailboxNumber, USIGN16 wErrorCode1, USIGN16 wErrorCode2)
{
    USIGN8 byFlags = 0;

    if (wErrorCode2 != 0x00)
    {
        return API_RECORD_READ_ERRORCODE2_INVALID;
    }

    if (dwMailboxNumber >= MAX_NUMBER_RECORDS)
    {
        return API_RECORD_READ_INVALID_MAILBOX;
    }

    /* Read the record flags!                                                */
    /*-----------------------------------------------------------------------*/
    TPS_GetValue8(g_zApiARContext.record_mb[dwMailboxNumber].pt_flags, &byFlags);
    if (byFlags == RECORD_FLAG_READ)
    {
        /* Add done flag. The stack can then work on the data                */
        /*-------------------------------------------------------------------*/
        byFlags |= RECORD_FLAG_DONE;

        /* Write the flag into the DPRAM.                                    */
        /*-------------------------------------------------------------------*/
        TPS_SetValue8(g_zApiARContext.record_mb[dwMailboxNumber].pt_flags, byFlags);
    }
    else
    {
        /* Error case                                                         */
        /*--------------------------------------------------------------------*/
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Record Read Done Error -> Wrong flag!\n");
#endif

        return(API_RECORD_READ_WRONG_FLAG);
    }

    /* Set ErrorCode1 in mailbox.                                              */
    /*------------------------------------------------------------------------*/
    if (wErrorCode1 != 0)
    {
        TPS_SetValue16(g_zApiARContext.record_mb[dwMailboxNumber].pt_errorcode1, wErrorCode1);
    }

    AppSetEventRegApp(APP_EVENT_RECORD_DONE);

#ifdef DEBUG_API_TEST
    TPS_GetValue8(g_zApiARContext.record_mb[dwMailboxNumber].pt_flags, &byFlags);
    printf("DEBUG_API > Record Read Done set. 0x%x\n", byFlags);
#endif

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function delivers the state of the mailbox. It should be called at the beginning of your record read/write callback
                functions to get the details of the request.
 *
 * \param[in]   dwMailboxNumber number of the record mailbox
 * \param[out]  pzMailBoxInfo pointer to the mail box info structure
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_MAILBOXINFO_INVALID_MAILBOX
 *              - API_RECORD_MAILBOXINFO_WRONG_FLAG
 */
USIGN32 TPS_GetMailboxInfo(USIGN32 dwMailboxNumber, RECORD_BOX_INFO *pzMailBoxInfo)
{
    USIGN8  byFlagBuff = 0;
    USIGN16 wSeqNrBuff = 0;
    USIGN32 dwApiBuff = 0;
    USIGN16 wSlotNrBuff = 0;
    USIGN16 wSubslotNrBuff = 0;
    USIGN16 wPaddingBuff = 0;
    USIGN16 wIndexBuff = 0;
    USIGN32 dwRecordDataLenBuff = 0;

    if (dwMailboxNumber >= MAX_NUMBER_RECORDS)
    {
        return API_RECORD_MAILBOXINFO_INVALID_MAILBOX;
    }


    /* Get the seq_number.                                                 */
    /*---------------------------------------------------------------------*/
    TPS_GetValue16((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER)),
        &wSeqNrBuff);

    /* ar_uuid not yet implemented. */

    /* Get the API number.                                                  */
    /*----------------------------------------------------------------------*/
    TPS_GetValue32((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER) + 2 + sizeof(UUID_TAG)),
        &dwApiBuff);
    /* Get the slot number.                                                 */
    /*----------------------------------------------------------------------*/
    TPS_GetValue16((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER) + 2 + sizeof(UUID_TAG) + 4),
        &wSlotNrBuff);
    /* Get the subslot number.                                              */
    /*----------------------------------------------------------------------*/
    TPS_GetValue16((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER) + 2 + sizeof(UUID_TAG) + 4 + 2),
        &wSubslotNrBuff);
    /* Get the padding.                                                     */
    /*----------------------------------------------------------------------*/
    TPS_GetValue16((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER) + 2 + sizeof(UUID_TAG) + 4 + 2 + 2),
        &wPaddingBuff);
    /* Get the index.                                                       */
    /*----------------------------------------------------------------------*/
    TPS_GetValue16((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER) + 2 + sizeof(UUID_TAG) + 4 + 2 + 2 + 2),
        &wIndexBuff);
    /* Get the record data_len.                                             */
    /*----------------------------------------------------------------------*/
    TPS_GetValue32((g_zApiARContext.record_mb[dwMailboxNumber].pt_req_header + sizeof(ETH_HEADER) +
        sizeof(UDP_IP_HEADER) +
        sizeof(RPC_HEADER) + sizeof(ARGS_REQ) + sizeof(BLOCK_HEADER) + 2 + sizeof(UUID_TAG) + 4 + 2 + 2 + 2 + 2),
        &dwRecordDataLenBuff);

    /* The targetaruuid_padding is not yet implemented. */

    /* Read the pt_flags.                                                   */
    /*----------------------------------------------------------------------*/
    TPS_GetValue8(g_zApiARContext.record_mb[dwMailboxNumber].pt_flags, &byFlagBuff);

    if (byFlagBuff == RECORD_FLAG_READ)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Read Record Request in Mailbox\n");
#endif
    }
    else if (byFlagBuff == RECORD_FLAG_WRITE)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Write Record Request in Mailbox\n");
#endif
    }
    else
    {
        /* invalid flag                                                      */
        /*-------------------------------------------------------------------*/
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Mailbox Info Error -> Wrong flag!\n");
#endif
        return(API_RECORD_MAILBOXINFO_WRONG_FLAG);
    }

    pzMailBoxInfo->dwAPINumber = TPS_htonl(dwApiBuff);
    pzMailBoxInfo->wSlotNumber = TPS_htons(wSlotNrBuff);
    pzMailBoxInfo->wSubSlotNumber = TPS_htons(wSubslotNrBuff);
    pzMailBoxInfo->wIndex = TPS_htons(wIndexBuff);
    pzMailBoxInfo->dwRecordDataLen = TPS_htonl(dwRecordDataLenBuff);

    return (TPS_ACTION_OK);
}

/*!
 * \brief       This function initiates an alarm notification to a PLC
 *
 * \param[in]   dwARNumber the AR number
 * \param[in]   dwAPINumber the API number
 * \param[in]   wSlotNumber the slot number
 * \param[in]   wSubslotNumber the subslot number
 * \param[in]   byAlarmPrio the alarm priority (ALARM_HIGH or ALARM_LOW)
 * \param[in]   byAlarmType the alarm type
 *              (PULL_ALARM, PLUG_ALARM, UPDATE_ALARM, PROCESS_ALARM, DIAGNOSIS_DISAPPEARS_ALARM, DIAGNOSIS_ALARM)
 * \param[in]   dwDataLength length of the alarm data
 * \param[in]   pbyAlarmData pointer to the buffer containing the alarm data
 * \param[in]   wUsi the user structure identifier that describes the structure of the alarm data
                (User defined between 0x0000 and 0x7FFF)
 * \param[in]   specific handle to identify the corresponding alarm acknowledge. Should be unique for each sent alarm.
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_ALARM_WRONG_AR_NUMBER
 *              - API_ALARM_PRIO_UNKNOWN
 *              - API_ALARM_DATA_TOO_LONG
 *              - API_ALARM_SUBSLOT_NOT_FOUND
 *              - API_ALARM_SUBSLOT_NOT_USED_BY_AR
 */
USIGN32 TPS_SendAlarm(USIGN32 dwARNumber, USIGN32 dwAPINumber, USIGN16 wSlotNumber,
    USIGN16 wSubslotNumber, USIGN8 byAlarmPrio, USIGN8 byAlarmType,
    USIGN32 dwDataLength, USIGN8* pbyAlarmData, USIGN16 wUsi, USIGN16 wUserHandle)
{
    USIGN8   byMailboxNr = 0;
    SUBSLOT* pzSubslot = NULL;
    SLOT*    pzSlot = NULL;
    USIGN8   byFlagBuff = 0;
    USIGN16  wOwnedByAr = 0;
    BOOL     bSubslotUsedInAr = TPS_FALSE;
    USIGN32  dwReturnValue = TPS_ACTION_OK;

#ifdef DEBUG_API_TEST
    /* Variables for the debug output!                                    */
    /*--------------------------------------------------------------------*/
    USIGN32 dwSizeAlarmMB = 0;
    USIGN32 dwAPINr = 0;
    USIGN16 wAlarmDataLen = 0;
    USIGN8  byAlTyp = 0;
#endif

    if ((dwARNumber != AR_0) && (dwARNumber != AR_1))
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Invalid AR number\n");
#endif

        return API_ALARM_WRONG_AR_NUMBER;
    }

    if ((byAlarmPrio != ALARM_HIGH) && (byAlarmPrio != ALARM_LOW))
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Invalid alarm prio\n");
#endif

        return API_ALARM_PRIO_UNKNOWN;
    }

    if (dwDataLength > (SIZE_ALARM_MB - 32))
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Alarm data too long(%d bytes). Longer than %d bytes\n", dwDataLength, (SIZE_ALARM_MB - 32));
#endif

        return API_ALARM_DATA_TOO_LONG;
    }

    if (wSubslotNumber != 0)
    {
        /* Check if the submodule is used in the given AR.                    */
        pzSubslot = AppGetSubslotHandle(dwAPINumber, wSlotNumber, wSubslotNumber);

        if (pzSubslot == NULL)
        {
            dwReturnValue = API_ALARM_SUBSLOT_NOT_FOUND;
        }
        else
        {
            TPS_GetValue16((USIGN8*)(pzSubslot->pt_wSubslotOwnedByAr), &wOwnedByAr);
            if ((wOwnedByAr & OWNED_BY_AR(dwARNumber)) != SUBSLOT_NOT_USED)
            {
                bSubslotUsedInAr = TPS_TRUE;
            }
        }
    }
    else
    {
        /* SubslotNumber = 0 -> module representative                         */
        /* Check if a submodule of this module is used in the given AR.       */
        pzSlot = AppGetSlotHandle(dwAPINumber, wSlotNumber);

        if (pzSlot == NULL)
        {
            dwReturnValue = API_ALARM_SLOT_NOT_FOUND;
        }
        else
        {
            for (pzSubslot = pzSlot->pSubslot; pzSubslot != NULL; pzSubslot = pzSubslot->poNextSubslot)
            {
                TPS_GetValue16((USIGN8*)pzSubslot->pt_wSubslotOwnedByAr, &wOwnedByAr);

                if ((wOwnedByAr & OWNED_BY_AR(dwARNumber)) != SUBSLOT_NOT_USED)
                {
                    bSubslotUsedInAr = TPS_TRUE;
                    break;
                }
            }
        }
    }

    /* If the subslot is used, select the correct mailbox and send the alarm. */
    if ((dwReturnValue == TPS_ACTION_OK) && (bSubslotUsedInAr == TPS_TRUE))
    {
        if ((byAlarmPrio == ALARM_HIGH) && (dwARNumber == AR_0))
        {
            byMailboxNr = 1;
        }
        else if ((byAlarmPrio == ALARM_LOW) && (dwARNumber == AR_0))
        {
            byMailboxNr = 0;
        }
        else if ((byAlarmPrio == ALARM_HIGH) && (dwARNumber == AR_1))
        {
            byMailboxNr = 3;
        }
        else if ((byAlarmPrio == ALARM_LOW) && (dwARNumber == AR_1))
        {
            byMailboxNr = 2;
        }
    }
    else if (dwReturnValue == TPS_ACTION_OK)
    {
        /* slot was found but not owned by this AR */
        dwReturnValue = API_ALARM_SUBSLOT_NOT_USED_BY_AR;
    }


    if (dwReturnValue == TPS_ACTION_OK)
    {
        /* Get the Mailbox Flag.                                              */
        /*--------------------------------------------------------------------*/
        TPS_GetValue8(g_zAlarmMailbox[byMailboxNr].pt_flags, &byFlagBuff);
        if (byFlagBuff != UNUSED_FLAG) /* Check if unused! */
        {
            /* Return error code                                              */
            /*----------------------------------------------------------------*/
            dwReturnValue = API_ALARM_MAILBOX_IN_USE;
        }
    }

    if (dwReturnValue == TPS_ACTION_OK)
    {
        TPS_SetValue8(g_zAlarmMailbox[byMailboxNr].pt_flags, ACTIVE_FLAG);
        TPS_SetValue32(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_api)), dwAPINumber);
        TPS_SetValue16(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_slot_nr)), wSlotNumber);
        TPS_SetValue16(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_subslot_nr)), wSubslotNumber);
        TPS_SetValue8(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_alarm_typ)), byAlarmType);
        TPS_SetValue16(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_usi)), TPS_htons(wUsi));
        TPS_SetValue16(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_usr_hdl)), wUserHandle);
        TPS_SetValue16(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_data_length)), (USIGN16)dwDataLength);
        TPS_SetValueData(g_zAlarmMailbox[byMailboxNr].pt_data, pbyAlarmData, dwDataLength);

#ifdef DEBUG_API_TEST
        /* Debug Info for the processed alarm mailbox.                    */
        /*----------------------------------------------------------------*/
        dwSizeAlarmMB = g_zAlarmMailbox[byMailboxNr].size_alarm_mailbox;
        TPS_GetValue8(g_zAlarmMailbox[byMailboxNr].pt_flags, &byFlagBuff);
        TPS_GetValue32(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_api)), &dwAPINumber);
        TPS_GetValue16(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_data_length)), &wAlarmDataLen);
        TPS_GetValue8(((USIGN8*)(g_zAlarmMailbox[byMailboxNr].pt_alarm_typ)), &byAlTyp);
        printf("*g_zAlarmMailbox[byMailbox_nr].size_alarm_mailbox=%d\n", dwSizeAlarmMB);
        printf("*g_zAlarmMailbox[byMailbox_nr].pt_flags=0x%x\n", byFlagBuff);
        printf("*g_zAlarmMailbox[byMailbox_nr].pt_api=0x%x\n", dwAPINr);
        printf("*g_zAlarmMailbox[byMailbox_nr].pt_data_length=0x%x\n", wAlarmDataLen);
        printf("*g_zAlarmMailbox[byMailbox_nr].pt_alarm_typ=0x%x\n", byAlTyp);
#endif

        /* Set the alarm request bit for the processed AR                     */
        /*--------------------------------------------------------------------*/
        AppSetEventAlarmReq(dwARNumber);
    }

    return dwReturnValue;
}
/*!@} Record and Alarm Interface*/

/*! \addtogroup ledhandling LED Interface
 *@{
 */

 /*!
 * \brief   This function reads the current LED states connected to the TPS-1. The result is a bit field with the following layout:
 *
 *          |Bit number | meaning |
 *          |:---:|:------------------------------------:|
 *          |0    | BF-LED: 0-on, 1-off                  |
 *          |1    | SF-LED: 0-on, 1-off                  |
 *          |2    | MD-LED: 0-on, 1-off                  |
 *          |3    | MR-LED: 0-on, 1-off                  |
 *          |4    | Link-LED port 1: 1-on, 0-off         |
 *          |5    | Link-LED port 2: 1-on, 0-off         |
 *          |6-7  | reserved                             |
 *          |8    | BF-LED: 1-blinking, 0-static         |
 *          |9    | SF-LED: 1-blinking, 0-static         |
 *          |10   | MD-LED: 1-blinking, 0-static         |
 *          |11   | MR-LED: 1-blinking, 0-static         |
 *          |12   | Link-LED port 1: 1-blinking, 0-static|
 *          |13   | Link-LED port 2: 1-blinking, 0-static|
 *          |14-15| reserved                             |
 *
 * \param[in]   no
 * \retval      value of the LED states
 */
USIGN16 TPS_GetLedState(VOID)
{
   USIGN16 wLedState = 0;

   TPS_GetValue16((USIGN8*)&(g_pzNrtConfigHeader->wLEDState ), &wLedState);

   return wLedState;
}

/*!
 * \brief       This function registers the callback function that is called when the LED states of the TPS-1 have changed
 *
 * \param[in]   a function pointer to the callback function that shall be linked to the event.
 * \retval      USIGN32
 *               - TPS_ACTION_OK : success
 *               - REGISTER_FUNCTION_NULL
 *
 */
USIGN32 TPS_RegisterLedStateCallback(VOID(*pfnFunction)(USIGN16))
{
    if (pfnFunction == NULL)
    {
        return REGISTER_FUNCTION_NULL;
    }

    g_zApiDeviceContext.OnLedStateChanged_CB = pfnFunction;

    return TPS_ACTION_OK;
}

/*!@} LED Interface*/

/*!
 * \brief       This function registers the callback function that is called when I&MData  was changed
 *
 * \param[in]   a function pointer to the callback function that shall be linked to the event.
 * \retval      SUBSLOT* , USIGN16
 *               - TPS_ACTION_OK : success
 *               - REGISTER_FUNCTION_NULL
 *
 */
USIGN32 TPS_RegisterIMDataCallback(VOID(*pfnFunction)(SUBSLOT*, USIGN16))
{
    if (pfnFunction == NULL)
    {
        return REGISTER_FUNCTION_NULL;
    }

    g_zApiDeviceContext.OnImDataChanged_CB = pfnFunction;

    return TPS_ACTION_OK;
}

/*! \addtogroup fwupdate Firmware Update Over Host Interface
 *@{
 */

#ifdef FW_UPDATE_OVER_HOST

 /*!
 * \brief       This function sets the timeout how long TPS_StartFwUpdater
 *              should wait until the function returns with an error
 *              TPS_ERROR_STACK_TIMEOUT
 *
 * \param[in]   dwTimeout TPS_StartFwUpdater() checks <dwTimeout> times if the TPS-1 Stack has rebooted into FW Update mode before the function returns with an error.
                <br><b>DEFAULT:</b> 500000
 * \retval      VOID
 */
VOID TPS_SetUpdaterTimeout(USIGN32 dwTimeout)
{
    g_dwUpdaterStartTimeout = dwTimeout;
}

/*!
 * \brief       This function returns the timeout for TPS_StartFwUpdater
 *
 * \retval      USIGN32 Number of cycles to check for the firmware updater life sign.
 */
USIGN32 TPS_GetUpdaterTimeout(VOID)
{
    return g_dwUpdaterStartTimeout;
}

/******************************************************************************
  FUNCTIONS FOR TPS FIRMWARE UPDATE OVER HOST-IF
******************************************************************************/
/*!
 * \brief      This function starts the fw update routine on tps-1 and must be called before
 *             uploading of the new firmware image with TPS_WriteFWImageToFlash()
 * \ingroup    allfunctions
 * \note       To use this function <b>FW_UPDATE_OVER_HOST</b> in TPS_1_user.h must be defined
 * \param[in]  VOID
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_STACK_TIMEOUT : starting the firmware took too long
 *              - API_AUTOCONF_WRONG_API_STATE
 */
USIGN32 TPS_StartFwUpdater(VOID)
{
    DPR_HOST_HEADER* pzDprHeader = (DPR_HOST_HEADER*)BASE_ADDRESS_NRT_AREA;
    USIGN8  byHeaderBuf[sizeof(DPR_HOST_HEADER)] = {0};
    USIGN32 dwHostSign = 0;
    USIGN32 dwTicker = 0;

    /* Check the API State. */
    if(g_byApiState != STATE_DEVICE_STARTED)
    {
        #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_GetModuleConfiguration() > Error: Wrong API State!\n");
        #endif
        return API_AUTOCONF_WRONG_API_STATE;
    }

    memset(byHeaderBuf, 0, sizeof(DPR_HOST_HEADER));

    TPS_SetValueData((USIGN8*)(pzDprHeader),
                     (USIGN8*)byHeaderBuf,
                     sizeof(DPR_HOST_HEADER));

    /* IMPORTANT:  write identifier to nrt area before the event            */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostSign), HOST_LIFE_SIGN);

    /* Send Stack RESET Event to the TPS-1 for starting TPS updater         */
    /*----------------------------------------------------------------------*/
    AppSetEventRegApp(APP_EVENT_RESET_PN_STACK);

    /* Wait for start of the updater program.                               */
    /*----------------------------------------------------------------------*/
    while(dwHostSign != UPDATER_LIFE_SIGN && dwTicker < g_dwUpdaterStartTimeout )
    {
        TPS_GetValue32((USIGN8*)&(pzDprHeader->dwHostSign), &dwHostSign);
        dwTicker++;
    }

    if( dwTicker >= g_dwUpdaterStartTimeout )
    {
        return TPS_ERROR_STACK_TIMEOUT;
    }

    /* Write identifier to nrt area                                         */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostSign), HOST_LIFE_SIGN);

    return TPS_ACTION_OK;
}

/*!
 * \brief      This function sends a TPS-1 firmware image to flash.
 *             This function is a blocking function until the complete image
 *             was sent or an error occurred. In case the update process hangs
 *             a user defined timeout handling should be implemented in the application.
 * \ingroup    allfunctions
 * \note       To use this function <b>FW_UPDATE_OVER_HOST</b> in TPS_1_user.h must be defined
 * \param[in]  pbyImage pointer to the complete firmware image (max. 256kb is needed)
 * \param[in]  dwLengthOfImage the actual length of the firmware image
 * \retval     USIGN32
 *              - TPS_ACTION_OK : success
 *              - 0x000FFFFF : all error cases
 */
USIGN32 TPS_WriteFWImageToFlash(USIGN8* pbyImage, USIGN32 dwLengthOfImage)
{
    DPR_HOST_HEADER *pzDprHeader = (DPR_HOST_HEADER*)BASE_ADDRESS_NRT_AREA;
    USIGN32 dwLenOfSendData = 0;
    USIGN32 dwMbxFlag = 0;
    USIGN32 dwTryTimer = 0;
    USIGN32 dwFragLen = 0;
    USIGN32 dwEventHost = 0;
    USIGN32 dwUpdEvent = 0;
    USIGN32 dwErrorCode = TPS_ACTION_OK;

    /* Set event for starting the update process!                            */
    /*-----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostSign), HOST_LIFE_SIGN);
    dwEventHost |= (0x1 << APP_EVENT_START_FW_UPDATE);

    TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostEvent), dwEventHost);

    /* Start data transfer!                                                  */
    /*-----------------------------------------------------------------------*/
    while((dwLenOfSendData < dwLengthOfImage) && (dwTryTimer < MAX_WAIT_CYCLES))
    {
        TPS_GetValue32((USIGN8*)&(pzDprHeader->pbFwStartAddr.dwFlag), &dwMbxFlag);

        if(dwMbxFlag == 0)
        {
            /* Write fragment to mailbox                                     */
            /*---------------------------------------------------------------*/
            if((dwLengthOfImage - dwLenOfSendData) < FW_FRAG_LEN)
            {
                /* Last fragment.                                            */
                /*-----------------------------------------------------------*/
                dwFragLen = (dwLengthOfImage - dwLenOfSendData);
            }
            else
            {
                dwFragLen = FW_FRAG_LEN;
            }

            /* Write data into the mailbox.                                  */
            /*---------------------------------------------------------------*/
            TPS_SetValueData((USIGN8*)&(pzDprHeader->pbFwStartAddr.bData),
                             pbyImage + dwLenOfSendData, dwFragLen);

            /* Write length of fragment to mailbox                           */
            /*---------------------------------------------------------------*/
            TPS_SetValue32((USIGN8*)&(pzDprHeader->pbFwStartAddr.dwFragLen), dwFragLen);

            /* Count sent data                                               */
            /*---------------------------------------------------------------*/
            dwLenOfSendData += dwFragLen;

            if(dwLenOfSendData == dwLengthOfImage)
            {
                dwMbxFlag = 2;
#ifdef DEBUG_API_TEST
                printf("Transfer Complete\n");
#endif
            }
            else
            {
                dwMbxFlag = 1;
            }

            dwTryTimer = 0;

            /* Set mailbox state to busy                                     */
            /*---------------------------------------------------------------*/
            TPS_SetValue32((USIGN8*)&(pzDprHeader->pbFwStartAddr.dwFlag), dwMbxFlag);

#ifdef DEBUG_API_TEST
            printf(".");
#endif
        }
        else
        {
            dwTryTimer++;
        }
    }

    #ifdef DEBUG_API_TEST
         printf("\n%d Bytes transfered to TPS\n", dwLenOfSendData);
         printf("wait until updater is ready and reset TPS-1..\n");
    #endif


    if(dwTryTimer < MAX_WAIT_CYCLES)
    {
        while(!((dwUpdEvent) & (0x1 << UPD_EVENT_FW_UPDATE_DONE)))
        {
            TPS_GetValue32((USIGN8*)&(pzDprHeader->dwUpdaterEvent), &dwUpdEvent);
            TPS_GetValue32((USIGN8*)&(pzDprHeader->dwErrorCode), &dwErrorCode);
        }
    }
    else
    {
        dwErrorCode = MAX_WAIT_CYCLES;
    }

    if((dwErrorCode & 0xFFFFFFFE) == 0)
    {
        #ifdef DEBUG_API_TEST
             printf("FW-Update was successful! Restarting TPS-1...\n");
        #endif
    }
    else
    {
        #ifdef DEBUG_API_TEST
            printf("ERROR: FW-Update was NOT successful! ErrorCode = 0x%X \n", dwErrorCode);
        #endif
    }

    /* Change the execution from TPS Updater to TPS Stack                    */
    /*-----------------------------------------------------------------------*/
    TPS_StartFwStack();

    return dwErrorCode;
}

/*!
 * \brief      This function sends a fragment of TPS-1 firmware image to flash.
 *             This function is a blocking function until the tps-1 response the received fragment.
 *             In case the update process hangs a user defined timeout handling should be implemented in the application.
 * \ingroup    allfunctions
 * \note       To use this function <b>FW_UPDATE_OVER_HOST</b> in TPS_1_user.h must be defined
 * \param[in]  pbyImageFrag pointer to the fragment of firmware image to be sent
 * \param[in]  dwFragLen the actual length of the fragment (max 1024 bytes)
 * \retval     USIGN32
 *              - TPS_ACTION_OK : success
 *              - 0x000FFFFF : by timeout or ErrorCode from TPS-1
 */
USIGN32 TPS_WriteFWImageFragmentToFlash(USIGN8* pbyImageFrag, USIGN32 dwFragLen, USIGN8 byLastFrag)
{
  DPR_HOST_HEADER *pzDprHeader = (DPR_HOST_HEADER*)BASE_ADDRESS_NRT_AREA;
 #ifdef DEBUG_API_TEST
    static USIGN32 dwLenOfSendData = 0;
#endif
    USIGN32 dwMbxFlag = 1;
    USIGN32 dwTryTimer = 0;
    USIGN32 dwEventHost = 0;
    USIGN32 dwUpdEvent = 0;
    static USIGN8 byFirstFragment = 0;
    USIGN32 dwErrorCode = TPS_ACTION_OK;

    if((byFirstFragment == 0) && (byLastFrag == 0))
    {  /* Set event for starting the update process!                            */
       /*-----------------------------------------------------------------------*/
       TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostSign), HOST_LIFE_SIGN);
       TPS_SetValue32((USIGN8*)&(pzDprHeader->dwErrorCode), dwErrorCode);
       dwEventHost |= (0x1 << APP_EVENT_START_FW_UPDATE);
       TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostEvent), dwEventHost);
       byFirstFragment = 1;
    }

while(dwTryTimer < MAX_WAIT_CYCLES)
    {
       TPS_GetValue32((USIGN8*)&(pzDprHeader->pbFwStartAddr.dwFlag), &dwMbxFlag);

       if(dwMbxFlag == 0)
       {
          /* Write data into the mailbox.                                  */
          /*---------------------------------------------------------------*/
          TPS_SetValueData((USIGN8*)&(pzDprHeader->pbFwStartAddr.bData),
                        pbyImageFrag, dwFragLen);

          /* Write length of fragment to mailbox                           */
          /*---------------------------------------------------------------*/
          TPS_SetValue32((USIGN8*)&(pzDprHeader->pbFwStartAddr.dwFragLen), dwFragLen);

          dwMbxFlag = 1 + ( byLastFrag & 0x01 ); /* last fragment ? */
          dwTryTimer = 0;

          /* Set mailbox state to busy                                     */
          /*---------------------------------------------------------------*/
          TPS_SetValue32((USIGN8*)&(pzDprHeader->pbFwStartAddr.dwFlag), dwMbxFlag);

          while((dwMbxFlag != 0) && (dwTryTimer < MAX_WAIT_CYCLES) && (dwErrorCode == 0))
          {
             TPS_GetValue32((USIGN8*)&(pzDprHeader->pbFwStartAddr.dwFlag), &dwMbxFlag);
             TPS_GetValue32((USIGN8*)&(pzDprHeader->dwErrorCode), &dwErrorCode);
             dwTryTimer++;
          }


#ifdef DEBUG_API_TEST
          dwLenOfSendData += dwFragLen;
          printf(".");
#endif
          break;
        }
        else
        {
           dwTryTimer++;
        }
    }
    /* if the transfer is complete, wait for tps-1 is ready and restart them */
    if(byLastFrag == 1)
    {
       if(dwTryTimer < MAX_WAIT_CYCLES)
       {
          while(!((dwUpdEvent) & (0x1 << UPD_EVENT_FW_UPDATE_DONE)))
          {
             TPS_GetValue32((USIGN8*)&(pzDprHeader->dwUpdaterEvent), &dwUpdEvent);
             TPS_GetValue32((USIGN8*)&(pzDprHeader->dwErrorCode), &dwErrorCode);
          }
       }
       else
       {
          dwErrorCode = MAX_WAIT_CYCLES;
       }


       if((dwErrorCode & 0xFFFFFFFE) == 0)
       {
          #ifdef DEBUG_API_TEST
             printf("FW-Update was successful! Restarting TPS-1...\n");
             printf("\n%d Bytes transfered to TPS\n", dwLenOfSendData);
          #endif
       }
       else
       {
          #ifdef DEBUG_API_TEST
             printf("ERROR: FW-Update was NOT successful! ErrorCode = 0x%X \n", dwErrorCode);
          #endif
       }


    #ifdef DEBUG_API_TEST
        printf("wait until updater is ready and reset TPS-1..\n");
        dwLenOfSendData = 0;
    #endif

       byFirstFragment = 0;

       /* Change the execution from TPS Updater to TPS Stack                    */
       /*-----------------------------------------------------------------------*/
       TPS_StartFwStack();
    }

    return dwErrorCode;

}
/*!
 * \brief      This function starts the downloaded firmware image and leaves the updater mode
 *
 * \note       To use this function <b>FW_UPDATE_OVER_HOST</b> in TPS_1_user.h must be defined
 * \param[in]  void
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - ERR_UPDATER_APP_NOT_STARTED
 */
USIGN32 TPS_StartFwStack(void)
{
    DPR_HOST_HEADER *pzDprHeader = (DPR_HOST_HEADER*)BASE_ADDRESS_NRT_AREA;
    USIGN32 dwEventHost = 0;
    USIGN32 dwHostSign = 0;
    USIGN32 dwError = TPS_ACTION_OK;

    TPS_GetValue32((USIGN8*)&(pzDprHeader->dwHostSign), &dwHostSign);

    if((dwHostSign != UPDATER_LIFE_SIGN) && (dwHostSign != HOST_LIFE_SIGN))
    {
        dwError = ERR_UPDATER_APP_NOT_STARTED;
    }
    else
    {
        /* Write identifier to NRT AREA                                      */
        /*-------------------------------------------------------------------*/
        TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostSign), HOST_LIFE_SIGN);

        /* Start the stack firmware again.                                   */
        /*-------------------------------------------------------------------*/
        dwEventHost |= (0x1 << APP_EVENT_START_FW);
        TPS_SetValue32((USIGN8*)&(pzDprHeader->dwHostEvent), dwEventHost);
    }

    return dwError;
}

/******************************************************************************
  END of FUNCTIONS FOR TPS FIRMWARE UPDATE OVER HOST-IF
******************************************************************************/
#endif /*FW_UPDATE_OVER_HOST*/

/*!@} Firmware Update Over Host Interface*/


/*! \addtogroup  versions Version Interface
 *@{
*/

/*!
 * \brief       This function returns the version this application as configured in TPS_AddDevice.
                The same software version should be used in the I&M0 data.
 *
 * \param[out]  pzStackVersionInfo
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - REGISTER_FUNCTION_NULL
 */
USIGN32 TPS_GetDeviceVersionInfo(T_DEVICE_SOFTWARE_VERSION* pzStackVersionInfo)
{
    if (pzStackVersionInfo == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }

    TPS_GetValue8(&g_pzNrtConfigHeader->byRevisionPrefix, &pzStackVersionInfo->byRevisionPrefix);
    TPS_GetValue8(&g_pzNrtConfigHeader->byRevisionFunctionalEnhancement, &pzStackVersionInfo->byRevisionFunctionalEnhancement);
    TPS_GetValue8(&g_pzNrtConfigHeader->byBugFix, &pzStackVersionInfo->byBugFix);
    TPS_GetValue8(&g_pzNrtConfigHeader->byInternalChange, &pzStackVersionInfo->byInternalChange);

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > API: Software Version %d.%d.%d.%d \n",
        pzStackVersionInfo->byRevisionPrefix,
        pzStackVersionInfo->byRevisionFunctionalEnhancement,
        pzStackVersionInfo->byBugFix,
        pzStackVersionInfo->byInternalChange);
#endif

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function returns the version information of the actual software driver (TPS API).
 *
 * \param[out]  pzVersionInfo pointer to the version structure
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_NULL_POINTER
 */
USIGN32 TPS_GetDriverVersionInfo(T_PNIO_VERSION_STRUCT* pzVersionInfo)
{
    if (pzVersionInfo == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }

    pzVersionInfo->wMajorRelease = TPS_DRV_MAJOR_RELEASE;
    pzVersionInfo->wBugFixVersion = TPS_DRV_BUG_FIX_VERSION;
    pzVersionInfo->wBuild = TPS_DRV_BUILD;

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > API: Software Version %d.%d.%d \n",
        pzVersionInfo->wMajorRelease, pzVersionInfo->wBugFixVersion, pzVersionInfo->wBuild);
#endif

    return(TPS_ACTION_OK);
}

/******************************************************************************
** Functions for I&M Data handling
******************************************************************************/
/*!@} Version Interface*/

#if defined(USE_AUTOCONF_MODULE) || defined(PLUG_RETURN_SUBMODULE_ENABLE)
/*! \addtogroup   Autoconfig AutoConfig Interface
*@{
*/

/*!
* \brief       This function returns the actual module ID of the given slot.
*
* \param[in]   poSlot handle to the slot structure
* \param[out]  pdwModuleIdentNumber The module ID is written at this pointer
* \retval      possible return values:
*              - TPS_ACTION_OK : success
*              - API_AUTOCONF_WRONG_API_STATE
*              - API_AUTOCONF_PARAMETER_NULL
*/
USIGN32 TPS_GetModuleConfiguration(SLOT* poSlot, USIGN32* pdwModuleIdentNumber)
{
    /* Check the API State. */
    if (g_byApiState != STATE_DEVICE_STARTED)
    {
#ifdef DEBUG_API_AUTOCONF
        printf("DEBUG_API > API: TPS_GetModuleConfiguration() > Error: Wrong API State!\n");
#endif
        return API_AUTOCONF_WRONG_API_STATE;
    }

    /* Check the parameter. */
    if ((poSlot == NULL) || (pdwModuleIdentNumber == NULL))
    {
#ifdef DEBUG_API_AUTOCONF
        printf("DEBUG_API > API: TPS_GetModuleConfiguration() > Error: Invalid parameter!\n");
#endif
        return API_AUTOCONF_PARAMETER_NULL;
    }

    /* Read out the module ident number. */
    TPS_GetValue32((USIGN8*)poSlot->pt_ident_number, pdwModuleIdentNumber);

#ifdef DEBUG_API_AUTOCONF
    printf("DEBUG_API > API: TPS_GetModuleConfiguration() > ModuleIdentNumber: 0x%2.2X\n", *pdwModuleIdentNumber);
#endif

    return TPS_ACTION_OK;
}

/*!
* \brief       This function returns the actual submodule ID and size of the IO data of the given subslot.
*
* \param[in]   poSubslot the handle of the subslot
* \param[out]  pdwSubmoduleIdentNumber the submodule ID is written at this pointer
* \param[out]  pwSizeOfInputData the size of the input data (in bytes) is written at this pointer
* \param[out]  pwSizeOfOutputData the size of the output data (in bytes) is written at this pointer
* \retval      possible return values:
*              - TPS_ACTION_OK : success
*              - API_AUTOCONF_WRONG_API_STATE
*              - API_AUTOCONF_PARAMETER_NULL
*/
USIGN32 TPS_GetSubmoduleConfiguration(SUBSLOT* poSubslot, USIGN32* pdwSubmoduleIdentNumber, USIGN16* pwSizeOfInputData, USIGN16* pwSizeOfOutputData)
{
    /* Check the API State. */
    if (g_byApiState != STATE_DEVICE_STARTED)
    {
#ifdef DEBUG_API_AUTOCONF
        printf("DEBUG_API > API: TPS_GetSubmoduleConfiguration() > Error: Wrong API State!\n");
#endif
        return API_AUTOCONF_WRONG_API_STATE;
    }

    /* Check the parameter. */
    if ((poSubslot == NULL) || (pdwSubmoduleIdentNumber == NULL) || (pwSizeOfInputData == NULL) || (pwSizeOfOutputData == NULL))
    {
#ifdef DEBUG_API_AUTOCONF
        printf("DEBUG_API > API: TPS_GetSubmoduleConfiguration() > Error: Invalid parameter!\n");
#endif
        return API_AUTOCONF_PARAMETER_NULL;
    }

    /* Read out the module ident number. */
    TPS_GetValue32((USIGN8*)poSubslot->pt_ident_number, pdwSubmoduleIdentNumber);

    /* Read out the IO configuration. */
    TPS_GetValue16((USIGN8*)poSubslot->pt_size_input_data, pwSizeOfInputData);
    TPS_GetValue16((USIGN8*)poSubslot->pt_size_output_data, pwSizeOfOutputData);

#ifdef DEBUG_API_AUTOCONF
    printf("DEBUG_API > API: TPS_GetSubmoduleConfiguration() > ModuleIdentNumber: 0x%2.2X "
        "SizeOfInputData: 0x%2.2X SizeOfOutputData: 0x%2.2X\n",
        *pdwSubmoduleIdentNumber, *pwSizeOfInputData, *pwSizeOfOutputData);
#endif

    return TPS_ACTION_OK;
}

/*!@} AutoConfig Interface*/
#endif

/*!
 * \brief       This function sets the state of the given module. Should be called for each configured slot
 *              in your OnConnect callback to create a proper ModuleDiffBlock in the Connect Response.
 *
 * \param[in]   pzSlot the handle of the slot
 * \param[in]   zModuleState the state of the module
 *              <br>MODULE_OK: Expected configuration of the PLC can be adapted by this application.
 *                         dwModuleIdentNumber is not evaluated.
 *              <br>MODULE_WRONG: Expected configuration of the PLC can not be adapted by this application.
 *                            The module ID in parameter dwModuleIdentNumber is used in the ModuleDiffBlock.
 *              <br>MODULE_SUBSTITUTE: Expected configuration of the PLC can be used with restrictions by this application.
 *                                 The module ID in parameter dwModuleIdentNumber is used in the ModuleDiffBlock.
 *              <br>MODULE_NO: No module e.g. a pulled module.
 *                         dwModuleIdentNumber is not evaluated.
 * \param[in]   dwModuleIdentNumber this value is evaluated only if zModuleState is MODULE_WRONG or MODULE_SUBSTITUTE.
 *                                  This module ID is used as the real ID in the ModuleDiffBlock.
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - API_AUTOCONF_PARAMETER_NULL
 *              - API_AUTOCONF_PARAM_OUT_OF_RANGE
 */
USIGN32 TPS_SetModuleState(SLOT* pzSlot, T_MODULE_STATE zModuleState, USIGN32 dwModuleIdentNumber)
{
    USIGN32 dwRetval = TPS_ACTION_OK;

    /* Check the API State. */
    if(g_byApiState != STATE_DEVICE_STARTED)
    {
        #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_SetModuleState() > Error: Wrong API State!\n");
        #endif
        return API_AUTOCONF_WRONG_API_STATE;
    }

    /* Check the parameter. */
    if(pzSlot == NULL)
    {
        #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_SetModuleState() > Error: Invalid parameter!\n");
        #endif
        return API_AUTOCONF_PARAMETER_NULL;
    }

#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    if(pzSlot->wProperties == MODULE_PULLED)
    {
        /* Forcing NO Module in case of pulled module. */
        zModuleState = MODULE_NO;
    }
#endif

    /* Set the new state. */
    switch(zModuleState)
    {
        case MODULE_OK:
            TPS_SetValue16((USIGN8*)pzSlot->pt_module_state, PROPER_MODULE);
            break;
        case MODULE_WRONG:
            /* Wrong module will show up in the module diff block as
               module substitute with each contained submodule wrong          */
            TPS_SetValue16((USIGN8*)pzSlot->pt_module_state, WRONG_MODULE);
            TPS_SetValue32((USIGN8*)pzSlot->pt_ident_number, dwModuleIdentNumber);
            break;
        case MODULE_SUBSTITUTE:
            TPS_SetValue16((USIGN8*)pzSlot->pt_module_state, SUBSTITUTE_MODULE);
            TPS_SetValue32((USIGN8*)pzSlot->pt_ident_number, dwModuleIdentNumber);
            break;

        case MODULE_NO:
            TPS_SetValue16((USIGN8*)pzSlot->pt_module_state, NO_MODULE);
            break;

        default:
            #ifdef DEBUG_API_AUTOCONF
                printf("DEBUG_API > API: TPS_SetModuleState() > Error: zModuleState out of range!\n");
            #endif
            dwRetval = API_AUTOCONF_PARAM_OUT_OF_RANGE;
            break;
    }

    return dwRetval;
}

/*!
 * \brief       This function sets the state of the given module. Should be called for each configured submodule
 *              in your OnConnect callback to create a proper ModuleDiffBlock in the Connect Response.
 *
 * \param[in]   pzSubslot the handle of the subslot
 * \param[in]   zSubmoduleState the state of the submodule
 *              <br>MODULE_OK: Expected configuration of the PLC can be adapted by this application.
 *                         <br>dwSubmoduleIdentNumber is not evaluated.
 *              <br>MODULE_WRONG: Expected configuration of the PLC can not be adapted by this application.
 *                            <br>The module ID in parameter dwSubmoduleIdentNumber is used in the ModuleDiffBlock.
 *              <br>MODULE_SUBSTITUTE: Expected configuration of the PLC can be used with restrictions by this application.
 *                                 <br>dwSubmoduleIdentNumber is not evaluated.
 *              <br>MODULE_NO: No module e.g. a pulled module.
 *                         <br>dwSubmoduleIdentNumber is not evaluated.
 * \param[in]   dwSubmoduleIdentNumber this value is evaluated only if zSubmoduleState is MODULE_WRONG.
 *                                  This submodule ID is used as the real ID in the ModuleDiffBlock.
 * \retval      USIGN32
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - API_AUTOCONF_PARAMETER_NULL
 *              - API_AUTOCONF_PARAM_OUT_OF_RANGE
 *              - API_AUTOCONF_SUBMODULE_IS_PULLED
 */
USIGN32 TPS_SetSubmoduleState(SUBSLOT* pzSubslot, T_MODULE_STATE zSubmoduleState, USIGN32 dwSubmoduleIdentNumber)
{
    USIGN32 dwRetval = TPS_ACTION_OK;
#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    USIGN16 wSubSlotProperties = 0x00;
#endif

    /* Check the API State. */
    if(g_byApiState != STATE_DEVICE_STARTED)
    {
        #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_SetModuleState() > Error: Wrong API State!\n");
        #endif
        return API_AUTOCONF_WRONG_API_STATE;
    }

    /* Check the parameter. */
    if(pzSubslot == NULL)
    {
        #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_SetSubmoduleState() > Error: Invalid parameter!\n");
        #endif
        return API_AUTOCONF_PARAMETER_NULL;
    }

    if((zSubmoduleState == MODULE_WRONG) && (dwSubmoduleIdentNumber == AUTOCONF_MODULE))
    {
        #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_SetSubmoduleState() > Error: In case of Wrong Module the IdentNumber 0x00 is not allowed!\n");
        #endif
        return API_AUTOCONF_PARAM_OUT_OF_RANGE;
    }

#ifdef PLUG_RETURN_SUBMODULE_ENABLE
    TPS_GetValue16((USIGN8*)pzSubslot->pt_properties, &wSubSlotProperties);
    if((wSubSlotProperties & SUBMODULE_PROPERTIES_MASK) == SUBMODULE_PULLED)
    {
        #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_SetSubmoduleState() > Error: Module is pulled, setting of ModuleState not allowed! Has to be plugged again first.\n");
        #endif
        return API_AUTOCONF_SUBMODULE_IS_PULLED;
    }
#endif

    /* Set the new state. */
    switch(zSubmoduleState)
    {
        case MODULE_OK:
            /* Module OK: Nothing to do here. */
            TPS_SetValue16((USIGN8*)pzSubslot->pt_submodule_state, SUBMODULE_OK);
            TPS_SetValue16((USIGN8*)pzSubslot->pt_operational_state, IOXS_GOOD);
            break;

        case MODULE_WRONG:
            /* Set the expected Module ID, this creates ModuleDiff with State: WrongModule. */
            TPS_SetValue32((USIGN8*)pzSubslot->pt_ident_number, dwSubmoduleIdentNumber);
            TPS_SetValue16((USIGN8*)pzSubslot->pt_submodule_state, WRONG_SUBMODULE);
            TPS_SetValue16((USIGN8*)pzSubslot->pt_operational_state, IOXS_BAD_BY_SUBSLOT);
            break;

        case MODULE_SUBSTITUTE:
            TPS_SetValue16((USIGN8*)pzSubslot->pt_submodule_state, SUBSTITUTE_SUBMODULE);
            TPS_SetValue16((USIGN8*)pzSubslot->pt_operational_state, IOXS_GOOD);
            break;

        case MODULE_NO:
            TPS_SetValue16((USIGN8*)pzSubslot->pt_submodule_state, NO_SUBMODULE);
            TPS_SetValue16((USIGN8*)pzSubslot->pt_operational_state, IOXS_BAD_BY_SUBSLOT);
            break;

        default:
            #ifdef DEBUG_API_AUTOCONF
            printf("DEBUG_API > API: TPS_SetSubmoduleState() > Error: zModuleState out of range!\n");
            #endif

            dwRetval = API_AUTOCONF_PARAM_OUT_OF_RANGE;
            break;
    }

    return dwRetval;
}


#ifdef PLUG_RETURN_SUBMODULE_ENABLE

/*! \addtogroup  PullPlug Pull & Plug Interface
*@{
*/

/*!
 * \brief       This function deactivates a module in the device configuration. Each
 *              submodule is also pulled. If successful the application is responsible
 *              to send a PULL_ALARM for the module representative (submodule number 0)
 *              afterwards if one or more ARs are established for the affected module.
 *
 * \param[in]   dwApi
 * \param[in]   wSlotNumber
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - PULL_MODULE_NOT_FOUND
 *              - PULL_MODULE_ALREADY_PULLED
 *              - PULL_SUB_MODULE_0_NOT_ALLOWED
 *              - PULL_PLUG_WRONG_OP_MODE
 */
USIGN32 TPS_PullModule(USIGN32 dwApi, USIGN16 wSlotNumber)
{
    USIGN32 dwRetval = TPS_ACTION_OK;
    SLOT* poSlot = NULL;
    SUBSLOT* poSubslot = NULL;
    USIGN16 wSubslotNumber = 0x00;

    poSlot = AppGetSlotHandle(dwApi, wSlotNumber);

    if (poSlot == NULL)
    {
        return PULL_MODULE_NOT_FOUND;
    }

    if (poSlot->wProperties != MODULE_NOT_PULLED)
    {
        dwRetval = PULL_MODULE_ALREADY_PULLED;
    }

    if (dwRetval == TPS_ACTION_OK)
    {
        poSlot->wProperties = MODULE_PULLED;
        /* Set the ModuleState to generate a diffblock on next connect. */
        dwRetval = TPS_SetModuleState(poSlot, MODULE_NO, 0x00);
    }

    if (dwRetval == TPS_ACTION_OK)
    {
        /* Now pull all submodules of this slot. */
        for (poSubslot = poSlot->pSubslot; poSubslot != NULL; poSubslot = poSubslot->poNextSubslot)
        {
            TPS_GetValue16((USIGN8*)poSubslot->pt_subslot_number, &wSubslotNumber);
            dwRetval = TPS_PullSubmodule(dwApi, wSlotNumber, wSubslotNumber);
            if ((dwRetval != TPS_ACTION_OK) &&
                (dwRetval != PULL_SUB_MODULE_ALREADY_REMOVED))
            {
                break;
            }
        }
    }

    if (dwRetval == PULL_SUB_MODULE_ALREADY_REMOVED)
    {
        dwRetval = TPS_ACTION_OK;
    }

    return dwRetval;
}

/*!
 * \brief       This function deactivates a submodule in the device
 *              configuration. If successful the application is responsible to send a
 *              pull alarm for the pulled submodule afterwards if an AR is established
 *              for the affected submodule.
 *
 * \param[in]   dwApi
 * \param[in]   wSlotNumber
 * \param[in]   wSubslotNumber
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - PULL_MODULE_NOT_FOUND
 *              - PULL_MODULE_ALREADY_PULLED
 *              - PULL_SUB_MODULE_0_NOT_ALLOWED
 *              - PULL_PLUG_WRONG_OP_MODE
 */
USIGN32 TPS_PullSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber)
{
    if (wSubslotNumber == 0x00)
    {
        return PULL_SUB_MODULE_0_NOT_ALLOWED;
    }

    return AppPullPlugSubmodule(dwApi, wSlotNumber, wSubslotNumber, OP_MODE_GO_PULL);
}

/*!
 * \brief       This function re-plugs a previously pulled module but not the included submodules.
                No alarm needs to be sent after replugging a module.
 *
 * \param[in]   dwApi
 * \param[in]   wSlotNumber
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - PULL_MODULE_NOT_FOUND
 *              - PULL_MODULE_NOT_PULLED
 *              - PULL_SUB_MODULE_0_NOT_ALLOWED
 *              - PULL_PLUG_WRONG_OP_MODE
 *              - TPS_ERROR_WRONG_API_STATE
 *              - API_AUTOCONF_PARAMETER_NULL
 *              - API_AUTOCONF_PARAM_OUT_OF_RANGE
 */
USIGN32 TPS_RePlugModule(USIGN32 dwApi, USIGN16 wSlotNumber)
{
    USIGN32 dwRetval = TPS_ACTION_OK;
    SLOT* pzSlot = NULL;

    pzSlot = AppGetSlotHandle(dwApi, wSlotNumber);

    if (pzSlot == NULL)
    {
        return PULL_MODULE_NOT_FOUND;
    }

    if (pzSlot->wProperties != MODULE_PULLED)
    {
        dwRetval = PULL_MODULE_NOT_PULLED;
    }

    if (dwRetval == TPS_ACTION_OK)
    {
        pzSlot->wProperties = MODULE_NOT_PULLED;
        /* Set as ok again. In case of Autoconfiguration this may be overwritten in PrmEnd. */
        dwRetval = TPS_SetModuleState(pzSlot, MODULE_OK, 0x00);
    }

    return dwRetval;
}

/*!
* \brief       This function re-plugs a previously pulled submodule.
*              If successful the submodule enters the state SUBMODULE_REPLUGGED and if an AR is
*              established for this submodule the application must send a Plug-Alarm for this submodule
*              (TPS_SendAlarm()). The PLC will then send the submodule's initial parameters so that the
*              registered OnPrmEndCallback() will be called. In this callback the parameters must be
*              evaluated and the submodule must be reactivated (TPS_ReactivateSubmodule() )
*
* \param[in]   dwApi
* \param[in]   wSlotNumber
* \param[in]   wSubslotNumber
* \retval      possible return values:
*              - TPS_ACTION_OK : success
*              - PULL_MODULE_NOT_FOUND
*              - PULL_MODULE_NOT_PULLED
*              - PULL_SUB_MODULE_0_NOT_ALLOWED
*              - PULL_PLUG_WRONG_OP_MODE
*              - TPS_ERROR_WRONG_API_STATE
*              - API_AUTOCONF_PARAMETER_NULL
*              - API_AUTOCONF_PARAM_OUT_OF_RANGE
*/

USIGN32 TPS_RePlugSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber)
{
    if (wSubslotNumber == 0x00)
    {
        return PULL_SUB_MODULE_0_NOT_ALLOWED;
    }

    return AppPullPlugSubmodule(dwApi, wSlotNumber, wSubslotNumber, OP_MODE_GO_RETURN);
}
/*!@} Pull & Plug Interface*/

#endif /*PLUG_RETURN_SUBMODULE_ENABLE*/

/*!
 * \brief       This function reactivates a submodule after the initial parameters were checked.
 *              This happens if a submodule is replugged or if a submodule was transferred from one PLC
 * 				to another in Shared Device Operation.
 *              Should be called in your registered OnPrmEndCallback (TPS_EVENT_ON_PRM_END_DONE_IOARx)
 *
 * \param[in]   dwApi
 * \param[in]   wSlotNumber
 * \param[in]   wSubslotNumber
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - PULL_MODULE_NOT_FOUND
 *              - PULL_MODULE_NOT_PULLED
 *              - PULL_SUB_MODULE_0_NOT_ALLOWED
 *              - PULL_PLUG_WRONG_OP_MODE
 *              - TPS_ERROR_WRONG_API_STATE
 *              - API_AUTOCONF_PARAMETER_NULL
 *              - API_AUTOCONF_PARAM_OUT_OF_RANGE
 */
USIGN32 TPS_ReactivateSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber)
{
    SUBSLOT* pzSubslot = NULL;

    if (wSubslotNumber == 0x00)
    {
        return REACTIVATE_SUBMODULE_0_NOT_ALLOWED;
    }

    pzSubslot = AppGetSubslotHandle(dwApi, wSlotNumber, wSubslotNumber);
    if(pzSubslot == NULL)
    {
        return TPS_GetLastError();
    }


    return AppReactivateSubmodule(pzSubslot);
}





#ifdef USE_TPS_COMMUNICATION_CHANNEL

/*! \addtogroup   TPS_Communication_Channel TPS Communication Interface
*@{
*/

/*!
 * \brief       This function initializes the receive and transmit buffers for special TSP communication.
                Through this interface PROFINET records can be read or written by sending a implicit
                record read or write request through the dual ported RAM.
                The answer to this request is indicated by the Event TPS_EVENT_TPS_MESSAGE. A callback
                function should be registered for this event.
 *
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined. See also TPS_RecordWriteReqToStack(), TPS_RecordReadReqToStack()
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - ETH_OUT_OF_MEMORY
 */
USIGN32 TPS_InitTPSComChannel(VOID)
{
    if(g_byApiState != STATE_SUBSLOT_RDY)
    {
        return TPS_ERROR_WRONG_API_STATE;
    }

    if(((g_pbyCurrentPointer + ((MAX_LEN_ETHERNET_FRAME + 4) * 2)) - g_pbyConfigNRTMem) > g_dwConfigNRTMemSize)
    {
        #ifdef DEBUG_API_TEST
            printf("DEBUG_API > init_eth_channel: Not enough memory for ethernet communication.\n");
        #endif
        return ETH_OUT_OF_MEMORY;
    }

    /* Configuration RX mailbox. */
    g_poTPSIntRXMailbox = (T_ETHERNET_MAILBOX*)((g_pbyCurrentPointer + 4) - ((USIGN32)g_pbyCurrentPointer % 4));

    TPS_SetValue32((USIGN8*)&g_poTPSIntRXMailbox->dwLength, MAX_LEN_ETHERNET_FRAME);
    TPS_SetValue32((USIGN8*)&g_poTPSIntRXMailbox->dwPortnumber, 3);
    TPS_SetValueData((USIGN8*)&g_poTPSIntRXMailbox->byFrame, (USIGN8*)&"\xC0\x0C\xBE\xED", 4);

    /* Configuration TX mailbox. */
    g_pbyCurrentPointer += MAX_LEN_ETHERNET_FRAME + 8;

    g_poTPSIntTXMailbox = (T_ETHERNET_MAILBOX*)((g_pbyCurrentPointer + 4) - ((USIGN32)g_pbyCurrentPointer % 4));
    TPS_SetValue32((USIGN8*)&g_poTPSIntTXMailbox->dwLength, MAX_LEN_ETHERNET_FRAME);
    TPS_SetValue32((USIGN8*)&g_poTPSIntTXMailbox->dwPortnumber, 3);
    TPS_SetValueData((USIGN8*)&g_poTPSIntTXMailbox->byFrame, (USIGN8*)&"\xED\xBE\x0C\xC0", 4);

    g_pbyCurrentPointer += MAX_LEN_ETHERNET_FRAME + 8;

    g_byApiState = STATE_TPS_CHANNEL_RDY;

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function returns the version information of the actual TPS stack (TPS Firmware). This version
 *              is transmitted through the TPS Communication Interface so it has to be initiated first.
 *              (see TPS_InitTPSComChannel())
 *
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_GetStackVersionInfo(VOID)
{
    USIGN16 wSlotNr = 0x00;
    USIGN16 wSubslotNr = 0x01;
    USIGN16 wIndex = 0x7FF0;
    USIGN32 dwDataLength = 0x1000;
    USIGN32 dwRetval = TPS_ACTION_OK;

    /* Read Version info from TPS-1.                                         */
    /*-----------------------------------------------------------------------*/
    dwRetval = TPS_RecordReadReqToStack(wSlotNr, wSubslotNr, wIndex, dwDataLength);

    return(dwRetval);
}

/*!
 * \brief       This function sends request for reading of the user data from TPS flash. The user data
 *              will be sent from TPS separately in response frame after reading the data from flash.
 *              This function uses the TPS Communication Interface so it has to be initiated first.
 *              (see TPS_InitTPSComChannel())
 * \param[in]   wAddr (< 4094)
 * \param[in]   wDataLength ((wAddr + wDataLength) <= 4094)
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_ReadUserDataFromFlash(USIGN16 wAddr, USIGN16 wLength)
{
   USIGN16 wSlotNr = 0x00;
   USIGN16 wSubslotNr = 0x01;
   USIGN16 wIndex = RECORD_INDEX_USER_DATA_IN_TPS_FLASH;
   USIGN32 dwDataLength = ((wAddr << 16) | wLength);
   USIGN32 dwRetval = TPS_ACTION_OK;

#ifdef DEBUG_API
   printf("\nDEBUG_API > TPS_ReadUserDataFromFlash: read %i Bytes @ %i\n", wLength, wAddr);
#endif
   if((wAddr + wLength) > 4094)
   {
      return API_TPS_FLASH_USER_AREA_OVERRUN;
   }
   else if((wLength == 0) || (wLength > MAX_RECORD_DATA_LENGTH))
   {
      return API_TPS_FLASH_USER_INVALID_LENGTH;
   }

   /* Read user data from the flash memory of TPS-1.                                         */
   /*-----------------------------------------------------------------------*/
   dwRetval = TPS_RecordReadReqToStack(wSlotNr, wSubslotNr, wIndex, dwDataLength);

   return(dwRetval);
}
/*!
 * \brief       This function writes user data to TPS flash. The response
 *              will be sent from TPS separately after writing the data to flash.
 *              This function uses the TPS Communication Interface so it has to be initiated first.
 *              (see TPS_InitTPSComChannel())
 * \param[in]   wAddr (< 4094)
 * \param[in]   wDataLength ((wAddr + wDataLength) <= 4094)
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_WriteUserDataToFlash(USIGN16 wAddr, USIGN8* pbyUserData, USIGN16 wLength)
{
    USIGN16 wSlotNr = 0x00;
    USIGN16 wSubslotNr = 0x01;
    USIGN16 wIndex = RECORD_INDEX_USER_DATA_IN_TPS_FLASH;
    USIGN32 dwDataLength = ((wAddr << 16) | wLength);
    USIGN32 dwRetval = TPS_ACTION_OK;

#ifdef DEBUG_API
    printf("\nDEBUG_API > TPS_WriteUserDataToFlash: write %i Bytes @ %i\n", wLength, wAddr);
#endif
    if((wAddr + wLength) > 4094)
    {
       return API_TPS_FLASH_USER_AREA_OVERRUN;
    }
    else if((wLength == 0) || (wLength > MAX_RECORD_DATA_LENGTH))
    {
       return API_TPS_FLASH_USER_INVALID_LENGTH;
    }

    /* Write user data into the flash memory of TPS-1.                           */
    /*-----------------------------------------------------------------------*/
    dwRetval = TPS_RecordWriteReqToStack(wSlotNr, wSubslotNr, wIndex, pbyUserData, dwDataLength);

    return(dwRetval);
}

/*!
 * \brief       <b>For testing testing purposes only.</b> This function sends a special record to the TPS-1 that
 *              creates a dead lock in the TPS-1 Firmware. This triggers the internal watchdog of TPS-1 CPU
 *              (see WD_OUT in TPS_Datasheet_EN.pdf). The TPS-1 will reset afterwards.
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined
 *              and the TPS 1 Communication Interface must be initialized.
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_TestInternalWatchdog(VOID)
{
    USIGN16 wSlotNr = 0x00;
    USIGN16 wSubslotNr = 0x01;
    USIGN16 wIndex = RECORD_INDEX_TPS1_WATCHDOG_TEST;
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN8 pbyUserData[2];

#ifdef DEBUG_API
    printf("\nDEBUG_API > TPS_TestInternalWatchdog\n");
#endif
    /* Write user data into the flash memory of TPS-1.                           */
    /*-----------------------------------------------------------------------*/
    dwRetval = TPS_RecordWriteReqToStack(wSlotNr, wSubslotNr, wIndex, pbyUserData, sizeof(pbyUserData));

    return(dwRetval);
}

/*!
 * \brief       <b>For testing purposes only.</b> This function sends a special record to the TPS-1 that
 *              set the LEDs of TPSs and deactivates/activates the TPS-1 as source for led control.
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined
 *              and the TPS 1 Communication Interface must be initialized.
 * \param[in]    byActivate: 0- normal PN operation of LEDs, followed [in] parameters are ignored;
 *                           1- setting of LEDs by Applikation. Disables the LED-operation of PN-Stack;
 * \param[in]    byRunLed:   RUN-LED is Off(1)/On(0);
 * \param[in]    byMtLed:    MT-LED is Off(1)/On(0);
 * \param[in]    bySfLed:    SF-LED is Off(1)/On(0);
 * \param[in]    byBfLed:    BF-LED is Off(1)/On(0);
 *
 * \retval       possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_TestLEDs(USIGN8 byActivate, USIGN8 byRunLed, USIGN8 byMtLed, USIGN8 bySfLed, USIGN8 byBfLed)
{
    USIGN16 wSlotNr = 0x00;
    USIGN16 wSubslotNr = 0x01;
    USIGN16 wIndex = RECORD_INDEX_TPS1_LED_TEST;
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN8 pbyUserData[2];

    pbyUserData[0] =  byActivate;
    pbyUserData[1] = (((byRunLed&0x1)<<3)|((byMtLed&0x1)<<2)|((bySfLed&0x1)<<1)|(byBfLed&0x1));

#ifdef DEBUG_API
    printf("\nDEBUG_API > TPS_Test LEDs\n");
#endif
    /* Write user data into the flash memory of TPS-1.                           */
    /*-----------------------------------------------------------------------*/
    dwRetval = TPS_RecordWriteReqToStack(wSlotNr, wSubslotNr, wIndex, pbyUserData, sizeof(pbyUserData));

    return dwRetval;
}

/*!
 * \brief       This function sends a record write request to the TPS-1 stack using the TPS Communication Interface.
 *              So this interface has to be initiated first.
 *              The answer to this request is indicated by the event TPS_EVENT_TPS_MESSAGE which calls the associated
 *              callback function.
 *
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \param[in]   wSlotNr slotnumber
 * \param[in]   wSubslotNr subslot number
 * \param[in]   wIndex record index
 * \param[in]   pbyData pointer to the data buffer that shall be sent
 * \param[in]   dwDataLength [15..0] length of the data at pbyData max MAX_RECORD_DATA_LENGTH
 *                           [31..16] memory offset used for ofset in flash
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_RecordWriteReqToStack(USIGN16 wSlotNr, USIGN16 wSubslotNr,
    USIGN16 wIndex, USIGN8* pbyData,
    USIGN32 dwDataLength)
{
    USIGN8* pbyPacket = NULL;
    BLOCK_HEADER* pzBlockHeader = NULL;
    USIGN8* pbyWriteRecord = NULL;
    USIGN8* pbyRecordData = NULL;
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN16 wValue = 0;
    USIGN32 dwValue = 0;
    USIGN32 dwDataLenShort = (dwDataLength & 0xFFFF);

    /* Create frame*/
    pbyPacket = malloc(dwDataLenShort + sizeof(RW_RECORD_REQ_BLOCK)
        + sizeof(BLOCK_HEADER) + sizeof(ARGS_REQ));

    pbyWriteRecord = pbyPacket + sizeof(BLOCK_HEADER);
    pzBlockHeader = (BLOCK_HEADER*)pbyPacket;
    pbyRecordData = pbyWriteRecord + 58;

    if (pbyPacket != NULL)
    {
        memset(pbyPacket, 0, dwDataLenShort + sizeof(RW_RECORD_REQ_BLOCK)
            + sizeof(BLOCK_HEADER) + sizeof(ARGS_REQ));

        /* Block_Header*/
        pzBlockHeader->type = TPS_htons(WRITERECORD_REQ); /* Record Write*/
        pzBlockHeader->length = TPS_htons(sizeof(RW_RECORD_REQ_BLOCK) + 2);
        pzBlockHeader->version = TPS_htons(0x0100);
        /* Record_WRITE */
        /*SeqNr.*/
        wValue = TPS_htons(0x01);
        memcpy(pbyWriteRecord, &wValue, sizeof(wValue));
        pbyWriteRecord += 2;
        /*api*/
        dwValue = TPS_htonl(0x00);
        memcpy(pbyWriteRecord, &dwValue, sizeof(dwValue));
        pbyWriteRecord += 4;
        /*Slot-Nr*/
        wValue = TPS_htons(wSlotNr);
        memcpy(pbyWriteRecord, &wValue, sizeof(wValue));
        pbyWriteRecord += 2;
        /*SubSlot-Nr*/
        wValue = TPS_htons(wSubslotNr);
        memcpy(pbyWriteRecord, &wValue, sizeof(wValue));
        pbyWriteRecord += 2;
        /*padding*/
        wValue = TPS_htons(0x00);
        memcpy(pbyWriteRecord, &wValue, sizeof(wValue));
        pbyWriteRecord += 2;
        /*Index*/
        wValue = TPS_htons(wIndex);
        memcpy(pbyWriteRecord, &wValue, sizeof(wValue));
        pbyWriteRecord += 2;
        /* Record-Data Length*/
        dwValue = TPS_htonl(dwDataLength);
        memcpy(pbyWriteRecord, &dwValue, sizeof(dwValue));
        pbyWriteRecord += 4;

        memcpy(pbyWriteRecord + 2, pbyWriteRecord - sizeof(UUID_TAG) + 2, sizeof(UUID_TAG));
        memcpy(pbyRecordData, pbyData, dwDataLenShort);

        dwRetval = TPS_SendEthernetFrame(pbyPacket, dwDataLenShort + sizeof(RW_RECORD_REQ_BLOCK) + sizeof(BLOCK_HEADER), PORT_NR_INTERNAL);

#ifdef DEBUG_API_TEST
        if (dwRetval != TPS_ACTION_OK)
        {
            printf("DEBUG_API > Error TPS_RecordWriteReqToStack(): MAILBOX IS NOT EMPTY!!\n");
        }
#endif

        free(pbyPacket);
    }
    else
    {
        dwRetval = API_RECORD_IF_OUT_OF_MEMORY;
    }

    return dwRetval;
}


/*!
 * \brief       This function sends a record read request to the TPS-1 stack using the TPS Communication Interface.
 *              So this interface has to be initiated first.
 *              The answer to this request is indicated by the event TPS_EVENT_TPS_MESSAGE which calls the associated
 *              callback function.
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \param[in]   wSlotNr slot number
 * \param[in]   wSubslotNr subslot number
 * \param[in]   wIndex record index
 * \param[in]   dwDataLength length of the data buffer at pbyData
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_RecordReadReqToStack(USIGN16 wSlotNr, USIGN16 wSubslotNr,
    USIGN16 wIndex, USIGN32 dwDataLength)
{
    USIGN8* pbyPacket = NULL;
    BLOCK_HEADER* pzBlockHeader = NULL;
    USIGN8* pbyReadRecord = NULL;
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN16 wValue = 0;
    USIGN32 dwValue = 0;

    /* Create frame*/
    pbyPacket = malloc(sizeof(RW_RECORD_REQ_BLOCK) + sizeof(BLOCK_HEADER) + sizeof(ARGS_REQ));

    pbyReadRecord = pbyPacket + sizeof(BLOCK_HEADER);
    pzBlockHeader = (BLOCK_HEADER*)pbyPacket;

    if (pbyPacket != NULL)
    {
        memset(pbyPacket, 0, sizeof(RW_RECORD_REQ_BLOCK) + sizeof(BLOCK_HEADER) + sizeof(ARGS_REQ));

        /* Block_Header*/
        pzBlockHeader->type = TPS_htons(READRECORD_REQ); /* Record Read */
        pzBlockHeader->length = TPS_htons(sizeof(RW_RECORD_REQ_BLOCK) + 2);
        pzBlockHeader->version = TPS_htons(0x0100);

        /* Record Read */
        /* SeqNr.      */
        wValue = TPS_htons(0x01);
        memcpy(pbyReadRecord, &wValue, sizeof(wValue));
        pbyReadRecord += 2;
        /* Api        */
        dwValue = TPS_htonl(0x00);
        memcpy(pbyReadRecord, &dwValue, sizeof(dwValue));
        pbyReadRecord += 4;
        /* Slot-Nr    */
        wValue = TPS_htons(wSlotNr);
        memcpy(pbyReadRecord, &wValue, sizeof(wValue));
        pbyReadRecord += 2;
        /* SubSlot-Nr */
        wValue = TPS_htons(wSubslotNr);
        memcpy(pbyReadRecord, &wValue, sizeof(wValue));
        pbyReadRecord += 2;
        /* Padding    */
        wValue = TPS_htons(0x00);
        memcpy(pbyReadRecord, &wValue, sizeof(wValue));
        pbyReadRecord += 2;
        /* Index      */
        wValue = TPS_htons(wIndex);
        memcpy(pbyReadRecord, &wValue, sizeof(wValue));
        pbyReadRecord += 2;
        /* Record-Data Length*/
        dwValue = TPS_htonl(dwDataLength);
        memcpy(pbyReadRecord, &dwValue, sizeof(dwValue));
        pbyReadRecord += 4;
        /* UUID Tag. */
        memcpy(pbyReadRecord + 2, (pbyReadRecord - sizeof(UUID_TAG)) + 2, sizeof(UUID_TAG));

        dwRetval = TPS_SendEthernetFrame(pbyPacket, sizeof(RW_RECORD_REQ_BLOCK) + sizeof(BLOCK_HEADER), PORT_NR_INTERNAL);

#ifdef DEBUG_API_TEST
        if (dwRetval != TPS_ACTION_OK)
        {
            printf("DEBUG_API > Error TPS_ReadRecordFromTps(): MAILBOX IS NOT EMPTY!!\n");
        }
#endif

        free(pbyPacket);
    }
    else
    {
        dwRetval = API_RECORD_IF_OUT_OF_MEMORY;
    }

    return dwRetval;
}


/*!
 * \brief       This function changes the configuration of the TPS-1 using the TPS Communication Interface.
 *              This is an alternate path to write the configuration into the TPS-1.
 *              A template file with memory offsets are generated by the TPS Configurator (FS_PROG.exe)
 *              and saved in the file <yourConfiguration>.c
 *
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \param[in]   pbyConfigurationData pointer to the configuration data buffer
 * \param[in]   wDataLength length of the configuration data
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_CONFIG_INVALID_PARAMETER
 *              - API_CONFIG_OUT_OF_MEMORY
 */
USIGN32 TPS_WriteConfigBlockToFlash(USIGN8* pbyConfigurationData, USIGN16 wDataLength)
{
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN8* pbyEthernetPacket = NULL;
    USIGN8* pbyCurrentPosition = NULL;
    USIGN8  pbyCurrentMac[MAC_ADDRESS_SIZE] = { 0 };
    UDP_IP_HEADER* ptIpUdpHdr = NULL;
    USIGN32 pdwIPAddress; 
    USIGN32 pdwSubnetMask; 
    USIGN32 pdwGateway;

    if ((pbyConfigurationData == NULL) || (wDataLength == 0x00))
    {
        return API_CONFIG_INVALID_PARAMETER;
    }

    /* Allocate the needed memory. */
    pbyEthernetPacket = calloc(ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE + wDataLength + ETHERNET_CRC_SIZE, 1);
    if (pbyEthernetPacket == NULL)
    {
        return API_CONFIG_OUT_OF_MEMORY;
    }

    pbyCurrentPosition = pbyEthernetPacket;

    /* The configuration is sent via the internal mailbox interface to the TPS-1.
    * The packet is coded like the packet from the TPS Configurator, so here it is
    * needed to build a reduced ethernet and udp header. */

    /* Get the current MAC Address, the packet will be sent to this Interface. */
    dwRetval = TPS_GetMacAddresses(pbyCurrentMac, NULL, NULL);
    
    if(dwRetval == TPS_ACTION_OK)
    {
        dwRetval = TPS_GetIPConfig(&pdwIPAddress, &pdwSubnetMask, &pdwGateway);
    }
    

    /* Build the packet. */
    if (dwRetval == TPS_ACTION_OK)
    {
        /* Ethernet Frame.
        * Only Target MAC is needed. */
        memcpy(pbyCurrentPosition, pbyCurrentMac, MAC_ADDRESS_SIZE);
        pbyCurrentPosition += ETHERNET_HEADER_SIZE;
        *(pbyCurrentPosition - 2) = TPS_htons(0x0800);

        ptIpUdpHdr = (UDP_IP_HEADER*)pbyCurrentPosition;
        ptIpUdpHdr->version_len = 0x45;
        ptIpUdpHdr->length = TPS_htons(IP_HEADER_SIZE + UDP_HEADER_SIZE + wDataLength);
        ptIpUdpHdr->protocol = UDP_PROTOCOL;
        ptIpUdpHdr->checksum = 0;
        ptIpUdpHdr->dst = TPS_htonl(pdwIPAddress);
        ptIpUdpHdr->src = TPS_htonl(pdwIPAddress);
        ptIpUdpHdr->dst_port = TPS_htons(65001);
        ptIpUdpHdr->udp_length = TPS_htons(UDP_HEADER_SIZE + wDataLength);
        ptIpUdpHdr->udp_checksum = 0;

        pbyCurrentPosition += sizeof(UDP_IP_HEADER);
        /* Payload. */
        memcpy(pbyCurrentPosition, pbyConfigurationData, wDataLength);
    }

    /* Sent the packet. */
    if (dwRetval == TPS_ACTION_OK)
    {
        dwRetval = TPS_SendEthernetFrame(pbyEthernetPacket,
            ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE + wDataLength + ETHERNET_CRC_SIZE,
            PORT_NR_INTERNAL);
    }

    free(pbyEthernetPacket);

    return dwRetval;
}
/*!@} TPS Communication Interface*/

/*! \addtogroup   dcpInterface DCP Interface
*@{
*/

/*!
 * \brief       Function for setting of device name permanently in flash memory
                of TPS-1 by using of PN-DCP request frame.
 *
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \param[in]   pbyDeviceName pointer to the device name to be set
 * \param[in]   wNameLength length of the name
 * \param[in]   bySetPermanent setting permanent (1) or not (0)
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 *              - API_DEVICE_NAME_TOO_LONG
 */
USIGN32 TPS_DcpSetDeviceName(USIGN8* pbyDeviceName, USIGN16 wNameLength, USIGN16 wSetPermanent)
{
    USIGN32 dwReturnValue = 0;

    USIGN8 byEthHeader[] = DCP_FRAME_HEADER;
    USIGN8* pbySendBuffer = 0;
    USIGN8* pdyActualPosition = 0;
    USIGN16 wLenOfSendBuffer = 0;

   static USIGN32 dwXidCounter = 0;

   if(wNameLength <= STATION_NAME_LEN)
   {

        wLenOfSendBuffer = sizeof(byEthHeader) + sizeof(DCP_SET_HEADER) + sizeof(DCP_BLOCK)*2 + wNameLength + wNameLength%2/*padding*/;

    pbySendBuffer = malloc(wLenOfSendBuffer);

    if(pbySendBuffer != 0)
    {
       pdyActualPosition = pbySendBuffer;

       /*Eth-Header*/
       memcpy(pdyActualPosition, &byEthHeader, sizeof(byEthHeader));
       /* Overwrite Destination with current MAC */
       TPS_GetMacAddresses(pdyActualPosition, NULL, NULL);
       pdyActualPosition += sizeof(byEthHeader);

       dwXidCounter += 1;

       ((DCP_SET_HEADER*)pdyActualPosition)->byServiceID = DCP_SERVICE_ID_SET;
           ((DCP_SET_HEADER*)pdyActualPosition)->byServiceType = DCP_SERVICE_ID_REQUEST;
           ((DCP_SET_HEADER*)pdyActualPosition)->dwXid = TPS_htonl(dwXidCounter);
           ((DCP_SET_HEADER*)pdyActualPosition)->wReserv = 0x0;
           ((DCP_SET_HEADER*)pdyActualPosition)->wDcpDataLength = TPS_htons(sizeof(DCP_BLOCK)*2 + wNameLength + wNameLength%2);

       pdyActualPosition += sizeof(DCP_SET_HEADER);

       ((DCP_BLOCK*)pdyActualPosition)->byOption = DCP_OPTION_DEV_PROP;    /*0x2 -> Device properties*/
           ((DCP_BLOCK*)pdyActualPosition)->bySubOption = DCP_SUBOPTION_NAME_OF_STATION; /*0x2 -> Name Of Sation*/
           ((DCP_BLOCK*)pdyActualPosition)->wDcpBlockLength = TPS_htons(0x2 + wNameLength);
           ((DCP_BLOCK*)pdyActualPosition)->wBlockQualifier = TPS_htons(!wSetPermanent & 0x1);

       pdyActualPosition += sizeof(DCP_BLOCK);

       memcpy(pdyActualPosition, pbyDeviceName, wNameLength);
       pdyActualPosition += wNameLength + wNameLength%2 ;

       ((DCP_BLOCK*)pdyActualPosition)->byOption = DCP_OPTION_CONTROL;    /*0x5 -> Control*/
           ((DCP_BLOCK*)pdyActualPosition)->bySubOption = DCP_SUBOPTION_END_TRANSACT; /*0x2 -> End Transaction*/
           ((DCP_BLOCK*)pdyActualPosition)->wDcpBlockLength = TPS_htons(2);
           ((DCP_BLOCK*)pdyActualPosition)->wBlockQualifier = 0;

       dwReturnValue = TPS_SendEthernetFrame( pbySendBuffer, wLenOfSendBuffer, PORT_NR_INTERNAL );

           free(pbySendBuffer);

    }
    else
    {
       dwReturnValue = API_RECORD_IF_OUT_OF_MEMORY;
        }
     }
     else
     {
        dwReturnValue = API_DEVICE_NAME_TOO_LONG;
     }

   return dwReturnValue;
}


/*!
 * \brief       Sends a DCP.SetIp request to the TPS-1 stack. This sets the IP configuration of the device permanently like it was set by an external
 *              source. This is Profinet compliant, so the TPS-1 Stack may reject the request for example if an AR is active.
 *              <br><br> If you want to temporarily set the ip configuration without caring for Profinet compliance, please refer to TPS_SetIPConfig().
 *
 * \note        To use this function <b>USE_TPS_COMMUNICATION_CHANNEL</b> in TPS_1_user.h must be defined.
 * \param[in]   dwIPAddr Ip Address
 * \param[in]   dwSubnetMask Subnet mask
 * \param[in]   dwGateway Default gateway
 * \param[in]   wSetPermanent use ip config permanently (0)
 *              or temporary (1). If set temporary the IP config is 0.0.0.0 after next reboot
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 *              - API_DEVICE_NAME_TOO_LONG
 */
USIGN32 TPS_DcpSetDeviceIP(USIGN32 dwIPAddr, USIGN32 dwSubnetMask, USIGN32 dwGateway, USIGN16 wSetPermanent)
{
    USIGN32 dwReturnValue = 0;

    USIGN8 byEthHeader[] = DCP_FRAME_HEADER;
    USIGN8* pbySendBuffer = 0;
    USIGN8* pdyActualPosition = 0;
    USIGN32 dwTemVarIpParam = 0;

   static USIGN32 dwXidCounter = 0;

   pbySendBuffer = malloc(MIN_LEN_ETHERNET_FRAME);

   if(pbySendBuffer != 0)
   {
       pdyActualPosition = pbySendBuffer;

       /*Eth-Header*/
       memcpy(pdyActualPosition, &byEthHeader, sizeof(byEthHeader));
       /* Overwrite Destination with current MAC */
       TPS_GetMacAddresses(pdyActualPosition, NULL, NULL);
       pdyActualPosition += sizeof(byEthHeader);
       dwXidCounter += 1;

       ((DCP_SET_HEADER*)pdyActualPosition)->byServiceID = DCP_SERVICE_ID_SET;
       ((DCP_SET_HEADER*)pdyActualPosition)->byServiceType = DCP_SERVICE_ID_REQUEST;
       ((DCP_SET_HEADER*)pdyActualPosition)->dwXid = TPS_htonl(dwXidCounter);
       ((DCP_SET_HEADER*)pdyActualPosition)->wReserv = 0x0;
       ((DCP_SET_HEADER*)pdyActualPosition)->wDcpDataLength = TPS_htons(sizeof(dwTemVarIpParam)*3 + sizeof(DCP_BLOCK));

       pdyActualPosition += sizeof(DCP_SET_HEADER);

       ((DCP_BLOCK*)pdyActualPosition)->byOption = DCP_OPTION_IP;    /*0x1 -> IP*/
       ((DCP_BLOCK*)pdyActualPosition)->bySubOption = DCP_SUBOPTION_IP_PARAM; /*0x2 -> IP parameter*/
       ((DCP_BLOCK*)pdyActualPosition)->wDcpBlockLength = TPS_htons(sizeof(dwTemVarIpParam)*3 + sizeof(wSetPermanent));
       ((DCP_BLOCK*)pdyActualPosition)->wBlockQualifier = TPS_htons(!wSetPermanent & 0x1);

       pdyActualPosition += sizeof(DCP_BLOCK);
       dwTemVarIpParam = TPS_htonl(dwIPAddr);
       memcpy(pdyActualPosition, &dwTemVarIpParam, sizeof(dwTemVarIpParam));
       pdyActualPosition+=4;
       dwTemVarIpParam = TPS_htonl(dwSubnetMask);
       memcpy(pdyActualPosition, &dwTemVarIpParam, sizeof(dwTemVarIpParam));
       pdyActualPosition+=4;
       dwTemVarIpParam = TPS_htonl(dwGateway);
       memcpy(pdyActualPosition, &dwTemVarIpParam, sizeof(dwTemVarIpParam));
       pdyActualPosition+=4;

       dwReturnValue = TPS_SendEthernetFrame( pbySendBuffer, MIN_LEN_ETHERNET_FRAME, PORT_NR_INTERNAL );

       free(pbySendBuffer);

   }
   else
   {
       dwReturnValue = API_RECORD_IF_OUT_OF_MEMORY;
   }


   return dwReturnValue;
}

/*!
 * \brief       Function for sending a DCP ResetToFactory request to the TPS-1 firmware.
 *
 * \param[in]   wBlockQualifier the block qualifier of the ResetToFactory-Request. Supported values: 2
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_RECORD_IF_OUT_OF_MEMORY
 */
USIGN32 TPS_DcpSetResetToFactory(USIGN16 wBlockQualifier)
{
   USIGN32 dwReturnValue = 0;
   USIGN8 byEthHeader[] = DCP_FRAME_HEADER;
   USIGN8* pbySendBuffer = 0;
   USIGN8* pdyActualPosition = 0;

   static USIGN32 dwXidCounter = 0;

   pbySendBuffer = malloc(MIN_LEN_ETHERNET_FRAME);

   if(pbySendBuffer != 0)
   {
       pdyActualPosition = pbySendBuffer;

       /*Eth-Header*/
       memcpy(pdyActualPosition, &byEthHeader, sizeof(byEthHeader));
       /* Overwrite Destination with current MAC */
       TPS_GetMacAddresses(pdyActualPosition, NULL, NULL);
       pdyActualPosition += sizeof(byEthHeader);

       dwXidCounter += 1;

       ((DCP_SET_HEADER*)pdyActualPosition)->byServiceID = DCP_SERVICE_ID_SET;
       ((DCP_SET_HEADER*)pdyActualPosition)->byServiceType = DCP_SERVICE_ID_REQUEST;
       ((DCP_SET_HEADER*)pdyActualPosition)->dwXid = TPS_htonl(dwXidCounter);
       ((DCP_SET_HEADER*)pdyActualPosition)->wReserv = 0x0;
       ((DCP_SET_HEADER*)pdyActualPosition)->wDcpDataLength = TPS_htons(12);

       pdyActualPosition += sizeof(DCP_SET_HEADER);

       ((DCP_BLOCK*)pdyActualPosition)->byOption = DCP_OPTION_CONTROL;    /*0x5 -> Control*/
       ((DCP_BLOCK*)pdyActualPosition)->bySubOption = DCP_SUBOPTION_RESET_2_FACTORY; /*0x6 -> Reset Factory Settings*/
       ((DCP_BLOCK*)pdyActualPosition)->wDcpBlockLength = TPS_htons(sizeof(wBlockQualifier));
       ((DCP_BLOCK*)pdyActualPosition)->wBlockQualifier = TPS_htons(wBlockQualifier<<1);

       pdyActualPosition += sizeof(DCP_BLOCK);

       ((DCP_BLOCK*)pdyActualPosition)->byOption = DCP_OPTION_CONTROL;    /*0x5 -> Control*/
       ((DCP_BLOCK*)pdyActualPosition)->bySubOption = DCP_SUBOPTION_END_TRANSACT; /*0x2 -> End Transaction*/
       ((DCP_BLOCK*)pdyActualPosition)->wDcpBlockLength = TPS_htons(sizeof(wBlockQualifier));
       ((DCP_BLOCK*)pdyActualPosition)->wBlockQualifier = 0;

       dwReturnValue = TPS_SendEthernetFrame( pbySendBuffer, MIN_LEN_ETHERNET_FRAME, PORT_NR_INTERNAL );

       free(pbySendBuffer);

   }
   else
   {
       dwReturnValue = API_RECORD_IF_OUT_OF_MEMORY;
   }


   return dwReturnValue;
}

/*!@} DCP Interface*/

#endif /*USE_TPS_COMMUNICATION_CHANNEL*/


#ifdef USE_ETHERNET_INTERFACE
/*! \addtogroup   Ethernet_Interface Ethernet Interface
*@{
*/

/*!
 * \brief       This function initializes the receive and transmit buffers for Ethernet communication.
                Through this interface Ethernet packets can be sent through the TPS-1 directly to your application.
                So for example a web server can be run in your application.
 *
 * \note        To use this function <b>USE_ETHERNET_INTERFACE</b> in TPS_1_user.h must be defined.
                * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - TPS_ERROR_WRONG_API_STATE
 *              - ETH_OUT_OF_MEMORY
 */
USIGN32 TPS_InitEthernetChannel(VOID)
{
    USIGN8 bySetApiState = STATE_SUBSLOT_RDY;

    #ifdef USE_TPS_COMMUNICATION_CHANNEL
        bySetApiState = STATE_TPS_CHANNEL_RDY;
    #endif

    if(g_byApiState != bySetApiState)
    {
        return TPS_ERROR_WRONG_API_STATE;
    }

    if(((g_pbyCurrentPointer + ((MAX_LEN_ETHERNET_FRAME + 4) * 2)) - g_pbyConfigNRTMem) > g_dwConfigNRTMemSize)
    {
        #ifdef DEBUG_API_TEST
            printf("DEBUG_API > init_eth_channel: Not enough memory for ethernet communication.\n");
        #endif
        return ETH_OUT_OF_MEMORY;
    }

    /* Configuration RX mailbox. */
    g_poEthernetRXMailbox = (T_ETHERNET_MAILBOX*)((g_pbyCurrentPointer + 4) - ((USIGN32)g_pbyCurrentPointer % 4));

    TPS_SetValue32((USIGN8*)&g_poEthernetRXMailbox->dwLength, MAX_LEN_ETHERNET_FRAME);
    TPS_SetValue32((USIGN8*)&g_poEthernetRXMailbox->dwPortnumber, 3);
    TPS_SetValueData((USIGN8*)&g_poEthernetRXMailbox->byFrame, (USIGN8*)&"\xBA\xBA\xDE\xDA", 4);
    g_pbyCurrentPointer += MAX_LEN_ETHERNET_FRAME + 12;

    /* Configuration TX mailbox. */
    g_poEthernetTXMailbox = (T_ETHERNET_MAILBOX*)((g_pbyCurrentPointer + 4) - ((USIGN32)g_pbyCurrentPointer % 4));
    TPS_SetValue32((USIGN8*)&g_poEthernetTXMailbox->dwLength, MAX_LEN_ETHERNET_FRAME);
    TPS_SetValue32((USIGN8*)&g_poEthernetTXMailbox->dwPortnumber, 3);
    TPS_SetValueData((USIGN8*)&g_poEthernetTXMailbox->byFrame, (USIGN8*)&"\xDE\xDA\xBA\xBA", 4);
    g_pbyCurrentPointer += MAX_LEN_ETHERNET_FRAME + 12;

    return TPS_ACTION_OK;
}


/*!
 * \brief       This function is called from event interface and calls the registered callback function of application
 *
 * \note        To use this function <b>USE_ETHERNET_INTERFACE</b> in TPS_1_user.h must be defined.
                * \retval      possible return values:
 *              - none
 */
static VOID AppOnEthernetReceive(VOID)
{

#ifdef DEBUG_API_TEST
    printf("APP: TPS_EVENT_ETH_FRAME_REC was received\n");
#endif

    if ((g_zApiEthernetContext.OnEthernetFrameRX_CB != NULL) && (g_poEthernetRXMailbox != NULL))
    {
        g_zApiEthernetContext.OnEthernetFrameRX_CB(g_poEthernetRXMailbox);
    }

    TPS_SetValue32((USIGN8*)&g_poEthernetRXMailbox->dwMailboxState, ETH_MBX_EMPTY);
}

/*!
* \brief       This function sends an Ethernet packet through the TPS-1 Ethernet interface.
*
* \param[in]   pbyPacket pointer to the complete frame buffer to send
* \param[in]   wLength data length of the Ethernet frame minus Frame Check Sequence (FCS) (max. 1524 bytes)
* \param[in]   wPortNr Ethernet port number to use (PORT_NR_1, PORT_NR_2, PORT_NR_1_2 or PORT_NR_INTERNAL)
* \note        If you use PORT_NR_INTERNAL the Ethernet packet is sent to the TPS-1 firmware and not over the network.
* \note        If you use PORT_ANY the TPS-1 firmware will try to discover, based on the destination mac address, over which port the data has to be sent.
*              This is usefull if the application doesn't know on which port a telegram was received.
* \retval      possible return values:
*              - TPS_ACTION_OK : success
*              - ETH_SEND_FRAME_TOO_LONG
*              - ETH_INVALID_PORT
*              - ETH_FRAME_CAN_NOT_BE_SEND : the TPS-1 firmware is busy with another packet and can't process the request.
*                                            If you get this return value just try to send the packet again.
*/
USIGN32 TPS_SendEthernetFrame(USIGN8* pbyPacket, USIGN16 wLength, USIGN16 wPortNr)
{

    USIGN32 dwRetValue = TPS_SendEthernetFrameFragmented(pbyPacket, wLength, wPortNr, MAX_LEN_ETHERNET_FRAME);

    if(dwRetValue == wLength)
    {
        return TPS_ACTION_OK;
    }

    return dwRetValue;

}

/*!
* \brief       This function sends an Ethernet packet through the TPS-1 Ethernet interface. Sending large frames through the SPI interface
*              can take some time, so frames can be fragmented by defining a maximum frame length. If the Parameter wMaxFragLen < wLength this
*              function sends only one fragment of the length wMaxFragLen. Each further call of the function sends the
*              next fragment until length of sent bytes < wMaxFragLen. The TPS-1 sends the full ethernet frame after all fragments are transmitted.
*
* \param[in]   pbyPacket pointer to the complete frame buffer to send
* \param[in]   wLength data length of the Ethernet frame minus Frame Check Sequence (FCS) (max. 1524 bytes)
* \param[in]   wPortNr Ethernet port number to use (PORT_NR_1, PORT_NR_2, PORT_NR_1_2 or PORT_NR_INTERNAL)
* \note        If you use PORT_NR_INTERNAL the Ethernet packet is sent to the TPS-1 firmware and not over the network.
* \note        If you use PORT_ANY the TPS-1 firmware will try to discover, based on the destination mac address, over which port the data has to be sent.
*              This is usefull if the application doesn't know on which port a telegram was received.
* \note        Changing pbyPacket before all fragments are transmitted, cancels the frame and starts a transmission.
* \retval      possible return values:
*              On success:
*              - Count of transfered bytes  (if equal wLength, the frame was sent)
*              Possible Errors:
*              - ETH_SEND_FRAME_TOO_LONG
*              - ETH_INVALID_PORT
*              - ETH_FRAME_CAN_NOT_BE_SEND : the TPS-1 firmware is busy with another packet and can't process the request.
*                                            If you get this return value just try to send the packet again.
*/
USIGN32 TPS_SendEthernetFrameFragmented(USIGN8* pbyPacket, USIGN16 wLength, USIGN16 wPortNr, USIGN16 wMaxFragLen)
{
    USIGN32 dwEventRegister = 0;
    USIGN32 dwMailboxState = ETH_MBX_EMPTY;
    USIGN8  byEventBitNr = 0;
    T_ETHERNET_MAILBOX* pzMailBox = NULL;

    static USIGN8 byWriteSessionPending = 0;
    static USIGN8* pbyLastFramAddr = NULL;
    static USIGN16 wByteNumberSent = 0;

#ifdef DEBUG_API_ETH_FRAME
    /* Debug-Print of the TPS_SendEthernetFrame() function. */
    USIGN32 dwIndex;

    printf("DEBUG_API > TPS_SendEthernetFrame(): Length: %d, Port %d\n", wLength, wPortNr);
    for (dwIndex = 0; dwIndex < wLength; dwIndex++)
    {
        printf("0x%2.2X ", pbyPacket[dwIndex]);
    }
    printf("\n");
#endif

    if ((wLength == 0) || (wLength > MAX_LEN_ETHERNET_FRAME))
    {
        return ETH_SEND_FRAME_TOO_LONG;
    }

    if ((wPortNr!=PORT_ANY) && (wPortNr != PORT_NR_1) && (wPortNr != PORT_NR_2) && (wPortNr != PORT_NR_1_2) && (wPortNr != PORT_NR_INTERNAL))
    {
        return ETH_INVALID_PORT;
    }


    TPS_GetValue32((USIGN8*)EVENT_REGISTER_APP, &dwEventRegister);

    if (wPortNr == PORT_NR_INTERNAL)
    {
#ifdef USE_TPS_COMMUNICATION_CHANNEL
        byEventBitNr = APP_EVENT_APP_MESSAGE;
        pzMailBox = g_poTPSIntTXMailbox;
#else
        return ETH_INVALID_PORT;
#endif
    }
    else
    {
        byEventBitNr = APP_EVENT_ETH_FRAME_SEND;
        pzMailBox = g_poEthernetTXMailbox;
    }

    if(pbyLastFramAddr != pbyPacket)
    {
       byWriteSessionPending = 0;
    }

    if(!byWriteSessionPending)
    {
       TPS_GetValue32((USIGN8*)&pzMailBox->dwMailboxState, &dwMailboxState);

       if (((dwEventRegister & (0x01 << byEventBitNr)) == (0x01 << byEventBitNr)) ||
           (pzMailBox == NULL) ||
           (dwMailboxState != ETH_MBX_EMPTY))
       {
#ifdef DEBUG_API_ETH_FRAME
           printf("DEBUG_API > Can not send the frame at the moment. The stack is still accessing the data.\n");
#endif

           return(ETH_FRAME_CAN_NOT_BE_SEND);
       }

       byWriteSessionPending = 1;
       pbyLastFramAddr = pbyPacket;
       wByteNumberSent = 0;

       TPS_SetValue32((USIGN8*)&pzMailBox->dwLength, wLength);
       TPS_SetValue32((USIGN8*)&pzMailBox->dwPortnumber, wPortNr);
    }

    if(wMaxFragLen >= wLength)
    {
       TPS_SetValueData((USIGN8*)&pzMailBox->byFrame, pbyPacket, wLength);
       wByteNumberSent = wLength;
    }
    else if((wLength - wByteNumberSent) < wMaxFragLen)
    {
       TPS_SetValueData(((USIGN8*)&pzMailBox->byFrame) + wByteNumberSent, pbyPacket + wByteNumberSent, (wLength - wByteNumberSent));
       wByteNumberSent = wLength;
    }
    else
    {
       TPS_SetValueData(((USIGN8*)&pzMailBox->byFrame) + wByteNumberSent, pbyPacket + wByteNumberSent, wMaxFragLen);
       wByteNumberSent += wMaxFragLen;
    }

    if(wByteNumberSent == wLength)
    {
        AppSetEventRegApp(byEventBitNr);
        byWriteSessionPending = 0;
        pbyLastFramAddr = NULL;
    }

    return wByteNumberSent;
}


/*!@} Ethernet_Interface Ethernet Interface*/

#endif /*USE_ETHERNET_INTERFACE*/

#ifdef DIAGNOSIS_ENABLE
/*! \addtogroup  Diagnose Diagnosis Interface
*@{
*/

/*!
* \brief       This function sends a diagnosis alarm notification to the connected PLC.
               Before sending this alarm a diagnosis must be added to Diagnosis ASE (TPS_DiagChannelAdd())
*
* \param[in]   dwArNumber AR_0 or AR_1
* \param[in]   dwAPINumber API number
* \param[in]   wSlotNumber slot number
* \param[in]   wSubSlotNumber subslot number
* \param[in]   byAlarmPrio priority of the alarm notification (should be ALARM_LOW)
* \param[in]   byAlarmType specifies if this diagnosis APPEARS or DISAPPEARS
* \param[in]   dwDiagAdress pointer the diagnosis entry created by TPS_DiagChannelAdd()
* \param[in]   wUserHandle specific handle to identify the corresponding alarm acknowledge. Should be unique for each sent alarm.
* \retval      possible return values:
*              - TPS_ACTION_OK : success
*              - API_DIAG_PROP_WRONG_PARAMETER
*              or any Error from TPS_SendAlarm()
*/
USIGN32 TPS_SendDiagAlarm(USIGN32 dwARNumber, USIGN32 dwAPINumber, USIGN16 wSlotNumber, USIGN16 wSubSlotNumber, USIGN8 byAlarmPrio,
    USIGN8 byAlarmType, USIGN32 dwDiagAddress, USIGN16 wUserHandle)
{
    USIGN8  byStatus = DIAGNOSIS_ALARM;
    USIGN16 wUsi = USI_EXT_CHANNEL_DIAGNOSIS;
    USIGN32 dwAlarmDataLength = sizeof(DPR_DIAG_ENTRY) - 1;
    DPR_DIAG_ENTRY zDiagData = { 0 };
    USIGN32 dwAlarmData[2];

    USIGN32 dwErrorCode = 0;

    /* Read the entry from the TPS-1 NRT Area. */
    dwErrorCode = TPS_GetValueData((USIGN8*)dwDiagAddress, (USIGN8*)&zDiagData, sizeof(DPR_DIAG_ENTRY));

    if (dwErrorCode == TPS_ACTION_OK)
    {
        if (byAlarmType > CHANPROP_SPECIFIER_MAX)
        {
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > Error in byAlarmType\n");
#endif

            dwErrorCode = API_DIAG_PROP_WRONG_PARAMETER;
        }
    }

    if (dwErrorCode == TPS_ACTION_OK)
    {
        if (byAlarmType != (USIGN8)APPEARS)
        {
            byStatus = DIAGNOSIS_DISAPPEARS_ALARM;
        }

        if((zDiagData.wChannelNumber < USI_CHANNEL_DIAGNOSIS) 
            && (zDiagData.wChannelNumber == zDiagData.wChannelProperties)
            && (zDiagData.wChannelProperties == zDiagData.wChannelErrortype))
        {
            dwAlarmDataLength = (USIGN32)(TPS_htons(zDiagData.wExtchannelErrortype));
            dwAlarmData[0] = TPS_htonl(zDiagData.dwExtchannelAddval);
            dwAlarmData[1] = TPS_htonl(zDiagData.dwQualifiedChannelQualifier); 
                        
            dwErrorCode = TPS_SendAlarm(dwARNumber, dwAPINumber, wSlotNumber, wSubSlotNumber,
                                        byAlarmPrio, byStatus, dwAlarmDataLength,
                                        (USIGN8*)&dwAlarmData, TPS_htons(zDiagData.wChannelNumber), wUserHandle);
            
        }
        else
        {
           if(zDiagData.dwQualifiedChannelQualifier != 0)
           {
              wUsi = USI_QUALIFIED_DIAGNOSIS;
           }
           else if (zDiagData.wExtchannelErrortype != 0)
           {
              wUsi = USI_EXT_CHANNEL_DIAGNOSIS;
              dwAlarmDataLength -= 4;
           }
           else
           {
              /* If no extended channel information present -> send as channeldiag.alarm */
              wUsi = USI_CHANNEL_DIAGNOSIS;
              dwAlarmDataLength -= 10;
           }

           dwErrorCode = TPS_SendAlarm(dwARNumber, dwAPINumber, wSlotNumber, wSubSlotNumber,
                                       byAlarmPrio, byStatus, dwAlarmDataLength,
                                       (USIGN8*)&zDiagData, wUsi, wUserHandle);
        }
    }


    return dwErrorCode;
}

/*!
 * \brief       This function builds the channel properties block used to add a diagnosis entry with TPS_DiagChannelAdd().
 *
 * \param[out]  pwChannelProperties Pointer where the channel properties shall be stored
 * \param[in]   byType              See ChannelProperties.Type of the PROFINET Standard.
                                    <br> defines the width of this of this channel (2^(byType-1) bytes. Must be 0 if used with ChannelNumber 0x8000 (whole submodule)
 * \param[in]   byAccumulative      See ChannelProperties.Accumulative of the PROFINET Standard.
                                    <br>Defines if this diagnosis is accumulated over more than one diagnosis channel
 * \param[in]   byMaintenance       See ChannelProperties.Maintenance of the PROFINET Standard.
                                    <br>Defines the severity of this diagnosis entry.
 *                                  <br>Values: MAINTENANCE_DIAGNOSIS, MAINTENANCE_REQUIRED,
 *                                  MAINTENANCE_DEMANDED, MAINTENANCE_QUALIFIED_DIAGNOSIS
 * \param[in]   bySpecifier         See ChannelProperties.Specifier of the PROFINET Standard.
 *                                  <br>ALL_DISAPPEAR, APPEARS, DISAPPEARS, DISAPPEARS_BUT_OTHER_REMAIN.
 * \param[in]   byDirection         See ChannelProperties.Direction of the PROFINET Standard.
                                    <br>Defines the direction of this diagnosis channel.
                                    <br>DIAG_DIRECTION_SPECIFIC, DIAG_DIRECTION_INPUT, DIAG_DIRECTION_OUTPUT, DIAG_DIRECTION_INOUT
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_DIAG_PROP_WRONG_PARAMETER
 */
USIGN32 TPS_DiagSetChannelProperties(USIGN16* pwChannelProperties, USIGN8 byType, USIGN8 byAccumulative,
    USIGN8 byMaintenance, USIGN8 bySpecifier, USIGN8 byDirection)
{
    if (pwChannelProperties == NULL)
    {
        return API_DIAG_PROP_NULL_PARAMETER;
    }

    if (byType > CHANPROP_TYPE_MAX)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Error in byType\n");
#endif

        return API_DIAG_PROP_WRONG_PARAMETER;
    }

    if (byAccumulative > CHANPROP_ACCUMULATIVE_MAX)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Error in byAccumulative\n");
#endif

        return API_DIAG_PROP_WRONG_PARAMETER;
    }

    if (byMaintenance > CHANPROP_MAINTENANCE_MAX)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Error in byMaintenance\n");
#endif

        return API_DIAG_PROP_WRONG_PARAMETER;
    }

    if (bySpecifier > CHANPROP_SPECIFIER_MAX)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Error in bySpecifier\n");
#endif
        return API_DIAG_PROP_WRONG_PARAMETER;
    }

    if (byDirection > CHANPROP_DIRECTION_MAX)
    {
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > Error in byDirection\n");
#endif

        return API_DIAG_PROP_WRONG_PARAMETER;
    }

    *pwChannelProperties = 0x0000;
    SET_CHANPROP_TYPE(*pwChannelProperties, byType);
    SET_CHANPROP_ACCU(*pwChannelProperties, byAccumulative);
    SET_CHANPROP_MAIN(*pwChannelProperties, byMaintenance);
    SET_CHANPROP_SPECIFIER(*pwChannelProperties, bySpecifier);
    SET_CHANPROP_DIRECTION(*pwChannelProperties, byDirection);

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function adds a diagnosis entry to the given channel.
 *              If there is an AR established for the affected subslot a diagnosis appears alarm must be sent afterwards.
 *
 * \param[in]   pzSubslot handle to the subslot structure
 * \param[in]   wChannelNumber diagnosis channel number ( e.g. 0x8000 for the whole submodule )
 * \param[in]   wChannelProperties diagnosis channel number ( see TPS_DiagSetChannelProperties())
 * \param[in]   wChannelErrortype diagnosis channel error type
 * \param[in]   wExtchannelErrortype diagnosis channel extended error type. <b>Not supported in this version, must be ECET_NONE.</b>
 * \param[in]   dwExtchannelAddval diagnosis channel extended additional value. <b>Not supported in this version, must be NULL.</b>
 * \retval      Address of the diagnosis entry in the NRT-Area or NULL if an error occurred.  The error code can be obtained by TPS_GetLastError()
*/
USIGN32 TPS_DiagChannelAdd(SUBSLOT* pzSubslot, USIGN16 wChannelNumber, USIGN16 wChannelProperties, USIGN16 wChannelErrortype, USIGN16 wExtchannelErrortype, USIGN32 dwExtchannelAddval)
{
    return AppDiagAdd( pzSubslot, wChannelNumber, wChannelProperties,  wChannelErrortype, wExtchannelErrortype, dwExtchannelAddval, 0);
}
/*!
 * \brief       This function adds a diagnosis entry to the given channel.
 *              If there is an AR established for the affected subslot a diagnosis appears alarm must be sent afterwards.
 *
 * \param[in]   pzSubslot handle to the subslot structure
 * \param[in]   wChannelNumber diagnosis channel number ( e.g. 0x8000 for the whole submodule )
 * \param[in]   wChannelProperties diagnosis channel properties ( see TPS_DiagSetChannelProperties())
 * \param[in]   wChannelErrortype diagnosis channel error type
 * \param[in]   wExtchannelErrortype diagnosis channel extended error type.
 * \param[in]   dwExtchannelAddval diagnosis channel extended additional value.
 * \param[in]   dwQualifiedChannelQualifier qualified channel qualifier value.
 * \retval      Address of the diagnosis entry in the NRT-Area or NULL if an error occurred. The error code can be obtained by TPS_GetLastError()
*/
USIGN32 TPS_DiagChannelQualifiedAdd(SUBSLOT* pzSubslot, USIGN16 wChannelNumber, USIGN16 wChannelProperties, USIGN16 wChannelErrortype, USIGN16 wExtchannelErrortype, USIGN32 dwExtchannelAddval, USIGN32 dwQualifiedChannelQualifier)
{
   if(0 != dwQualifiedChannelQualifier)
   {
      USIGN32 i = 0;
      USIGN32 dwBitCount = 0;
      if(MAINTENANCE_QUALIFIED_DIAGNOSIS != CHANPROP_MAIN(wChannelProperties))
      {
         AppSetLastError(API_DIAG_ADD_QUALIFIED_PROPERTY_NOT_SET);
         return NULL;
      }

      /* check if dwQualifiedChannelQualifier is valid - only bit in the range of bit 3..31 shall be set */
      if(0 != (dwQualifiedChannelQualifier & DIAG_QUALIFIER_RESERVED_BITMASK))
      {
         AppSetLastError(API_DIAG_ADD_QUALIFIER_INVALID);
         return NULL;
      }

      for( i = 3; i < 32; i++ )
      {
         dwBitCount += (0x00000001 & (dwQualifiedChannelQualifier >> i));
      }

      if(dwBitCount > 1)
      {
         AppSetLastError(API_DIAG_ADD_QUALIFIER_INVALID);
         return NULL;
      }
   }

   return AppDiagAdd( pzSubslot, wChannelNumber, wChannelProperties,  wChannelErrortype, wExtchannelErrortype, dwExtchannelAddval, dwQualifiedChannelQualifier);
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/*!
 * \brief       This function adds a manufacturer specific diagnosis.
 *              If there is an AR established for the affected subslot a diagnosis appears alarm must be sent afterwards.
 *
 * \param[in]   pzSubslot handle to the subslot structure
 * \param[in]   wUsi UserStructureIdentifier
 * \param[in]   wLen Length of diagnosis data (maximum 8)
 * \param[in]   pbDiagData pointer to diagnosis data 
 * \retval      Address of the diagnosis entry in the NRT-Area or NULL if an error occurred. The error code can be obtained by TPS_GetLastError()
*/
USIGN32 TPS_DiagManufactorSpecdAdd(SUBSLOT* pzSubslot, USIGN16 wUsi, USIGN16 wLen, USIGN8* pbDiagData)
{
    USIGN32 dwDiagDataBytes0to3 = 0;
    USIGN32 dwDiagDataBytes4to7 = 0;
    
    if(wLen > (sizeof(USIGN32) * 2))
    {
       return API_MAN_SPEC_DIAG_TO_LONG;
    }
    else  if(wLen != 0)
    {
       memcpy((USIGN8*)&dwDiagDataBytes0to3, pbDiagData, ((wLen > 4)? 4 : wLen));
            
       if(wLen > 4)
       {
          memcpy((USIGN8*)&dwDiagDataBytes4to7, (pbDiagData + 4), (wLen - 4));
       }
    }

   return AppDiagAdd( pzSubslot, wUsi, wUsi, wUsi, wLen, dwDiagDataBytes0to3, dwDiagDataBytes4to7);
}
#endif
/*!
 * \brief       This function removes an existing channel diagnosis entry.
 *              If an AR is established for the affected subslot a diagnosis disappears alarm must be
 *              sent to the connected PLC before removing the entry.
 *
 * \param[in]   dwDiagAddress Diagnosis channel number that was created with TPS_DiagChannelAdd()
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_DIAG_PROP_WRONG_PARAMETER
 *              - API_DIAG_REMOVE_WRONG_RESULT
 *              - API_DIAG_REMOVE_NO_ACK
 */
USIGN32 TPS_DiagChannelRemove(USIGN32 dwDiagAddress)
{
    DPR_DIAG_ENTRY zDiagEntry = { 0 };
    USIGN32 dwErrorCode = 0;
    USIGN32 dwEventRegister = 0;

#ifdef DEBUG_API_TEST
    printf("TPS_DiagChannelRemove -> Started \n");
#endif

    if (dwDiagAddress == 0x00)
    {
        return API_DIAG_REMOVE_WRONG_PARAMETER;
    }

    /* Check if Event is free */
    TPS_GetValue32((USIGN8*)EVENT_REGISTER_APP, &dwEventRegister);
    if ( (dwEventRegister & (0x01 << APP_EVENT_DIAG_CHANGED)) == (0x01 << APP_EVENT_DIAG_CHANGED) )
    {
        return API_DIAG_REMOVE_EVENT_IN_USE;
    }

    /* Read one entry from the TPS-1 diag buffer */
    dwErrorCode = TPS_GetValueData((USIGN8*)dwDiagAddress, (USIGN8*)&zDiagEntry, sizeof(DPR_DIAG_ENTRY));
    if (dwErrorCode != TPS_ACTION_OK)
    {
        return API_DIAG_REMOVE_WRONG_RESULT;
    }

    /* Remove entry from TPS-1 diag buffer */
    if (zDiagEntry.byFlags == APPEAR_ACK_FLAG)
    {
        /* OK: mark the diag entry as "disappeared" */
        zDiagEntry.byFlags = DISAPPEAR_FLAG; /* disappear and changed */

        /* Send the new diag entry to the TPS-1 */
        dwErrorCode = TPS_SetValueData((USIGN8*)dwDiagAddress, (USIGN8*)&zDiagEntry, sizeof(DPR_DIAG_ENTRY));
        if (dwErrorCode != TPS_ACTION_OK)
        {
            return API_DIAG_REMOVE_WRONG_RESULT;
        }
#ifdef DIAGNOSIS_ENABLE
        /* decrease global counter of diagnosis entries */
        g_wNumberOfDiagEntries--;
#endif 

        AppSetEventRegApp(APP_EVENT_DIAG_CHANGED);

        return TPS_ACTION_OK;
    }
    else
    {
        /* ERROR */
#ifdef DEBUG_API_TEST
        printf("DEBUG_API > ..ERROR: APPEAR_ACK_FLAG not yet received! \n");
#endif
        return API_DIAG_REMOVE_NO_ACK;
    }
}

/*!
 * \brief       This function changes the maintenance status and specifier of an already defined diagnosis entry.
 *              After changing the severity or the specifier to disappears a diagnosis alarm must be sent to the
 *              connected PLC to inform it about the changed diagnosis.
 *
 * \param[in]   dwDiagAddress Diagnosis channel number
 * \param[in]   byNewMaintenanceStatus
 * \param[in]   byNewSpecifier
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - API_DIAG_PROP_WRONG_PARAMETER
 *              - API_DIAG_REMOVE_WRONG_RESULT
 *              - API_DIAG_REMOVE_NO_ACK
 */
USIGN32 TPS_DiagSetChangeState(USIGN32 dwDiagAddress, USIGN8 byNewMaintenanceStatus, USIGN8 byNewSpecifier)
{
    DPR_DIAG_ENTRY zDiagEntry = { 0 };
    DPR_DIAG_ENTRY* pzDiagEntry = NULL;

    USIGN16 wChannelProperties = 0;
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    USIGN32 dwEventRegister;


#ifdef DEBUG_API_TEST
    printf("TPS_DiagSetChangeState -> Started \n");
#endif

    /* Read old entry and build the new entry */
    /******************************************/
    if (dwDiagAddress == 0x00)
    {
        dwErrorCode = API_DIAG_CHANGE_WRONG_PARAMETER;
    }

    /* Check if Event is free */
    TPS_GetValue32((USIGN8*)EVENT_REGISTER_APP, &dwEventRegister);
    if ( (dwEventRegister & (0x01 << APP_EVENT_DIAG_CHANGED)) == (0x01 << APP_EVENT_DIAG_CHANGED) )
    {
        dwErrorCode = API_DIAG_CHANGE_EVENT_IN_USE;
    }

    if (dwErrorCode == TPS_ACTION_OK)
    {
        pzDiagEntry = &zDiagEntry;

        /* Read the entry from the TPS-1 diag buffer */
        dwErrorCode = TPS_GetValueData((USIGN8*)dwDiagAddress, (USIGN8*)pzDiagEntry, sizeof(DPR_DIAG_ENTRY));
        if (TPS_ACTION_OK != dwErrorCode)
        {
            dwErrorCode = API_DIAG_CHANGE_WRONG_RESULT;
        }
    }

    if (dwErrorCode == TPS_ACTION_OK)
    {
        wChannelProperties = TPS_htons(pzDiagEntry->wChannelProperties);

        if (byNewSpecifier <= CHANPROP_SPECIFIER_MAX)
        {
            /* appears/disappears */
            SET_CHANPROP_SPECIFIER(wChannelProperties, byNewSpecifier);
        }
        else
        {
            dwErrorCode = API_DIAG_CHANGE_WRONG_PARAMETER;
        }

        if (byNewMaintenanceStatus <= CHANPROP_MAINTENANCE_MAX)
        {
            /* req/demanded */
            SET_CHANPROP_MAIN(wChannelProperties, byNewMaintenanceStatus);
        }
        else
        {
            dwErrorCode = API_DIAG_CHANGE_WRONG_PARAMETER;
        }
    }

    if (dwErrorCode == TPS_ACTION_OK)
    {
        /* Insert new properties */
        pzDiagEntry->wChannelProperties = TPS_htons(wChannelProperties);

        /* Send the new entry                               */
        /****************************************************/
        if (pzDiagEntry->byFlags == APPEAR_ACK_FLAG)
        {
            /* OK */
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > changing properties and overwriting previous entry..\n");
#endif

            /* Appear flag */
            pzDiagEntry->byFlags = APPEAR_FLAG;

            /* Send the changed diag entry to the TPS-1 and overwrite previous entry */
            dwErrorCode = TPS_SetValueData((USIGN8*)dwDiagAddress, (USIGN8*)pzDiagEntry, sizeof(DPR_DIAG_ENTRY));
            if (TPS_ACTION_OK != dwErrorCode)
            {
                dwErrorCode = API_DIAG_CHANGE_WRONG_RESULT;
            }
        }
        else
        {
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > ..ERROR: APPEAR_ACK_FLAG not yet received! \n");
#endif

            dwErrorCode = API_DIAG_CHANGE_NO_ACK;
        }
    }

    if (dwErrorCode == TPS_ACTION_OK)
    {
        AppSetEventRegApp(APP_EVENT_DIAG_CHANGED);
    }

    return dwErrorCode;
}

/*!@} Diagnose Diagnosis Interface*/

#endif /* DIAGNOSIS_ENABLE */

/*****************************************************************************/
/*****************************************************************************/
/* Start internal functions.                                                 */
/*****************************************************************************/
/*****************************************************************************/

/*! \addtogroup   Internal_Interface Internal Functions
    \ingroup      allfunctions
*@{
*/

/*!
 * \brief       This for API internal use only and adds a diagnosis with or not QualifiedChannel entry to the given channel. Depending on value of
 *              dwQualifiedChannelQualifier -Parameter.
 *              If there is an AR established for the affected subslot a diagnosis appears alarm must be sent afterwards.
 *
 * \param[in]   pzSubslot handle to the subslot structure
 * \param[in]   wChannelNumber diagnosis channel number ( e.g. 0x8000 for the whole submodule )
 * \param[in]   wChannelProperties diagnosis channel number ( see TPS_DiagSetChannelProperties())
 * \param[in]   wChannelErrortype diagnosis channel error type
 * \param[in]   wExtchannelErrortype diagnosis channel extended error type.
 * \param[in]   dwExtchannelAddval diagnosis channel extended additional value.
 * \param[in]   dwQualifiedChannelQualifier qualified channel qualifier value.
 * \retval      Address of the diagnosis entry in the NRT-Area or NULL if an error occurred.  The error code can be obtained by TPS_GetLastError()
*/
USIGN32 AppDiagAdd(SUBSLOT* pzSubslot, USIGN16 wChannelNumber, USIGN16 wChannelProperties, USIGN16 wChannelErrortype, USIGN16 wExtchannelErrortype, USIGN32 dwExtchannelAddval, USIGN32 dwQualifiedChannelQualifier)
{
    DPR_DIAG_ENTRY zDiagEntry = { 0 };
    DPR_DIAG_ENTRY* pzDiagEntry = NULL;
    USIGN32 dwErrorCode = TPS_ACTION_OK;
    USIGN32 dwSize = 0;
    USIGN32 dwDiagEntryIndex  = 0;
    USIGN32 dwEventRegister   = 0;

#ifdef DEBUG_API_TEST
    printf("TPS_DiagChannelAdd -> Started \n");
#endif

    if (pzSubslot == NULL)
    {
        AppSetLastError(API_DIAG_ADD_NULL_POINTER);
        return 0x00;
    }

    /* Check if Event is free */
    TPS_GetValue32((USIGN8*)EVENT_REGISTER_APP, &dwEventRegister);
    if ( (dwEventRegister & (0x01 << APP_EVENT_DIAG_CHANGED)) == (0x01 << APP_EVENT_DIAG_CHANGED) )
    {
         AppSetLastError(API_DIAG_ADD_EVENT_IN_USE);
         return 0x00;
    }

    /* Get the size of the diag buffer */
    dwErrorCode = TPS_GetValue32((USIGN8*)pzSubslot->pt_size_chan_diag, &dwSize);
    if (dwErrorCode != TPS_ACTION_OK)
    {
        AppSetLastError(API_DIAG_ADD_MEM_ACCESS_ERROR);
        return 0x00;
    }

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > search for matching diag entry\n");
#endif

    pzDiagEntry = &zDiagEntry; /*set the diag-pointer to the local diag-buffer*/

    /* Analyse each existing entry, one after another */
    for (dwDiagEntryIndex = 0; (dwDiagEntryIndex * sizeof(DPR_DIAG_ENTRY)) < dwSize; dwDiagEntryIndex++)
    {
        /* Read one entry from the TPS-1 diag buffer */
        dwErrorCode = TPS_GetValueData((USIGN8*)pzSubslot->pt_chan_diag + (dwDiagEntryIndex * sizeof(DPR_DIAG_ENTRY)), (USIGN8*)pzDiagEntry, sizeof(DPR_DIAG_ENTRY));
        if (dwErrorCode != TPS_ACTION_OK)
        {
            AppSetLastError(API_DIAG_ADD_MEM_ACCESS_ERROR);
            return 0x00;
        }

        /* Compare the entry with the new entry */
        if ((pzDiagEntry->wChannelNumber == TPS_htons(wChannelNumber)) &&
            (pzDiagEntry->wChannelProperties == TPS_htons(wChannelProperties)) &&
            (pzDiagEntry->wChannelErrortype == TPS_htons(wChannelErrortype)) &&
            (pzDiagEntry->wExtchannelErrortype == TPS_htons(wExtchannelErrortype)) &&
            /* The values of removed entries are not cleared. Only the DISAPPEAR_ACK_FLAG
            * is set after removing the entry. */
            (pzDiagEntry->byFlags != EMPTY_ENTRY) &&
            (pzDiagEntry->byFlags != DISAPPEAR_ACK_FLAG))
            /*&& (pt_diag->dwExtchannelAddval == TPS_htons(dwExtchannelAddval)))*/
        {
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > ERROR: entry already exists..(?) \n");
#endif

            AppSetLastError(API_DIAG_ADD_ENTRY_ALREADY_EXISTS);
            return 0x00;
        }
        else if ((pzDiagEntry->wChannelNumber == TPS_htons(wChannelNumber)) &&
            (pzDiagEntry->wChannelErrortype == TPS_htons(wChannelErrortype)) &&
            (pzDiagEntry->wExtchannelErrortype == TPS_htons(wExtchannelErrortype)) &&
            /* The values of removed entries are not cleared. Only the DISAPPEAR_ACK_FLAG
            * is set after removing the entry. */
            (pzDiagEntry->byFlags != EMPTY_ENTRY) &&
            (pzDiagEntry->byFlags != DISAPPEAR_ACK_FLAG))
        {

#ifdef DEBUG_API_TEST
            printf("DEBUG_API > entry exists but differs..\n");
#endif

            /* entry exists but differs */
            AppSetLastError(API_DIAG_EXISTS_BUT_DIFFERS);
            return NULL;
        }
    }

#ifdef DIAGNOSIS_ENABLE
    /* Stack supports a maximum of MAX_NUMBER_DIAG diag entries only */
    if (g_wNumberOfDiagEntries >= MAX_NUMBER_DIAG)
    {
        AppSetLastError(API_DIAG_ADD_GLOBAL_LIMIT_REACHED);
        return 0x00;
    }
#endif 

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > no matching diag entry found\n");
    printf("DEBUG_API > looking for free diag slot..\n");
#endif

    /* Analyze each existing entry, one after another */
    for (dwDiagEntryIndex = 0; (dwDiagEntryIndex * sizeof(DPR_DIAG_ENTRY)) < dwSize; dwDiagEntryIndex++)
    {
        dwErrorCode = TPS_GetValueData((USIGN8*)pzSubslot->pt_chan_diag + (dwDiagEntryIndex * sizeof(DPR_DIAG_ENTRY)), (USIGN8*)pzDiagEntry, sizeof(DPR_DIAG_ENTRY));
        if (dwErrorCode != TPS_ACTION_OK)
        {
            AppSetLastError(API_DIAG_ADD_MEM_ACCESS_ERROR);
            return 0x00;
        }

        if ((pzDiagEntry->byFlags != EMPTY_ENTRY) && (pzDiagEntry->byFlags != DISAPPEAR_ACK_FLAG))
        {
            /* slot in use */
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > in use..trying next\n");
#endif
        }
        else
        {
            /* slot empty */
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > found free diag slot\nadding...\n");
#endif

            /* prepare the new diag entry */
            pzDiagEntry->wChannelNumber = TPS_htons(wChannelNumber);
            pzDiagEntry->wChannelProperties = TPS_htons(wChannelProperties);
            pzDiagEntry->wChannelErrortype = TPS_htons(wChannelErrortype);
            pzDiagEntry->wExtchannelErrortype = TPS_htons(wExtchannelErrortype);
            pzDiagEntry->dwExtchannelAddval = TPS_htonl(dwExtchannelAddval);
            pzDiagEntry->dwQualifiedChannelQualifier = TPS_htonl(dwQualifiedChannelQualifier);
            pzDiagEntry->byFlags = APPEAR_FLAG;

            /* send the new diag entry to the TPS-1 */
            dwErrorCode = TPS_SetValueData((USIGN8*)pzSubslot->pt_chan_diag + (dwDiagEntryIndex * sizeof(DPR_DIAG_ENTRY)), (USIGN8*)pzDiagEntry, sizeof(DPR_DIAG_ENTRY));
            if (dwErrorCode != TPS_ACTION_OK)
            {
                AppSetLastError(API_DIAG_ADD_MEM_ACCESS_ERROR);
                return 0x00;
            }

#ifdef DIAGNOSIS_ENABLE
            /* increase global counter of diagnosis entries */
            g_wNumberOfDiagEntries++;
#endif 
            AppSetEventRegApp(APP_EVENT_DIAG_CHANGED);

            return (USIGN32)(pzSubslot->pt_chan_diag + (dwDiagEntryIndex * sizeof(DPR_DIAG_ENTRY)));
        }
    }

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > no free diag slot\n");
#endif

    AppSetLastError(API_DIAG_ADD_DIAG_LIST_FULL);
    return 0x00;
}

/*!
 * \brief       This function sets an event bit in the application event acknowledge register (high -> 0x24)
 *              to acknowledge an event from the TPS stack.
 *
 * \param[in]   dwEventBit
 * \retval      TPS_ACTION_OK
 */
USIGN32 AppSetEventRegAppAckn(USIGN32 dwEventBit)
{
    USIGN32 dwReg;

    TPS_GetValue32((USIGN8*)EVENT_REGISTER_APP_ACKN, &dwReg);
    dwReg ^= ((USIGN32)0x00000001 << dwEventBit);
    TPS_SetValue32((USIGN8*)EVENT_REGISTER_APP_ACKN, dwReg);

    return(TPS_ACTION_OK);
}


/*!
 * \brief       This function clears the TypeOfStation buffer.
 *
 * \param[in]   pbyData buffer to be cleared
 * \retval      VOID
 */
static VOID AppClearTypeOfStation(USIGN8 *pbyData)
{
    USIGN8 byBuffer[TYPE_OF_STATION_STRING_LEN];

    /* Clear buffer                                                          */
    /*-----------------------------------------------------------------------*/
    memset(byBuffer, ' ', TYPE_OF_STATION_STRING_LEN);

    TPS_SetValueData(pbyData, &byBuffer[0], TYPE_OF_STATION_STRING_LEN);
}


/*!
 * \brief       This function sets an event bit in the application event register (an event to the TPS-1)
 *
 * \param[in]   dwEventBit
 * \retval      TPS_ACTION_OK
 */
USIGN32 AppSetEventRegApp(USIGN32 dwEventBit)
{
    USIGN32 dwReg;

    TPS_GetValue32( (USIGN8*)EVENT_REGISTER_APP, &dwReg);
    dwReg ^= ((USIGN32)0x00000001 << dwEventBit);
    TPS_SetValue32((USIGN8*)EVENT_REGISTER_APP, dwReg);

    return(TPS_ACTION_OK);
}



/*!
 * \brief       This functin set the global variables that indicate the
 *              state of the AR0, AR1 and IOSR. This function is typically
 *              called after receiving a Connect.req.
 *
 * \param[in]   dwARNumber (AR_0, AR_1, AR_IOSR - number of application relation)
 * \param[in]   dwArState state to be set for this AR (AR_NOT_IN_OPERATION, AR_ESTABLISH)
 * \retval      none
 */
VOID AppSetArEstablished (USIGN32 dwARNumber, USIGN32 dwArState)
{
    switch(dwARNumber)
    {
        case AR_0:
            g_dwArEstablished_0    = dwArState;
            break;
        case AR_1:
            g_dwArEstablished_1    = dwArState;
             break;
        case AR_IOSR:
            g_dwArEstablished_IOSR = dwArState;
            break;

        default:
            break;
    }
}

/*!
 * \brief       This function calls the registered user function when a Connect.Req arrives.
 *
 * \param[in]   dwARNumber (AR_0, AR_1, AR_IOSR - number of application relation
 * \retval      none
*/
static VOID AppOnConnect(USIGN32 dwARNumber)
{
    #ifdef DEBUG_API_TEST
          printf("DEBUG_API > API: Connection request Received.\nCalling CB-Function.\n");
    #endif

    if(g_zApiARContext.OnConnect_CB != NULL)
    {
        g_zApiARContext.OnConnect_CB(dwARNumber);
    }

    /*----------------------------------------------------------------------*/
    /* Acknowledge the request.                                             */
    /*----------------------------------------------------------------------*/
    AppSetEventOnConnectOK(dwARNumber);
}


/*!
 * \brief       Function acknowledges an incoming Connect-Event.
 *
 * \param[in]   dwARNumber (AR_0, AR_1, AR_IOSR - number of application relation
 * \retval      0 or ErrorCode
*/
USIGN32 AppSetEventOnConnectOK(USIGN32 dwARNumber)
{
    if(dwARNumber == AR_0)
    {
        AppSetEventRegApp(APP_EVENT_ONCONNECT_OK_0);
    }
    else if(dwARNumber == AR_1)
    {
        AppSetEventRegApp(APP_EVENT_ONCONNECT_OK_1);
    }
    else if(dwARNumber == AR_IOSR)
    {
        AppSetEventRegApp(APP_EVENT_ONCONNECT_OK_2);
    }

    return TPS_ACTION_OK;
}

/*!
 * \brief       This function calls the user function after establishing an AR.
 *
 * \param[in]   dwARNumber (AR_0, AR_1, AR_IOSR - number of application relation
 * \retval      none
*/
static VOID AppOnConnect_Done(USIGN32 dwARNumber)
{
    #ifdef DEBUG_API_TEST
         printf("DEBUG_API > API: Connection %x established. Calling OnConnectDone_CB.\n", dwARNumber);
    #endif

    AppSetArEstablished(dwARNumber, AR_ESTABLISH);

    if(g_zApiARContext.OnConnectDone_CB != NULL)
    {
        g_zApiARContext.OnConnectDone_CB(dwARNumber);
    }
}

/*!
 * \brief       Calls the user function after PRM End Request was received response was received.
 *
 * \param[in]   dwArNumber -> AR number for this connection.
 * \retval      none
*/
static VOID AppOnPRMENDDone(USIGN32 dwARNumber)
{
    if(g_zApiARContext.OnPRMEND_Done_CB != NULL)
    {
        g_zApiARContext.OnPRMEND_Done_CB(dwARNumber);
    }

    App_GetAPDUOffsetForAR(dwARNumber);

    AppSetEventAppRdy(dwARNumber);
}


/*!
 * \brief       This function is called in order to signal to the stack that the application is ready
 *
 * \param[in]   dwArNumber -> AR number for this connection.
 * \retval      none
*/
static USIGN32 AppSetEventAppRdy(USIGN32 dwARNumber)
{
    USIGN32 dwReturnValue = TPS_ACTION_OK;

    switch(dwARNumber)
    {
        case AR_0:
            AppSetEventRegApp(APP_EVENT_APP_RDY_0);
            break;
        case AR_1:
            AppSetEventRegApp(APP_EVENT_APP_RDY_1);
            break;
        case AR_IOSR:
            AppSetEventRegApp(APP_EVENT_APP_RDY_2);
            break;
        default:
            dwReturnValue = API_WRONG_AR_NUMBER;
            break;
    }

    return dwReturnValue;
}


/*!
 * \brief       Calls the user function if the AR was aborted.
 *
 * \param[in]   dwARNumber  (number of the application relation)
 * \retval      none
*/
static VOID AppOnAbort(USIGN32 dwARNumber)
{
    AppSetArEstablished(dwARNumber, AR_NOT_IN_OPERATION);

    /* Call the registered callback function.                               */
    /*----------------------------------------------------------------------*/
    if(g_zApiARContext.OnAbort_CB != NULL)
    {
        g_zApiARContext.OnAbort_CB(dwARNumber);
    }
}


/*!
 * \brief       Calls the callback function after receiving a read request for an user index.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnReadRecord()
{
    USIGN8 byFlagBuff = 0;
    USIGN32 dwARNumber = 0;
    RECORD_BOX_INFO mailBoxInfo = {0};
    USIGN8 byArrMailboxData[SIZE_RECORD_MB0] = {0};
    SIGN32 dwDataLength = 0;
    USIGN16 wErrorCode1 = 0;
    USIGN16 wErrorCode2 = 0;
    BOOL bRecordHandled = TPS_FALSE;

    #ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: Record Data read.\n");
    #endif

    for(dwARNumber = 0; dwARNumber < MAX_NUMBER_RECORDS; dwARNumber++)
    {
        
        wErrorCode1 = 0;
        wErrorCode2 = 0;
        /* Get the mailbox flag                                            */
        /*******************************************************************/
        TPS_GetValue8(g_zApiARContext.record_mb[dwARNumber].pt_flags, &byFlagBuff);

        if(byFlagBuff == RECORD_FLAG_READ)
        {
            TPS_GetMailboxInfo(dwARNumber, &mailBoxInfo);

            #ifdef DEBUG_API_TEST
                printf("DEBUG_API > API: Read for AR %d, Index: 0x%4.4X, Slot: 0x%4.4X, Subslot: 0x%4.4X.\n",
                       dwARNumber, mailBoxInfo.wIndex, mailBoxInfo.wSlotNumber, mailBoxInfo.wSubSlotNumber);
            #endif

            switch(mailBoxInfo.wIndex)
            {
                /* Select if the record is handled internally or by the application  */
                /*-------------------------------------------------------------------*/
                case RECORD_INDEX_IM0: /* I&M0 */
                case RECORD_INDEX_IM1: /* I&M1 */
                case RECORD_INDEX_IM2: /* I&M2 */
                case RECORD_INDEX_IM3: /* I&M3 */
                case RECORD_INDEX_IM4: /* I&M4 */
                    dwDataLength = AppReadIMData(mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber, mailBoxInfo.wSubSlotNumber,
                                                 mailBoxInfo.wIndex, byArrMailboxData);
                    break;

                case RECORD_INDEX_RECORD_INPUT_DATA_OBJECT:
                    dwDataLength = AppReadRecordDataObject(RECORD_INPUT_DATA_OBJECT, dwARNumber,
                                                           mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber,
                                                           mailBoxInfo.wSubSlotNumber, byArrMailboxData);
                    break;

                case RECORD_INDEX_RECORD_OUTPUT_DATA_OBJECT:
                    dwDataLength = AppReadRecordDataObject(RECORD_OUTPUT_DATA_OBJECT, dwARNumber,
                                                           mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber,
                                                           mailBoxInfo.wSubSlotNumber, byArrMailboxData);
                    break;

                default:
                    /* Not handled by the driver, route it to the application. */
                    if (g_zApiARContext.OnReadRecord_CB != NULL)
                    {
                        g_zApiARContext.OnReadRecord_CB(dwARNumber);
                        bRecordHandled = TPS_TRUE;
                    }
                    break;
            }

            /* Only write the mailbox data here if the record was handled by the driver. */
            if(bRecordHandled == TPS_FALSE)
            {
                if(dwDataLength == RECORD_ERROR_INVALID_INDEX)
                {
                    wErrorCode1 = 0xB0; /* PNIORW-ErrorClass: Access, ErrorCode: Invalid Index */
                    wErrorCode2 = 0x00;
                    dwDataLength = 0;
                }

                TPS_WriteMailboxData(dwARNumber, byArrMailboxData, dwDataLength);
                TPS_RecordReadDone(dwARNumber, wErrorCode1, wErrorCode2);
            }
        }
    }
}


/*!
 * \brief       Calls the callback function after receiving a write request for an user index.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnWriteRecord()
{
    USIGN32 dwIndex = 0;
    USIGN8  byFlagBuff = 0;
    USIGN8  byArrMailboxData[SIZE_RECORD_MB0] = {0};
    SIGN32  dwDataLength = 0x00;
    USIGN16 wErrorCode1 = 0x00;
    USIGN16 wErrorCode2 = 0x00;
    BOOL    bRecordHandled = TPS_FALSE;
    RECORD_BOX_INFO mailBoxInfo = {0};

    #ifdef DEBUG_API_TEST
        printf("DEBUG_API > API: AppOnWriteRecord()\n");
    #endif

    for(dwIndex = 0; dwIndex < MAX_NUMBER_RECORDS; dwIndex++)
    {
        TPS_GetValue8(g_zApiARContext.record_mb[dwIndex].pt_flags, &byFlagBuff);

        if(byFlagBuff == RECORD_FLAG_WRITE)
        {
            TPS_GetMailboxInfo(dwIndex, &mailBoxInfo);
            TPS_ReadMailboxData(dwIndex, (USIGN8*) (&byArrMailboxData), mailBoxInfo.dwRecordDataLen);

            #ifdef DEBUG_API_TEST
                printf("DEBUG_API > API: Write for Index: 0x%4.4X, Slot: 0x%4.4X, Subslot: 0x%4.4X.\n",
                       mailBoxInfo.wIndex, mailBoxInfo.wSlotNumber, mailBoxInfo.wSubSlotNumber);
            #endif

            switch (mailBoxInfo.wIndex)
            {
#ifndef USE_TPS1_TO_SAVE_IM_DATA
                case RECORD_INDEX_IM1:
                    dwDataLength = AppWriteIMData(IM1_SUPPORTED,
                                                  mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber, mailBoxInfo.wSubSlotNumber,
                                                  byArrMailboxData);
                    break;
                case RECORD_INDEX_IM2:
                    dwDataLength = AppWriteIMData(IM2_SUPPORTED,
                                                  mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber, mailBoxInfo.wSubSlotNumber,
                                                  byArrMailboxData);
                    break;
                case RECORD_INDEX_IM3:
                    dwDataLength = AppWriteIMData(IM3_SUPPORTED,
                                                  mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber, mailBoxInfo.wSubSlotNumber,
                                                  byArrMailboxData);
                    break;
                case RECORD_INDEX_IM4:
                    dwDataLength = AppWriteIMData(IM4_SUPPORTED,
                                                  mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber, mailBoxInfo.wSubSlotNumber,
                                                  byArrMailboxData);
                    break;
#endif

                default:
                    if(g_zApiARContext.OnWriteRecord_CB != NULL)
                    {
                        g_zApiARContext.OnWriteRecord_CB(dwIndex);
                        bRecordHandled = TPS_TRUE;
                    }
                    break;
            }

            /* Only write the mailbox here if the record was handled by the driver. */
            if(bRecordHandled == TPS_FALSE)
            {
                if(dwDataLength == RECORD_ERROR_INVALID_INDEX)
                {
                    wErrorCode1 = 0xB0; /*PNIORW-ErrorClass: Access, ErrorCode: Invalid Index */
                    wErrorCode2 = 0x00;
                    dwDataLength = 0;
                }

                TPS_RecordWriteDone(dwIndex, wErrorCode1, wErrorCode2);
            }
        }
    }
}


/*!
 * \brief       Acknowledge of an alarm message sent to the contoller.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnAlarmAck(VOID)
{
    USIGN32 dwIndexI = 0;
    USIGN32 dwIndexJ = 0;
    USIGN8  byFlagBuff = 0;
    USIGN16 wUsrHdlBuff = 0;

    for(dwIndexJ = 0; dwIndexJ < MAX_NUMBER_IOAR; dwIndexJ++)
    {
        for(dwIndexI = 0; dwIndexI < 2; dwIndexI++)
        {
            TPS_GetValue8(g_zAlarmMailbox[(dwIndexJ*2) + dwIndexI].pt_flags, &byFlagBuff);

            switch(byFlagBuff)
            {
                case RTA_WAAK_TIMEOUT:
                case RTA_TIMEOUT:
                case ERR_FLAG:
                case NACK_FLAG:
                case ERR_AACK:
                case ACK_FLAG:
                     #ifdef DEBUG_API_TEST
                          printf("APP: ALARM_ACKN was received = 0x%x\n\n", byFlagBuff);
                     #endif

                     if(g_zApiARContext.OnAlarm_Ack_CB != NULL)
                     {
                          TPS_GetValue16(((USIGN8*)(g_zAlarmMailbox[(dwIndexJ*2) + dwIndexI].pt_usr_hdl)), &wUsrHdlBuff);
                          g_zApiARContext.OnAlarm_Ack_CB(wUsrHdlBuff, byFlagBuff);
                     }

                     TPS_SetValue8(g_zAlarmMailbox[(dwIndexJ*2) + dwIndexI].pt_flags, UNUSED_FLAG);
                     break;

                default:
                   break;
            } /* End of switch() */
        } /* End of for(;;) */
    } /* End of for(;;) */
}


/*!
 * \brief       Acknowledge of an diag message send to the contoller.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnDiagAckn( VOID )
{
    USIGN16 wDummy   = 0;
    USIGN32 dwDummy  = 0;

    #ifdef DEBUG_API_TEST
         printf("APP: DIAG_ACKN was received\n");
    #endif

    if(g_zApiARContext.OnDiag_Ack_CB != NULL)
    {
        g_zApiARContext.OnDiag_Ack_CB(wDummy, dwDummy);
    }
}


/*!
 * \brief       An ethernet frame was received (internal communication TPS <-> Driver).
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnTPSMessageReceive( VOID )
{
    #ifdef DEBUG_API_TEST
         printf("APP: TPS_EVENT_TPS_MESSAGE was received\n");
    #endif

    #ifdef USE_TPS_COMMUNICATION_CHANNEL
        if((g_oApiTpsMsgContext.OnTpsMessageRX_CB != NULL) && (g_poTPSIntRXMailbox != NULL))
        {
            g_oApiTpsMsgContext.OnTpsMessageRX_CB(g_poTPSIntRXMailbox);
        }

      TPS_SetValue32((USIGN8*)&g_poTPSIntRXMailbox->dwMailboxState, ETH_MBX_EMPTY);
    #endif
}


/*!
 * \brief       The TPS-1 firmware signaled a reset event to the application.
 *              The application can define a special callback for handling
 *              a reset situation. After 1 second after signaling the reset
 *              event the TPS-1 will perform the reset even though the callback
 *              hasn't finished it's operation.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnTPSReset( VOID )
{
#ifdef DEBUG_API_TEST
    printf("APP: TPS_EVENT_RESET was received\n");
#endif

    if(g_zApiARContext.OnReset_CB != NULL)
    {
        g_zApiARContext.OnReset_CB(0);
    }
}


/*!
 * \brief       Calls the user function after DCP_Set_Station_Name was received and already processed.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnSetStationName(T_DCP_SET_MODE zMode)
{
    if(g_zApiDeviceContext.OnSetStationName_CB != NULL)
    {
        g_zApiDeviceContext.OnSetStationName_CB();
    }
    else if(g_zApiDeviceContext.OnSetStationNameTP_CB  != NULL )
    {
       g_zApiDeviceContext.OnSetStationNameTP_CB(zMode);
    }
}

/*!
 * \brief       Calls the user function after FSU Parameter were updatet by TPS-1 firmware or pn controller.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnFSUParameterChange(VOID)
{
#ifdef USE_FS_APP
    if(g_zApiARContext.OnFSUParamChange_CB != NULL)
    {
      g_zApiARContext.OnFSUParamChange_CB(0);
    }
#endif
}

/*!
 * \brief       Calls the user function after DCP_Set_Station_IP_Addr was received and already processed.
 *
 * \param[in]   T_DCP_SET_MODE zMode
 * \retval      none
*/
static VOID AppOnSetStationIpAddr(T_DCP_SET_MODE zMode)
{
    if(g_zApiDeviceContext.OnSetStationIpAddr_CB != NULL)
    {
        g_zApiDeviceContext.OnSetStationIpAddr_CB(zMode);
    }
}


/*!
 * \brief       Calls the user function after DCP_Blink_Req was received.
 *
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnSetStationDcpSignal(VOID)
{
    if(g_zApiDeviceContext.OnSetStationDcpSignal_CB != NULL)
    {
        g_zApiDeviceContext.OnSetStationDcpSignal_CB();
    }
}


/*!
 * \brief       The TPS stack sends the message DCP_Set_FactorySettings to the application CPU.
 *
 * \param[in]   USIGN32 dwMode
 * \retval      none
*/
static VOID AppOnResetFactorySettings(VOID)
{
    USIGN16 wResetOption = 0;

    TPS_GetValue16((USIGN8*)&(g_pzNrtConfigHeader->wResetOption), &wResetOption);
    wResetOption = wResetOption >> 1;

    /* Reset of I&M 1-4 Informations */
    if(wResetOption == DCP_R2F_OPT_ALL)
    {
        AppFactoryResetForIM1_4();
    }

    /* Notify the application. */
    if(g_zApiDeviceContext.OnResetFactorySettings_CB != NULL)
    {
        g_zApiDeviceContext.OnResetFactorySettings_CB((USIGN32)wResetOption);
    }
}
#ifdef USE_TPS1_TO_SAVE_IM_DATA
/*!
 * \brief       Must be called by application after all application data(i.e. IMData) were reset
 * \param[in]   none
 * \retval      none
*/
VOID TPS_ResetToFactory_Done(VOID)
{
   AppSetEventRegAppAckn(TPS_EVENT_ONDCP_RESET_TO_FACTORY);
}
#endif
/*!
 * \brief   Calls user callback function after LED state of TPS-1 was
 *                  changed.
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnTPSLedChanged(VOID)
{
   if(g_zApiDeviceContext.OnLedStateChanged_CB != NULL)
   {
      g_zApiDeviceContext.OnLedStateChanged_CB(TPS_GetLedState());
   }
}

/*!
 * \brief   Calls user callback function after I&MData was changed by write request
 *          from engeneering tool
 *                  changed.
 * \param[in]   none
 * \retval      none
*/
static VOID AppOnIMDataChanged(SUBSLOT* poSubslot, USIGN16 IMSelector)
{
   if(g_zApiDeviceContext.OnImDataChanged_CB != NULL)
   {
      g_zApiDeviceContext.OnImDataChanged_CB(poSubslot, IMSelector);
   }
}


/*!
 * \brief       Search for the given subslot and returns it API, Slot- and Subslotnumber.
 *
 * \param[in]   SUBSLOT* poSubslot -pointer to subslot structure
 * \param[out]  USIGN32* pdwApiNumber -pointer to write in the API number from poSubslot
 * \param[out]  USIGN16* pwSlotNumber -pointer to write in the Slot number from poSubslot
 * \param[out]  USIGN16* pwSubslotNumber -pointer to write in the SubSlot number from poSubslot
 * \retval      TPS_ERROR_NULL_POINTER, TPS_ACTION_OK
*/
static USIGN32 AppGetSlotInfoFromHandle(SUBSLOT* poSubslot,
                                        USIGN32* pdwApiNumber, USIGN16* pwSlotNumber, USIGN16* pwSubslotNumber)
{
    if((poSubslot == NULL) ||
       (pdwApiNumber == NULL) ||
       (pwSlotNumber == NULL) ||
       (pwSubslotNumber == NULL))
    {
        return TPS_ERROR_NULL_POINTER;
    }

    if(poSubslot->pzSlot == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }
    if(poSubslot->pzSlot->poApi == NULL)
    {
        return TPS_ERROR_NULL_POINTER;
    }

    /* Read API. */
    *pdwApiNumber = poSubslot->pzSlot->poApi->index;
    /* Read Slotnumber. */
    TPS_GetValue16((USIGN8*)poSubslot->pzSlot->pt_slot_number, pwSlotNumber);
    /* Read Subslotnumber. */
    TPS_GetValue16((USIGN8*)poSubslot->pt_subslot_number, pwSubslotNumber);

    return TPS_ACTION_OK;
}


/*!
 * \brief       This function returns a pointer to the slot.
 *
 * \param[in]   dwApiNumber        API Index
 * \param[in]   wSlotNumber       Slot Number
 * \retval      Pointer to a slot or NULL on failure.
*/
static SLOT* AppGetSlotHandle(USIGN32 dwApiNumber, USIGN16 wSlotNumber)
{
    API_LIST *pzApi = NULL;
    SLOT     *pzSlot = NULL;
    USIGN32  dwErrorCode = GET_HANDLE_SLOT_NOT_FOUND;
    USIGN16  wReturnValue = 0;

    for(pzApi = g_zApiARContext.api_list; pzApi != NULL; pzApi = pzApi->next)
    {
        if(pzApi->index == dwApiNumber)
        {
            for(pzSlot = pzApi->firstslot; pzSlot != NULL; pzSlot = pzSlot->pNextSlot)
            {
                TPS_GetValue16((USIGN8*)pzSlot->pt_slot_number, &wReturnValue);
                if(wReturnValue == wSlotNumber)
                {
                    /* Slot found -> Set no error code => 0            */
                    /*--------------------------------------------------*/
                    AppSetLastError(TPS_ACTION_OK);
                    return pzSlot;
                }
            } /* End of second for-loop: Iterate over slots. */

            /* Slot not found -> Set error code                 */
            /*--------------------------------------------------*/
            dwErrorCode = GET_HANDLE_SLOT_NOT_FOUND;
        }
        else
        {
            /* API not found -> Set error code                            */
            /*------------------------------------------------------------*/
            dwErrorCode = GET_HANDLE_API_NOT_FOUND;
        }
    } /* End of first for-loop: Iterate over API. */

    AppSetLastError(dwErrorCode);

#ifdef DEBUG_API_TEST
    printf("DEBUG_API > Requested handle was not found (AR: %x SlotNr.:" \
           "%x)\n",dwApiNumber, wSlotNumber);
#endif
    return NULL;
}


/*!
 * \brief       This function returns a pointer to the subslot.
 *              You can get the error code by calling the function
 *              TPS_GetLastError().
 *
 * \param[in]   dwApiNumber        API Index
 * \param[in]   wSlot_Number       Slot Number
 * \param[in]   wSubslot_Number    Subslot Number
 * \retval      Pointer to a slot or NULL on failure.
*/
static SUBSLOT* AppGetSubslotHandle(USIGN32 dwApiNumber, USIGN16 wSlotNumber, USIGN16 wSubslotNumber)
{
    SLOT     *pzSlot = NULL;
    SUBSLOT  *pzSubslot = NULL;
    USIGN32  dwErrorCode = GET_HANDLE_SUBSLOT_NOT_FOUND;
    USIGN16  wReturnValue = 0;

    pzSlot = AppGetSlotHandle(dwApiNumber, wSlotNumber);

    if(pzSlot != NULL)
    {
        for(pzSubslot = pzSlot->pSubslot; pzSubslot != NULL; pzSubslot = pzSubslot->poNextSubslot)
        {
            TPS_GetValue16((USIGN8*)pzSubslot->pt_subslot_number, &wReturnValue);
            if(wReturnValue == wSubslotNumber)
            {
                dwErrorCode = TPS_ACTION_OK;
                break;
            }
        } /* End of for-loop: Iterate over Subslots. */
    }
    else
    {
        dwErrorCode = TPS_GetLastError();
    }

    if(dwErrorCode != TPS_ACTION_OK)
    {
        pzSubslot = NULL;
        AppSetLastError(GET_HANDLE_SUBSLOT_NOT_FOUND);
    }

    return pzSubslot;
}

/*---------------------------------------------------------------------------*/
/* APPLICATION EVENTS                                                        */
/*---------------------------------------------------------------------------*/

/*!
 * \brief       This function sets an alarm request event bit.
 *
 * \param[in]   USIGN8 byAR_NR
 * \retval      0                           on success
 *              API_ALARM_AR_NOT_KNOWN      invalid AR number
*/
static USIGN32 AppSetEventAlarmReq(USIGN32 dwArNumber)
{
    USIGN32 dwReturnValue = TPS_ACTION_OK;

    switch(dwArNumber)
    {
    case AR_0:
        AppSetEventRegApp(APP_EVENT_ALARM_SEND_REQ_AR0);
        break;
    case AR_1:
        AppSetEventRegApp(APP_EVENT_ALARM_SEND_REQ_AR1);
        break;
    case AR_IOSR:
        AppSetEventRegApp(APP_EVENT_ALARM_SEND_REQ_AR2);
        break;
    default:
        /* Not allowed AR number  */
        dwReturnValue = API_ALARM_AR_NOT_KNOWN;
        break;
    }

    return dwReturnValue;
}


/*!
 * \brief       writes the errorcode into a global variable for further use.
 *
 * \param[in]   USIGN32 dwErrorCode (last error) or 0 if no error
 * \retval      none
*/
VOID AppSetLastError (USIGN32 dwErrorCode)
{
    g_dwLastErrorCode = dwErrorCode;

#ifdef DEBUG_API_TEST
    if ( g_dwLastErrorCode != TPS_ACTION_OK )
    {
        printf("DEBUG_API > API: SetLastError() Set Error Code: 0x%X .\n", dwErrorCode);
    }
#endif
}


/*!
 * \brief       Function initializes an alarm mailbox and sets the pointers in the struct API_AR_CTX
 *
 * \param[in]   USIGN32 dwBoxNumber       number of mailbox
 * \param[in]   USIGN8 *g_pbyCurrentPointer   startpointer of the datablock
 *                                          inside the Dual-Ported RAM
 * \retval      dwErrorCode
*/
static USIGN32 AppCreateAlarmMailBox(USIGN32 dwBoxNumber)
{
    /* Create Mailbox for ALARM_LOW                                          */
    /*-----------------------------------------------------------------------*/
    g_zAlarmMailbox[dwBoxNumber].size_alarm_mailbox = SIZE_ALARM_MB;
    TPS_SetValue32(g_pbyCurrentPointer, SIZE_ALARM_MB);
    g_zAlarmMailbox[dwBoxNumber].pt_flags =       (USIGN8*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_flags);
    g_zAlarmMailbox[dwBoxNumber].pt_api =         (USIGN32*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_api);
    g_zAlarmMailbox[dwBoxNumber].pt_slot_nr =     (USIGN16*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_slot_nr);
    g_zAlarmMailbox[dwBoxNumber].pt_subslot_nr =  (USIGN16*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_subslot_nr);
    g_zAlarmMailbox[dwBoxNumber].pt_alarm_typ =   (USIGN16*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_alarm_typ);
    g_zAlarmMailbox[dwBoxNumber].pt_usi =         (USIGN16*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_usi);
    g_zAlarmMailbox[dwBoxNumber].pt_usr_hdl =     (USIGN16*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_usr_hdl);
    g_zAlarmMailbox[dwBoxNumber].pt_data_length = (USIGN16*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_data_length);
    g_zAlarmMailbox[dwBoxNumber].pt_data =        (USIGN8*)&(((ALARM_MB*)g_pbyCurrentPointer)->pt_data);
    g_pbyCurrentPointer += (SIZE_ALARM_MB + 4);

    memset(&g_byTransmitBuffer[0], 0x00, SIZE_ALARM_MB);
    TPS_SetValueData(g_zAlarmMailbox[dwBoxNumber].pt_flags,
                     &g_byTransmitBuffer[0],
                     g_zAlarmMailbox[dwBoxNumber].size_alarm_mailbox);

    return TPS_ACTION_OK;
}


/*!
 * \brief       This function returns I&M0Data for the respective read request
 *
 * \param[in]   dwSlotNr     for which the Data requested
 * \param[in]   dwSubslotNr  for which the Data requested
 * \retval      length on success
*/
static SIGN32 AppReadIM0(SUBSLOT* pzSubslot, USIGN8* pbyResponse)
{
    USIGN8* pbyResponseBuf = pbyResponse;
    BLOCK_HEADER* pzBlockHeader = (BLOCK_HEADER*) pbyResponse;
    USIGN32 dwLength = 0;
    USIGN16 wValue = 0x00;
    USIGN32 dwValue = 0x00;

    pbyResponseBuf += sizeof(BLOCK_HEADER);

    /* Write record specific data to the response buffer */
    *pbyResponseBuf = pzSubslot->poIM0->VendorIDHigh;
    pbyResponseBuf += 1;

    *pbyResponseBuf = pzSubslot->poIM0->VendorIDLow;
    pbyResponseBuf += 1;

    memcpy(pbyResponseBuf, pzSubslot->poIM0->OrderID, IM0_ORDERID_LEN);
    pbyResponseBuf += IM0_ORDERID_LEN;

    memcpy(pbyResponseBuf, pzSubslot->poIM0->IM_Serial_Number, IM0_SERIALNUMBER_LEN);
    pbyResponseBuf += IM0_SERIALNUMBER_LEN;

    wValue = TPS_htons(pzSubslot->poIM0->IM_Hardware_Revision);
    memcpy(pbyResponseBuf, &wValue, 2);
    pbyResponseBuf += 2;

    dwValue = TPS_htonl(pzSubslot->poIM0->IM_Software_Revision);
    memcpy(pbyResponseBuf, &dwValue, 4);
    pbyResponseBuf += 4;

    wValue = TPS_htons(pzSubslot->poIM0->IM_Revision_Counter);
    memcpy(pbyResponseBuf, &wValue, 2);
    pbyResponseBuf += 2;

    wValue = TPS_htons(pzSubslot->poIM0->IM_Profile_ID);
    memcpy(pbyResponseBuf, &wValue, 2);
    pbyResponseBuf += 2;

    wValue = TPS_htons(pzSubslot->poIM0->IM_Profile_Specific_Type);
    memcpy(pbyResponseBuf, &wValue, 2);
    pbyResponseBuf += 2;

    wValue = TPS_htons(pzSubslot->poIM0->IM_Version);
    memcpy(pbyResponseBuf, &wValue, 2);
    pbyResponseBuf += 2;

    wValue = TPS_htons(pzSubslot->poIM0->IM_Supported);
    memcpy(pbyResponseBuf, &wValue, 2);
    pbyResponseBuf += 2;

    /* calculate length */
    dwLength = pbyResponseBuf - pbyResponse;

    /* build block header */
    pzBlockHeader->length = TPS_htons(dwLength - 4);
    pzBlockHeader->type = TPS_htons(BLOCK_TYPE_IM0);
    pzBlockHeader->version = 1;

    return dwLength;
}


/*!
 * \brief       This function returns I&M0Data for the respective read request
 *
 * \param[in]   dwSlotNr     for which the Data requested
 * \param[in]   dwSubslotNr  for which the Data requested
 * \retval      length on success
*/
static SIGN32 AppReadIM1(SUBSLOT* pzSubslot, USIGN8* pbyResponse)
{
    USIGN8* pbyResponseBuf = pbyResponse;
    BLOCK_HEADER* pzBlockHeader = (BLOCK_HEADER*) pbyResponse;
    USIGN32 dwLength = 0;

    if(((pzSubslot->poIM0->IM_Supported) & IM1_SUPPORTED) != IM1_SUPPORTED)
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    pbyResponseBuf += sizeof(BLOCK_HEADER);

    memcpy(pbyResponseBuf, pzSubslot->poIM0->p_im1->IM_Tag_Function, IM_TAG_FUNCTION_LEN);
    pbyResponseBuf += IM_TAG_FUNCTION_LEN;

    memcpy(pbyResponseBuf, pzSubslot->poIM0->p_im1->IM_Tag_Location, IM_TAG_LOCATION_LEN);
    pbyResponseBuf += IM_TAG_LOCATION_LEN;

    dwLength = pbyResponseBuf - pbyResponse;

    pzBlockHeader->length = TPS_htons(dwLength - 4);
    pzBlockHeader->type = TPS_htons(BLOCK_TYPE_IM1);
    pzBlockHeader->version = 1;

    return dwLength;
}


/*!
 * \brief       This function returns I&M2Data for the respective read request
 *
 * \param[in]   dwSlotNr     for which the Data requested
 * \param[in]   dwSubslotNr  for which the Data requested
 * \retval      length on success, -1 otherwise.
*/
static SIGN32 AppReadIM2(SUBSLOT* pzSubslot, USIGN8* pbyResponse)
{
    USIGN8* pbyResponseBuf = pbyResponse;
    BLOCK_HEADER* pzBlockHeader = (BLOCK_HEADER*) pbyResponse;
    USIGN32 dwLength = 0;

    if(((pzSubslot->poIM0->IM_Supported) & IM2_SUPPORTED) != IM2_SUPPORTED)
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    pbyResponseBuf += sizeof(BLOCK_HEADER);

    memcpy(pbyResponseBuf, pzSubslot->poIM0->p_im2->IM_Date, IM_DATE_LEN);
    pbyResponseBuf += IM_DATE_LEN;

    dwLength = pbyResponseBuf - pbyResponse;

    pzBlockHeader->length = TPS_htons(dwLength - 4);
    pzBlockHeader->type = TPS_htons(BLOCK_TYPE_IM2);
    pzBlockHeader->version = 1;

    return dwLength;
}



/*!
 * \brief       This function returns I&M3Data for the respective read request
 *
 * \param[in]   dwSlotNr     for which the Data requested
 * \param[in]   dwSubslotNr  for which the Data requested
 * \retval      length on success, -1 otherwise.
*/
static SIGN32 AppReadIM3(SUBSLOT* poSubslot, USIGN8* pbyResponse)
{
    USIGN8* pbyResponseBuf = pbyResponse;
    BLOCK_HEADER* pzBlockHeader = (BLOCK_HEADER*) pbyResponse;
    USIGN32 dwLength = 0;

    if(((poSubslot->poIM0->IM_Supported) & IM3_SUPPORTED) != IM3_SUPPORTED)
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    pbyResponseBuf += sizeof(BLOCK_HEADER);

    memcpy(pbyResponseBuf, poSubslot->poIM0->p_im3->IM_Descriptor, IM_DESCRIPTOR_LEN);
    pbyResponseBuf += IM_DESCRIPTOR_LEN;

    dwLength = pbyResponseBuf - pbyResponse;

    pzBlockHeader->length = TPS_htons(dwLength - 4);
    pzBlockHeader->type = TPS_htons(BLOCK_TYPE_IM3);
    pzBlockHeader->version = 1;

    return dwLength;
}

/*!
 * \brief       This function returns I&M4Data for the respective read request
 *
 * \param[in]   dwSlotNr     for which the Data requested
 * \param[in]   dwSubslotNr  for which the Data requested
 * \retval      length on success, -1 otherwise.
*/
static SIGN32 AppReadIM4(SUBSLOT* pzSubslot, USIGN8* pbyResponse)
{
    USIGN8* pbyResponseBuf = pbyResponse;
    BLOCK_HEADER* pzBlockHeader = (BLOCK_HEADER*) pbyResponse;
    USIGN32 dwLength = 0;

    if(((pzSubslot->poIM0->IM_Supported) & IM4_SUPPORTED) != IM4_SUPPORTED)
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    pbyResponseBuf += sizeof(BLOCK_HEADER);
    memcpy(pbyResponseBuf, pzSubslot->poIM0->p_im4->IM_Signature, IM_SIGNATURE_LEN);
    pbyResponseBuf += IM_SIGNATURE_LEN;

    dwLength = pbyResponseBuf - pbyResponse;

    pzBlockHeader->length = TPS_htons(dwLength - 4);
    pzBlockHeader->type = TPS_htons(BLOCK_TYPE_IM4);
    pzBlockHeader->version = 1;

    return dwLength;
}


/*!
 * \brief       This function handles the reading of I&M Data. It will select
 *              the correct carrier of the data. Note: At the moment, only one
 *              carrier (the DAP) is supported. Therefore all read accesses to
 *              existing slots are routet to the DAP.
 *
 * \param[in]   dwApi       The requested API.
 * \param[in]   wSlotNr     The requested Slot.
 * \param[in]   wSubslotNr  The requested Subslot.
 * \retval      length on success, -1 otherwise.
*/
static SIGN32 AppReadIMData(USIGN32 dwApi, USIGN16 wSlotNr, USIGN16 wSubslotNr, USIGN16 wIndex, USIGN8* pbyResponse)
{
    SIGN32 dwLength = 0;
    SUBSLOT *pzSubslot = NULL;

    if(((wSlotNr == DAP_MODULE) && (wSubslotNr == DAP_INTSUBMODULE)) ||
       ((wSlotNr == DAP_MODULE) && (wSubslotNr == DAP_INTSUBMODULE_PORT_1)) ||
       ((wSlotNr == DAP_MODULE) && (wSubslotNr == DAP_INTSUBMODULE_PORT_2)))
    {
        pzSubslot = AppGetSubslotHandle(dwApi, DAP_MODULE, DAP_SUBMODULE);
    }
    else
    {
        pzSubslot = AppGetSubslotHandle(dwApi, wSlotNr, wSubslotNr);
    }

    if(pzSubslot == NULL)
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    /* Check if I&M0 data is supported. */
    if(pzSubslot->poIM0 == NULL)
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    switch(wIndex)
    {
        case RECORD_INDEX_IM0:
            dwLength = AppReadIM0(pzSubslot, pbyResponse);
            break;
        case RECORD_INDEX_IM1:
            dwLength = AppReadIM1(pzSubslot, pbyResponse);
            break;
        case RECORD_INDEX_IM2:
            dwLength = AppReadIM2(pzSubslot, pbyResponse);
            break;
        case RECORD_INDEX_IM3:
            dwLength = AppReadIM3(pzSubslot, pbyResponse);
            break;
        case RECORD_INDEX_IM4:
            dwLength = AppReadIM4(pzSubslot, pbyResponse);
            break;
        default:
            dwLength = RECORD_ERROR_INVALID_INDEX;
            break;
    }

    return dwLength;
}


/*!
 * \brief       Store the I&M Data which are delivered in the record data.
 *
 * \param[in]   byImNr      The number of the IM Dataset.
 * \param[in]   dwApi       The requested API.
 * \param[in]   wSlotNr     The requested Slot.
 * \param[in]   wSubslotNr  The requested Subslot.
 * \param[in]   pbyResponse  Pointer to the response buffer.
 * \retval      length on success, -1 otherwise.
*/
static SIGN32 AppWriteIMData(USIGN8 byIMSupportedFlag, USIGN32 dwApi, USIGN16 wSlotNr, USIGN16 wSubslotNr, USIGN8* pbyResponse)
{
    USIGN8* pbyReqBuff = pbyResponse + sizeof(BLOCK_HEADER);
    SUBSLOT* pzSubslot = NULL;
    SIGN32 dwReturnVal = RECORD_ERROR_INVALID_INDEX;

    /* Writing is only allowed to the Carrier. At the moment this has to be the DAP. */
    if(!( (dwApi == API_0) &&
          (wSlotNr == DAP_MODULE) &&
          (wSubslotNr == DAP_SUBMODULE) ))
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    /* Get the Carrier-Subslot handle. */
    pzSubslot = AppGetSubslotHandle(API_0, DAP_MODULE, DAP_SUBMODULE);
    if((pzSubslot == NULL) || (pzSubslot->poIM0 == NULL))
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    /* Check if the data is supported. */
    if(((pzSubslot->poIM0->IM_Supported) & byIMSupportedFlag) == 0)
    {
        return RECORD_ERROR_INVALID_INDEX;
    }

    switch(byIMSupportedFlag)
    {
        case IM1_SUPPORTED:
        {
            /* Write I&M1 data */
            memcpy(pzSubslot->poIM0->p_im1->IM_Tag_Function, pbyReqBuff, IM_TAG_FUNCTION_LEN);
            pbyReqBuff += IM_TAG_FUNCTION_LEN;
            memcpy(pzSubslot->poIM0->p_im1->IM_Tag_Location, pbyReqBuff, IM_TAG_LOCATION_LEN);
            pbyReqBuff += IM_TAG_LOCATION_LEN;

            dwReturnVal = IM_TAG_FUNCTION_LEN + IM_TAG_LOCATION_LEN;
            break;
        }

        case IM2_SUPPORTED:
        {
            /* Write I&M2 data*/
            memcpy(pzSubslot->poIM0->p_im2->IM_Date, pbyReqBuff, IM_DATE_LEN);
            dwReturnVal = IM_DATE_LEN;
            break;
        }

        case IM3_SUPPORTED:
        {
            /* Write I&M3 data */
            memcpy(pzSubslot->poIM0->p_im3->IM_Descriptor, pbyReqBuff, IM_DESCRIPTOR_LEN);
            dwReturnVal = IM_DESCRIPTOR_LEN;
            break;
        }

        case IM4_SUPPORTED:
        {
            /* Write I&M4 data*/
            memcpy(pzSubslot->poIM0->p_im4->IM_Signature, pbyReqBuff, IM_SIGNATURE_LEN);
            dwReturnVal = IM_SIGNATURE_LEN;
            break;
        }

        default:
        {
            /* error: not supported*/
            dwReturnVal = RECORD_ERROR_INVALID_INDEX;
            break;
        }
    }

    AppOnIMDataChanged(pzSubslot, byIMSupportedFlag);

    return dwReturnVal;
}

#ifdef USE_TPS1_TO_SAVE_IM_DATA
/*!
 * \brief       This function called from application and redirects IM Data back to application area
 *              for storing IM-Data to user space in flash memory of tps-1.
 *
 * \param[in]   wIMSupportedFlag e.g. IM1_SUPPORTED
 * \param[in]   dwApi       The requested API.
 * \param[in]   wSlotNr     The requested Slot.
 * \param[in]   wSubslotNr  The requested Subslot.
 * \param[in]   pbyResponse  Pointer to the response buffer.
 * \retval      length on success, -1 otherwise.
*/
SIGN32  TPS_WriteIMDataToFlash(USIGN16 wIMSupportedFlag, USIGN32 dwApi, USIGN16 wSlotNr, USIGN16 wSubslotNr, USIGN8* pbyResponse)
{
   USIGN16 wIMDataNr = 0;
   USIGN8* pbyFrame = pbyResponse;

   switch(wIMSupportedFlag)
   {
     case RECORD_INDEX_IM1:
         wIMDataNr = IM1_SUPPORTED;
     break;
     case RECORD_INDEX_IM2:
         wIMDataNr = IM2_SUPPORTED;
     break;
     case RECORD_INDEX_IM3:
         wIMDataNr = IM3_SUPPORTED;
     break;
     case RECORD_INDEX_IM4:
         wIMDataNr = IM4_SUPPORTED;
     break;

    default:
       return API_RECORD_WRITE_INDEX_UNKNOWN;

   }

   return AppWriteIMData((USIGN8)wIMDataNr, dwApi, wSlotNr, wSubslotNr, pbyFrame);
}
#endif
/*!
 * \brief       This function resets I&M1-4 data
 *
 * \param[in]   none
 * \retval      TPS_ACTION_OK
*/
static USIGN32 AppFactoryResetForIM1_4(VOID)
{
    SLOT* pzSlot = NULL;
    SUBSLOT* pzSubslot = NULL;
    API_LIST* pzApis = NULL;

    for(pzApis = g_zApiARContext.api_list; pzApis != NULL; pzApis = pzApis->next)
    {
        for(pzSlot = pzApis->firstslot; pzSlot != NULL; pzSlot = pzSlot->pNextSlot)
        {
            for(pzSubslot = pzSlot->pSubslot; pzSubslot != NULL; pzSubslot = pzSubslot->poNextSubslot)
            {
                if((pzSubslot->poIM0DataAvailable != 0) && (pzSubslot->poIM0 != NULL))
                {
                    if(((pzSubslot->poIM0->IM_Supported) & IM1_SUPPORTED) == IM1_SUPPORTED)
                    {
                        if(pzSubslot->poIM0->p_im1 != NULL)
                        {
                            memset(pzSubslot->poIM0->p_im1, 0x20, sizeof(T_IM1_DATA));
                        }
                    }

                    if(((pzSubslot->poIM0->IM_Supported) & IM2_SUPPORTED) == IM2_SUPPORTED)
                    {
                        if(pzSubslot->poIM0->p_im2 != NULL)
                        {
                            memset(pzSubslot->poIM0->p_im2, 0x20, sizeof(T_IM2_DATA));
                        }
                    }

                    if(((pzSubslot->poIM0->IM_Supported) & IM3_SUPPORTED) == IM3_SUPPORTED)
                    {
                        if(pzSubslot->poIM0->p_im3 != NULL)
                        {
                            memset(pzSubslot->poIM0->p_im3, 0x20, sizeof(T_IM3_DATA));
                        }
                    }

                    if(((pzSubslot->poIM0->IM_Supported) & IM4_SUPPORTED) == IM4_SUPPORTED)
                    {
                        if(pzSubslot->poIM0->p_im4 != NULL)
                        {
                            memset(pzSubslot->poIM0->p_im4, 0x20, sizeof(T_IM4_DATA));
                        }
                    }
                } /* if(I&M availabble) */
            } /* for(Subslots) */
        } /* for(Slots) */
    } /* for(APIs) */

    return TPS_ACTION_OK;
}



/*!
 * \brief       This function handles the read request for the RecordInputDataObject and RecordOutputDataObject
 *
 * \param[in]   oObjectToRead      Are inputs or outputs red.
 * \param[in]   dwApiNumber        The API-Number
 * \param[in]   wSlotnumber        The slot of the data.
 * \param[in]   wSubslotnumber     The subslot of the data.
 * \param[in]   pbyArrMailboxData  Pointer to the mailbox.
 * \retval      length on success or 0 on error
*/
static SIGN32 AppReadRecordDataObject(T_RECORD_DATA_OBJECT_TYPE zObjectToRead, USIGN32 dwARNumber,
                                      USIGN32 dwApiNumber,  USIGN16 wSlotNumber, USIGN16 wSubslotNumber, USIGN8* pbyMailboxData)
{
    USIGN32  dwResult = TPS_ACTION_OK;
    BOOL     bErrorOccured = TPS_FALSE;
    SUBSLOT* pzSubslot = NULL;
    SIGN32   lRecordLength = RECORD_ERROR_INVALID_INDEX;
    USIGN16  wDataLength = 0;
    USIGN16  wSubstituteActiveFlag = 0x00;
    USIGN8   byIocs = IOXS_BAD_BY_SUBSLOT;
    USIGN8   byIops = IOXS_BAD_BY_SUBSLOT;

    if(g_zApiARContext.OnReadRecordDataObject_CB == NULL)
    {
        bErrorOccured = TPS_TRUE;
    }

    /* Get the subslot-handle. */
    if(bErrorOccured == TPS_FALSE)
    {
        pzSubslot = AppGetSubslotHandle(dwApiNumber, wSlotNumber, wSubslotNumber);
        if(pzSubslot == NULL)
        {
            bErrorOccured = TPS_TRUE;
        }
    }

    /* check if record must be answered for this subslot */
    if(bErrorOccured == TPS_FALSE)
    {
       /* PN_SPEC: If the submodule is not owned by an AR, ...,  the  service  returns  “access  /  invalid index”. */
        USIGN16  wUsedInCr = 0x00;
        TPS_GetValue16((USIGN8*) (pzSubslot->pt_used_in_cr), &wUsedInCr);
        
        if(RECORD_OUTPUT_DATA_OBJECT == zObjectToRead)
        {
            TPS_GetValue16((USIGN8*)pzSubslot->pt_size_output_data, &wDataLength);
            if(wDataLength == 0)
            {
               bErrorOccured = TPS_TRUE;
            }
        }

        if(RECORD_INPUT_DATA_OBJECT == zObjectToRead)
        {
            TPS_GetValue16((USIGN8*)pzSubslot->pt_size_input_data, &wDataLength);
            if(wDataLength == 0)
            {
               bErrorOccured = TPS_TRUE;
            }
        }
    }

    if(bErrorOccured == TPS_FALSE)
    {
       /* PN_SPEC: If the owner sees  OK  or  SUBSTITUTE,  the  input  data  is  returned  accordingly,
       if the owner sees WRONG or NO_SUBMODULE, the service returns “access / invalid index”. */
       USIGN16 wSubmoduleState = 0;
       TPS_GetValue16((USIGN8*)pzSubslot->pt_submodule_state, &wSubmoduleState);

       if((wSubmoduleState != SUBMODULE_OK) && (wSubmoduleState != SUBSTITUTE_SUBMODULE))
       {
         bErrorOccured = TPS_TRUE;
       }
    }

    /* call registered callback routine that must fill in the current data */
    if(bErrorOccured == TPS_FALSE)
    {
        USIGN8*  pbyUserData = NULL;

        if(zObjectToRead == RECORD_INPUT_DATA_OBJECT)
        {
            TPS_GetValue16((USIGN8*)pzSubslot->pt_size_input_data, &wDataLength);
            lRecordLength = wDataLength + 12;
            pbyUserData = pbyMailboxData + 12;
        }
        else if(zObjectToRead == RECORD_OUTPUT_DATA_OBJECT)
        {
            TPS_GetValue16((USIGN8*)pzSubslot->pt_size_output_data, &wDataLength);
            lRecordLength = (wDataLength * 2) + 24;
            pbyUserData = pbyMailboxData + 13;
        }

        dwResult = g_zApiARContext.OnReadRecordDataObject_CB(zObjectToRead, pzSubslot,
                                                             &byIocs, &byIops, pbyUserData, wDataLength, &wSubstituteActiveFlag);

        if(dwResult != TPS_ACTION_OK)
        {
            bErrorOccured = TPS_TRUE;
        }
    }


    if(bErrorOccured == TPS_FALSE)
    {
        USIGN32  dwIndex = 0;
        USIGN16  wValue = 0x00;

        /***************************************************************************
        Necessary data were correctly collected. Now build the record read response.
        ***************************************************************************/
        if(zObjectToRead == RECORD_INPUT_DATA_OBJECT)
        {
            /* Block Header */
            /** Block Type. */
            wValue = TPS_htons(BLOCK_TYPE_RECORD_INPUT_DATA_OBJECT);
            memcpy(pbyMailboxData + dwIndex, &wValue, 2);
            dwIndex += 2;
            /** Block length without BlockType and BlockVersion. */
            wValue = TPS_htons(lRecordLength - 4);
            memcpy(pbyMailboxData + dwIndex, &wValue, 2);
            dwIndex += 2;
            /** Block Version High and low. */
            pbyMailboxData[dwIndex++] = 0x01;
            pbyMailboxData[dwIndex++] = 0x00;

            /* Length_IOCS and IOCS. */
            pbyMailboxData[dwIndex++] = 0x01;
            pbyMailboxData[dwIndex++] = byIocs;

            /* Length_IOPS and IOPS. */
            pbyMailboxData[dwIndex++] = 0x01;
            pbyMailboxData[dwIndex++] = byIops;

            /* Data length */
            wValue = TPS_htons(wDataLength);
            memcpy(pbyMailboxData + dwIndex, &wValue, 2);
            dwIndex += 2;

            /* Data: Already set by user callback. */
        }
        else if(zObjectToRead == RECORD_OUTPUT_DATA_OBJECT)
        {
            /* Block Type. */
            wValue = TPS_htons(BLOCK_TYPE_RECORD_OUTPUT_DATA_OBJECT);
            memcpy(pbyMailboxData + dwIndex, &wValue, 2);
            dwIndex += 2;
            /* Block length without BlockType and BlockVersion. */
            wValue = TPS_htons(lRecordLength - 4);
            memcpy(pbyMailboxData + dwIndex, &wValue, 2);
            dwIndex += 2;
            /* Block Version High and low. */
            pbyMailboxData[dwIndex++] = BLOCK_VERSION_HIGH;
            pbyMailboxData[dwIndex++] = BLOCK_VERSION_LOW;

            /* SubstituteActiveFlag. */
            wValue = TPS_htons(wSubstituteActiveFlag);
            memcpy(pbyMailboxData + dwIndex, &wValue, 2);
            dwIndex += 2;

            /* Length IOCS. */
            pbyMailboxData[dwIndex++] = 0x01;

            /* Length IOPS. */
            pbyMailboxData[dwIndex++] = 0x01;

            /* Data length*/
            wValue = TPS_htons(wDataLength);
            memcpy(pbyMailboxData + dwIndex, &wValue, 2);
            dwIndex += 2;

            /* Data-Item. */
            /* IOCS */
            pbyMailboxData[dwIndex++] = byIocs;

            /* Data: Already set by user callback. */
            dwIndex += wDataLength;

            /* IOPS */
            pbyMailboxData[dwIndex++] = byIops;

            /* SubstituteValue */
            /* BlockHeader */
            /* BlockType */
            wValue = TPS_htons(BLOCK_TYPE_SUBSTITUTE_VALUE);
            memcpy((pbyMailboxData + dwIndex), &wValue, 2);
            dwIndex += 2;

            /* BlockLength */
            wValue = TPS_htons(wDataLength + 6);
            memcpy((pbyMailboxData + dwIndex), &wValue, 2);
            dwIndex += 2;

            /* BlockVersion */
            pbyMailboxData[dwIndex++] = BLOCK_VERSION_HIGH;
            pbyMailboxData[dwIndex++] = BLOCK_VERSION_LOW;

            /* SubstitutionMode */
            wValue = TPS_htons(SUBSTITUTION_MODE_ZERO);
            memcpy((pbyMailboxData + dwIndex), &wValue, 2);
            dwIndex += 2;

            /* SubstituteDataItem */
            /* IOCS ( = DataItem.IOCS) */
            pbyMailboxData[dwIndex++] = byIocs;

            /* SubstituteDataObjectElement */
            /* Data */
            memset((pbyMailboxData + dwIndex), 0x00, wDataLength);
            dwIndex += wDataLength;

            /* SubstituteDataValid ( uses coding of IOxS) */
            if( byIops!=IOXS_GOOD)
            {
                pbyMailboxData[dwIndex++] = IOXS_GOOD;
            }
            else
            {
                pbyMailboxData[dwIndex++] = IOXS_BAD_BY_DEVICE;
            }

        }
    }
    else
    {
        lRecordLength = RECORD_ERROR_INVALID_INDEX;
    }

    return lRecordLength;
}


/*********************************************************************************************************
  END of CHANGING DEVICE CONFIGURATIONS
*********************************************************************************************************/


/*********************************************************************************************************
FUNCTIONS FOR PULL AND RETURN OF SUBMODULES
*********************************************************************************************************/
#ifdef PLUG_RETURN_SUBMODULE_ENABLE

/*!
 * \brief       Deactivates a Submodule in the Device configuration and sends a Pull/Plug Alarms
 *
 * \param[in]   dwApiNumber        The API-Number
 * \param[in]   wSlotnumber        The slot of the data.
 * \param[in]   wSubslotnumber     The subslot of the data.
 * \param[in]   wOpMode            Operation Mode(PULL/RETURN/ACTIVATE).
 * \retval      TPS_ACTION_OK     0
 *              Action failed     ErrorCode
*/
static USIGN32 AppPullPlugSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber, USIGN16 wOpMode)
{
    SUBSLOT* pzSubslot = NULL;
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN8 bySubmoduleIOXS = 0;

    if((wOpMode != OP_MODE_GO_PULL) &&
       (wOpMode != OP_MODE_GO_RETURN))
    {
        return PULL_PLUG_WRONG_OP_MODE;
    }

    pzSubslot = AppGetSubslotHandle(dwApi, wSlotNumber, wSubslotNumber);
    if(pzSubslot == NULL)
    {
        return TPS_GetLastError();
    }


    if(wOpMode == OP_MODE_GO_PULL)
    {
       if(pzSubslot->pzSlot->wProperties == MODULE_PULLED)
        {
            bySubmoduleIOXS = IOXS_BAD_BY_SLOT;
        }
        else
        {
            bySubmoduleIOXS = IOXS_BAD_BY_SUBSLOT;
        }

        dwRetval = AppPullSubModule(pzSubslot, bySubmoduleIOXS);
    }
    else if(wOpMode == OP_MODE_GO_RETURN)
    {
       dwRetval = AppRePlugSubmodule(pzSubslot);
    }

    return dwRetval;
}

/*!
 * \brief       Helper function: Indicates the removing of a sub module to TPS
 *
 * \param[in]   pzSubslot handle to the subslot structure
 * \param[in]   bySubmoduleIOXS status to be set
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - PULL_SUB_MODULE_NOT_FOUND
 *              - PULL_SUB_MODULE_ALREADY_REMOVED
 */
USIGN32 AppPullSubModule(SUBSLOT* pzSubslot, USIGN8 bySubmoduleIOXS)
{
    USIGN16 wSubslotProperties = 0;
    USIGN32 dwRetval = TPS_ACTION_OK;

    if (pzSubslot == NULL)
    {
        dwRetval = PULL_SUB_MODULE_NOT_FOUND;
    }

    if (dwRetval == TPS_ACTION_OK)
    {
        TPS_GetValue16((USIGN8*)pzSubslot->pt_properties, &wSubslotProperties);

        if ((wSubslotProperties & SUBMODULE_PROPERTIES_MASK) == SUBMODULE_NOT_PULLED)
        {
            /* Set IOCS for this module. */
            TPS_SetOutputIocs(pzSubslot, bySubmoduleIOXS);

            /* Set IOPS for this module. */
            TPS_SetInputIops(pzSubslot, bySubmoduleIOXS);

            /* This will generate a diffblock on connect. */
            TPS_SetSubmoduleState(pzSubslot, MODULE_NO, 0x00);

            /* Mark Module as pulled. */
            wSubslotProperties = (wSubslotProperties & ~SUBMODULE_PROPERTIES_MASK) | SUBMODULE_PULLED;
            TPS_SetValue16((USIGN8*)pzSubslot->pt_properties, wSubslotProperties);
        }
        else
        {
            dwRetval = PULL_SUB_MODULE_ALREADY_REMOVED;
        }
    }

    return dwRetval;
}

/*!
 * \brief       Helper function : indicates the TPS the return of a pulled Submodule.
 *
 * \param[in]   pzSubslot pointer to the sub slot structure
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - PULL_SUB_MODULE_NOT_FOUND
 *              - PLUG_SUB_SLOT_ALREADY_EXISTS
 */
USIGN32 AppRePlugSubmodule(SUBSLOT *pzSubslot)
{
    USIGN16 wSubslotProperties = 0;
    USIGN32 dwRetval = TPS_ACTION_OK;

    if (pzSubslot == NULL)
    {
        dwRetval = PULL_SUB_MODULE_NOT_FOUND;
    }

    if (dwRetval == TPS_ACTION_OK)
    {
        TPS_GetValue16((USIGN8*)pzSubslot->pt_properties, &wSubslotProperties);

        if ((wSubslotProperties & SUBMODULE_PROPERTIES_MASK) == SUBMODULE_PULLED)
        {
            /* Mark the Module 'Replugged'.                                   */
            wSubslotProperties = (wSubslotProperties & ~SUBMODULE_PROPERTIES_MASK) | SUBMODULE_REPLUGGED;
            TPS_SetValue16((USIGN8*)pzSubslot->pt_properties, wSubslotProperties);

            /* Set the Module State MODULE_OK. Can be overwritten in PrmEnd. */
            TPS_SetSubmoduleState(pzSubslot, MODULE_OK, 0x00);
        }
        else
        {
#ifdef DEBUG_API_TEST
            printf("DEBUG_API > State mismatch SubModuleProperties %04X \n", wSubslotProperties);
#endif
            dwRetval = PLUG_SUB_SLOT_ALREADY_EXISTS;
        }
    }
    return dwRetval;
}
#endif

/*!
 * \brief       Helper function - reactivates a returned submodule
 *
 * \param[in]   pzSubslot handle to the sub slot structure
 * \retval      possible return values:
 *              - TPS_ACTION_OK : success
 *              - REACTIVATE_SUBMODULE_NOT_FOUND
 *              - REACTIVATE_SUBMODULE_NOT_REPLUGGED
 */
USIGN32 AppReactivateSubmodule(SUBSLOT *pzSubslot)
{
    USIGN16 wSubslotProperties = 0;
    USIGN32 dwResult = TPS_ACTION_OK;

    if (pzSubslot == NULL)
    {
        dwResult = REACTIVATE_SUBMODULE_NOT_FOUND;
    }

    if (dwResult == TPS_ACTION_OK)
    {
        TPS_GetValue16((USIGN8*)(pzSubslot->pt_properties), &wSubslotProperties);

        if ((wSubslotProperties & SUBMODULE_PROPERTIES_MASK) == SUBMODULE_REPLUGGED)
        {
            wSubslotProperties = (wSubslotProperties & ~SUBMODULE_PROPERTIES_MASK) | SUBMODULE_NOT_PULLED;
            TPS_SetValue16((USIGN8*)pzSubslot->pt_properties, wSubslotProperties);
        }
        else
        {
            dwResult = REACTIVATE_SUBMODULE_NOT_REPLUGGED;
        }
    }

    return dwResult;
}


/*!
 * \brief       This function returns the offset of the APDU for the output data of an AR
 *
 * \param[in]   byArNr the number of the AR
 * \retval      APDU offset value or 0 if AR number was invalid
*/
USIGN32 App_GetAPDUOffsetForAR(USIGN8 byArNr)
{
   USIGN32 dwOffsetForApduOCR = 0;

   if ( byArNr<MAX_ARS_SUPPORTED )
   {
       TPS_GetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwApduAddrForCr[byArNr]), &dwOffsetForApduOCR);
       g_pbyApduAddr[byArNr] = (USIGN8*)dwOffsetForApduOCR;

       return (USIGN32)g_pbyApduAddr[byArNr];
   }
   return 0;
}

#ifdef USE_FS_APP
/*!
 * \brief       This function returns the startup mode
 *
 * \param[in]   no
 * \retval      "fast 1"/"normal 0" are possible values
*/
USIGN32 TPS_GetFSUMode(VOID)
{
  USIGN32 dwFSUModeFromNRT = 0;

  TPS_GetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwFastStarupParam), &dwFSUModeFromNRT);

  return dwFSUModeFromNRT;
}

/*!
 * \brief       This function initiates the read request for FSUParameterUUID
 * \param[in]   no
 * \retval      return values of function TPS_RecordReadReqToStack(..)
*/
USIGN32 TPS_ReadFSUParameter(USIGN8* pbyFsuParameter)
{

    USIGN32 dwRetval = TPS_ACTION_OK;

        /* Get the data out of the DPRAM.                                       */
    /*----------------------------------------------------------------------*/
    TPS_GetValueData((USIGN8*)&(g_pzNrtConfigHeader->dwFastStarupParam), pbyFsuParameter, SIZE_FSU_PARAMETER);

    return(dwRetval);
}
#endif

/*!
 * \brief   Debug function: prints a slot subslot configuration.
 * \ingroup allfunctions
 * \param[in]   none
 * \retval      TPS_ACTION_OK
*/
USIGN32 TPS_PrintSlotSubslotConfiguration(VOID)
{
    API_LIST *pzApi = NULL;
    SLOT     *pzSlot = NULL;
    SUBSLOT  *pzSubslot = NULL;
    USIGN16  wReturnValue = 0x00;
    USIGN32  dwReturnValue = 0x00;


    printf("APPLICATION: \n--------PRINT DEVICE CONFIGURATON-------\n");

    for(pzApi = g_zApiARContext.api_list; pzApi != NULL; pzApi = pzApi->next)
    {
        printf("API Nr: %d\n\n", pzApi->index);

        for(pzSlot = pzApi->firstslot; pzSlot != NULL; pzSlot = pzSlot->pNextSlot)
        {
            TPS_GetValue16((USIGN8*)pzSlot->pt_slot_number, &wReturnValue);
            printf("-- SlotNr 0x%X (at:0x%X)---------\n", wReturnValue, (USIGN32)pzSlot->pt_slot_number);
            TPS_GetValue32((USIGN8*)pzSlot->pt_ident_number, &dwReturnValue);
            printf("        ModulIdentNr:       0x%X (at: 0x%X)\n", dwReturnValue, (USIGN32)pzSlot->pt_ident_number);
            TPS_GetValue16((USIGN8*)pzSlot->pt_number_of_subslot, &wReturnValue);
            printf("        Number_of_Subslots: 0x%X (at: 0x%X) \n\n", wReturnValue, (USIGN32)pzSlot->pt_number_of_subslot);

            for(pzSubslot = pzSlot->pSubslot; pzSubslot != NULL; pzSubslot = pzSubslot->poNextSubslot)
            {
                TPS_GetValue16((USIGN8*)pzSubslot->pt_subslot_number, &wReturnValue);
                printf("--------- SubSlotNr 0x%X (at:0x%X)----------\n", wReturnValue, (USIGN32)pzSubslot->pt_subslot_number);
                TPS_GetValue32((USIGN8*)pzSubslot->pt_ident_number, &dwReturnValue);
                printf("\t  SubModuleIdent Number 0x%X (at:0x%X)\n", dwReturnValue, (USIGN32)pzSubslot->pt_ident_number);
                printf("\t  -----------Subslot-Data-------------\n");

                TPS_GetValue32((USIGN8*)pzSubslot->pt_size_init_records, &dwReturnValue);
                printf("\t  Size of Init Records: %d (at:0x%X)\n", dwReturnValue, (USIGN32)pzSubslot->pt_size_init_records);

                TPS_GetValue32((USIGN8*)pzSubslot->pt_size_chan_diag, &dwReturnValue);
                printf("\t  Size of Channel Diag: %d (at:0x%X)\n", dwReturnValue, (USIGN32)pzSubslot->pt_size_chan_diag);

                TPS_GetValue16((USIGN8*)pzSubslot->pt_properties, &wReturnValue);
                printf("\t  Properties          : 0x%X\n", wReturnValue);

                TPS_GetValue16((USIGN8*)pzSubslot->pt_size_input_data, &wReturnValue);
                printf("\t  Size of Input Data  : %d (at:0x%X)\n", wReturnValue, (USIGN32)pzSubslot->pt_size_input_data);

                TPS_GetValue16((USIGN8*)pzSubslot->pt_offset_input_data, &wReturnValue);
                printf("\t  Offset Input Data   : %d (at:0x%X)\n", wReturnValue, (USIGN32)pzSubslot->pt_offset_input_data);

                TPS_GetValue32((USIGN8*)pzSubslot->pt_input_data, &dwReturnValue);
                printf("\t  Pointer Input Data  : 0x%X\n", dwReturnValue);

                TPS_GetValue16((USIGN8*)pzSubslot->pt_size_output_data, &wReturnValue);
                printf("\t  Size of Output Data : %d (at:0x%X)\n", wReturnValue, (USIGN32)pzSubslot->pt_size_output_data);

                TPS_GetValue16((USIGN8*)pzSubslot->pt_offset_output_data, &wReturnValue);
                printf("\t  Offset Output Data  : %d (at:0x%X)\n", wReturnValue, (USIGN32)pzSubslot->pt_offset_output_data);

                TPS_GetValue32((USIGN8*)pzSubslot->pt_output_data, &dwReturnValue);
                printf("\t  Pointer Output Data : 0x%X\n\n", dwReturnValue);
            }
        }
    }

    printf("APPLICATION PRINT END\n\n");

    return TPS_ACTION_OK;
} /* TPS_PrintSlotSubslotConfiguration() */

 /*!
 * \brief       This function selects the protocols which should  be forwarded by tps1 to host application directly
 *
 * \param[in]   no
 * \retval      possible return values:
 *              - TPS_ACTION_OK
 *              - TPS_ERROR_WRONG_API_STATE
*/
USIGN32 TPS_ForwardProtocolsToHost(USIGN32 dwProtoSelector)
{
  USIGN32 dwRetVal = TPS_ACTION_OK;
  if(g_byApiState != STATE_INITIAL)
  {
     TPS_SetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwHostProtoSelector), dwProtoSelector);
     TPS_GetValue32((USIGN8*)&(g_pzNrtConfigHeader->dwHostProtoSelector), &dwRetVal);
  }
  else
  {
     return (TPS_ERROR_WRONG_API_STATE);
  }
  return dwRetVal;
} /* TPS_ForwardProtocolsToHost */

/*!
 * \brief       This function disables starting of FW-Updates over Ethernet
 *
 * \param[in]   no
 * \retval      "fast 1"/"normal 0" are possible values
*/
USIGN32 TPS_DisableExternalFwUpdate(VOID)
{
   USIGN32 dwRetVal = 0;
   dwRetVal = TPS_ForwardProtocolsToHost(SEL_DISABLE_FWUP);

   return dwRetVal;
} /* TPS_DisableExternalFwUpdate */

/*!@} Internal_Interface Internal Functions*/


/*---------------------------------------------------------------------------*/
/* End of internal functions.                                                */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/

/* Functions only for the unit test of the Driver.                           */
/*---------------------------------------------------------------------------*/
#ifdef TPS_DRIVER_TEST
#include "TPS_1_API_Accessor.c"
#endif
