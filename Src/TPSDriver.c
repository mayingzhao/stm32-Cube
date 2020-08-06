/*---------------------------------------------------------------------------*/
/* Application Includes                                                      */
/*---------------------------------------------------------------------------*/
#include <TPS_1_API.h>
#include "main.h"
#include "stm32f1xx_hal.h"
/*---------------------------------------------------------------------------*/
/* Defines                                                                   */
/*---------------------------------------------------------------------------*/
#define SIZE_EXCHANGE_BUFFER 0x0180
#define INIT_PARAMETER_EXAMPLE_SIZE  2
#define INIT_PARAMETER_SUBSTITUTE_CONFIG_SIZE 7
#define EXAMPLE_RECORD_INDEX 1234
#define SUBSTITUTE_CONFIG_RECORD_INDEX 0x0022

#define MODULE_ID1     0x02 /* ID of Module 1 */
#define SUBMODULE_ID1  0x02 /* ID of Submodule 1 in Slot 1. */
#define CHANNEL_DIAGNOSIS_NR  0x8000

#define SAMPLE_ORDER_ID       "1234567" /* max. 20 byte */

/*---------------------------------------------------------------------------*/
/* Global variables.                                                         */
/*---------------------------------------------------------------------------*/
/* This variable is set to 0x01 if the external switch is pressed.
 * Used for the alarm or diagnosis example. */
extern USIGN8 g_byDoContinue;

#ifdef DIAGNOSIS_ENABLE
/* Stores the information wether a diagnosis was added. */
static USIGN8 g_bDiagnosisAdded = TPS_FALSE;
/* Stores the handle to the added diagnosis (Slot 1, Subslot 1). */
static USIGN32 g_dwDiagnosisHandle_11 = 0;
#endif

/* Definition of the slot handle.                                            */
/*---------------------------------------------------------------------------*/
static SLOT    *g_pzModule_0;
static SLOT    *g_pzModule_1;

/* Definition of the subslot handle.                                         */
/*---------------------------------------------------------------------------*/
static SUBSLOT *g_pzSubmodule_01;
static SUBSLOT *g_pzSubmodule_11;
static SUBSLOT *g_pzSubmodule_12;

static USIGN8 g_byIOData[SIZE_EXCHANGE_BUFFER];

/* Definition of the I&M Data.                                         */
/*---------------------------------------------------------------------------*/
static T_IM0_DATA* g_pIM0_Data;
static T_IM1_DATA* g_pIM1_Data;
static T_IM2_DATA* g_pIM2_Data;
static T_IM3_DATA* g_pIM3_Data;
static T_IM4_DATA* g_pIM4_Data;

#ifdef DIAGNOSIS_ENABLE
static USIGN8 g_bAlarmAckReceived = TPS_FALSE;
#endif
/*---------------------------------------------------------------------------*/
/* Functions                                                                 */
/*---------------------------------------------------------------------------*/
VOID    enableIODataInterrupt(VOID);
VOID    onConnectReq(USIGN32 dwARNumber);
VOID    onConnectDoneRes(USIGN32 dwARNumber);
VOID    onPrmEndReq(USIGN32 dwARNumber);
VOID    onAbortReq(USIGN32 dwARNumber);
VOID    onRecordReadReq(USIGN32 dwARNumber);
VOID    onRecordWriteReq(USIGN32 dwARNumber);
USIGN32 onReadRecordDataObjectElement(T_RECORD_DATA_OBJECT_TYPE oObjectToRead, SUBSLOT* poSubslot,
                                   USIGN8* bIocs, USIGN8* bIops, USIGN8* pbyData, USIGN16 wDatalength, USIGN16* wSubstituteActiveFlag);
VOID    onAlarmAck(USIGN16 wAlarmHandler, USIGN32 dwResponseMsg);
VOID    onDiagAlarmAck(USIGN16 wAlarmHandler, USIGN32 dwResponseMsg);
VOID    onDcpSetStationName(VOID);
VOID    onDcpSetIpSuite(USIGN32 dwMode);
VOID    onDcpSignalReq(VOID);
VOID    onDcpResetToFactory(USIGN32 dwmode);
VOID    onEthPacketReceived(T_ETHERNET_MAILBOX* poRXMailbox);
VOID    onTpsMessageReceived(T_ETHERNET_MAILBOX* poEthernetMailbox);
VOID    onRebootTpsReq(USIGN32 dwDummy);
VOID    onImDataChanged(SUBSLOT *poSubslot, USIGN16 IMDataSelect);
VOID    onLedChanged(USIGN16 wLedState);

VOID    checkSendAlarmButton(VOID);
VOID    initImData(VOID);
VOID    printHexData(USIGN8* pbyData, USIGN32 dwDataLength);


/*****************************************************************************
**
** FUNCTION NAME: configDevice
**
** DESCRIPTION:   Function initialize the communication between the TPS
**                and the application. All neccessary Slots, Subslots, etc.
**                are configured.
**
** RETURN:        USIGN32 dwErrorCode
**
** Return_Type:   USIGN32
**
** PARAMETER:     no
**
*******************************************************************************
*/
USIGN32 configDevice (VOID)
{
    const USIGN16 PXC_VENDOR_ID = 0x0174;
    const USIGN16 DEMO_DEVICE_ID = 0x1234;
    USIGN32   dwDiagnosisListSize = 0;
    USIGN32   dwInitialparameterSize = 0;
    USIGN32   dwResult = TPS_ACTION_OK;
    API_LIST* pzApi = NULL;
    USIGN32   dwModuleId1 = MODULE_ID1; /* Module ID for Slot 1. IDs are defined in the GSD */
    USIGN32   dwSubmoduleId11 = SUBMODULE_ID1; /* Submodule ID for Subslot 1 in Slot 1. */
    T_DEVICE_SOFTWARE_VERSION oSoftwareVersion = {0};

    /* Set the software version.                                             */
    /* Note: This version has to be the same as in the I&M0 Record!          */
    /*-----------------------------------------------------------------------*/
    oSoftwareVersion.byRevisionPrefix = (USIGN8)SOFTWARE_REVISION_PREFIX;
    oSoftwareVersion.byRevisionFunctionalEnhancement = TPS_DRV_MAJOR_RELEASE;
    oSoftwareVersion.byBugFix = TPS_DRV_BUG_FIX_VERSION;
    oSoftwareVersion.byInternalChange = TPS_DRV_BUILD;

    /* Configure the device!                                                 */
    /*-----------------------------------------------------------------------*/
    dwResult = TPS_AddDevice(PXC_VENDOR_ID, DEMO_DEVICE_ID, (CHAR*)"TPS-1", &oSoftwareVersion);

    if(TPS_ACTION_OK == dwResult)
    {
        pzApi = TPS_AddAPI(API_0); /* Add "Application Process Identifier"    */
        if(pzApi == NULL)
        {
            dwResult = TPS_GetLastError();
        }
    }

    /**************************************************************************/
    /* ADD DEVICE ACCESS POINT                                                */
    /* The DAP must be the first module to be added to the configuration      */
    /**************************************************************************/

    if( TPS_ACTION_OK == dwResult )
    {
       /* fill the structures of I&M0..I&M4 */
       initImData();

       if(g_pIM0_Data != NULL)
       {
           if(TPS_ACTION_OK != TPS_SetOrderId(g_pIM0_Data->OrderID, IM0_ORDERID_LEN) )
           {
               printf("ERROR: TPS_SetOrderId() failed");
           }
       }

       g_pzModule_0 = TPS_PlugModule(pzApi, DAP_MODULE, 0x00000001);
       if (g_pzModule_0 == NULL)
       {
           dwResult = TPS_GetLastError();
       }
    }

    /* Configure the Device Subslot (DAP_SUBMODULE)                           */
    /* I&M0 Data must be registered to the DAP submodule                      */
    /* these I&M Data will be used for all further submodules                 */
    /**************************************************************************/
    if( TPS_ACTION_OK == dwResult)
    {
        g_pzSubmodule_01 = TPS_PlugSubmodule(g_pzModule_0,       /* slot handle */
                                             DAP_SUBMODULE,     /* submodule nr */
                                             0x01,              /* submodule ID */
                                             0,               /* parameter size */
                                             0,                /* diag channels */
                                             0,                   /* input data */
                                             0,                  /* output data */
                                             g_pIM0_Data, TPS_TRUE);          /* I&M Data */
        if( NULL == g_pzSubmodule_01 )
        {
            dwResult = TPS_GetLastError();
        }
        else
    /* optional: register I&M1..I&M4 to the DAP submodule */
        {
            TPS_RegisterIMDataForSubslot(g_pzSubmodule_01, g_pIM0_Data, g_pIM1_Data,
                                 g_pIM2_Data, g_pIM3_Data, g_pIM4_Data);
        }
    }



    /* End of ADD DEVICE ACCESS POINT                                        */
    /*************************************************************************/

    /* Add Slot 1 with 2 submodules to the device.                            */
    /*-----------------------------------------------------------------------*/
#ifdef DIAGNOSIS_ENABLE
    /* The maximum number of diagnosis entries on this subslot is 1. */
    dwDiagnosisListSize = 1;
#endif

#ifdef USE_AUTOCONF_MODULE
    /* If the autoconfiguration of Modules is enabled set the IDs to 0x00.
    * ID 0x00 enables module diff block generation by the application for this
    * module. Hereby it is possible to dynamically adapt the modules to the
    * expected configuration (see callback function onConnectReq). */
    dwModuleId1 = AUTOCONF_MODULE;
    dwSubmoduleId11 = AUTOCONF_MODULE;
#endif

    if( TPS_ACTION_OK == dwResult )
    {
        g_pzModule_1 = TPS_PlugModule(pzApi, 1, dwModuleId1);
        if(g_pzModule_1 == NULL)
        {
            dwResult = TPS_GetLastError();
        }
    }

    /* add two submodules to module 1 */
    /* submodules other than the DAP must not contain I&M Data */
    if( TPS_ACTION_OK == dwResult )
    {
       /* The sample GSD file describes 2 initial parameters for submodule 1/1:
       * One is an example on index 1234, a second is used to show
       * how the substitute value can be configured in local io mode. */

        dwInitialparameterSize = INIT_PARAMETER_HEADER_SIZE +
                                 INIT_PARAMETER_EXAMPLE_SIZE + /* Size of example parameter. */
                                 INIT_PARAMETER_HEADER_SIZE +
                                 INIT_PARAMETER_SUBSTITUTE_CONFIG_SIZE; /* Size of substitute config. */

        g_pzSubmodule_11 = TPS_PlugSubmodule(  g_pzModule_1,
                                             0x0001, dwSubmoduleId11,
                                             dwInitialparameterSize,
                                             dwDiagnosisListSize,
                                             2, 2, g_pIM0_Data, TPS_FALSE);
        if(NULL == g_pzSubmodule_11)
        {
            dwResult = TPS_GetLastError();
        }
    }

    if( TPS_ACTION_OK == dwResult )
    {
        g_pzSubmodule_12 = TPS_PlugSubmodule(  g_pzModule_1,
                                             0x0002, dwSubmoduleId11,
                                             dwInitialparameterSize,
                                             dwDiagnosisListSize,
                                             2, 2, g_pIM0_Data, TPS_FALSE);
        if(NULL == g_pzSubmodule_12)
        {
            dwResult = TPS_GetLastError();
        }
    }

#ifdef DEBUG_API_SUBPLUG_MODULE_PRINT
    TPS_PrintSlotSubslotConfiguration();
#endif

    return(dwResult);
}


