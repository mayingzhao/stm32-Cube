/*
+-----------------------------------------------------------------------------+
| ****************************** TPS_1_API.h *******************************  |
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

/*! \file TPS_1_API.h
 *  \brief header defintion of the driver implementation TPS_1_API.c
 */

#ifndef _API_NEW_H_
#define _API_NEW_H_

/*---------------------------------------------------------------------------*/
/* Application Includes                                                      */
/*---------------------------------------------------------------------------*/
#include <TPS_1_user.h>
#include <SPI1_Master.h>
#include <stdio.h>

/*---------------------------------------------------------------------------*/
/* library functions.                                                        */
/*---------------------------------------------------------------------------*/

/* Caution: do not define USE_INT_APP! Only for internal use!                */
/*---------------------------------------------------------------------------*/
#ifdef USE_INT_APP
  #define __packed __attribute__ ((packed,aligned(1)))
#endif

#define PRIVATE_MALLOC

#define t_malloc(x) lib_malloc(x)

/* Align pointer p to Double-Word boundaries. */
#define DWORD_ALIGN(p)   p += (4 - ((USIGN32)p % 4))

#ifdef LITTLE_ENDIAN_FORMAT
    #define TPS_htonl(l) (((l & 0x000000ff) << 24) | \
                          ((l & 0x0000ff00) << 8)  | \
                          ((l & 0x00ff0000) >> 8)  | \
                          ((l & 0xff000000) >> 24) )
    #define TPS_ntohl(l) TPS_htonl(l)
    #define TPS_htons(s) ((USIGN16)(((USIGN16)(s) >> 8) | ((USIGN16)(s) << 8)))
    #define TPS_ntohs(s) TPS_htons(s)
#else
    #define TPS_htonl(l) (l)
    #define TPS_ntohl(l) (l)
    #define TPS_htons(s) (s)
    #define TPS_ntohs(s) (s)
#endif

#ifndef NULL
    #define NULL 0
#endif

#ifdef USE_INT_APP

#define DIAGNOSIS_ENABLE
#define USE_ETHERNET_INTERFACE
#define USE_AUTOCONF_MODULE
#endif

#define CHANPROP_TYPE_MAX               0x07
#define CHANPROP_ACCUMULATIVE_MAX       0x01
#define CHANPROP_MAINTENANCE_MAX        0x03
#define CHANPROP_SPECIFIER_MAX          0x03
#define CHANPROP_DIRECTION_MAX          0x03

/*! \defgroup  macroDefs Macro definitions
*@{
*/
#define CHANPROP_TYPE(x)                (x&0x00ff)
#define CHANPROP_ACC(x)                 ((x&0x0100)>>8)
#define CHANPROP_MAIN(x)                ((x&0x0600)>>9)
#define CHANPROP_SPECIFIER(x)           ((x&0x1800)>>11)
#define CHANPROP_DIRECTION(x)           ((x&0xe000)>>13)
#define SET_CHANPROP_TYPE(x,y)          (x=(x&0xfff0) | y)
#define SET_CHANPROP_ACCU(x,y)          (x=(x&0xfeff) | (y<<8))
#define SET_CHANPROP_MAIN(x,y)          (x=(x&0xf9ff) | (y<<9))
#define SET_CHANPROP_SPECIFIER(x,y)     (x=(x&0xe7ff) | (y<<11))
#define SET_CHANPROP_DIRECTION(x,y)     (x=(x&0x1fff) | (y<<13))

#define GET_APDU_STATUS_STATE(apdu)             ((apdu & 1) == 1)     /*!< returns 1 if the state bit is set in the APDU status */
#define GET_APDU_STATUS_REDUNDANCY(apdu)        ((apdu & 2) == 2)     /*!< returns 1 if the redundancy bit is set in the APDU status */
#define GET_APDU_STATUS_DATAVALID(apdu)         ((apdu & 4) == 4)     /*!< returns 1 if the DataValid bit is set in the APDU status */
#define GET_APDU_STATUS_PROVIDERSTATE(apdu)     ((apdu & 16) == 16)   /*!< returns 1 if the ProdiderState bit is set in the APDU status */
#define GET_APDU_STATUS_PROBLEMINDICATOR(apdu)  ((apdu & 32) == 32)   /*!< returns 1 if the ProblemIndicator bit is set in the APDU status */
#define GET_APDU_STATUS_IGNORE(apdu)            ((apdu & 128) == 128) /*!< returns 1 if the Ignore/Evaluate bit is set in the APDU status */

/*!@}*/


/* Use UNREFERENCED_PARAMETER() if a parameter will never be referenced.     */
#define UNREFERENCED_PARAMETER(p)       (p = p)

/* The software version of the driver                                        */
/*---------------------------------------------------------------------------*/
#define TPS_DRV_MAJOR_RELEASE 16
#define TPS_DRV_BUG_FIX_VERSION 2
#define TPS_DRV_BUILD 3

/* The software version of the coresponding firmware is build with this two  */
/* defines                                                                   */
/*---------------------------------------------------------------------------*/
#define STACK_START_NUMBER   0x04000000
/* The version number of the corresponding TPS Firmware.                      */
/* If this number is different between Stack and Firmware the parts are not  */
/* compatible!                                                               */
/* 0x08 is Firmware 1.0.x                                                    */
/* 0x09 if Firmware 1.1.x and above                                          */
/*---------------------------------------------------------------------------*/
#define STACK_VERSION_NUMBER 0x00000009

/* Maximum values for the device                                              */
/*----------------------------------------------------------------------------*/
#define MAX_NUMBER_IOAR     2   /* max 2 io-ars are supported now             */
#define MAX_NUMBER_OF_API   2
#define MAX_NUMBER_SLOT     64
#define MAX_NUMBER_SUBSLOT  64
#define MAX_NUMBER_RECORDS  4  /* 2x IOAR, Device Access, Implicit Read. */
#define MAX_NUMBER_DIAG     32 /* TPS-1 firmware supports a maximum of 32
                                  diagnosis entries */


/* Definitions for the Device Access Point.                                  */
/*---------------------------------------------------------------------------*/
#define DAP_MODULE              0x00   /* do not change */
#define DAP_SUBMODULE           0x01   /* do not change */
#define DAP_INTSUBMODULE        0x8000 /* do not change */
#define DAP_INTSUBMODULE_PORT_1 0x8001 /* do not change */
#define DAP_INTSUBMODULE_PORT_2 0x8002 /* do not change */

/* Use this value as Module- or SubmoduleID to enable the autoconfiguration  */
/* within a connect.                                                         */
/*---------------------------------------------------------------------------*/
#define AUTOCONF_MODULE         0x00

/* Definitions for the Device Access Point.                                  */
/*---------------------------------------------------------------------------*/
#define IM0_DATA_NOT_AVAIL      0
#define IMO_DATA_AVAIL          1

#define DCP_SET_ALLOWED      0x00
#define DCP_SET_PROTECTED    0x01

#ifndef USE_INT_APP
/*---------------------------------------------------------------------------*/
/*  Base Address Definitions                                                 */
/*---------------------------------------------------------------------------*/
    #define BASE_ADDRESS_DPRAM       0x00000000 /* Start Address Dual Ported RAM */
    #define BASE_ADDRESS_NRT_AREA    0x00008000 /* Start Address NRT Area        */
    #define BASE_NRT_AREA_SIZE       0x00008000 /* Size dual ported ram (const)  */
/*---------------------------------------------------------------------------*/
#else
/*---------------------------------------------------------------------------*/
    #define BASE_ADDRESS_DPRAM       0x10200000 /* Start Address Dual Ported RAM */
    #define BASE_ADDRESS_NRT_AREA    0x04000000 /* Start Address NRT Area        */
    #define BASE_NRT_AREA_SIZE       0x00008000 /* Size dual ported ram (const)  */
#endif

/* Registers for events: ext.application -< stack                            */
/*---------------------------------------------------------------------------*/
#define EVENT_REGISTER_APP         (BASE_ADDRESS_DPRAM+0x1C)

/* Registers for events: stack -> ext.application                            */
/*---------------------------------------------------------------------------*/
#define EVENT_REGISTER_TPS         (BASE_ADDRESS_DPRAM+0x40)

/* Registers for acknowledge of events: ext.application -> stack              */
/*---------------------------------------------------------------------------*/
#define EVENT_REGISTER_APP_ACKN	   (BASE_ADDRESS_DPRAM+0x24)
#define PN_EVENT_LOW               (BASE_ADDRESS_DPRAM+0x3C)
#define PN_EVENT_HIGH              (BASE_ADDRESS_DPRAM+0x40)

/* Registers of the interupt event mode.                                        */
/*---------------------------------------------------------------------------*/
#define HOST_IRQ_LOW               (BASE_ADDRESS_DPRAM+0x08)
#define HOST_IRQ_HIGH              (BASE_ADDRESS_DPRAM+0x0C)
#define HOST_IRQ_MASK_LOW          (BASE_ADDRESS_DPRAM+0x10)
#define HOST_EVENT_HIGH            (BASE_ADDRESS_DPRAM+0x1C)
#define HOST_IRQ_MASK_HIGH         (BASE_ADDRESS_DPRAM+0x14)
#define HOST_IRQ_ACK_LOW           (BASE_ADDRESS_DPRAM+0x20)
/* Same address as EVENT_REGISTER_APP_ACKN, Acknowledge is the same for Polling and IRQ mode. */
#define HOST_IRQ_ACK_HIGH          (BASE_ADDRESS_DPRAM+0x24)
#define HOST_EOI                   (BASE_ADDRESS_DPRAM+0x28)
#define EVENT_PN_IRO_MASK_HIGH     (BASE_ADDRESS_DPRAM+0x38)
#define PN_EVENT_LOW               (BASE_ADDRESS_DPRAM+0x3C)
#define PN_EVENT_HIGH              (BASE_ADDRESS_DPRAM+0x40)

#define PN_EOI                     (BASE_ADDRESS_DPRAM+0x4C)

/*---------------------------------------------------------------------------*/
/*  API Version > 1.2                                                        */
/*---------------------------------------------------------------------------*/
#define API_VERSION 0x16

/*---------------------------------------------------------------------------*/
/*  SUBSLOT configuration  ( input / output )                                */
/*---------------------------------------------------------------------------*/
#define SUBSLOT_NOT_USED      0x0000
#define INPUT_USED            0x0001
#define OUTPUT_USED           0x0002
#define INPUT_OUTPUT_USED     0x0003

/*---------------------------------------------------------------------------*/
/* Macro and defines to mask the content of subslot->pt_used_in_cr           */
/* Possible parameters: AR_0, AR_1 or AR_IOSR.                               */
/*---------------------------------------------------------------------------*/
#define SUBSLOT_FOR_AR(x)     (0x0100<<x)
#define USED_IN_IOCR          0x0300
#define USED_IN_ANY_AR        0x0F00

/*---------------------------------------------------------------------------*/
/* Macro to mask the content of subslot->pt_wOwnedByAr                       */
/* Possible parameters: AR_0, AR_1 or AR_IOSR.                               */
/*---------------------------------------------------------------------------*/
#define OWNED_BY_AR(x)        (0x0001<<x)

/*---------------------------------------------------------------------------*/
/*  SUBSLOT properties  ( ok -> removed -> removed ok -> returned -> ok)     */
/*---------------------------------------------------------------------------*/
#define SUBMODULE_PROPERTIES_MASK  0x0F00

#define MODULE_NOT_PULLED          0x0000
#define MODULE_PULLED              0x0100

