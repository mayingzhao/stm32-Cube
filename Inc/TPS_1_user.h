/*
+-----------------------------------------------------------------------------+
| ****************************** TPS_1_user.h ******************************  |
+-----------------------------------------------------------------------------+
|                  Copyright by PHOENIX CONTACT Software GmbH                 |
|                                                                             |
|                       PHOENIX CONTACT Software GmbH                         |
|                       Langenbruch 6                                         |
|                       D-32657 Lemgo                                         |
|                       Germany                                               |
|                                                                             |
+-----------------------------------------------------------------------------+
| ***************** ADVICE FOR USAGE OF DELIVERED EXAMPLES ****************** |
+-----------------------------------------------------------------------------+
| The illustrations given by Phoenix Contact Software are mere examples which |
|                                                                             |
| are intended to explain the general procedure more clearly. We expressly    |
| point out that the examples do not represent complete solutions for         |
| concrete problems or errors, but are simply suggestions for an approach     |
| you can use as a basis for your specific implementations. Furthermore       |
| we would like to stress that the examples make no claim of being complete,  |
| but are simply extracts. Phoenix Contact Software has not performed any     |
| risk analysis, full-scale validation or respective tests for these examples,|
| neither based on the error pattern nor following any troubleshooting.       |
|                                                                             |
| Before delivery or implementation into the licensed software,               |
| each software component supplied by Phoenix Contact Software as master      |
| version must be checked and approved by you:                                |
|    - individually and as part of the customer system                        |
|    - with regard to integration and functionality                           |
|    - for each release                                                       |
|    - completely.                                                            |
+-----------------------------------------------------------------------------+
*/

/*! \file TPS_1_user.h
 *  \brief platform and project dependend settings
 */
 
#ifndef __TPS_1_user_H
#define __TPS_1_user_H

/*---------------------------------------------------------------------------*/
/* Includes                                                                  */
/*---------------------------------------------------------------------------*/
/* Application Includes                                                      */
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------------*/
/* For IDE's other than IAR Workbench you have to define your own          */
/* packing directive. Otherwise the structure alignment might be wrong!    */
/*-------------------------------------------------------------------------*/
#if defined(__IAR_SYSTEMS_ICC__)
    #define PRE_PACKED _Pragma("pack(1)")
    #define POST_PACKED _Pragma("pack()")
#else
    #define PRE_PACKED 
    #define POST_PACKED 
#endif


/* Caution: do not define USE_INT_APP! Only for internal use!                */
/*---------------------------------------------------------------------------*/
#ifndef USE_INT_APP
  /* V850 processor specific.                                                */
  /*-------------------------------------------------------------------------*/
  //#include <intrinsics.h>
  //#include <io70f3717.h>

  /*-------------------------------------------------------------------------*/
  /* Macro define (specific for V850)                                        */
  /*-------------------------------------------------------------------------*/
  #define   DI   __disable_interrupt
  #define   EI   __enable_interrupt
  #define   NOP  __no_operation
  #define   __packed
#endif
/*---------------------------------------------------------------------------*/
/* Big Endian must be defined!                                               */
/*---------------------------------------------------------------------------*/
#define LITTLE_ENDIAN_FORMAT

/* Serial or parallel interface for data wrapping.                           */
/*---------------------------------------------------------------------------*/
//#define PARALLEL_INTERFACE
#define SPI_INTERFACE
#define DEBUG_MAIN
#define DEBUG_API_TEST
/* Start Address Dual Ported RAM. Review:                                    */
/*---------------------------------------------------------------------------*/
#ifndef USE_INT_APP
    #define BASE_ADDRESS_OFFSET_DPRAM  0x00100000
#else
    #define BASE_ADDRESS_OFFSET_DPRAM  0x00000000
#endif

/* The Version of the user application (I&M0)                                */
/*---------------------------------------------------------------------------*/
#define SOFTWARE_REVISION_PREFIX                'V'