/*****************************************************************************
**
** FUNCTION NAME: registerCallbacks
**
** DESCRIPTION:   All registrations for the call back function of the
**                event unit are done here.
**
** RETURN:        none
**
** Return_Type:   none
**
** PARAMETER:     no
**
*******************************************************************************
*/
VOID registerCallbacks(VOID)
{
   /*----------------------------------------------------------------------*/
   /*  Resgister Callback functions!                                       */
   /*----------------------------------------------------------------------*/
   TPS_RegisterRpcCallback(ONCONNECT_CB, &onConnectReq);
   TPS_RegisterRpcCallback(ONCONNECTDONE_CB, &onConnectDoneRes);
   TPS_RegisterRpcCallback(ON_PRM_END_DONE_CB, &onPrmEndReq);
   TPS_RegisterRpcCallback(ONABORT_CB, &onAbortReq);
   TPS_RegisterRpcCallback(ONREADRECORD_CB, &onRecordReadReq);
   TPS_RegisterRpcCallback(ONWRITERECORD_CB, &onRecordWriteReq);
   TPS_RegisterRpcCallback(ONRESET_CB, &onRebootTpsReq);

   /*----------------------------------------------------------------------*/
   /*  Register Alarm callback function!                                   */
   /*----------------------------------------------------------------------*/
   TPS_RegisterAlarmDiagCallback(ONALARM_CB, &onAlarmAck);
   TPS_RegisterAlarmDiagCallback(ONDIAG_CB, &onDiagAlarmAck);

   /*----------------------------------------------------------------------*/
   /*  Register DCP callback function!                                     */
   /*----------------------------------------------------------------------*/
   TPS_RegisterDcpCallback(ONDCP_SET_NAME_CB, &onDcpSetStationName);
   TPS_RegisterDcpCallbackPara(ONDCP_SET_IP_ADDR_CB, &onDcpSetIpSuite);
   TPS_RegisterDcpCallback(ONDCP_BLINK_CB, &onDcpSignalReq);
   TPS_RegisterDcpCallbackPara(ONDCP_FACT_RESET_CB, &onDcpResetToFactory);
   TPS_RegisterRecordDataObjectCallback(onReadRecordDataObjectElement);

   /*----------------------------------------------------------------------*/
   /*  Init communication channel TPS <-> Host!                            */
   /*----------------------------------------------------------------------*/
#ifdef USE_TPS_COMMUNICATION_CHANNEL
   TPS_InitTPSComChannel();
   TPS_RegisterTPSMessageCallback(&onTpsMessageReceived);
#endif

   /*----------------------------------------------------------------------*/
   /*  Register Ethernet callback function!                                */
   /*----------------------------------------------------------------------*/
#ifdef USE_ETHERNET_INTERFACE
   TPS_InitEthernetChannel();
   TPS_RegisterEthernetReceiveCallback(onEthPacketReceived);
#endif

   TPS_RegisterLedStateCallback(&onLedChanged);
   TPS_RegisterIMDataCallback(&onImDataChanged);
}