#define SUBMODULE_NOT_PULLED       0x0000
#define SUBMODULE_PULLED           0x0100
#define SUBMODULE_REPLUGGED        0x0300

#define IOXS_BAD_BY_SUBSLOT        0x00
#define IOXS_BAD_BY_SLOT           0x20
#define IOXS_BAD_BY_DEVICE         0x40
#define IOXS_BAD_BY_CONTROLLER     0x60
#define IOXS_GOOD                  0x80


/*****************************************************************************
****  Data Size Definitions                                               ****
*****************************************************************************/
#define MAX_ARS_SUPPORTED          3
#define TYPE_OF_STATION_STRING_LEN 25
#define STATION_NAME_LEN           240 /* Length of name of station without terminating 0x00. */
#define TYPE_OF_STATION_PADDING    92
#define SIZE_OF_REQ_HEADER         206

#define NRT_EVENT_UNIT_SIZE        0x0C
#define VENDOR_ID_SIZE             0x02
#define DEVICE_ID_SIZE             0x02
#define INTERFACE_ID_SIZE          0x02
#define MAC_ADDRESS_SIZE           0x06
#define IP_ADDRESS_SIZE            0x04
#define INIT_PARAMETER_HEADER_SIZE 0x06
#define MAX_LEN_ETHERNET_FRAME     1522
#define MIN_LEN_ETHERNET_FRAME     60
#define MAX_RECORD_DATA_LENGTH     (MAX_LEN_ETHERNET_FRAME - ETHERNET_HEADER_SIZE - sizeof(UDP_IP_HEADER) - sizeof(RPC_HEADER) - sizeof(BLOCK_HEADER) - sizeof(RW_RECORD_REQ_BLOCK))

/* I&M0 specific lengths                                                    */
/*--------------------------------------------------------------------------*/

#define IM0_ORDERID_LEN             0x14
#define IM0_SERIALNUMBER_LEN        0x10

#define UUID_PADDING                0x18
#define IM_SIGNATURE_LEN            0x36
#define IM_DESCRIPTOR_LEN           0x36
#define IM_DATE_LEN                 0x10
#define IM_TAG_FUNCTION_LEN         0x20
#define IM_TAG_LOCATION_LEN         0x16
#define SIZE_FSU_PARAMETER          0x14

/* Constants for IP and Ethernet Frames                                      */
/*---------------------------------------------------------------------------*/
#define ETHERNET_HEADER_SIZE         14
#define ETHERNET_CRC_SIZE             4
#define IP_HEADER_SIZE               20
#define UDP_HEADER_SIZE               8
#define UDP_PROTOCOL               0x11

/* Record Mailboxes (definitions)                                            */
/*---------------------------------------------------------------------------*/
#define SIZE_RECORD_MB0      1024                           /* IO-AR 1       */
#define SIZE_RECORD_MB1      1024                           /* IO-AR 2       */
#define SIZE_RECORD_MB2      1024                           /* Supervisor-AR */
#define SUPERVISOR_MB_NUM    2
#define SIZE_RECORD_MB3      1024                           /* Implicite-AR  */
#define IMPLICITE_MB_NUM     3

#define WRITERECORD_REQ      0x0008
#define WRITERECORD_RES      0x8008
#define READRECORD_REQ       0x0009
#define READRECORD_RES       0x8009

#define RECORD_FLAG_READ     0x01
#define RECORD_FLAG_WRITE    0x02
#define RECORD_FLAG_CANCEL   0x04
#define RECORD_FLAG_DONE     0x08

/* Record indexes                                                           */
/*--------------------------------------------------------------------------*/
#define RECORD_INDEX_RECORD_INPUT_DATA_OBJECT  0x8028
#define RECORD_INDEX_RECORD_OUTPUT_DATA_OBJECT 0x8029
#define RECORD_INDEX_IM0                       0xAFF0
#define RECORD_INDEX_IM1                       0xAFF1
#define RECORD_INDEX_IM2                       0xAFF2
#define RECORD_INDEX_IM3                       0xAFF3
#define RECORD_INDEX_IM4                       0xAFF4
#define RECORD_INDEX_IM_FILTERDATA             0xF840
#define RECORD_INDEX_TPS_VERSION               0x7FF0
#define RECORD_INDEX_ARFSUDATAADJUST           0xE050
#define RECORD_INDEX_USER_DATA_IN_TPS_FLASH    0x2012
#define RECORD_INDEX_TPS1_WATCHDOG_TEST        0x2013
#define RECORD_INDEX_TPS1_LED_TEST             0x2014

/* Managing Application Relations                                            */
/*---------------------------------------------------------------------------*/
#define AR_0                 0      /* Application Relation 0                */
#define AR_1                 1      /* Application Relation 1                */
#define AR_IOSR              2      /* Application Relation Supervisor       */

#define AR_ESTABLISH         1      /* AR is up and in operation.            */
#define AR_NOT_IN_OPERATION  0      /* AR is not in operation.               */

/* API                                                                       */
/*---------------------------------------------------------------------------*/
#define API_0                0x0000
#define API_PROFIDRIVE       0x3A00

/* Alarm Mailboxes (definitions)                                             */
/*---------------------------------------------------------------------------*/
#define SIZE_ALARM_MB        1024   /* Size of alarm mailbox                 */
#define NR_ALARM_MAILBOXES   4      /* Count of alarm mailboxes HIGH&LOW     */
#define ALARM_LOW            0
#define ALARM_HIGH           1

/* ONLY USE IN ALARM MAILBOX FLAGS                                           */
/*---------------------------------------------------------------------------*/
#define UNUSED_FLAG      0x00
#define ACTIVE_FLAG      0x01
#define BUSY_FLAG        0x02
#define ACK_FLAG         0x04

#define NACK_FLAG        0x05
#define ERR_FLAG         0x06
#define RTA_TIMEOUT      0x07

#define NAACK_FLAG       0x05
#define ERR_AACK         0x08
#define RTA_WAAK_TIMEOUT 0x09

/* ALARM ChannelProperties.Specifier and diagnosis alarm specifier          */
/*--------------------------------------------------------------------------*/
#define ALL_DISAPPEAR                   0x00
#define APPEARS                         0x01
#define DISAPPEARS                      0x02
#define DISAPPEARS_BUT_OTHER_REMAIN     0x03

/* ChannelProperties.Maintenance                                            */
/*--------------------------------------------------------------------------*/
#define MAINTENANCE_DIAGNOSIS           0x00
#define MAINTENANCE_REQUIRED            0x01
#define MAINTENANCE_DEMANDED            0x02
#define MAINTENANCE_QUALIFIED_DIAGNOSIS 0x03

/* Alarmtypes                                                               */
/*--------------------------------------------------------------------------*/
#define DIAGNOSIS_ALARM                 0x0001
#define DIAGNOSIS_DISAPPEARS_ALARM      0x000C
#define PROCESS_ALARM                   0x0002
#define PULL_ALARM                      0x0003
#define PLUG_ALARM                      0x0004
#define UPDATE_ALARM                    0x0006
#define PLUGWRONG_ALARM                 0x000A
#define RETURN_OF_SUBMODULE_ALARM       0x000B

/* TPS_DiagSetChannelProperties                                             */
/*--------------------------------------------------------------------------*/
#define DIAG_DIRECTION_SPECIFIC         0x00
#define DIAG_DIRECTION_INPUT            0x01
#define DIAG_DIRECTION_OUTPUT           0x02
#define DIAG_DIRECTION_INOUT            0x03

/*****************************************************************************
****  USER STRUCTURE IDENTIFIER                                           ****
*****************************************************************************/
#define USI_CHANNEL_DIAGNOSIS           0x8000
#define USI_EXT_CHANNEL_DIAGNOSIS       0x8002
#define USI_QUALIFIED_DIAGNOSIS         0x8003

/*****************************************************************************
****  CHANNELERRORTYPE DEFINITIONS                                        ****
*****************************************************************************/
#define CET_SHORT_CIRCUIT                 0x0001
#define CET_UNDERVOLTAGE                  0x0002
#define CET_OVERVOLTAGE                   0x0003
#define CET_OVERLOAD                      0x0004
#define CET_OVERTEMPERATURE               0x0005
#define CET_PARAMETER_MISSING             0x000F
#define CET_PARAMETERIZATION_FAULT        0x0010

#define ECET_NONE                         0x0000

#define DIAG_QUALIFIER_RESERVED_BITMASK        7

/*---------------------------------------------------------------------------*/
/* ONLY USE IN DPR DIAGNOSIS MAILBOX FLAGS                                   */
/*---------------------------------------------------------------------------*/
#define EMPTY_ENTRY         0x00
#define APPEAR_FLAG         0x01
#define APPEAR_ACK_FLAG     0x02
#define DISAPPEAR_FLAG      0x04
#define DISAPPEAR_ACK_FLAG  0x08
#define CHANGED_FLAG        0x10

/* Definitions of the callback functions.                                    */
/*---------------------------------------------------------------------------*/
#define ONCONNECTDONE_CB        0
#define ONABORT_CB              1
#define ONREADRECORD_CB         2
#define ONWRITERECORD_CB        3
#define ON_PRM_END_DONE_CB      4
#define ONWRITE_DONE_CB         5
#define ONREAD_CB               6
#define ONCANCELREQ_CB          7
#define ONCONNECT_CB            8
#define ONDCP_SET_NAME_CB       9
#define ONDCP_SET_IP_ADDR_CB    10
#define ONDCP_BLINK_CB          11
#define ONDCP_FACT_RESET_CB     12
#define ONALARM_CB              13
#define ONDIAG_CB               14
#define ONRECEIVE_FRAME_CB      15
#define ONRESET_CB              16
#define ONFSUPARAM_CB           17

/* I&M support flags                                                         */
/*---------------------------------------------------------------------------*/
#define IM_NOT_SUPPORTED                    0x0000
#define IM_PROFILE_SPECIFIC                 0x0001
#define IM1_SUPPORTED                       0x0002
#define IM2_SUPPORTED                       0x0004
#define IM3_SUPPORTED                       0x0008
#define IM4_SUPPORTED                       0x0010
#define IM5_SUPPORTED                       0x0020
#define IM6_SUPPORTED                       0x0040
#define IM7_SUPPORTED                       0x0080
#define IM8_SUPPORTED                       0x0100
#define IM9_SUPPORTED                       0x0200
#define IM10_SUPPORTED                      0x0400
#define IM11_SUPPORTED                      0x0800
#define IM12_SUPPORTED                      0x1000
#define IM13_SUPPORTED                      0x2000
#define IM14_SUPPORTED                      0x4000
#define IM15_SUPPORTED                      0x8000

#define RESPONSE_PENDING                    0xA00
#define FACTORYRESET_PENDING                0x500

/* Ports for the function TPS_SendEthernetFrame()                            */
/*---------------------------------------------------------------------------*/
#define PORT_ANY          0x00
#define PORT_NR_1         0x01
#define PORT_NR_2         0x02
#define PORT_NR_1_2       0x03

/* "Virtual" Port for communication with the TPS-1 Firmware.                 */
/*---------------------------------------------------------------------------*/
#define PORT_NR_INTERNAL  0x04

/* States for Modules                                                        */
/*---------------------------------------------------------------------------*/

#define NO_MODULE         0x0000  /* E.g. module not plugged */
#define WRONG_MODULE      0x0001  /* E.g. ModuleIdentNumber wrong / This coding should be avoided if possible see PN-Spec. Tab 457 */
#define PROPER_MODULE     0x0002  /*  */
#define SUBSTITUTE_MODULE 0x0003  /* Module is not the same as requested but the IO device was able to adapt by its own knowledge */