#ifndef USE_INT_APP
/*---------------------------------------------------------------------------*/
/* Data Type must be checked for each CPU!                                   */
/*---------------------------------------------------------------------------*/
typedef  unsigned char  USIGN8;    /* 8 bits, 0 to 255                       */
typedef  signed char    SIGN8;     /* 8 bits, -128 to 127                    */
typedef  unsigned short USIGN16;   /* 16 bits, 0 to 65535                    */
typedef  signed short   SIGN16;    /* 16 bits, -32768 to 32767               */
typedef  unsigned int   USIGN32;   /* 32 bits, 0 to 2exp32-1                 */
typedef  signed int     SIGN32;    /* 32 bits, -2exp31 to 2exp31-1           */
typedef  unsigned char  CHAR;      /* character (8-bit)                      */
typedef  unsigned char  BOOL;      /* Bool (TPS_TRUE or TPS_FALSE)           */

#else

#define USIGN32 unsigned int
#define SIGN32 int
#define USIGN16 unsigned short
#define SIGN16 short
#define USIGN8 unsigned char
#define SIGN8 signed char
#define VOID void
#define BOOL char
#define CHAR unsigned char
#endif
/* --------------------------------------------------------------------------*/
/* Logical Values                                                            */
/* --------------------------------------------------------------------------*/
#define TPS_TRUE  1
#define TPS_FALSE (!TPS_TRUE)

#define VOID void

/* Maximum values for the device                                             */
/* The maximum number of subslots is 64. The values USED_NUMBER_SLOT and     */
/* USED_NUMBER_SUBSLOT may be changed by the user between 1 and 64.          */
/*---------------------------------------------------------------------------*/
#define USED_NUMBER_OF_API  2
#define USED_NUMBER_SLOT    8
#define USED_NUMBER_SUBSLOT 8

/* Debug Information                                                         */
/*---------------------------------------------------------------------------*/
#undef DEBUG_API_TEST              /* Enable test messages                   */
#undef DEBUG_API_PLUG_MODULE       /* Enable test messages                   */
#undef DEBUG_API_SUBPLUG_MODULE    /* Enable test messages                   */
#undef DEBUG_API_ETH_FRAME         /* Enable messages when receiving an ethernet frames. */
#undef DEBUG_API_AUTOCONF          /* Enable test messages for the autoconfiguration of subslots. */

/* If active, the Host-Diagnosis functionality is used.                      */
/*---------------------------------------------------------------------------*/
#undef DIAGNOSIS_ENABLE

/* If active, print of the Slot and Subslot Configuration.                   */
/*---------------------------------------------------------------------------*/
#undef DEBUG_API_SUBPLUG_MODULE_PRINT

/* If active, the Ethernet MAC Interface is used.                            */
/*---------------------------------------------------------------------------*/
#undef USE_ETHERNET_INTERFACE

/* If active, the example application for mirroring of ethernet frames is    */
/* active. Every received frame will be returned to sender.                  */
/*---------------------------------------------------------------------------*/
#undef USE_ETHERNET_MIRROR_APPLICATION

/* If active, the driver will enable the autoconfiguration of modules.       */
/* This will allow the TPS-1 to accept the Slot/Subslot configuration given  */
/* by the PROFINET controller.                                               */
/*---------------------------------------------------------------------------*/
#define USE_AUTOCONF_MODULE

/* If active, the driver will enable the communication channel between       */
/* driver and the TPS-1. With this channel it is possible to access all      */
/* PROFINET records of the TPS.                                              */
/* For correct functions this feature also needs the ethernet interface      */
/* USE_ETHERNET_INTERFACE.                                                   */
/*---------------------------------------------------------------------------*/
#undef USE_TPS_COMMUNICATION_CHANNEL

/* This define activates the handling for 4 kB pages. Use only if 4 kB pages */
/* are activated by the TPS Configurator.                                    */
/*---------------------------------------------------------------------------*/
#undef USE_4KB_PAGES

/* This define activates code for the firmware update by the host CPU.       */
/*---------------------------------------------------------------------------*/
#undef FW_UPDATE_OVER_HOST

/* This define activates code for usage of pull and plug-submodule functions */
/*---------------------------------------------------------------------------*/
#undef PLUG_RETURN_SUBMODULE_ENABLE


/* This define activates code for usage of api fuctions for getting of fast   */
/* startup parameters                                                         */
/*----------------------------------------------------------------------------*/
#ifdef USE_TPS_COMMUNICATION_CHANNEL
#define USE_FS_APP
#endif

#endif /* #ifndef __TPS_1_user_H */