/*****************************************************************************
**
** FUNCTION NAME: main
**
** DESCRIPTION:   Initialization of the interface and start of
**                the application.
**
** RETURN:        none
**
** Return_Type:   int
**
** PARAMETER:     no
**
*******************************************************************************
*/
int StartTPS1 (void)
{
    USIGN32 dwResult = 0;
    USIGN16 wSubslotUsedInCr = 0;
    USIGN16 wSizeOutputData   = 0x0000;
    USIGN16 wSizeInputData    = 0x0000;
    USIGN8  bActiveIOAR = 0;
    USIGN8  byDataStatus = 0x00;
    SUBSLOT *pzSubmodule;
    USIGN16 wSubModuleNr = 0;
    //JM:__enable_interrupt();

    /* All TPS-PN events are signalized by an interrupt. The interrupt 1
     * of the V850 processes it.
     *----------------------------------------------------------------------*/
    /* enableIODataInterrupt(); */
    
    /* Check if the TPS Stack was started correctly by calling
     * TPS_CheckStackStart() until successful                               */
    printf("Waiting for initialization of the TPS...\r\n");
    //ResetTPS1();
    do
    {
        dwResult = TPS_CheckStackStart();

        if(dwResult == TPS_ERROR_STACK_VERSION_FAILED)
        {
            printf("ERROR: TPS Driver and TPS Firmware are not compatible!\n");
            return -1;
        }
        HAL_Delay(10);
    } while(dwResult != TPS_ACTION_OK);

    printf("TPS started\r\n");

    /*----------------------------------------------------------------------
     * Initialize TPS-1 API
     *----------------------------------------------------------------------*/
    dwResult = TPS_InitApplicationInterface();

    #ifdef DEBUG_MAIN
    if (dwResult != TPS_ACTION_OK)
    {
        printf("DEBUG_API > API: TPS_InitApplicationInterface failed!\n!");
    }
    else
    {
        printf("DEBUG_API > API: TPS_InitApplicationInterface OK!\r\n");
    }
    #endif

    /*----------------------------------------------------------------------
     *  Configure the modules and submodules of the device.
     *----------------------------------------------------------------------*/
    dwResult = configDevice();
    if(TPS_ACTION_OK != dwResult)
    {
        printf("ERROR: configDevice() returned: 0x%08X\n", dwResult);
    }

    /*----------------------------------------------------------------------*/
    /*  Register Callback functions!                                        */
    /*----------------------------------------------------------------------*/
    registerCallbacks();

    /*----------------------------------------------------------------------
     * Call TPS_StartDevice() to finish device configuration. Multiple calls
     * may be needed to complete the hand shake. When TPS_StartDevice() was
     * successful the PLCs can start communication to the TPS firmware.
     *----------------------------------------------------------------------*/
    do
    {
        dwResult = TPS_StartDevice();
    }
    while(TPS_ACTION_OK != dwResult);

    memset(&g_byIOData[0], 0x00, sizeof(g_byIOData));
    /* Get the TPS firmware version by using the internal record mailbox.
     * The answer is received by the callback: onTpsMessageReceived()
     *----------------------------------------------------------------------*/
    #ifdef USE_TPS_COMMUNICATION_CHANNEL
    TPS_GetStackVersionInfo();
    #endif

    /* Start checking the events from the TPS-1                             */
    /*----------------------------------------------------------------------*/
    while(1)
    {
        /* check if the hardware button was pressed */
        checkSendAlarmButton();

        /* Cyclic check for new events. If an event occured the previously
         * registered callback function for this event is called            */
        TPS_CheckEvents();

        /* When an AR was established start reading output data
         * and mirror them as input data
         *------------------------------------------------------------------*/
        for (bActiveIOAR = 0; bActiveIOAR < MAX_NUMBER_IOAR; bActiveIOAR++)
        {
            if(TPS_GetArEstablished(bActiveIOAR) == AR_ESTABLISH)
            {
                /* update the output buffer to receive the latest output data */
                TPS_UpdateOutputData(bActiveIOAR);

                /* Iterate over each configured submodule */
                for( wSubModuleNr = 0; wSubModuleNr < 3; wSubModuleNr++)
                {

                    switch(wSubModuleNr)
                    {
                    case 0:
                        pzSubmodule = g_pzSubmodule_01;
                        break;
                    case 1:
                        pzSubmodule = g_pzSubmodule_11;
                        break;
                    case 2:
                        pzSubmodule = g_pzSubmodule_12;
                        break;
                    default:
                        pzSubmodule = NULL;
                        break;
                    }

                    TPS_GetValue16((USIGN8*)pzSubmodule->pt_used_in_cr, &wSubslotUsedInCr);

                    /* if the current submodule is used and has input data */
                    if( (wSubslotUsedInCr & INPUT_USED) != SUBSLOT_NOT_USED)
                    {
                        /* get IO data size of the current submodule */
                        TPS_GetValue16((USIGN8*)(pzSubmodule->pt_size_output_data), &wSizeOutputData);
                        TPS_GetValue16((USIGN8*)(pzSubmodule->pt_size_input_data), &wSizeInputData);

                        /* Read the output data out of the output buffer */
                        TPS_ReadOutputData(pzSubmodule, g_byIOData, wSizeOutputData, &byDataStatus);
                        if(0x01 == (g_byIOData[0] & 0x01))
                        {
                          HAL_GPIO_WritePin(D2_GPIO_Port,D2_Pin,GPIO_PIN_SET);
                        }
                        else
                        {
                          HAL_GPIO_WritePin(D2_GPIO_Port,D2_Pin,GPIO_PIN_RESET);
                        }
                        /* If the subslot has more Input than Output data, fill the remaining bytes with 0x00 */
                        if(wSizeInputData > wSizeOutputData)
                        {
                            memset(&g_byIOData[wSizeOutputData], 0x00, wSizeInputData - wSizeOutputData);
                        }

                        /* write input data and iops to the input buffer */
                        TPS_WriteInputData(pzSubmodule, g_byIOData, wSizeInputData, IOXS_GOOD);
                    }

                    /* write the iocs of the current subslot to the input buffer */
                    TPS_SetOutputIocs(pzSubmodule, IOXS_GOOD);

                } /* for wSubModuleNr < 3 */

                /* When the data of each submodule were written into the input
                 * buffer, call TPS_UpdateInputData to send the input
                 * frame to the PLC */
                TPS_UpdateInputData(bActiveIOAR);

            } /* if TPS_GetArEstablished(bActiveIOAR) */

        } /* for bActiveIOAR < MAX_NUMBER_IOAR */

    } /* while (1) */
}

/*****************************************************************************
**
** FUNCTION NAME: enableIODataInterrupt
**
** DESCRIPTION:   All settings for interrupt processing are made in
**                the application.
**
** RETURN:        none
**
** Return_Type:   none
**
** PARAMETER:     no
**
*******************************************************************************
*/
VOID enableIODataInterrupt(VOID)
{
    #ifdef DEBUG_MAIN
        USIGN32 dwReturnValue;
    #endif

    /* Set the HOST_IRQmask_high; Enable Interrupt TCP/IP Channel.          */
    /* (Bit 23 (TPS_EVENT_ETH_FRAME_REC) set to 0).                                                   */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32((USIGN8*)HOST_IRQ_MASK_HIGH,(0xFFFFFFFF  & ~(0x01 << TPS_EVENT_ETH_FRAME_REC)));

    #ifdef DEBUG_MAIN
       /* Print all activated interrpt events.                              */
       /*-------------------------------------------------------------------*/
       TPS_GetValue32(((USIGN8 *)HOST_IRQ_LOW), &dwReturnValue);
       printf("HOST_IRQ_LOW: 0x%X \n", dwReturnValue);
       TPS_GetValue32(((USIGN8 *)HOST_IRQ_HIGH), &dwReturnValue);
       printf("HOST_IRQ_HIGH: 0x%X \n", dwReturnValue);
    #endif

    /* Set interrupt time for repetition.                                   */
    /*----------------------------------------------------------------------*/
    TPS_SetValue32(((USIGN8*)HOST_EOI), 0x0003FFFF);
}


/*****************************************************************************
**
** FUNCTION NAME: checkSendAlarmButton
**
** DESCRIPTION:   Sends an alarm when the Button is pressed. Used for testing.
**                Note: The variable g_byDoContinue is part of the user specific
**                code and not part of this example code.
**
** RETURN:        none
**
** Return_Type:   none
**
** PARAMETER:     no
**
*******************************************************************************
*/
VOID checkSendAlarmButton(VOID)
{
    /* If the diagnosis interface is disabled send an alarm if the button is
     * pressed.
     * If the Diagnosis Interface is enabled add ar remove an diagnosis to the Slot.
     * The diagnosis function then generates an alarm too. */
#if defined(TEST_ALARM)

    if(g_byDoContinue != 0)
    {
        /* Button was pressed, now send an alarm. */
        USIGN32 dwRetval = TPS_ACTION_OK;
        USIGN8 pbyAlarmItem[200] = {0};

                                /* AR, API, Slot Subslot        AlarmType  */
        dwRetval = TPS_SendAlarm(AR_0, 0,  0,   1,   1, ALARM_LOW, UPDATE_ALARM, sizeof(pbyAlarmItem), pbyAlarmItem, 0x00, 0x01);

        printf("Returnvalue of TPS_SendAlarm(): 0x%X\n", dwRetval);

        g_byDoContinue = 0;
    }

#elif defined(PLUG_RETURN_SUBMODULE_ENABLE)

    static USIGN16 wModuleRemoved = 0;
    USIGN32 dwRetval = TPS_ACTION_OK;

    if(g_byDoContinue)
    {
        g_byDoContinue = 0;

        if(wModuleRemoved == 0)
        {
            /* Remove module and submodules. */
            dwRetval = TPS_PullModule(0, 1);

            if( dwRetval == TPS_ACTION_OK)
            {
                wModuleRemoved = 1;

                dwRetval = TPS_SendAlarm(AR_0, /* API */0x00, /* Slot */1, /* Subslot */0,
                                         ALARM_LOW, PULL_ALARM, 0, NULL, 0x100, 0xABCD);

                if(dwRetval != TPS_ACTION_OK)
                {
                    printf("  Error: Pull Alarm not sent!\n");
                }

                printf("SubModule removed!\n" );
            }
        }
        else if(wModuleRemoved == 1)
        {
            /* Return module */
            dwRetval = TPS_RePlugModule(0, 1);

            if( dwRetval == TPS_ACTION_OK)
            {
                dwRetval = TPS_RePlugSubmodule(0, 1, 1);
            }

            if(dwRetval == TPS_ACTION_OK)
            {
                dwRetval = TPS_SendAlarm(AR_0, /* API */0x00, /* Slot */1, /* Subslot */1,
                                         ALARM_LOW, PLUG_ALARM, 0, NULL, 0x100, 0xABCD);
                if(dwRetval != TPS_ACTION_OK)
                {
                    printf("  Error: Plug Alarm not sent!\n");
                }
                wModuleRemoved = 0;
                printf("SubModule returned!\n" );
            }
        }
    }
#elif defined(DIAGNOSIS_ENABLE)

    USIGN32 dwRetval = 0;
    USIGN16 wChannelProperties = 0;

    if(g_byDoContinue != 0)
    {
        #ifdef DEBUG_MAIN
            printf("DEBUG_API > API: checkSendAlarmButton entered\n");
        #endif

        /* Button was pressed, now add or remove the diagnosis entry. */
        g_byDoContinue = 0;

        if(g_bDiagnosisAdded == TPS_FALSE)
        {
            /* Add a diagnosis entry. */
            printf("Adding a diagnosis to Slot 1, Subslot 1...\n");

            /* Set the channel properties. */
            dwRetval = TPS_DiagSetChannelProperties(&wChannelProperties, 0, 0, MAINTENANCE_REQUIRED, APPEARS, DIAG_DIRECTION_SPECIFIC);
            if(dwRetval != TPS_ACTION_OK)
            {
                printf("TPS_DiagSetChannelProperties -> ERROR: %x\n", dwRetval);
                return;
            }

            /* Add the diagnosis entry.              Subslothandle,    Channel,              Channelproperties,  Channelerrortype,    Extchannelerrortype, Additional value. */
            g_dwDiagnosisHandle_11 = TPS_DiagChannelAdd(g_pzSubmodule_11, CHANNEL_DIAGNOSIS_NR, wChannelProperties, CET_OVERTEMPERATURE, 0x00000000,          0x00000000);
            if(g_dwDiagnosisHandle_11 == 0x00)
            {
                printf("TPS_DiagChannelAdd -> ERROR \n");
                return;
            }

            /* Send the diagnosis alarm. */
            dwRetval = TPS_SendDiagAlarm(0, 0, 1, 1, ALARM_LOW, APPEARS, g_dwDiagnosisHandle_11, 0x0000);
            if(dwRetval != TPS_ACTION_OK)
            {
                printf("TPS_SendDiagAlarm Appears -> ERROR: %x\n", dwRetval);
                return;
            }

            g_bDiagnosisAdded = TPS_TRUE;
        }
        else if(g_bAlarmAckReceived)
        {
            /* Remove the previously added diagnosis entry. */
            printf("Removing the diagnosis entry from the slot...\n");

            dwRetval = TPS_DiagSetChangeState(g_dwDiagnosisHandle_11, MAINTENANCE_REQUIRED, DISAPPEARS);
            if(dwRetval != TPS_ACTION_OK)
            {
                printf("TPS_DiagSetChangeState to DISAPPEARS - ERROR: 0x%08X\n", dwRetval);
            }

            /* Send the disappears alarm. */
            dwRetval = TPS_SendDiagAlarm(0, 0, 1, 1, ALARM_LOW, DISAPPEARS, g_dwDiagnosisHandle_11, 0x0000);
            if(dwRetval != TPS_ACTION_OK)
	        {
                /* Error can be ignored, it is ok if there is no AR. */
                printf("TPS_SendDiagAlarm Disappears -> ERROR: %x\n", dwRetval);
            }

            /* Removing the diagnosis. */
            dwRetval = TPS_DiagChannelRemove(g_dwDiagnosisHandle_11);
            if(dwRetval != TPS_ACTION_OK)
	        {
                printf("Second TPS_DiagChannelRemove -> ERROR\n");
                return;
            }

            g_bDiagnosisAdded = TPS_FALSE;
        }
    }

#endif
}