/* States for Submodules for displaying in the ModuleDiff-Block by using of  */
/* compatible or autoconfigurated Submodules					             */
/*---------------------------------------------------------------------------*/
#define SUBMODULE_OK         0x0000
#define SUBSTITUTE_SUBMODULE 0x0001
#define WRONG_SUBMODULE      0x0002
#define NO_SUBMODULE         0x0003

/* Flags for operation-mode or substitute mode. */
#define SUBSTITUTE_ACTIVE_FLAG_OPERATION  0x0000
#define SUBSTITUTE_ACTIVE_FLAG_SUBSTITUTE 0x0001

#define DCP_R2F_OPT_ALL       8

/* DCP Options defines*/
#define DCP_OPTION_IP         0x1
#define DCP_OPTION_DEV_PROP   0x2
#define DCP_OPTION_CONTROL    0x5

/* DCP SubOptions defines*/
#define DCP_SUBOPTION_NAME_OF_STATION  0x2
#define DCP_SUBOPTION_END_TRANSACT     0x2
#define DCP_SUBOPTION_IP_PARAM         0x2
#define DCP_SUBOPTION_RESET_2_FACTORY  0x6

/* DCP Service defines */
#define DCP_SERVICE_ID_SET     0x4
#define DCP_SERVICE_ID_REQUEST 0x0



#define DCP_FRAME_HEADER {0x00,0xa0,0x45,0x4c,0x7e,0x10,0x00,0xa0,0x45,0x4c,0x7e,0x10,0x88,0x92,0xfe,0xfd}


typedef enum
{
    DCP_SET_PERMANENT = 0, /* The IP Address is set permanently. */
    DCP_SET_TEMPORARY = 1  /* The IP Address is set temporary.*/
} T_DCP_SET_MODE;

typedef enum
{
    RECORD_INPUT_DATA_OBJECT,
    RECORD_OUTPUT_DATA_OBJECT
} T_RECORD_DATA_OBJECT_TYPE;

enum _selector_bit_numbers
{
	PROTO_SELECTOR_BIT_ARP = 0,
	PROTO_SELECTOR_BIT_PTCP,
	PROTO_SELECTOR_BIT_PNALARM,
	PROTO_SELECTOR_BIT_DCP,
	PROTO_SELECTOR_BIT_RTIRT,
	PROTO_SELECTOR_BIT_SNMP,
	PROTO_SELECTOR_BIT_RPC,
	PROTO_SELECTOR_BIT_UDP,
	PROTO_SELECTOR_BIT_ICMP,
	PROTO_SELECTOR_BIT_LLDP,
	PROTO_SELECTOR_BIT_IPALL,
	FKT_SELECTOR_BIT_NOFWUP
};

#define SEL_PROTO_ARP     (0x00010001 << PROTO_SELECTOR_BIT_ARP)
#define SEL_PROTO_PTCP    (0x00010001 << PROTO_SELECTOR_BIT_PTCP)
#define SEL_PROTO_PNALARM (0x00010001 << PROTO_SELECTOR_BIT_PNALARM)
#define SEL_PROTO_DCP     (0x00010001 << PROTO_SELECTOR_BIT_DCP)
#define SEL_PROTO_RTIRT   (0x00010001 << PROTO_SELECTOR_BIT_RTIRT)
#define SEL_PROTO_SNMP    (0x00010001 << PROTO_SELECTOR_BIT_SNMP)
#define SEL_PROTO_RPC     (0x00010001 << PROTO_SELECTOR_BIT_RPC)
#define SEL_PROTO_UDP     (0x00010001 << PROTO_SELECTOR_BIT_UDP)
#define SEL_PROTO_ICMP    (0x00010001 << PROTO_SELECTOR_BIT_ICMP)
#define SEL_PROTO_LLDP    (0x00010001 << PROTO_SELECTOR_BIT_LLDP)
#define SEL_PROTO_IPALL   (0x00010001 << PROTO_SELECTOR_BIT_IPALL)
#define SEL_DISABLE_FWUP  (0x00010001 << FKT_SELECTOR_BIT_NOFWUP)

/*---------------------------------------------------------------------------*/
/* API Events: Do not change the order of the enum elements. The order has   */
/*             to correspond with the Firmware order (Stack).                */
/*---------------------------------------------------------------------------*/
/*! \brief  Events from the host application to the TPS-1 firmware */
enum AppEvents
{
     APP_EVENT_CONFIG_FINISHED=0,       /*!< \brief Device configuration (slots/ subslots) is finished. The TPS-1 can start Profinet communication. */
     APP_EVENT_APP_RDY_0=1,             /*!< \brief The application has processed all parameters for AR_0. The TPS-1 can send the Application Ready Request to the connected PLC. */
     APP_EVENT_APP_RDY_1=2,             /*!< \brief The application has processed all parameters for AR_1. The TPS-1 can send the Application Ready Request to the connected PLC. */
     APP_EVENT_APP_RDY_2=3,             /*!< \brief Device Access AR was established */
     APP_EVENT_RECORD_DONE=4,           /*!< \brief The application has finished processing of a record request. The TPS-1 can send the response. */
     APP_EVENT_DIAG_CHANGED=5,          /*!< \brief The application has added, edited or removed a diagnosis. */
     APP_EVENT_ONCONNECT_OK_0=6,        /*!< \brief The application has finished the OnConnect Event for AR_0 */
     APP_EVENT_ONCONNECT_OK_1=7,        /*!< \brief The application has finished the OnConnect Event for AR_1 */
     APP_EVENT_ONCONNECT_OK_2=8,        /*!< \brief The application has finished the OnConnect Event for IOSR ( device access ) */
     APP_EVENT_ABORT_AR_0=9,            /*!< \brief The application has finished the OnAbort Event for AR_0 */
     APP_EVENT_ABORT_AR_1=10,           /*!< \brief The application has finished the OnAbort Event for AR_1 */
     APP_EVENT_ABORT_AR_2=11,           /*!< \brief The application has finished the OnAbort Event for IOSR ( device access ) */
     APP_EVENT_PULL_SUBMODULE=12,       /*!< \brief <b>RESERVED</b> not used in this version */
     APP_EVENT_RETURN_SUBMODULE=13,     /*!< \brief <b>RESERVED</b> not used in this version */
     APP_EVENT_ALARM_SEND_REQ_AR0=14,   /*!< \brief The application filled the alarm mailbox for AR_0 with data. The TPS-1 can send these data. */
     APP_EVENT_ALARM_SEND_REQ_AR1=15,   /*!< \brief The application filled the alarm mailbox for AR_1 with data. The TPS-1 can send these data. */
     APP_EVENT_ALARM_SEND_REQ_AR2=16,   /*!< \brief The application filled the alarm mailbox for device access with data. Not used, because Device Access ARs don't support alarms*/
     APP_EVENT_RESET_STACK_CONFIG=17,   /*!< \brief The TPS-1 firmware shall clear the device configuration ( slot / subslot structures) . */
     APP_EVENT_ETH_FRAME_SEND=18,       /*!< \brief TheApplication filled the Ethernet mailbox with data. The TPS-1 can send these data over the Ethernet. */
     APP_EVENT_WRITE_INPUT_DATA=19,     /*!< \brief <b>RESERVED</b> not used in this version */
     APP_EVENT_READ_OUTPUT_DATA=20,     /*!< \brief <b>RESERVED</b> not used in this version */
     APP_EVENT_RESET_PN_STACK=21,       /*!< \brief Initiate a soft reset of the TPS-1 */
     APP_EVENT_APP_MESSAGE=22           /*!< \brief The application filled the Ethernet mailbox with a record request for the TPS Communication Interface. The TPS-1 will read the request and send a response to the application. */
};

/*---------------------------------------------------------------------------*/
/* TSP Events: Do not change the order of the enum elements. The order has   */
/*             to correspond with the Firmware order (Stack).                */
/*---------------------------------------------------------------------------*/
/*! \brief  Events from the TPS-1 firmware to the host application*/
enum StackEvents
{
	TPS_EVENT_ONCONNECTDONE_IOAR0=0,       /*!< \brief An application relation (AR_0) to a PLC has been established. */
	TPS_EVENT_ONCONNECTDONE_IOAR1=1,       /*!< \brief An application relation (AR_1) has been established. */
	TPS_EVENT_ONCONNECTDONE_IOSAR=2,       /*!< \brief A device access AR (IOSR) has been established. */
	TPS_EVENT_ON_PRM_END_DONE_IOAR0=3,     /*!< \brief A Parameterization End Request from a PLC (AR_0) was received, which signals that all parameters were sent. */
	TPS_EVENT_ON_PRM_END_DONE_IOAR1=4,     /*!< \brief A Parameterization End Request from a PLC (AR_1) was received, which signals that all parameters were sent. */
	TPS_EVENT_ON_PRM_END_DONE_IOSAR=5,     /*!< \brief Prm End Req for IOSAR received, should never happen. */
	TPS_EVENT_ONABORT_IOAR0=6,             /*!< \brief An application relation (AR_0) was disconnected. */
	TPS_EVENT_ONABORT_IOAR1=7,             /*!< \brief An application relation (AR_1) was disconnected. */
	TPS_EVENT_ONABORT_IOSAR=8,             /*!< \brief An application relation (IOSR) was disconnected. */
	TPS_EVENT_ONREADRECORD=9,              /*!< \brief A record read request for has been received. This event only is set for indexes that are not replied by the firmware. */
	TPS_EVENT_ONWRITERECORD=10,            /*!< \brief A record write request for has been received. This event only is set for indexes that are not replied by the firmware. */
	TPS_EVENT_ONALARM_ACK_0=11,            /*!< \brief An alarm acknowledge notification (low priority) was received. */
	TPS_EVENT_ONDIAG_ACK=12,               /*!< \brief <b>RESERVED</b> not used in this version */
	TPS_EVENT_ONCONNECT_REQ_REC_0=13,      /*!< \brief A Connect Request for the first AR (AR_0) was received. */
	TPS_EVENT_ONCONNECT_REQ_REC_1=14,      /*!< \brief A Connect Request for the second AR (AR_1) was received. */
	TPS_EVENT_ONCONNECT_REQ_REC_2=15,      /*!< \brief A Connect Request for a device access  AR (IOSR) was received. */
	TPS_EVENT_ON_SET_DEVNAME_TEMP=16,      /*!< \brief A dcp.set request to change the name of station was received. */
	TPS_EVENT_ON_SET_IP_PERM=17,           /*!< \brief A dcp.set request to change the IP configuration permanently was received. */
	TPS_EVENT_ON_SET_IP_TEMP=18,           /*!< \brief A dcp.set request to change the IP configuration temporarily was received. */
	TPS_EVENT_ONDCP_BLINK_START=19,        /*!< \brief A dcp.signal request was received */
	TPS_EVENT_ONDCP_RESET_TO_FACTORY=20,   /*!< \brief A dcp.reset to factory settings request was received */
	TPS_EVENT_ONALARM_ACK_1=21,            /*!< \brief An alarm acknowledge notification (high priority) was received. */
	TPS_EVENT_RESET=22,                    /*!< \brief The TPS-1 will restart into firmware update mode after 1 second or when this event is acknowledged. */
	TPS_EVENT_ETH_FRAME_REC=23,            /*!< \brief An Ethernet packet for the application was received and is available in the Ethernet mailbox. */
	TPS_EVENT_TPS_MESSAGE=24,              /*!< \brief A TPS communication packet was received and is available in the Ethernet mailbox.*/
	TPS_EVENT_ON_LED_STATE_CHANGE = 27,    /*!< \brief New state of TPS LEDs was set .*/
        TPS_EVENT_ON_SET_DEVNAME_PERM = 28,    /*!< \brief A dcp.set request to change the name of station was received. */
        TPS_EVENT_ON_FSUDATA_CHANGE = 29       /*!< \brief FSU-Parameter was written or changed by TPS-1 firmware */
};

#ifdef FW_UPDATE_OVER_HOST
/*---------------------------------------------------------------------------*/
/* Events for updating of fw over host interface                             */
/*---------------------------------------------------------------------------*/
enum UpdaterEvents
{
    UPD_EVENT_FW_UPDATER_ACTIVE = 0,
    UPD_EVENT_FW_UPDATE_IN_PROGRESS,
    UPD_EVENT_FW_UPDATE_DONE
};

enum HostEvents
{
    APP_EVENT_START_FW_UPDATE = 0,
    APP_EVENT_START_UPDR_UPDATE,
    APP_EVENT_START_FW               /* Start TPS Stack                       */
};
#endif

/*----------------------------------------------------------------------------*/
/* Channel for receiving/sending of eth frames.                               */
/*----------------------------------------------------------------------------*/
PRE_PACKED
typedef struct __packed ethernet_mailbox
{
    USIGN32 dwMailboxState; /* Empty = ETH_MBX_EMPTY, Full = ETH_MBX_INUSE, Error = MBX_ERROR&ETH_MBX_INUSE*/
    USIGN32 dwLength;
    USIGN32 dwPortnumber;
    USIGN8  byFrame[MAX_LEN_ETHERNET_FRAME];
} T_ETHERNET_MAILBOX;
POST_PACKED

/* Allowed values for T_ETHERNET_MAILBOX.dwMailboxState                       */
/*----------------------------------------------------------------------------*/
#define ETH_MBX_EMPTY  0x00
#define ETH_MBX_INUSE  0x01
#define MBX_ERROR      0x10

enum api_states
{
    STATE_INITIAL=0,
    STATE_API_INIT_RDY,
    STATE_DEVICE_RDY,
    STATE_API_RDY,
    STATE_SLOT_RDY,
    STATE_SUBSLOT_RDY,
    STATE_TPS_CHANNEL_RDY,
    STATE_DEVICE_START_INITIATED,
    STATE_DEVICE_STARTED
};

/*****************************************************************************/
PRE_PACKED
typedef struct __packed BlockH
{
    USIGN16   type;
    USIGN16   length;
    USIGN16   version;
} BLOCK_HEADER;
POST_PACKED

PRE_PACKED
typedef struct __packed args_reqH
{
    USIGN32   max;
    USIGN32   length;
    USIGN32   arraymaxcount;
    USIGN32   arrayoffset;
    USIGN32   arrayactualcount;
} ARGS_REQ;
POST_PACKED

PRE_PACKED
typedef  struct __packed
{
    USIGN8      Dst_Address[6];
    USIGN8      Src_Address[6];
    USIGN16     Eth_Type;
} ETH_HEADER;
POST_PACKED

PRE_PACKED
typedef struct __packed _eth_header_ext
{
   USIGN8   bDestMacAddr[6];
   USIGN8   bSourceMacAddr[6];
   USIGN16  wEthType;
   USIGN16  wFrameID;

}ETH_HEADER_EXT;
POST_PACKED

PRE_PACKED
typedef struct __packed udp_ip
{
    /* IP HEADER */
    USIGN8      version_len;
    USIGN8      tos;
    USIGN16     length;
    USIGN16     id;
    USIGN16     offset;
    USIGN8      ttl;
    USIGN8      protocol;
    USIGN16     checksum;
    USIGN32     src;
    USIGN32     dst;

    /* UDP HEADER */
    USIGN16     src_port;
    USIGN16     dst_port;
    USIGN16     udp_length;
    USIGN16     udp_checksum;
} UDP_IP_HEADER;
POST_PACKED

PRE_PACKED
typedef struct __packed uuid_field
{
    USIGN32   time_low;
    USIGN16   time_mid;
    USIGN16   version_time_hi;
    USIGN8    variant_clock_seq;
    USIGN8    clock_seq;
    USIGN8    node[6];
} UUID_TAG;
POST_PACKED

PRE_PACKED
typedef struct __packed rpc_h
{
    USIGN8    version;
    USIGN8    type;
    USIGN8    flags;
    USIGN8    flags2;
    USIGN8    data_repr[3];
    USIGN8    serial_high;
    UUID_TAG  object_uuid;
    UUID_TAG  interface;
    UUID_TAG  activity;
    USIGN32   ser_boot_time;
    USIGN32   interface_version;
    USIGN32   sequence;
    USIGN16   opnum;
    USIGN16   in_hint;
    USIGN16   ac_hint;
    USIGN16   len;
    USIGN16   fragment_num;
    USIGN8    auth_proto;
    USIGN8    serial_low;
} RPC_HEADER;
POST_PACKED

/*****************************************************************************/
/* NRT AREA structs                                                          */
/*****************************************************************************/
PRE_PACKED
typedef struct __packed config_Header
{
    USIGN32     dwMagicNumber;
    USIGN32     dwNrtMemSize;
    USIGN16     wVendorID;
    USIGN16     wDeviceID;
    USIGN8      byStationName[STATION_NAME_LEN];
    USIGN16     wHWVersion;
    USIGN16     wDeviceVendorTypeLength;
    USIGN8      byDeviceVendorType[TYPE_OF_STATION_STRING_LEN];
    USIGN8      bySerialnumber[IM0_SERIALNUMBER_LEN];
    USIGN8      byOrderId[IM0_ORDERID_LEN];
    USIGN8      byPadding[TYPE_OF_STATION_PADDING];
    USIGN8      dwFastStarupParam[SIZE_FSU_PARAMETER];
    USIGN32     dwIOBufferLenAr[MAX_ARS_SUPPORTED];
    USIGN32     dwApduAddrForCr[MAX_ARS_SUPPORTED];
    USIGN32     dwHostProtoSelector;    /* bit(1: forward the frame to host/0:process the frame in stack): 0:ARP, 1:PTCP, 2:PN-Alarm,3:DCP, 4:RT/IRT, 5:SNMP, 6:RPC, 7:UDP, 8:ICMP ...*/
    USIGN16     wLEDState;              /* bit(1:on/0:Off): 0:BF, 1:SF, 2:MD, 3:MR, 4:LinkP1, 5:LinkP2, 6-31:Reserved  */
    USIGN16     wResetOption;
    USIGN32     dwTPSFWVersion;
    USIGN32     dwAPIVersion;
    USIGN32     dwReservedValue;
    USIGN8      byAppConfMode;
    USIGN8      byInterfaceMac[MAC_ADDRESS_SIZE];
    USIGN8      byPort1Mac[MAC_ADDRESS_SIZE];
    USIGN8      byPort2Mac[MAC_ADDRESS_SIZE];
    USIGN8      byRevisionPrefix;
    USIGN8      byRevisionFunctionalEnhancement;
    USIGN8      byBugFix;
    USIGN8      byInternalChange;
    USIGN32     dwIPAddress;
    USIGN32     dwSubnetMask;
    USIGN32     dwGateway;
} NRT_APP_CONFIG_HEAD;
POST_PACKED


#ifdef FW_UPDATE_OVER_HOST

/* defines for fw update functions                                          */
/*--------------------------------------------------------------------------*/
#define FW_FRAG_LEN         1024 /* DO NOT CHANGE! */
#define MAX_WAIT_CYCLES     0x000FFFFF
#define UPDATER_LIFE_SIGN   0xEEEEEEEE
#define HOST_LIFE_SIGN      0x5555555A

/* Host-Updater errors                                                      */
/*--------------------------------------------------------------------------*/
#define ERR_HOST_EVENT_UNKNOWN_OR_ZERO             0x1
#define ERR_BUFFER_OVERFLOW_FW_FILE_IS_TO_LONG     0x2
#define ERR_FW_UPD_TIMEOUT_UPDATE_NOT_COMPLETE     0x3
#define ERR_FRAG_LENGTH_IS_ZERO	                   0x4
#define ERR_UPDATER_APP_NOT_STARTED                0x5

/* Mailbox for transferring the fragmented firmware from host to d-tcm       */
/*---------------------------------------------------------------------------*/
typedef struct _dpr_fragment_mbx
{
    USIGN32 dwFlag; /*0 - empty, 1 - busy 2 - last*/
    USIGN32 dwFragLen;
    USIGN8  bData[FW_FRAG_LEN];
} FW_UPD_MAILBOX;

typedef struct _dpr_header
{
    USIGN32 dwHostSign;
    USIGN32 dwHostEvent;
    USIGN32 dwUpdaterEvent;
    USIGN32 dwErrorCode;
    FW_UPD_MAILBOX pbFwStartAddr;
} DPR_HOST_HEADER;

#endif

/*****************************************************************************/
/*                                                                           */
/*****************************************************************************/

typedef struct record_list
{
    USIGN16             wIndex;
    USIGN32             dwLength;
    USIGN8*             byData;
    struct zRecord_list *next;
} RECORD_LIST;

PRE_PACKED
typedef struct  __packed record_entry
{
    USIGN16             wIndex;
    USIGN32             dwLength;
    USIGN8*             byData;
} INIT_RECORD_ENTRY;
POST_PACKED

typedef struct mail_box_info
{
    USIGN32     dwAPINumber;
    USIGN16     wSlotNumber;
    USIGN16     wSubSlotNumber;
    USIGN16     wIndex;
    USIGN32     dwRecordDataLen;
} RECORD_BOX_INFO;

typedef struct _req_header
{
  USIGN8 request_header[SIZE_OF_REQ_HEADER];
} REQ_HEADER;


typedef struct record_mb
{
    USIGN32  dwSizeRecordMailbox;
    USIGN8*  pt_flags; /* bit 1: read_flag 2: write flag 3: cancel  4: done */
    USIGN8*  pt_errorcode1;
    USIGN8*  pt_req_header;
    USIGN8*  pt_data;
} RECORD_MB;

PRE_PACKED
typedef struct __packed alarm_mb
{
    USIGN32  size_alarm_mailbox;
    USIGN8*  pt_flags; /* USED/ACK_FLAG */
    USIGN32* pt_api;
    USIGN16* pt_slot_nr;
    USIGN16* pt_subslot_nr;
    USIGN16* pt_alarm_typ;
    USIGN16* pt_usi;
    USIGN16* pt_usr_hdl;
    USIGN16* pt_data_length;
    USIGN8*  pt_data;
} ALARM_MB;
POST_PACKED

PRE_PACKED
typedef struct __packed _api_record_entry
{
    USIGN16 index;
    USIGN32 length;
} API_RECORD_ENTRY;
POST_PACKED

PRE_PACKED
typedef struct __packed slot
{
    USIGN16*            pt_slot_number;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the slot number */
    USIGN32*            pt_ident_number;	/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to module id */
    USIGN16*            pt_number_of_subslot;	/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the count of subslots that are configured for this slot*/
    struct subslot*     pSubslot;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the first subslot handle of this slot */
    struct slot*        pNextSlot;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the next plugged slot */
    USIGN16*            pt_module_state;	/*!< \brief <b>!DO NOT CHANGE!</b> (Module OK=0, Substitute=1, Wrong=2, NoSubmodule=3) */
    USIGN16             wProperties;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to some properties of this slot. This tracks for example the pull and plug status */
    struct api_list*    poApi;				/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the API in which this slot is plugged. */
} SLOT;
POST_PACKED