/*****************************************************************************
**
** FUNCTION NAME: checkSubslotOwnedByAr
**
** DESCRIPTION:   Helper function: Checks if subslot is owned by given AR number
**
*******************************************************************************
*/
BOOL checkSubslotOwnedByAr(SUBSLOT* pzSubslot, USIGN32 dwArNumber)
{
    USIGN16 wSubslotOwner = 0;

    if(pzSubslot != NULL)
    {
        TPS_GetValue16((USIGN8*)(pzSubslot->pt_wSubslotOwnedByAr), &wSubslotOwner);
    }

    if( wSubslotOwner & OWNED_BY_AR(dwArNumber) )
    {
        return TPS_TRUE;
    }
    else
    {
        return TPS_FALSE;
    }
}


/*---------------------------------------------------------------------------
 * Callback functions
 * ==================
 * These Callback functions may added by the user. These callback functions
 * are called when TPS_CheckEvents() detected an event
 *---------------------------------------------------------------------------*/

/*****************************************************************************
**
** FUNCTION NAME: onConnectReq
**
** DESCRIPTION:   This will be registered with the function
**                TPS_RegisterRpcCallback to the context managment.
**                The TPS-1 received a Connect Indication
**                (Connect.Req) and sets up the AR and all necessary data for
**                the communication with the Controller.
**
** RETURN:        VOID
**
** PARAMETER:     USIGN32 dwARNumber  (number of the application relation)
**
*******************************************************************************
*/
VOID onConnectReq(USIGN32 dwARNumber)
{
#ifdef USE_AUTOCONF_MODULE
    T_MODULE_STATE oModuleState = MODULE_OK;
    USIGN32 dwErrorcode = TPS_ACTION_OK;
    USIGN32 dwModuleID;
    USIGN32 dwSubmoduleID;
    USIGN16 wSizeOfInputData;
    USIGN16 wSizeOfOutputData;
#endif

#ifdef DEBUG_MAIN
    printf("DEBUG_API > API: OnConnectRequest Event 0x%X\n", dwARNumber);
#endif

#ifdef USE_AUTOCONF_MODULE
    /* This example code shows how to use the autoconf feature of the TPS-1 Stack.
    * This feature allows the application to adapt the device to the configuration
    * which the profinet controller expects.
    * This code will accept modules and submodules with the ID 0x02. This is the same
    * configuration as without the autoconf module. Change this code for your own needs. */

    /* Set Submodule state of DAP OK */
    dwErrorcode = TPS_SetSubmoduleState(g_pzSubmodule_01, MODULE_OK, NULL);

    dwErrorcode = TPS_GetModuleConfiguration(g_pzModule_1, &dwModuleID);
    if(dwErrorcode != TPS_ACTION_OK)
    {
        printf("ERROR: TPS_GetModuleConfiguration: 0x%X\n", dwErrorcode);
    }

    dwErrorcode = TPS_GetSubmoduleConfiguration(g_pzSubmodule_11, &dwSubmoduleID, &wSizeOfInputData, &wSizeOfOutputData);
    if(dwErrorcode != TPS_ACTION_OK)
    {
        printf("ERROR: TPS_GetSubmoduleConfiguration: 0x%X\n", dwErrorcode);
    }

#ifdef DEBUG_MAIN
    printf("DEBUG_API > API: Controller expects: Module ID 0x%2.2X, Submodule ID: 0x%2.2X, input bytes: %d, output bytes: %d\n",
           dwModuleID, dwSubmoduleID, wSizeOfInputData, wSizeOfOutputData);
#endif

    /* Set the state of the Module 1.
    * In case of Wrong: Set Module to substitute and set all Submodules to Wrong. */
    oModuleState = dwModuleID == MODULE_ID1 ? MODULE_OK : MODULE_SUBSTITUTE;

    dwErrorcode = TPS_SetModuleState(g_pzModule_1, oModuleState, MODULE_ID1);
    if(dwErrorcode != TPS_ACTION_OK)
    {
        printf("ERROR: TPS_SetModuleState: 0x%X\n", dwErrorcode);
    }

    /* Set the state of the Submodule 1.1 */
    if((dwSubmoduleID == SUBMODULE_ID1) &&
       (oModuleState == MODULE_OK))
    {
        oModuleState = MODULE_OK;
    }
    else
    {
        oModuleState = MODULE_WRONG;
    }

    dwErrorcode = TPS_SetSubmoduleState(g_pzSubmodule_11, oModuleState, SUBMODULE_ID1);
    if(dwErrorcode != TPS_ACTION_OK)
    {
        printf("ERROR: TPS_SetSubmoduleState 0x%X\n", dwErrorcode);
    }
#else
    /* if autoconf is not used, each configured submodule must be set OK  */
    TPS_SetSubmoduleState(g_pzSubmodule_01, MODULE_OK, NULL);

    TPS_SetModuleState(g_pzModule_1, MODULE_OK, NULL);
    TPS_SetSubmoduleState(g_pzSubmodule_11, MODULE_OK, NULL);
    TPS_SetSubmoduleState(g_pzSubmodule_12, MODULE_OK, NULL);

#endif
}

/*****************************************************************************
**
** FUNCTION NAME: onConnectDoneRes
**
** DESCRIPTION:   This will be registered with the function
**                TPS_RegisterRpcCallback to the context managment.
**                After receiving a Connect.req, all necessary data for
**                the communication of this AR is checked. When the
**                connection is established this function is called.
**
** RETURN:        0 or ErrorCode
**
** Return_Type:   USIGN32
**
** PARAMETER:     USIGN32 dwARNumber  (number of the application relation)
**
*******************************************************************************
*/
VOID onConnectDoneRes (USIGN32 dwARNumber)
{
    #ifdef DEBUG_MAIN
        printf("DEBUG_API > API: Application Relation 0x%X established\n",dwARNumber);
    #endif
}