typedef struct subslot
{
    USIGN16*     pt_subslot_number;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to subslot number */
    USIGN32*     pt_ident_number;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to submodule ID */
    USIGN32*     pt_size_init_records;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to maximum size of initial records for this submodule */
    USIGN32*     pt_size_init_records_used;	/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the size of initial records that were sent by the PLC */
    USIGN8*      pt_init_records;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the initial records that were sent by the PLC */
    USIGN32*     pt_size_chan_diag;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to maximum number of diagnosis channels. */
    USIGN32*     pt_nmbr_chan_diag;			/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN8*      pt_chan_diag;				/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the first diagnosis entry of this subslot */

    USIGN16*     pt_size_input_data;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the size of input data of this subslot */
    USIGN32*     pt_input_data;				/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to input frame buffer used by this subslot */

    USIGN16*     pt_used_in_cr;				/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to a structure that describes the status of this submodule e.g used by which AR or input or output <br> see INPUT_USED, OUTPUT_USED, INPUT_OUTPUT_USED, SUBSLOT_FOR_AR */
    USIGN16*     pt_offset_input_data;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the offset of the input data of this subslot in the input frame buffer */
    USIGN16*     pt_offset_input_iocs;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the offset of the IOCS of this subslot in the input frame buffer */
    USIGN8*      pt_input_iocs;				/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_offset_input_iops;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the offset of the IOPS of this subslot in the input frame buffer */
    USIGN8*      pt_operational_state;		/*!< \brief <b>!DO NOT CHANGE!</b> internally used value to track submodule state. Not the actual IOPS!. */
    USIGN16*     pt_offset_input_DataStatus;/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_input_DataStatus;		/*!< \brief <b>RESERVED</b> not used in this version */

    USIGN16*     pt_offset_indata_shared;				/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_offset_input_iocs_shared;			/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN8*      pt_input_iocs_shared;					/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_offset_input_iops_shared;			/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN8*      pt_operational_state_shared;					/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_offset_input_shared_DataStatus;		/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_wSubslotOwnedByAr;					/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to a value that defines which AR is the owner of this submodule. <br>See OWNED_BY_AR */

    USIGN16*     pt_size_output_data;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the size of output data of this subslot */
    USIGN32*     pt_output_data;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to output frame buffer used by this subslot */
    USIGN16*     pt_offset_output_data;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the offset of the output data of this subslot in the output frame buffer */
    USIGN16*     pt_offset_output_iocs;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the offset of the received IOCS of this subslot in the output frame buffer */
    USIGN8*      pt_output_iocs;			/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_offset_output_iops;		/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the offset of the received IOPS of this subslot in the output frame buffer */
    USIGN8*      pt_output_iops;			/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_offset_output_DataStatus;/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN16*     pt_output_DataStatus;		/*!< \brief <b>RESERVED</b> not used in this version */

    USIGN16*     pt_substitute_mode;		/*!< \brief <b>RESERVED</b> not used in this version */
    USIGN8*      pt_substitute_data;		/*!< \brief <b>RESERVED</b> not used in this version */

    USIGN16*     pt_properties;				/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to some properties of this subslot. This tracks for example the pull and plug status */
    USIGN16      wAdaptiveIos;				/*!< \brief <b>!DO NOT CHANGE!</b> 0 if subslot is configured with a fixed ID, 1 if configured with id 0x00 (AUTOCONF_MODULE) */
    USIGN16*     poIM0DataAvailable;		/*!< \brief <b>!DO NOT CHANGE!</b> Set to IMO_DATA_AVAIL if this subslot is I&M0 carrier */

    struct subslot* poNextSubslot;			/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the next subslot in this slot */
    struct _im0*    poIM0;					/*!< \brief Pointer to the I&M0 data of this subslot */
    struct isochrnParam* poSyncAppParam;	/*!< \brief Pointer to the Isochronous Mode Data. Only available in the DAP submodule and if the PLC requested a synchronous application */

    USIGN16*     pt_submodule_state;		/*!< \brief <b>!DO NOT CHANGE!</b> (Module OK=0, Substitute=1, Wrong=2, NoSubmodule=3) */
    SLOT*        pzSlot;					/*!< \brief <b>!DO NOT CHANGE!</b> Pointer to the slot this subslot belongs to */
} SUBSLOT;

typedef struct api_list
{
    USIGN32          index;
    SLOT*            firstslot;
    struct api_list* next;
} API_LIST;

typedef struct _api_ctx
{
    VOID      (*OnConnect_CB)(USIGN32 dwARNumber);
    VOID      (*OnConnectDone_CB)(USIGN32 dwARNumber);
    VOID      (*OnAbort_CB)(USIGN32 dwARNumber);
    VOID      (*OnReadRecord_CB)(USIGN32 dwARNumber);
    VOID      (*OnWriteRecord_CB)(USIGN32 dwARNumber);
    VOID      (*OnPRMEND_Done_CB)(USIGN32 dwARNumber);
    VOID      (*OnAlarm_Ack_CB)(USIGN16 hdlr, USIGN32 dwResp);
    VOID      (*OnDiag_Ack_CB)(USIGN16 hdlr, USIGN32 dwResp);
    VOID      (*OnWriteDone)(USIGN32 dwDummy);
    VOID      (*OnRead)(USIGN32 dwDummy);
    USIGN32   (*OnReadRecordDataObject_CB)(T_RECORD_DATA_OBJECT_TYPE oObjectToRead, SUBSLOT* pzSubslot,
                                           USIGN8* bIocs, USIGN8* bIops, USIGN8* pbyData, USIGN16 wDatalength, USIGN16* wSubstituteActiveFlag);
    VOID      (*OnReset_CB)(USIGN32 dwDummy);
    VOID      (*OnFSUParamChange_CB)(USIGN32 dwDummy);

    API_LIST  *api_list;
    RECORD_MB record_mb[MAX_NUMBER_RECORDS];
} API_AR_CTX;

typedef struct _dev_ctx
{
    VOID      (*OnSetStationName_CB)(VOID);
    VOID      (*OnSetStationIpAddr_CB)(USIGN32);
    VOID      (*OnSetStationDcpSignal_CB)(VOID);
    VOID      (*OnResetFactorySettings_CB)(USIGN32);
    VOID      (*OnLedStateChanged_CB)(USIGN16);
    VOID      (*OnImDataChanged_CB)(SUBSLOT*, USIGN16);
    VOID      (*OnSetStationNameTP_CB)(USIGN32);
} API_DEV_CTX;

typedef struct _eth_ctx
{
    VOID (*OnEthernetFrameRX_CB)(T_ETHERNET_MAILBOX*);
} T_API_ETHERNET_CTX;

typedef struct _tps_message_ctx
{
    VOID (*OnTpsMessageRX_CB)(T_ETHERNET_MAILBOX*);
} T_API_TPS_MSG_CTX;

PRE_PACKED
typedef struct __packed
{
    USIGN16   wChannelNumber;
    USIGN16   wChannelProperties;
    USIGN16   wChannelErrortype;
    USIGN16   wExtchannelErrortype;
    USIGN32   dwExtchannelAddval;
    USIGN32   dwQualifiedChannelQualifier;  /* API >= 0x15 */
    USIGN8    byFlags; /* 1 appear; 2 appear ack; 3 disappear; 4 disappear ack; 5 changed; 0-empty */

} DPR_DIAG_ENTRY;
POST_PACKED


typedef struct softwareVersion
{
    USIGN16     wMajorRelease;
    USIGN16     wBugFixVersion;
    USIGN16     wBuild;
} T_PNIO_VERSION_STRUCT;

PRE_PACKED
typedef struct isochrnParam
{
    USIGN16 wConAppCycleFactor;
    USIGN16 wTimeDataCycle;
    USIGN32 dwTimeIOInput;
    USIGN32 dwTimeIOOutput;
    USIGN32 dwTimeIOInputValid;
    USIGN32 dwTimeIOOutputValid;
    USIGN32 dwIntTriggerOffset;
    USIGN16 wParameterValid;
} T_ISOCHRON_PARAMETERS;
POST_PACKED

#ifdef USE_INT_APP
PRE_PACKED
typedef struct __packed _usign16p
{
  USIGN16     value;
} U16P;
POST_PACKED

PRE_PACKED
typedef struct __packed _usign32p
{
  USIGN32     value;
} U32P;
POST_PACKED
#endif

/* Typedefs for I&M data                                                    */
/*--------------------------------------------------------------------------*/
typedef struct _im0
{
    USIGN8  VendorIDHigh;
    USIGN8  VendorIDLow;
    USIGN8  OrderID[IM0_ORDERID_LEN];
    USIGN8  IM_Serial_Number[IM0_SERIALNUMBER_LEN];
    USIGN16 IM_Hardware_Revision;
    USIGN32 IM_Software_Revision;
    /* SoftwareRevisionPrefix                                               */
    /* This attribute contains the device revision prefix
         'V':  officially released version
         'R':  Revision (of a virtual or physically modular product)
         'P':  Prototype
         'U':  Under Test (field test)
         'T':  Test Device */
    /* Software Revision Functional Enhancement                             */
    /* This attribute contains the device revision number
         of functional enhancements */
    /* Software Revision Bugfix                                             */
    /* This attribute contains the device revision number
         of bug fixes */
    /* Software Revision Internal Change                                    */
    /* This attribute contains the device revision number
         of internal changes */
    USIGN16 IM_Revision_Counter;
    USIGN16 IM_Profile_ID;
    USIGN16 IM_Profile_Specific_Type;
    USIGN16 IM_Version;
    USIGN16 IM_Supported;
    /* This attribute contains the IM support information
         Bit 0:  IM_Supported.Profil_specific
                 This field shall be set related to the profiles.
         Bit 1:  IM_Supported.I&M1
                 This field shall be set if the I&M1 record contains data.
         Bit 2:  IM_Supported.I&M2
                 This field shall be set if the I&M1 record contains data.
                 ...
         Bit 15: IM_Supported.I&M15
                  This field shall be set if the I&M15 record contains data. */
    struct _im1* p_im1;
    struct _im2* p_im2;
    struct _im3* p_im3;
    struct _im4* p_im4;
} T_IM0_DATA;


typedef struct _im1
{
    USIGN8 IM_Tag_Function[IM_TAG_FUNCTION_LEN];
    USIGN8 IM_Tag_Location[IM_TAG_LOCATION_LEN];
} T_IM1_DATA;

typedef struct _im2
{
    USIGN8 IM_Date[IM_DATE_LEN];
} T_IM2_DATA;

typedef struct _im3
{
    USIGN8 IM_Descriptor[IM_DESCRIPTOR_LEN];
} T_IM3_DATA;

typedef struct _im4
{
    USIGN8 IM_Signature[IM_SIGNATURE_LEN];
} T_IM4_DATA;

PRE_PACKED
typedef struct __packed RWRecordBlockReq
{
    USIGN16   seq_number;
    UUID_TAG  ar_uuid;
    USIGN32   api;
    USIGN16   slot_number;
    USIGN16   subslot_number;
    USIGN16   padding;
    USIGN16   index;
    USIGN32   record_data_len;
    USIGN8    rwpadding[UUID_PADDING];
} RW_RECORD_REQ_BLOCK;
POST_PACKED

PRE_PACKED
typedef struct __packed FsParamBlk
{
    BLOCK_HEADER     BlockHeader;
    USIGN8           padding[2];
    USIGN32          dwFsParameterMode;
    UUID_TAG         FSParameterUUID;
} FS_PARAMETER_BLK;