/*****************************************************************************
**
** FUNCTION NAME: initSubslotIoxs()
**
** DESCRIPTION:   This function is used to set the Input Data to 0x00 and
**                IOPS and IOCS to Good if the module is correctly configured.
*
** RETURN:        TPS_ACTION_OK or Errorcode.
**
** PARAMETER:     dwARNumber  number of the application relation.
**                poSubslot   Pointer to the Subslot structure.
**
*******************************************************************************
*/
USIGN32 initSubslotIoxs(USIGN32 dwARNumber, SUBSLOT* pzSubslot)
{
    /* return if the subslot is not used by this AR                */
    if(TPS_FALSE == checkSubslotOwnedByAr(pzSubslot, dwARNumber))
    {
        return 1;
    }

    /* write IOCS into the frame buffer */
    TPS_SetOutputIocs(pzSubslot, IOXS_GOOD);

    /* write IOPS into the frame buffer */
    /* optional: use TPS_WriteInputData() instead to write initial data */
    TPS_SetInputIops(pzSubslot, IOXS_GOOD);

    return TPS_ACTION_OK;
}

/*****************************************************************************
**
** FUNCTION NAME: onPrmEndReq()
**
** DESCRIPTION:   This function will be registered with the function
**                TPS_RegisterRpcCallback.
**
**
**
** RETURN:        VOID
**
** Return_Type:   VOID
**
** PARAMETER:     USIGN32 dwARNumber  (number of the application relation)
**
*******************************************************************************
*/
VOID onPrmEndReq(USIGN32 dwARNumber)
{
    USIGN16 wIndex;
    USIGN32 dwDataLength;
    USIGN32 dwSizeInitRecordsUsed;
    USIGN8* pbyCurrentPointer;

    #ifdef DEBUG_MAIN
        USIGN8  byInitParameter[INIT_PARAMETER_SUBSTITUTE_CONFIG_SIZE];
        USIGN16 wPosition;
        printf("DEBUG_API > API: OnPRMEMDCallback for AR 0x%X is called.\n",dwARNumber);
    #endif

    /* Read the initialparameter which are written by the controller.
     * Iterate over all the parameters in the NRT Area. */
    if(g_pzSubmodule_11 != NULL)
    {
        TPS_GetValue32((USIGN8*)g_pzSubmodule_11->pt_size_init_records_used, &dwSizeInitRecordsUsed);
        pbyCurrentPointer = (USIGN8*)g_pzSubmodule_11->pt_init_records;

        while(dwSizeInitRecordsUsed > INIT_PARAMETER_HEADER_SIZE)
        {
            /* Get the initialparameter.        */
            /* They are stored in the format    */
            /*  2 Byte: record index            */
            /*  4 Byte: Length of data          */
            /*  x Bytes: The data.              */
            TPS_GetValue16(pbyCurrentPointer, &wIndex);
            pbyCurrentPointer += sizeof(USIGN16);

            TPS_GetValue32(pbyCurrentPointer, &dwDataLength);
            pbyCurrentPointer += sizeof(USIGN32);

            #ifdef DEBUG_MAIN
                printf("DEBUG_API > API: OnPRMEMDCallback() Parameter from controller: Index 0x%4.4X, Length 0x%2.2X, Data: ",
                       wIndex, dwDataLength);

                /* If the data fits into the array, read and print it. */
                if(dwDataLength <= sizeof(byInitParameter))
                {
                    TPS_GetValueData(pbyCurrentPointer, byInitParameter, dwDataLength);
                    for(wPosition = 0; wPosition < dwDataLength; wPosition++)
                    {
                        printf(" 0x%2.2X", byInitParameter[wPosition]);
                    }
                }
                printf("\n");
            #endif
            pbyCurrentPointer += dwDataLength;
            dwSizeInitRecordsUsed -= dwDataLength + INIT_PARAMETER_HEADER_SIZE;
        }
    }
   /* initialize the 3 output data buffers with valid data from the plc
      this ensures that TPS_ReadOutputData() always returns valid data */
   TPS_UpdateOutputData(dwARNumber);
   TPS_UpdateOutputData(dwARNumber);
   TPS_UpdateOutputData(dwARNumber);

    /* Set IOPS and IOCS of each configured submodule to the proper state
     * to ensure that the ioxs are set in the first cyclic frame after
     * application ready */
    initSubslotIoxs(dwARNumber, g_pzSubmodule_01);
    initSubslotIoxs(dwARNumber, g_pzSubmodule_11);
    initSubslotIoxs(dwARNumber, g_pzSubmodule_12);

    /* Output the set data. */
    TPS_UpdateInputData(dwARNumber);
}

/*****************************************************************************
**
** FUNCTION NAME: onAbortReq()
**
** DESCRIPTION:   The protocol stack informs the application that
**                the link to the controller for a AR is lost.
**
**                This function will be registered with the function
**                TPS_RegisterRpcCallback.
**
** RETURN:        none
**
** Return_Type:   VOID
**
** PARAMETER:     USIGN32 dwARNumber  (number of the application relation)
**
*******************************************************************************
*/
VOID onAbortReq(USIGN32 dwARNumber)
{
    #ifdef DEBUG_MAIN
        printf("DEBUG_API > API: Application Relation 0x%X aborted\n", dwARNumber);
    #endif
}

/*****************************************************************************
**
** FUNCTION NAME: onRecordReadReq()
**
** DESCRIPTION:   When an "TPS_EVENT_ONRECORDREAD" indication occurs
**                this function is called.
**
**                This function will be registered with the function
**                TPS_RegisterRpcCallback.
**
** RETURN:        none
**
** Return_Type:   VOID
**
** PARAMETER:     USIGN32 dwARNumber  (number of the application relation)
**
*******************************************************************************
*/
VOID onRecordReadReq(USIGN32 dwARNumber)
{
    SIGN32  dwDataLen = 3;
    RECORD_BOX_INFO mailBoxInfo;
    USIGN8 byArrMailboxData[10] = {0,0,0,0,0,0,0,0,0,0};
    USIGN16 wErrorCode1 = 0;
    USIGN16 wErrorCode2 = 0;

    TPS_GetMailboxInfo(dwARNumber, &mailBoxInfo);


    #ifdef DEBUG_MAIN
        printf("DEBUG_API > API: RecordRead.req: (MailboxNr.: 0x%X) API 0x%X,  \
                Slot %d, Subslot %d, Index 0x%X, RecordDataLength %d\n",dwARNumber,  \
                mailBoxInfo.dwAPINumber, mailBoxInfo.wSlotNumber, \
                mailBoxInfo.wSubSlotNumber, mailBoxInfo.wIndex, mailBoxInfo.dwRecordDataLen);
    #endif

    switch(mailBoxInfo.wIndex)
    {
        /* In this Switch statement you can add an index of your application       */
        /*-------------------------------------------------------------------*/
        /*
        case 0xXXXX:
            break;  */

        case EXAMPLE_RECORD_INDEX:
            /* The example startup parameter as defined in the GSDML. */
            dwDataLen = 0x02;
            byArrMailboxData[0] = 0x12;
            byArrMailboxData[1] = 0x34;
            break;
            
        case 0x8030: /* Index: 0x8030                     */
            /* The example startup parameter as defined in the GSDML. */
            dwDataLen = 0x00;
            break;    

        default:
            dwDataLen = 0;
            wErrorCode1 = 0xB0; /* PNIORW-ErrorClass: Access, ErrorCode: Invalid Index */
            wErrorCode2 = 0x00;
            break;
    }

    if(dwDataLen == -1)
    {
        dwDataLen = 0;
        wErrorCode1 = 0xA0; /* PNIORW-ErrorClass: Application, ErrorCode: Read Error */
        wErrorCode2 = 0;
    }

    TPS_WriteMailboxData(dwARNumber, byArrMailboxData, dwDataLen);
    TPS_RecordReadDone(dwARNumber, wErrorCode1, wErrorCode2);
}


/*****************************************************************************
**
** FUNCTION NAME: onReadRecordDataObjectElement()
**
** DESCRIPTION:   This function handles the read request for the RecordInputDataObject
**                and RecordOutputDataObject. If the callback returns a value != TPS_ACTION_OK
**                or is not registered, the TPS will reject the record access with invalid index.
**
** RETURN:        TPS_ACTION_OK
**
** PARAMETER:     USIGN32 dwARNumber  (number of the application relation)
**
*******************************************************************************
*/
USIGN32 onReadRecordDataObjectElement(T_RECORD_DATA_OBJECT_TYPE oObjectToRead, SUBSLOT* poSubslot,
                                   USIGN8* bIocs, USIGN8* bIops, USIGN8* pbyData, USIGN16 wDatalength, USIGN16* wSubstituteActiveFlag)
{
    USIGN16 wCounter = 0;
    USIGN16 wUsedInCr = 0x00;
    USIGN8 bIsSlotUsed = TPS_FALSE;
    USIGN8 bIoxs = IOXS_BAD_BY_SUBSLOT;

    #ifdef DEBUG_MAIN
        printf("DEBUG_API > API: onReadRecordDataObjectElement called. Type: %d\n", oObjectToRead);
    #endif

    /* Check if the slot is used. */
    TPS_GetValue16((USIGN8*)poSubslot->pt_used_in_cr, &wUsedInCr);
    if(wUsedInCr & USED_IN_ANY_AR != 0x00)
    {
        bIsSlotUsed = TPS_TRUE;
        bIoxs = IOXS_GOOD;
    }

    *bIocs = bIoxs;
    *bIops = bIoxs;

    /* Simulate some data. */
    for(wCounter = 0; wCounter < wDatalength; wCounter++)
    {
        pbyData[wCounter] = wCounter;
    }

    /* Set the substitution flag according to the IOPS. */
    if(oObjectToRead == RECORD_OUTPUT_DATA_OBJECT)
    {
        *wSubstituteActiveFlag = bIsSlotUsed ? SUBSTITUTE_ACTIVE_FLAG_OPERATION : SUBSTITUTE_ACTIVE_FLAG_SUBSTITUTE;
    }

    return TPS_ACTION_OK;
}