typedef struct __packed FSUDataAdjustBlock
{
    BLOCK_HEADER     BlockHeader;
    USIGN8           padding[2];
    FS_PARAMETER_BLK FsParameterBlock;

} FSU_DATA_ADJUST_BLK;


typedef struct __packed FSUParameter
{
    USIGN32     FSParameterMode;
    UUID_TAG    FSParameterUUID;
}FSU_PARAMETER;

POST_PACKED



PRE_PACKED
typedef struct __packed _dcp_header
{
   USIGN8   byServiceID;    /*Set = 4*/
   USIGN8   byServiceType;  /*Request = 0*/
   USIGN32  dwXid;
   USIGN16  wReserv;
   USIGN16  wDcpDataLength;

}DCP_SET_HEADER;
POST_PACKED

PRE_PACKED
typedef struct __packed _dcp_block
{
   USIGN8   byOption;
   USIGN8   bySubOption;
   USIGN16  wDcpBlockLength;
   USIGN16  wBlockQualifier;

}DCP_BLOCK;
POST_PACKED

PRE_PACKED
typedef struct __packed _apdu_state
{
   USIGN16 wCycleCounter;
   USIGN8  byDataStatus;
   USIGN8  byTransferStatus;

}APDU_STATUS;
POST_PACKED


typedef struct deviceSoftwareVersion
{
    /* This attribute contains the device revision prefix
        'V':  officially released version
        'R':  Revision (of a virtual or physically modular product)
        'P':  Prototype
        'U':  Under Test (field test)
        'T':  Test Device */
    USIGN8 byRevisionPrefix;
    /* This attribute contains the device revision number
       of functional enhancements */
    USIGN8 byRevisionFunctionalEnhancement;
    /* This attribute contains the device revision number
         of bug fixes */
    USIGN8 byBugFix;
    /* This attribute contains the device revision number
       of internal changes */
    USIGN8 byInternalChange;
} T_DEVICE_SOFTWARE_VERSION;

/* This enum is used bye the functions TPS_SetModuleState()	and              */
/* TPS_SetSubmoduleState() to create a module diffblock.                     */
/*---------------------------------------------------------------------------*/
typedef enum
{
    MODULE_OK = 0x00,
    MODULE_WRONG = 0x01,
    MODULE_SUBSTITUTE = 0x02,
    MODULE_NO  = 0x3
} T_MODULE_STATE;

/*---------------------------------------------------------------------------*/
/* function prototypes                                                       */
/*---------------------------------------------------------------------------*/
USIGN32   TPS_GetLastError(VOID);
USIGN32   TPS_InitApplicationInterface(VOID);
USIGN32   TPS_StartDevice(VOID);
USIGN32   TPS_CheckStackStart(VOID);
USIGN32   TPS_AddDevice(USIGN16 wVendorID, USIGN16 wDeviceID, CHAR *pszDeviceType, T_DEVICE_SOFTWARE_VERSION* pzSoftwareVersion);
USIGN32   TPS_AddDeviceAdvanced (USIGN16 wVendorID, USIGN16 wDeviceID, CHAR *pszDeviceType, T_DEVICE_SOFTWARE_VERSION* pzSoftwareVersion, USIGN16 wHWVersion);
API_LIST* TPS_AddAPI(USIGN32 dwApi);
SLOT*     TPS_PlugModule(API_LIST* pzApi, USIGN16 wSlotNumber, USIGN32 dwModuleIdentNumber);
SUBSLOT*  TPS_PlugSubmodule(SLOT*       pzSlotHandle,
                            USIGN16     wSubslotNumber,
                            USIGN32     dwSubmoduleIdentNumber,
                            USIGN16     wInitParameterSize,
                            USIGN16     wNumberOfChannelDiag,
                            USIGN16     wSizeOfInputData,
                            USIGN16     wSizeOfOutputData,
                            T_IM0_DATA* pzIM0Data,
                            BOOL        bIM0Carrier);
USIGN32 TPS_CheckEvents(VOID);
USIGN32 TPS_GetArEstablished(USIGN32 dwARNumber);
USIGN32 TPS_RegisterRpcCallback(USIGN8 byWhich, VOID (*pfnFunction)(USIGN32));
USIGN32 TPS_RegisterDcpCallbackPara(USIGN8 byWhich, VOID (*pfnFunction)(USIGN32));
USIGN32 TPS_RegisterDcpCallback(USIGN8 byWhich, VOID (*pfnFunction)(VOID));
USIGN32 TPS_RegisterAlarmDiagCallback(USIGN8 which, VOID (*pfnFunction)(USIGN16,USIGN32));
USIGN32 TPS_RegisterRecordDataObjectCallback(USIGN32 (*pfnFunction)(T_RECORD_DATA_OBJECT_TYPE zObjectToRead, SUBSLOT* pzSubslot,
                                                                 USIGN8* pbyIocs, USIGN8* pbyIops, USIGN8* pbyData, USIGN16 wDatalength, USIGN16* pwSubstituteActiveFlag));
USIGN32 TPS_RegisterLedStateCallback(VOID(*pfnFunction)(USIGN16));
USIGN32 TPS_RegisterIMDataCallback(VOID(*pfnFunction)(SUBSLOT*, USIGN16));

USIGN32 TPS_PrintSlotSubslotConfiguration(VOID);
USIGN32 TPS_GetDriverVersionInfo(T_PNIO_VERSION_STRUCT* pzVersionInfo);
USIGN32 TPS_UpdateInputData(USIGN8 byARNumber);
USIGN32 TPS_UpdateOutputData(USIGN8 byARNumber);
USIGN32 TPS_RegisterIMDataForSubslot(SUBSLOT* pzSubslot, T_IM0_DATA* pzIM0Data, T_IM1_DATA* pzIM1Data,  T_IM2_DATA* pzIM2Data, T_IM3_DATA* pzIM3Data, T_IM4_DATA* pzIM4Data);
USIGN32 TPS_GetNameOfStation(USIGN8* pbyName, USIGN32 dwBufferLength);
USIGN32 TPS_GetSerialnumber( USIGN8 *pbySerialnumber, USIGN32 dwSerialLength );
USIGN32 TPS_GetOrderId( USIGN8 *pbyOrderId, USIGN32 dwOrderIdLength );
USIGN32 TPS_SetOrderId( USIGN8 *pbyOrderId, USIGN32 dwOrderIdLength );
USIGN32 TPS_GetStackVersionInfo(VOID);
USIGN32 TPS_GetDeviceVersionInfo(T_DEVICE_SOFTWARE_VERSION* pzStackVersionInfo);
USIGN16 TPS_GetLedState(VOID);
USIGN32 TPS_GetAPDUStatusOutCR(USIGN8 byARNumber, APDU_STATUS* ptApduStatus);
USIGN16 TPS_GetIOBufLength(USIGN8 byARNumber, USIGN8 byCrType);

#ifdef USE_TPS1_TO_SAVE_IM_DATA
VOID TPS_ResetToFactory_Done(VOID);
#endif

#ifndef USE_INT_APP
USIGN32 TPS_SetNameOfStation(USIGN8* pbyNameBuf, USIGN32 dwLenOfName, USIGN8 byAttr);
USIGN32 TPS_SetIPConfig(USIGN32 dwIPAddr, USIGN32 dwSubNetMask, USIGN32 dwGateway, USIGN8 byAttr);
#endif

#ifdef FW_UPDATE_OVER_HOST
USIGN32 TPS_StartFwUpdater(VOID);
USIGN32 TPS_WriteFWImageToFlash(USIGN8* pbyImage, USIGN32 dwLengthOfImage);
USIGN32 TPS_StartFwStack(VOID);
VOID    TPS_SetUpdaterTimeout(USIGN32 dwTimeout);
USIGN32 TPS_GetUpdaterTimeout(VOID);
USIGN32 TPS_WriteFWImageFragmentToFlash(USIGN8* pbyImageFrag, USIGN32 dwFragLen, USIGN8 byLastFrag);
#endif

/*---------------------------------------------------------------------------*/
/* Functions for Pull&Plug                                                   */
/*---------------------------------------------------------------------------*/
#ifdef PLUG_RETURN_SUBMODULE_ENABLE
USIGN32 TPS_PullModule(USIGN32 dwApi, USIGN16 wSlotNumber);
USIGN32 TPS_PullSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber);
USIGN32 TPS_RePlugModule(USIGN32 dwApi, USIGN16 wSlotNumber);
USIGN32 TPS_RePlugSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber);
#endif
USIGN32 TPS_ReactivateSubmodule(USIGN32 dwApi, USIGN16 wSlotNumber, USIGN16 wSubslotNumber);
/*---------------------------------------------------------------------------*/
/* Functions for the communication channel TPS <-> Host application          */
/*---------------------------------------------------------------------------*/
#ifdef USE_TPS_COMMUNICATION_CHANNEL
USIGN32 TPS_RegisterTPSMessageCallback(VOID (*pfnFunction)(T_ETHERNET_MAILBOX*));
USIGN32 TPS_InitTPSComChannel(VOID);
USIGN32 TPS_RecordWriteReqToStack(USIGN16 wSlotNr, USIGN16 wSubslotNr, USIGN16 wIndex, USIGN8* pbyData, USIGN32 dwDataLength);
USIGN32 TPS_RecordReadReqToStack(USIGN16 wSlotNr, USIGN16 wSubSlotNr, USIGN16 wIndex, USIGN32 dwDataLength);
USIGN32 TPS_WriteConfigBlockToFlash(USIGN8* pbyConfigurationData, USIGN16 wDataLength);
USIGN32 TPS_DcpSetDeviceName(USIGN8* pbyDeviceName, USIGN16 wNameLength, USIGN16 wSetPermanent);
USIGN32 TPS_DcpSetDeviceIP(USIGN32 dwIPAddr, USIGN32 dwSubnetMask, USIGN32 dwGateway, USIGN16 wSetPermanent);
USIGN32 TPS_DcpSetResetToFactory(USIGN16 wBlockQualifier);
USIGN32 TPS_WriteUserDataToFlash(USIGN16 wAddr, USIGN8* pbyUserData, USIGN16 wLength);
USIGN32 TPS_ReadUserDataFromFlash(USIGN16 wAddr, USIGN16 wLength);
USIGN32 TPS_TestInternalWatchdog(VOID);
#ifdef USE_TPS1_TO_SAVE_IM_DATA
SIGN32  TPS_WriteIMDataToFlash(USIGN16 wIMSupportedFlag, USIGN32 dwApi, USIGN16 wSlotNr, USIGN16 wSubslotNr, USIGN8* pbyResponse);
#endif
USIGN32 TPS_TestLEDs(USIGN8 byActivate, USIGN8 byRunLed, USIGN8 byMtLed, USIGN8 bySfLed, USIGN8 byBfLed);
#endif

#ifdef USE_FS_APP
USIGN32 TPS_GetFSUMode(VOID);
USIGN32 TPS_ReadFSUParameter(USIGN8* pbyFsuParameter);
#endif
/*---------------------------------------------------------------------------*/
/* Diagnosis functions                                                       */
/*---------------------------------------------------------------------------*/
#ifdef DIAGNOSIS_ENABLE
USIGN32 TPS_DiagSetChannelProperties(USIGN16* pwChannelProperties, USIGN8 byType, USIGN8 byAccumulative,
                                     USIGN8 byMaintenance, USIGN8 bySpecifier, USIGN8 byDirection);