/*****************************************************************************
**
** FUNCTION NAME: onLedChanged()
**
** DESCRIPTION:   Registered with the function TPS_RegisterLedStateCallback().
**
**                Called from API-Driver after change detecting by TPS-LEDs.
**                Gives actual state of TPS-LEDs
**
*******************************************************************************/
VOID onLedChanged(USIGN16 wLedState)
{
#ifdef DEBUG_MAIN
   printf("wLedState = %x\n", wLedState);
#endif
}

/*****************************************************************************
**
** FUNCTION NAME: onImDataChanged()
**
** DESCRIPTION:   Registered with the function TPS_RegisterIMDataCallback().
**
**                Called from API-Driver after a Record-Write for I&M0-4
**                was received. Here should be placed user code to save the
**                received data remanent.
**
*******************************************************************************/
VOID    onImDataChanged(SUBSLOT *poSubslot, USIGN16 wIMDataSelect)
{
   USIGN16 wSubslotNumber = 0;

   TPS_GetValue16((USIGN8*)poSubslot->pt_subslot_number, &wSubslotNumber);

   switch(wIMDataSelect)
   {

      case IM1_SUPPORTED:
        {
        printf("Write I&MData_1 to flash\n");
        /*write_data_to_flash(IM1_DATA, g_pIM1_Data, sizeof(T_IM1_DATA));*/
        }
      break;
      case IM2_SUPPORTED:
        {
        printf("Write I&MData1_2 to flash\n");
        /*write_data_to_flash(IM2_DATA, g_pIM2_Data, sizeof(T_IM2_DATA));*/
        }
      break;
      case IM3_SUPPORTED:
        {
        printf("Write I&MData1_3 to flash\n");
        /*write_data_to_flash(IM3_DATA, g_pIM3_Data, sizeof(T_IM3_DATA));*/
        }
      break;
      case IM4_SUPPORTED:
        {
        printf("Write I&MData1_4 to flash\n");
        /*write_data_to_flash(IM4_DATA, g_pIM4_Data, sizeof(T_IM4_DATA));*/
        }
      break;
      default:
        {
            /* error: not supported*/
            /* place here your code for handling of this case */
           break;
        }

   }
#ifdef DEBUG_MAIN
   printf("I&M Data(0x%x) was changed for Subslot 0x%x\n", wIMDataSelect, wSubslotNumber);
#endif
}

/*****************************************************************************
**
** FUNCTION NAME: onRecordWriteReq()
**
** DESCRIPTION:   When an "TPS_EVENT_ONRECORDWRITE" indication occurs
**                this function is called.
**
**                This function is be registered with the function
**                TPS_RegisterRpcCallback.
**
** RETURN:        none
**
** Return_Type:   VOID
**
** PARAMETER:     USIGN32 dwMbNr  mail box number
**
*******************************************************************************
*/
VOID onRecordWriteReq(USIGN32 dwMbNr)
{
    RECORD_BOX_INFO oMailBoxInfo;
    USIGN16 wErrorCode1 = 0x00;
    USIGN16 wErrorCode2 = 0x00;
    USIGN8* byArrMailboxData = 0;

    TPS_GetMailboxInfo(dwMbNr, &oMailBoxInfo);

    #ifdef DEBUG_MAIN
        printf("DEBUG_API > API: RecordWrite.req: (MailboxNr.: 0x%X) API 0x%X,  \
               Slot %d, Subslot %d, Index 0x%X, RecordDataLength %d\n",dwMbNr,  \
               oMailBoxInfo.dwAPINumber, oMailBoxInfo.wSlotNumber, \
               oMailBoxInfo.wSubSlotNumber, oMailBoxInfo.wIndex, oMailBoxInfo.dwRecordDataLen);
    #endif

    byArrMailboxData = malloc(oMailBoxInfo.dwRecordDataLen);

    if(byArrMailboxData == NULL)
    {
        /*ERROR*/
    }
    else
    {
        TPS_ReadMailboxData(dwMbNr, byArrMailboxData, oMailBoxInfo.dwRecordDataLen);
    }

    switch(oMailBoxInfo.wIndex)
    {
       case RECORD_INDEX_IM1:
       case RECORD_INDEX_IM2:
       case RECORD_INDEX_IM3:
       case RECORD_INDEX_IM4:
         /* Toolkit > 1.4: Should never happen: Write I&M data is processed tps_1_api.c and
            then indicated to the registered IMData Callback function.*/
         break;

        case EXAMPLE_RECORD_INDEX:
            /* The example startup parameter as defined in the GSDML. */
            wErrorCode1 = 0x00;
            wErrorCode2 = 0x00;
            break;

       /* Add your own record indexes here*/

        default:
            wErrorCode1 = 0xB0; /* PNIORW-ErrorClass: Access, ErrorCode: Invalid Index */
            wErrorCode2 = 0;
            break;
    }

    TPS_RecordWriteDone(dwMbNr, wErrorCode1, wErrorCode2);
    free(byArrMailboxData);
}

/*****************************************************************************
**
** FUNCTION NAME: onAlarmAck()
**
** DESCRIPTION:   The protocol stack received an acknowdeged
**                for an alarm of the device.
**
** RETURN:        none
**
** Return_Type:   VOID
**
** PARAMETER:     USIGN16 wAlarmHandler (identifier)
**                USIGN32 dwResponseMsg ??
**
**                ALARM_HIGH = 0 : only one alarm mailbox availiable now!
**
*******************************************************************************
*/
VOID onAlarmAck(USIGN16 wAlarmHandler, USIGN32 dwResponseMsg)
{
    #ifdef DEBUG_MAIN
        printf("APP: ALARM_ACKN was received\n");
    #endif
#ifdef DIAGNOSIS_ENABLE
    g_bAlarmAckReceived = TPS_TRUE;
#endif
}

/******************************************************************************
**
** FUNCTION NAME: onDiagAlarmAck()
**
** DESCRIPTION:   The protocol stack received an acknowdeged
**                for an alarm of the device.
**
** RETURN:        none
**
** Return_Type:   VOID
**
** PARAMETER:     USIGN16 wAlarmHandler (identifier)
**                USIGN32 dwResponseMsg ??
**
**                ALARM_HIGH = 0 : only one alarm mailbox availiable now!
**
*******************************************************************************
*/
/* Codereview: Dokumentation und Namen anpassen: Parameter sind Dummy Parameter!
 * Wunschliste: Eigene Registrierungsfunktion für Funktion ohne Paramter. */
VOID onDiagAlarmAck(USIGN16 wAlarmHandler, USIGN32 dwResponseMsg)
{
    #ifdef DEBUG_MAIN
        printf("APP: DIAG_ACKN was received\n");
    #endif
}

/*****************************************************************************
**
** FUNCTION NAME:   onDcpSetStationName()
**
** DESCRIPTION:     The TPS stack sends the Device Name to the
**                  application CPU.
**
** RETURN:          none
**
** PARAMETER:       VOID
**
*******************************************************************************
*/
VOID onDcpSetStationName(VOID)
{
    #ifdef DEBUG_MAIN
        printf("DEBUG_API > APP: DCP_Set_Station_Name request was received!\n");
    #endif
}

VOID onRebootTpsReq(USIGN32 dwDummy )
{
    printf("reset signal from TPS-1\n");
}

/*****************************************************************************
**
** FUNCTION NAME:   App_OnDcpSetStationIpCallback()
**
** DESCRIPTION:     The TPS stack sends the Device IP Address to the
**                  application CPU.
**
** RETURN:          none
**
** PARAMETER:       USIGN32 dwMode save permanent of not
**
*******************************************************************************
*/
VOID onDcpSetIpSuite(USIGN32 dwMode)
{
    #ifdef DEBUG_MAIN
        USIGN32 dwIPAddress;
        USIGN32 dwSubnetMask;
        USIGN32 dwGateway;

        if(dwMode == DCP_SET_PERMANENT)
        {
            printf("DEBUG_API > APP: DCP_Set_IP_Addr permanent!\n");
        }
        else
        {
            printf("DEBUG_API > APP: DCP_Set_IP_Addr temporary!\n");
        }

        TPS_GetIPConfig(&dwIPAddress, &dwSubnetMask, &dwGateway);
        printf("IP Adress %d.%d.%d.%d, SubnetMask: %d.%d.%d.%d Gateway: %d.%d.%d.%d\n",
               (dwIPAddress >> 24) & 0xFF,  (dwIPAddress >> 16) & 0xFF,  (dwIPAddress >> 8) & 0xFF,  (dwIPAddress) & 0xFF,
               (dwSubnetMask >> 24) & 0xFF, (dwSubnetMask >> 16) & 0xFF, (dwSubnetMask >> 8) & 0xFF, (dwSubnetMask) & 0xFF,
               (dwGateway >> 24) & 0xFF,    (dwGateway >> 16) & 0xFF,    (dwGateway >> 8) & 0xFF,    (dwGateway) & 0xFF);
    #endif
}

/*****************************************************************************
**
** FUNCTION NAME:   onDcpSignalReq()
**
** DESCRIPTION:     The TPS stack sends the message DCP_Blink_Req to the
**                  application CPU.
**
** RETURN:          none
**
** PARAMETER:       USIGN32 dwMode
**
*******************************************************************************
*/
VOID onDcpSignalReq (VOID)
{
    #ifdef DEBUG_MAIN
        printf("APP: DCP_Blink START request was received!\n");
    #endif
}

/*****************************************************************************
**
** FUNCTION NAME:   onDcpResetToFactory()
**
** DESCRIPTION:     The TPS stack sends the message DCP_Set_FactorySettings
**                  to the application CPU.
**
** RETURN:          none
**
** PARAMETER:       dwResetOption
**
*******************************************************************************
*/
VOID onDcpResetToFactory(USIGN32 dwResetOption)
{
    #ifdef DEBUG_MAIN
        printf("onDcpResetToFactory Option = %x\n", dwResetOption);
    #endif

   /* Current I&M1 ..4 data were already cleared by tps_1_api.c. now delete these from flash memory */
    if(dwResetOption == DCP_R2F_OPT_ALL)
    {
       /* clear I&M data in non volatile memory */
    }
}


/*****************************************************************************
**
** FUNCTION NAME: onTpsMessageReceived()
**
** DESCRIPTION:   This Callback function is called when the TPS Firmware answers
**                an internal request sent by TPS_RecordReadReqToStack()
**                to the internal interface.
**
** RETURN:        none
**
** Return_Type:   void
**
** PARAMETER:     poEthernetMailbox: The pointer to the ethernet mailbox.
**
**
*******************************************************************************
*/
VOID onTpsMessageReceived(T_ETHERNET_MAILBOX* poEthernetMailbox)
{
    USIGN32 dwLength = 0;
    USIGN32 dwPortNr = 0;
    USIGN32 dwRetval = TPS_ACTION_OK;
    USIGN16 wType = 0x00;
    USIGN16 wSlotNr = 0x00;
    USIGN16 wSubslotNr = 0x00;
    USIGN16 wIndex = 0x00;
    USIGN32 dwDataLength = 0x00;
    USIGN32 dwOffset = 0x00;
    USIGN8* pbyRecordReadData = NULL;
    USIGN8* pbyData = NULL;
    T_IM0_DATA* poIM0Data = NULL;

    TPS_GetValue32((USIGN8*)&poEthernetMailbox->dwLength, &dwLength);
    TPS_GetValue32((USIGN8*)&poEthernetMailbox->dwPortnumber, &dwPortNr);

    printf("DEBUG_API > API: Message from TPS firmware received. Length: 0x%X (LAN port %x)\n", dwLength, dwPortNr);

    /* Check the data. */
    if(dwLength < sizeof(RW_RECORD_REQ_BLOCK))
    {
        /* Invalid Length. */
        dwRetval = 0x01;
    }

    /* Read the data. */
    if(dwRetval == TPS_ACTION_OK)
    {
        dwOffset = 0x00;
        /* Type. Offset: 0x00 */
        TPS_GetValue16(poEthernetMailbox->byFrame + dwOffset, &wType);
        wType = TPS_ntohs(wType);
    }

    if(dwRetval == TPS_ACTION_OK && wType == READRECORD_RES)
    {
        /* Slot. */
        dwOffset += sizeof(BLOCK_HEADER) + 22; /* 22: SeqNumber(2) + ARUUID(16) + API(4).*/
        TPS_GetValue16(poEthernetMailbox->byFrame + dwOffset, &wSlotNr);
        wSlotNr = TPS_ntohs(wSlotNr);

        /* Subslot. */
        dwOffset += 2;
        TPS_GetValue16(poEthernetMailbox->byFrame + dwOffset, &wSubslotNr);
        wSubslotNr = TPS_ntohs(wSubslotNr);

        /* Index. */
        dwOffset += 4; /* 4: Subslot(2) + Pading(2) */
        TPS_GetValue16(poEthernetMailbox->byFrame + dwOffset, &wIndex);
        wIndex = TPS_ntohs(wIndex);

        /* Data length. */
        dwOffset += 2;
        TPS_GetValue32(poEthernetMailbox->byFrame + dwOffset, &dwDataLength);
        dwDataLength = TPS_ntohl(dwDataLength);

        /* The Data.                                                         */
        /* 28: RecordDataLength(4), AdditionalValue1(2),                     */
        /*     AdditionalValue2(2), RWPadding(20).                           */
        /*-------------------------------------------------------------------*/
        dwOffset += 28;
        if(dwDataLength > 0)
        {
            pbyRecordReadData = malloc(dwDataLength);
            if(pbyRecordReadData != NULL)
            {
                TPS_GetValueData(poEthernetMailbox->byFrame + dwOffset,
                                 pbyRecordReadData, dwDataLength);
            }
        }
    }

    #ifdef DEBUG_MAIN
        /* Print information. */
        if(dwRetval == TPS_ACTION_OK)
        {
            if(wType == READRECORD_RES)
            {
                printf("DEBUG_API > API: Record Read Response received. SlotNr: 0x%X SubslotNr: 0x%X Index: 0x%X Length: 0x%X\n",
                       wSlotNr, wSubslotNr, wIndex, dwDataLength);
                printf("Data: ");

                printHexData(pbyRecordReadData, dwDataLength);
            }
            else
            {
                printf("DEBUG_API > ERROR: Unknown response.\n");
                dwRetval = 0x01;
            }
        }
    #endif

    /*-----------------------------------------------------------------------*/
    /* The example shows how to decode the TPS-1 Version information.        */
    /* The user could add here handling of further records.                  */
    /*-----------------------------------------------------------------------*/
    if((dwRetval == TPS_ACTION_OK) &&
       (wType == READRECORD_RES) &&
       (pbyRecordReadData != NULL))
    {
        switch(wIndex)
        {
            case RECORD_INDEX_TPS_VERSION:
                /* The index RECORD_INDEX_TPS_VERSION answers a stack        */
                /* version request. The TPS Version is coded in the I&M0     */
                /* Format. Decode and print it.                              */
                /*-----------------------------------------------------------*/
                poIM0Data = (T_IM0_DATA*)(pbyRecordReadData + sizeof(BLOCK_HEADER));

                #ifdef DEBUG_MAIN
                    printf("Data is TPS Version\n");
                    printf("\tVendorIDHigh: 0x%X\n", poIM0Data->VendorIDHigh);
                    printf("\tVendorIDLow: 0x%X\n", poIM0Data->VendorIDLow);
                    printf("\tOrderID: ");
                    printHexData(poIM0Data->OrderID, IM0_ORDERID_LEN);
                    printf("\tIM_Serial_Number: ");
                    printHexData(poIM0Data->IM_Serial_Number, IM0_SERIALNUMBER_LEN);
                    printf("\tIM_Hardware_Revision: 0x%X\n", poIM0Data->IM_Hardware_Revision);
                    printf("\tIM_Software_Revision: 0x%X\n", poIM0Data->IM_Software_Revision );
                    printf("\tIM_Revision_Counter: 0x%X\n", poIM0Data->IM_Revision_Counter );
                #endif

                pbyData = (USIGN8*)&poIM0Data->IM_Software_Revision;
                printf("\tTPS Firmware Version: %c.%u.%u.%u.%u \n",
                       pbyData[0], pbyData[1] / 10, pbyData[1] % 10, pbyData[2], pbyData[3]);
                break;

            default:
                break;
        }
    }

    /* Free the memory again. */
    if(pbyRecordReadData != NULL)
    {
        free(pbyRecordReadData);
        pbyRecordReadData = NULL;
    }
}