USIGN32 AppDiagAdd(SUBSLOT* pzSubslot, USIGN16 wChannelNumber, USIGN16 wChannelProperties,
                           USIGN16 wChannelErrortype, USIGN16 wExtchannelErrortype, USIGN32 dwExtchannelAddval,
                           USIGN32 dwQualifiedChannelQualifier);
USIGN32 TPS_DiagChannelRemove(USIGN32 dwDiagAddress);
USIGN32 TPS_DiagSetChangeState(USIGN32 dwDiagAddress, USIGN8 byNewMaintenanceStatus, USIGN8 byNewSpecifier);
USIGN32 TPS_SendDiagAlarm(USIGN32 dwARNumber, USIGN32 dwAPINumber, USIGN16 wSlotNumber,
                          USIGN16 wSubSlotNumber, USIGN8 byAlarmPrio, USIGN8 byAlarmType,
                          USIGN32 dwDiagAddress, USIGN16 wUserHandle);
USIGN32 TPS_DiagChannelAdd(SUBSLOT* pzSubslot, USIGN16 wChannelNumber, USIGN16 wChannelProperties,
                           USIGN16 wChannelErrortype, USIGN16 wExtchannelErrortype, USIGN32 dwExtchannelAddval);
USIGN32 TPS_DiagChannelQualifiedAdd(SUBSLOT* pzSubslot, USIGN16 wChannelNumber, USIGN16 wChannelProperties,
                                    USIGN16 wChannelErrortype, USIGN16 wExtchannelErrortype, USIGN32 dwExtchannelAddval,
                                    USIGN32 dwQualifiedChannelQualifier);
USIGN32 TPS_DiagManufactorSpecdAdd(SUBSLOT* pzSubslot, USIGN16 wUsi, USIGN16 wLen, USIGN8* pbDiagData);

#endif

/*---------------------------------------------------------------------------*/
/* Functions for mailbox and alarm handling                                  */
/*---------------------------------------------------------------------------*/
USIGN32 TPS_GetMailboxInfo(USIGN32 dwMailboxNumber, RECORD_BOX_INFO* pzMailBoxInfo);
USIGN32 TPS_ReadMailboxData(USIGN8 byMBNumber, USIGN8* pbyData, USIGN32 dwLength);
USIGN32 TPS_RecordReadDone(USIGN32 dwARNumber, USIGN16 wErrorCode1, USIGN16 wErrorCode2);
USIGN32 TPS_WriteMailboxData(USIGN8 byMBNumber, USIGN8 *pbyData, USIGN32 dwLength);
USIGN32 TPS_RecordWriteDone(USIGN32 dwMailboxNumber, USIGN16 wErrorCode1, USIGN16 wErrorCode2);
USIGN32 TPS_SendAlarm(USIGN32 dwARNumber, USIGN32 dwAPINumber, USIGN16 wSlotNumber,
                      USIGN16 wSubSlotNumber, USIGN8 byAlarmPrio, USIGN8 byAlarmType,
                      USIGN32 dwDataLength, USIGN8 *pbyAlarmData, USIGN16 wUsi, USIGN16 wUserHandle);

/*---------------------------------------------------------------------------*/
/* Functions for reset handling                                              */
/*---------------------------------------------------------------------------*/
USIGN32 TPS_ResetDeviceConfig(VOID);
VOID    TPS_SoftReset(VOID);
USIGN32 TPS_CleanApiConf(VOID);

/*---------------------------------------------------------------------------*/
/* Functions for processing input and output data                            */
/*---------------------------------------------------------------------------*/
USIGN32 TPS_ReadOutputData(SUBSLOT* pzSubslot, USIGN8* pbyData, USIGN16 wDataLength, USIGN8* pbyState);
USIGN32 TPS_WriteInputData(SUBSLOT* pzSubslot, USIGN8* pbyData, USIGN16 wDataLength, USIGN8 byState);
USIGN32 TPS_SetOutputIocs(SUBSLOT* pzSubslot, USIGN8 byState);
USIGN32 TPS_SetInputIops(SUBSLOT* pzSubslot, USIGN8 byState);
/*---------------------------------------------------------------------------*/
/* Functions for the ethernet interface                                      */
/*---------------------------------------------------------------------------*/
USIGN32 TPS_GetMacAddresses(USIGN8* pbyInterfaceMac, USIGN8* pbyPort1Mac, USIGN8* pbyPort2Mac);
USIGN32 TPS_GetIPConfig(USIGN32* pdwIPAddress, USIGN32* pdwSubnetMask, USIGN32* pdwGateway);
#ifdef USE_ETHERNET_INTERFACE
USIGN32 TPS_InitEthernetChannel(VOID);
USIGN32 TPS_RegisterEthernetReceiveCallback(VOID (*pfnFunction)(T_ETHERNET_MAILBOX*));
USIGN32 TPS_SendEthernetFrame(USIGN8* pbyPacket, USIGN16 wLength, USIGN16 wPortNr);
USIGN32 TPS_GetEthernetTXStatus(USIGN8* pzTXMailBox);
USIGN32 TPS_SendEthernetFrameFragmented(USIGN8* pbyPacket, USIGN16 wLength, USIGN16 wPortNr, USIGN16 wMaxFragLen);
#endif

USIGN32 TPS_ForwardProtocolsToHost(USIGN32 dwProtoSelector);
USIGN32 TPS_DisableExternalFwUpdate(VOID);
/*---------------------------------------------------------------------------*/
/* Functions for the autoconfiguration of the Slot/Subslots                  */
/*---------------------------------------------------------------------------*/
#if defined(USE_AUTOCONF_MODULE) || defined(PLUG_RETURN_SUBMODULE_ENABLE)
USIGN32 TPS_GetModuleConfiguration(SLOT* pzSlot, USIGN32* pdwModuleIdentNumber);
USIGN32 TPS_GetSubmoduleConfiguration(SUBSLOT* pzSubslot, USIGN32* pdwSubmoduleIdentNumber, USIGN16* pwSizeOfInputData, USIGN16* pwSizeOfOutputData);
#endif
USIGN32 TPS_SetModuleState(SLOT* pzSlot, T_MODULE_STATE zModuleState, USIGN32 dwModuleIdentNumber);
USIGN32 TPS_SetSubmoduleState(SUBSLOT* pzSubslot, T_MODULE_STATE zSubmoduleState, USIGN32 dwSubmoduleIdentNumber);


/*---------------------------------------------------------------------------*/
/* Error Codes for API                                                       */
/*---------------------------------------------------------------------------*/
#define TPS_ACTION_OK                     0x00000000

#define TPS_ERROR_STACK_START_FAILED      0x00000010
#define TPS_ERROR_STACK_VERSION_FAILED    0x00000020
#define TPS_ERROR_WRONG_API_STATE         0x00000030
#define TPS_ERROR_NULL_POINTER            0x00000040
#define TPS_ERROR_BUFFER_TOO_SMALL        0x00000050
#define TPS_ERROR_STACK_TIMEOUT           0x00000060
#define TPS_ERROR_AR_STILL_ACTIVE         0x00000070
#define TPS_ERROR_BUFFER_TOO_BIG          0x00000080

/*---------------------------------------------------------------------------*/
/* TPS_AddDevice()                                                           */
/*---------------------------------------------------------------------------*/
#define DEVICE_TYPE_TO_LONG               0x00000100
#define CONFIG_MEMORY_TO_SHORT            0x00000110
#define SOFTWARE_VERSION_IS_NULL          0x00000120

/*---------------------------------------------------------------------------*/
/* TPS_AddAPI()                                                              */
/*---------------------------------------------------------------------------*/
#define ADD_API_OUT_OF_MEMORY             0x00000200

/*---------------------------------------------------------------------------*/
/* TPS_PlugModule()                                                          */
/*---------------------------------------------------------------------------*/
#define PLUGMODULE_OUT_OF_MEMORY          0x00000310
#define PLUGMODULE_INVALID_SLOT_HANDLE    0x00000330
#define PLUGMODULE_SLOT_ALREADY_EXISTS    0x00000340
#define PLUGMODULE_INVALID_MODULE_ID      0x00000370

/*---------------------------------------------------------------------------*/
/* TPS_PlugSubmodule()                                                       */
/*---------------------------------------------------------------------------*/
#define PLUG_SUB_OUT_OF_MEMORY                  0x00000400
#define PLUG_SUB_INVALID_SLOT_HANDLE            0x00000410
#define PLUG_SUB_MISSING_API                    0x00000420
#define PLUG_SUB_INVALID_SUBSLOT_HANDLE         0x00000430
#define PLUG_SUB_SLOT_ALREADY_EXISTS            0x00000440
#define PLUG_SUB_INVALID_SUBMODULE_ID           0x00000450
#define PLUG_SUB_SLOT_NOT_FOUND                 0x00000460
#define PLUG_SUB_IM_DATA_NOT_ALLOWED            0x00000461
#define PLUG_SUB_MISSING_IM0_DATA_FOR_DAP       0x00000462
#define PLUG_SUB_MISSING_IM0_CARRIER_FOR_DAP    0x00000463
#define PLUG_SUB_MISSING_IM0_DATA               0x00000464

/*---------------------------------------------------------------------------*/
/* TPS_PullSubmodule()                                                       */
/*---------------------------------------------------------------------------*/
#define PULL_SUB_MODULE_ALREADY_REMOVED   0x00000470
#define PULL_SUB_MODULE_NOT_FOUND         0x00000480
#define PULL_SUB_MODULE_0_NOT_ALLOWED     0x00000481

/*---------------------------------------------------------------------------*/
/* TPS_RePlugSubmodule()                                                   */
/*---------------------------------------------------------------------------*/
#define RETURN_OF_SUBMODULE_FAULT         0x00000490

/*---------------------------------------------------------------------------*/
/* TPS_ReactivateSubmodule()                                                   */
/*---------------------------------------------------------------------------*/
#define REACTIVATE_SUBMODULE_NOT_FOUND      0x00000495
#define REACTIVATE_SUBMODULE_0_NOT_ALLOWED  0x00000496
#define REACTIVATE_SUBMODULE_NOT_REPLUGGED  0x00000497
/*---------------------------------------------------------------------------*/
/* TPS_InitEthernetChannel()                                                      */
/*---------------------------------------------------------------------------*/
#define ETH_OUT_OF_MEMORY                 0x00000500

/*---------------------------------------------------------------------------*/
/* TPS_PullModule(), TPS_RePlugModule()                                      */
/*---------------------------------------------------------------------------*/
#define PULL_MODULE_NOT_FOUND             0x00000520
#define PULL_MODULE_ALREADY_PULLED        0x00000521
#define PULL_MODULE_NOT_PULLED            0x00000522

/*---------------------------------------------------------------------------*/
/* TPS_EthSendFrame                                                          */
/*---------------------------------------------------------------------------*/
#define ETH_SEND_FRAME_TOO_LONG           0x00000600
#define ETH_FRAME_CAN_NOT_BE_SEND         0x00000610
#define ETH_INVALID_PORT                  0x00000620

/*---------------------------------------------------------------------------*/
/* TPS_RegisterRpcCallback / TPS_RegisterDcpCallback /                       */
/* TPS_RegisterAlarmDiagCallback                                             */
/*---------------------------------------------------------------------------*/
#define REGISTER_RPC_ERROR                0x00000700
#define REGISTER_DCP_ERROR                0x00000710
#define REGISTER_ALARM_ERROR              0x00000720
#define REGISTER_FUNCTION_NULL            0x00000730

/*---------------------------------------------------------------------------*/
/* ErrorCodes handling I&M data                                              */
/*---------------------------------------------------------------------------*/
#define APPREAD_IM0_SUBSLOT_NOT_FOUND      0x00000800
#define APPREAD_IM0_DATA_NOT_FOUND         0x00000810

#define APPREAD_IM1_SUBSLOT_NOT_FOUND      0x00000820
#define APPREAD_IM1_DATA_NOT_FOUND         0x00000830
#define APPREAD_IM1_DATA_NOT_SUPPORTED     0x00000840

#define APPREAD_IM2_SUBSLOT_NOT_FOUND      0x00000850
#define APPREAD_IM2_DATA_NOT_FOUND         0x00000860
#define APPREAD_IM2_DATA_NOT_SUPPORTED     0x00000870

#define APPREAD_IM3_SUBSLOT_NOT_FOUND      0x00000880
#define APPREAD_IM3_DATA_NOT_FOUND         0x00000890
#define APPREAD_IM3_DATA_NOT_SUPPORTED     0x00000900

#define APPREAD_IM4_SUBSLOT_NOT_FOUND      0x00000910
#define APPREAD_IM4_DATA_NOT_FOUND         0x00000920
#define APPREAD_IM4_DATA_NOT_SUPPORTED     0x00000930

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_RecordWriteDone()                                      */
/*---------------------------------------------------------------------------*/
#define API_RECORD_WRITE_WRONG_FLAG         0x00000931
#define API_RECORD_WRITE_ERRORCODE2_INVALID 0x00000932
#define PENDING_WRITE_RESP_NOT_FOUND        0x00000933

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_RecordReadDone()                                       */
/*---------------------------------------------------------------------------*/
#define API_RECORD_READ_WRONG_FLAG          0x00000935
#define API_RECORD_READ_ERRORCODE2_INVALID  0x00000936
#define API_RECORD_READ_INVALID_MAILBOX     0x00000937

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_ReadMailboxData()                                      */
/*---------------------------------------------------------------------------*/
#define API_READ_MB_BUFFER_TOO_SMALL        0x00000940
#define API_READ_MB_INVALID_MAILBOX         0x00000941
#define API_READ_MB_WRONG_FLAG              0x00000942

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_WriteMailboxData()                                     */
/*---------------------------------------------------------------------------*/
#define API_WRITE_MB_TOO_MUCH_DATA          0x00000950
#define API_WRITE_MB_INVALID_MAILBOX        0x00000951
#define API_WRITE_MB_WRONG_FLAG             0x00000952

/*---------------------------------------------------------------------------*/
/* ErrorCode for TPS_OnRecordWriteCallback()                                 */
/*---------------------------------------------------------------------------*/
#define API_RECORD_WRITE_INDEX_UNKNOWN      0x00000961

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_GetMailboxInfo()                                       */
/*---------------------------------------------------------------------------*/
#define API_RECORD_MAILBOXINFO_WRONG_FLAG       0x00000964
#define API_RECORD_MAILBOXINFO_INVALID_MAILBOX  0x00000965

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_DiagSetChannelProperties()                             */
/*---------------------------------------------------------------------------*/
#define API_DIAG_PROP_WRONG_PARAMETER      0x00000970
#define API_DIAG_PROP_NULL_PARAMETER       0x00000971

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_DiagChannelRemove()                                    */
/*---------------------------------------------------------------------------*/
#define API_DIAG_REMOVE_WRONG_PARAMETER    0x00000980
#define API_DIAG_REMOVE_WRONG_RESULT       0x00000981
#define API_DIAG_REMOVE_NO_ACK             0x00000982
#define API_DIAG_REMOVE_EVENT_IN_USE       0x00000983

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_DiagSetChangeState()                                   */
/*---------------------------------------------------------------------------*/
#define API_DIAG_CHANGE_WRONG_PARAMETER    0x00000990
#define API_DIAG_CHANGE_WRONG_RESULT       0x00000991
#define API_DIAG_CHANGE_NO_ACK             0x00000992
#define API_DIAG_CHANGE_EVENT_IN_USE       0x00000993

/*---------------------------------------------------------------------------*/
/* ErrorCodes for AppPullPlugSubmodule()                                     */
/*---------------------------------------------------------------------------*/

#define PULL_PLUG_WRONG_OP_MODE            0x00001000

/*---------------------------------------------------------------------------*/
/* ErrorCodes for AppGetSlotHandle() and AppGetSubslotHandle()               */
/*---------------------------------------------------------------------------*/
#define GET_HANDLE_API_NOT_FOUND           0x00001050
#define GET_HANDLE_SLOT_NOT_FOUND          0x00001060
#define GET_HANDLE_SUBSLOT_NOT_FOUND       0x00001070

/*---------------------------------------------------------------------------*/
/* ErrorCodes processing TCP/IP frames                                       */
/*---------------------------------------------------------------------------*/
#define API_ETH_FRAME_NOT_SEND             0x00001100

/*---------------------------------------------------------------------------*/
/* ErrorCodes for processing alarms and diagnostic                           */
/*---------------------------------------------------------------------------*/
#define API_ALARM_AR_NOT_KNOWN             0x00002000

/*---------------------------------------------------------------------------*/
/* ErrorCodes for the function TPS_SendAlarm()                               */
/*---------------------------------------------------------------------------*/
#define API_ALARM_PRIO_UNKNOWN             0x00002010
#define API_ALARM_WRONG_AR_NUMBER          0x00002020
#define API_ALARM_MAILBOX_IN_USE           0x00002030
#define API_ALARM_SLOT_NOT_FOUND           0x00002040
#define API_ALARM_SUBSLOT_NOT_FOUND        0x00002050
#define API_ALARM_SUBSLOT_NOT_USED_BY_AR   0x00002060
#define API_ALARM_DATA_TOO_LONG            0x00002070

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_ReadOutputData() and TPS_WriteInputData()              */
/*---------------------------------------------------------------------------*/
#define API_IODATA_NULL_POINTER            0x00002100
#define API_IODATA_WRONG_BUFFER_LENGTH     0x00002110
#define API_IODATA_INPUT_FRAME_BUFFER      0x00002111
#define API_IODATA_WRONG_MODULE_PROJECTED  0x00002120
#define API_IODATA_MODULE_HAS_NO_OUTPUTS   0x00002130
#define API_IODATA_NOT_USED_IN_CR          0x00002140

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_DiagChannelAdd()                                       */
/*---------------------------------------------------------------------------*/
#define API_DIAG_ADD_NULL_POINTER               0x00002200
#define API_DIAG_ADD_EXT_CHANNEL_NOT_SUPPORTED  0x00002201
#define API_DIAG_ADD_MEM_ACCESS_ERROR           0x00002202
#define API_DIAG_ADD_ENTRY_ALREADY_EXISTS       0x00002203
#define API_DIAG_ADD_CHANGE_STATE_ERROR         0x00002204
#define API_DIAG_ADD_DIAG_LIST_FULL             0x00002206
#define API_DIAG_ADD_GLOBAL_LIMIT_REACHED       0x00002207
#define API_DIAG_EXISTS_BUT_DIFFERS             0x00002208
#define API_DIAG_ADD_EVENT_IN_USE               0x00002209
#define API_DIAG_ADD_QUALIFIED_PROPERTY_NOT_SET 0x00002210
#define API_DIAG_ADD_QUALIFIER_INVALID          0x00002211

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_DiagManufactorSpecdAdd()                               */
/*---------------------------------------------------------------------------*/
#define API_MAN_SPEC_DIAG_TO_LONG               0x00002212

/*---------------------------------------------------------------------------*/
/* ErrorCodes for processing SPI and parallel access                         */
/*---------------------------------------------------------------------------*/
#define SPI_INTERFACE_SET_BYTE_FAULT       0x00003000
#define SPI_INTERFACE_SET_WORD_FAULT       0x00003010
#define SPI_INTERFACE_SET_DWORD_FAULT      0x00003020
#define SPI_INTERFACE_SET_BUFFER_FAULT     0x00003030

#define SPI_INTERFACE_GET_BYTE_FAULT       0x00003040
#define SPI_INTERFACE_GET_WORD_FAULT       0x00003050
#define SPI_INTERFACE_GET_DWORD_FAULT      0x00003060
#define SPI_INTERFACE_GET_BUFFER_FAULT     0x00003070
#define SPI_INTERFACE_GET_ADDRESS_FAULT    0x00003080

#define SPI_INTERFACE_OUT_OD_ORDER         0x00003090
#define SPI_INTERFACE_READ_FAULT           0x00003100
#define SPI_INTERFACE_READ_PARAM_FAULT     0x00003110
#define SPI_INTERFACE_WRITE_FAULT          0x00003150
#define SPI_INTERFACE_WRITE_PARAM_FAULT    0x00003160

#define SPI_INTERFACE_PARAM_FAULT          0x00003200

#define SPI_PARALLEL_INTERFACE_NOT_ALLOWED 0x00003300

#define SPI_INVALID_4KB_ADDRES             0x00003310

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_WriteConfigurationToFlash                              */
/*---------------------------------------------------------------------------*/
#define API_RECORD_WRITE_ERROR             0x00003400

/*---------------------------------------------------------------------------*/
/* ErrorCodes for                                                            */
/*   TPS_GetModuleConfiguration()                                            */
/*   TPS_GetSubmoduleConfiguration()                                         */
/*   TPS_SetModuleState()                                                    */
/*   TPS_SetSubmoduleState()                                                 */
/*---------------------------------------------------------------------------*/
#define API_AUTOCONF_PARAMETER_NULL        0x00003500
#define API_AUTOCONF_WRONG_API_STATE       0x00003510
#define API_AUTOCONF_PARAM_OUT_OF_RANGE    0x00003520
#define API_AUTOCONF_SUBMODULE_IS_PULLED   0x00003530
#define API_AUTOCONF_WRONG_NOT_ALLOWED     0x00003531

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_SetNameOfStation                                       */
/*---------------------------------------------------------------------------*/
#define API_DEVICE_NAME_TOO_LONG           0x00004000
#define API_WRONG_PARAMETER                0x00004010

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_RecordReadReqToStack(), TPS_RecordWriteReqToStack()    */
/*---------------------------------------------------------------------------*/
#define API_RECORD_IF_OUT_OF_MEMORY        0x00004100
#define API_RECORD_IF_MAILBOX_NOT_EMPTY    0x00004110

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_WriteConfigBlockToFlash()                              */
/*---------------------------------------------------------------------------*/
#define API_CONFIG_INVALID_PARAMETER       0x00004200
#define API_CONFIG_OUT_OF_MEMORY           0x00004210
#define API_TPS_FLASH_USER_AREA_OVERRUN    0x00004211
#define API_TPS_FLASH_USER_INVALID_LENGTH  0x00004212

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_SetIPConfig()                                          */
/*---------------------------------------------------------------------------*/
#define API_WRONG_ATTR_PARAMETER           0x00004300

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_UpdateOutputData(), TPS_UpdateInputData()              */
/*---------------------------------------------------------------------------*/
#define API_WRONG_AR_NUMBER                0x00004400

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_SetOutputIocs()                                        */
/*---------------------------------------------------------------------------*/
#define SET_IOCS_NULL_POINTER              0x00004500

/*---------------------------------------------------------------------------*/
/* ErrorCodes for TPS_RegisterIMDataForSubslot()                             */
/*---------------------------------------------------------------------------*/
#define REGISTER_IM_SUBSLOT_NOT_FOUND      0x00004600
#define REGISTER_IM_SUBSLOT_NOT_DAP        0x00004601


#endif /* _API_NEW_H_ */