/*****************************************************************************
**
** FUNCTION NAME:   onEthPacketReceived()
**
** DESCRIPTION:     This Callback is called if an ethernet frame is received by
**                  the TPS-1.
**
** RETURN:          none
**
** PARAMETER:       T_ETHERNET_MAILBOX* poRXMailbox Pointer to the ethernet mailbox
**                  with the received data.
**
*******************************************************************************
*/
VOID onEthPacketReceived(T_ETHERNET_MAILBOX* poRXMailbox)
{
    USIGN32 dwLength = 0;
    USIGN32 dwPortNr = PORT_NR_1_2;
    #ifdef USE_ETHERNET_MIRROR_APPLICATION
        USIGN8 bySourceMac[MAC_ADDRESS_SIZE];
        USIGN8 byTargetMac[MAC_ADDRESS_SIZE];
        USIGN8 byTpsInterfaceMac[MAC_ADDRESS_SIZE];
        USIGN8 byReceivedFrame[MAX_LEN_ETHERNET_FRAME];
    #endif

    TPS_GetValue32((USIGN8*)&poRXMailbox->dwLength, &dwLength);
    TPS_GetValue32((USIGN8*)&poRXMailbox->dwPortnumber, &dwPortNr);

    #ifdef DEBUG_API_ETH_FRAME
        printf("DEBUG_API > API: Ethernet frame received at LAN port 0x%X, Length: 0x%X\n",
               dwPortNr, dwLength);
    #endif


    #ifdef USE_ETHERNET_MIRROR_APPLICATION
        /* Send every received Frame back the the sender.                      */
        /* But only return packages which are directly send to the interface,  */
        /* not broadcasts.                                                     */
        /*---------------------------------------------------------------------*/
        TPS_GetValueData((USIGN8*)&poRXMailbox->byFrame, byTargetMac, MAC_ADDRESS_SIZE);
        TPS_GetMacAddresses(byTpsInterfaceMac, NULL, NULL);

        if((memcmp(byTargetMac, byTpsInterfaceMac, MAC_ADDRESS_SIZE) == 0) &&
           (dwPortNr == PORT_NR_INTERFACE))
        {
            /* Read the package from the mailbox. */
            /* The returned Length contains the CRC of the ethernet frame, don't return it. */
            dwLength -= 0x04;
            /* Ethernet Frame: TARGET_MAC[6], SOURCE_MAC[6], TYPE[2], DATA[] */
            TPS_GetValueData((USIGN8*)&poRXMailbox->byFrame, byReceivedFrame, dwLength);
            /* Read source MAC. */
            memcpy(bySourceMac, byReceivedFrame + MAC_ADDRESS_SIZE, MAC_ADDRESS_SIZE);

            #ifdef DEBUG_API_ETH_FRAME
                printf("DEBUG_API >     Target: 0x%X:0x%X:0x%X:0x%X:0x%X:0x%X Source: 0x%X:0x%X:0x%X:0x%X:0x%X:0x%X Sending packet back\n",
                       byTargetMac[0], byTargetMac[1], byTargetMac[2], byTargetMac[3], byTargetMac[4], byTargetMac[5],
                       bySourceMac[0], bySourceMac[1], bySourceMac[2], bySourceMac[3], bySourceMac[4], bySourceMac[5]);
            #endif

            /* Switch source / target mac. */
            memcpy(byReceivedFrame, bySourceMac, MAC_ADDRESS_SIZE);
            memcpy(byReceivedFrame + MAC_ADDRESS_SIZE, byTpsInterfaceMac, MAC_ADDRESS_SIZE);

            /* Send the frame. */
            TPS_SendEthernetFrame(byReceivedFrame, dwLength, dwPortNr);
        }
    #endif

    /* Mark the mailbox as free. */
    TPS_SetValue32((USIGN8*)&poRXMailbox->dwMailboxState, ETH_MBX_EMPTY);
}


/******************************************************************************
*
*  FUNCTION NAME:    initImData
*
*  DESCRIPTION:     This is the example function which is initiating
*                   the I&M0-4 data for one subslot.
*
*  PARAMETER:       VOID
*
*  RETURN:          0
*
*******************************************************************************
*/
VOID  initImData(VOID)
{
    g_pIM0_Data = (T_IM0_DATA*) calloc(1, sizeof(T_IM0_DATA));

    if(g_pIM0_Data == 0)
    {
        /* error no memory */
    }
    else
    {
       USIGN8 pbyBuffer[IM0_ORDERID_LEN+1];
       /*
       read the I&M0 parameter from non volatile memory and fill it here in the
       globally defined variables
       i.e. read_flash_setting(IM0_DATA, (USIGN8*)IM0_Data, sizeof(T_IM0_DATA));
       */

       /* sample values are used here */
       g_pIM0_Data->VendorIDHigh = 0x01;
       g_pIM0_Data->VendorIDLow =  0x74;

       /* read the order id */
       memset(g_pIM0_Data->OrderID, ' ', sizeof(g_pIM0_Data->OrderID));
       TPS_GetOrderId(pbyBuffer, IM0_ORDERID_LEN+1);
       for(USIGN8 i = 0; (i < IM0_ORDERID_LEN) && (pbyBuffer[i] != 0x00); i++)
       {
          g_pIM0_Data->OrderID[i] = pbyBuffer[i];
       }

       /* read the serial number from the NRT area */
       TPS_GetSerialnumber( pbyBuffer, IM0_SERIALNUMBER_LEN+1);
       memcpy(g_pIM0_Data->IM_Serial_Number, pbyBuffer, IM0_SERIALNUMBER_LEN);

       g_pIM0_Data->IM_Hardware_Revision = 0x0001;
       g_pIM0_Data->IM_Software_Revision = (SOFTWARE_REVISION_PREFIX << 24) |
          (TPS_DRV_MAJOR_RELEASE << 16) |
             (TPS_DRV_BUG_FIX_VERSION << 8) |
                (TPS_DRV_BUILD);
       g_pIM0_Data->IM_Revision_Counter = 0x00;
       g_pIM0_Data->IM_Profile_ID = 0x00;
       g_pIM0_Data->IM_Profile_Specific_Type = 0x00;
       g_pIM0_Data->IM_Version = 0x0101;
    }

    g_pIM1_Data = (T_IM1_DATA*)malloc(sizeof(T_IM1_DATA));
    if(g_pIM1_Data == 0)
    {
        /* error no memory */
    }
    else
    {
        /*
        read the  I&M1 parameter from non volatile memory and fill it here in the
        globaly defined variables
        i.e. read_flash_setting(IM1_DATA, (USIGN8*)IM1_Data, sizeof(T_IM1_DATA));
        */
        /*here only dummy data are written jet. should be replaced*/
        memset(g_pIM1_Data, 0x31, sizeof(T_IM1_DATA));
    }

    g_pIM2_Data = (T_IM2_DATA*)malloc(sizeof(T_IM2_DATA));
    if(g_pIM2_Data == 0)
    {
        /* error no memory */
    }
    else
    {
        /*
        read the  I&M2 parameter from non volatile memory and fill it here in the
        globaly defined variables
        i.e. read_flash_setting(IM2_DATA, (USIGN8*)IM2_Data, sizeof(T_IM2_DATA));
        */
        /*here only dummy data are written jet. should be replaced*/
         memset(g_pIM2_Data, 0x32, sizeof(T_IM2_DATA));
    }

    g_pIM3_Data = (T_IM3_DATA*)malloc(sizeof(T_IM3_DATA));
    if(g_pIM3_Data == 0)
    {
        /* error no memory */
    }
    else
    {
        /*
        read the  I&M3 parameter from non volatile memory and fill it here in the
        globaly defined variables
        i.e. read_flash_setting(IM3_DATA, (USIGN8*)IM3_Data, sizeof(T_IM3_DATA));
        */
        /*here only dummy data are written jet. should be replaced*/
        memset(g_pIM3_Data, 0x33, sizeof(T_IM3_DATA));
    }

    g_pIM4_Data = (T_IM4_DATA*)malloc(sizeof(T_IM4_DATA));
    if(g_pIM4_Data == 0)
    {
        /* error no memory */
    }
    else
    {
        /*
        read the  I&M4 parameter from non volatile memory and fill it here in the
        globaly defined variables
        i.e. read_flash_setting(IM4_DATA, (USIGN8*)IM4_Data, sizeof(T_IM4_DATA));
        */
        /*here only dummy data are written jet. should be replaced*/
        memset(g_pIM4_Data, 0x34, sizeof(T_IM4_DATA));
    }
}

/******************************************************************************
**
**  FUNCTION NAME:   printHexData
**
**  DESCRIPTION:     Helper function to print an array to the stdout.
**
**  PARAMETER:       pbyData      The array with the data.
**                   dwDataLength The length of the array.
**
**
*******************************************************************************
*/
VOID printHexData(USIGN8* pbyData, USIGN32 dwDataLength)
{
    USIGN32 i;

    if((pbyData == NULL) || (dwDataLength == 0))
    {
        printf(" NULL\n");
        return;
    }

    for(i=0; i < dwDataLength; i++)
    {
        printf(" 0x%2.2X", pbyData[i]);
    }
    printf("\n");
}
