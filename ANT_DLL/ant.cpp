/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/

/*
 **************************************************************************
 *
 * DESCRIPTION:
 *t
 * This module is an example wrapper for the ANT communications library
 * It sets up and maintains the link, and provides a simplified API to the
 * application.
 *
 **************************************************************************
 */


#include <string.h>  // memcpy
#include <stdio.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

#include "ant.h"
#include "version.h"
#include "types.h"
#include "defines.h"
#include "antdefines.h"
#include "usb_device_handle.hpp"
#include "dsi_serial_generic.hpp"
#include "dsi_framer_ant.hpp"
#include "dsi_thread.h"
#if defined(DEBUG_FILE)
   #include "dsi_debug.hpp"
#endif


#define MAX_CHANNELS ((UCHAR) 8)

#define MESG_CHANNEL_OFFSET                  0
#define MESG_EVENT_ID_OFFSET                 1
#define MESG_EVENT_CODE_OFFSET               2



// Struct to define channel event callback functions
// and recieve buffer.
typedef struct
{
   CHANNEL_EVENT_FUNC pfLinkEvent;
   UCHAR *pucRxBuffer;
} CHANNEL_LINK;



// Local variables.
static DSISerial* pclSerialObject = NULL;
static DSIFramerANT* pclMessageObject = NULL;
static DSI_THREAD_ID uiDSIThread;
static DSI_CONDITION_VAR condTestDone;
static DSI_MUTEX mutexTestDone;



// Local Data
static RESPONSE_FUNC pfResponseFunc = NULL;  //pointer to main response callback function
static UCHAR *pucResponseBuffer = NULL;            //pointer to buffer used to hold data from the response message
static CHANNEL_LINK sLink[MAX_CHANNELS];             //array of pointer for each channel
static BOOL bInitialized = FALSE;
static UCHAR ucAutoTransferChannel = 0xFF;
static USHORT usNumDataPackets = 0;
static BOOL bGoThread = FALSE;
static DSI_THREAD_IDNUM eTheThread;


// Local funcs
static DSI_THREAD_RETURN MessageThread(void *pvParameter_);
static void SerialHaveMessage(ANT_MESSAGE& stMessage_, USHORT usSize_);
static void MemoryCleanup(); //Deletes internal objects from memory

WINE_DEFAULT_DEBUG_CHANNEL(ANT_DLL);

extern "C" EXPORT
BOOL ANT_Init(UCHAR ucUSBDeviceNum, ULONG ulBaudrate)
{
    return ANT_InitExt(ucUSBDeviceNum, ulBaudrate, PORT_TYPE_USB, FRAMER_TYPE_BASIC);
}

//Initializes and opens USB connection to the module
extern "C" EXPORT
BOOL ANT_InitExt(UCHAR ucUSBDeviceNum, ULONG ulBaudrate, UCHAR ucPortType_, UCHAR ucSerialFrameType_)
{
   DSI_THREAD_IDNUM eThread = DSIThread_GetCurrentThreadIDNum();

   assert(eTheThread != eThread); // CANNOT CALL THIS FUNCTION FROM DLL THREAD (INSIDE DLL CALLBACK ROUTINES).

   assert(!bInitialized);         // IF ANT WAS ALREADY INITIALIZED, DO NOT CALL THIS FUNCTION BEFORE CALLING ANT_Close();


#if defined(DEBUG_FILE)
   DSIDebug::Init();
   DSIDebug::SetDebug(TRUE);
#endif

   //Create Serial object.
   pclSerialObject = NULL;

   switch(ucPortType_)
   {
      case PORT_TYPE_USB:
        pclSerialObject = new DSISerialGeneric();
        break;
      default: //Invalid port type selection
         return(FALSE);
   }

   if(!pclSerialObject)
      return(FALSE);

   //Initialize Serial object.
   //NOTE: Will fail if the module is not available.
   if(!pclSerialObject->Init(ulBaudrate, ucUSBDeviceNum))
   {
      MemoryCleanup();
      return(FALSE);
   }

   //Create Framer object.
   pclMessageObject = NULL;
   switch(ucSerialFrameType_)
   {
      case FRAMER_TYPE_BASIC:
         pclMessageObject = new DSIFramerANT(pclSerialObject);
         break;


      default:
         MemoryCleanup();
         return(FALSE);
   }

   if(!pclMessageObject)
   {
      MemoryCleanup();
      return(FALSE);
   }

   //Initialize Framer object.
   if(!pclMessageObject->Init())
   {
      MemoryCleanup();
      return(FALSE);
   }

   //Let Serial know about Framer.
   pclSerialObject->SetCallback(pclMessageObject);

   //Open Serial.
   if(!pclSerialObject->Open())
   {
      MemoryCleanup();
      return(FALSE);
   }

   //Create message thread.
   UCHAR ucCondInit= DSIThread_CondInit(&condTestDone);
   assert(ucCondInit == DSI_THREAD_ENONE);

   UCHAR ucMutexInit = DSIThread_MutexInit(&mutexTestDone);
   assert(ucMutexInit == DSI_THREAD_ENONE);

   bGoThread = TRUE;
   uiDSIThread = DSIThread_CreateThread(MessageThread, NULL);
   if(!uiDSIThread)
   {
      MemoryCleanup();
      bGoThread = FALSE;
      return(FALSE);
   }

   bInitialized = TRUE;
   return(TRUE);

}

///////////////////////////////////////////////////////////////////////
// Called by the application to close the usb connection
// MUST NOT BE CALLED IN THE CONTEXT OF THE MessageThread. That is,
// At the application level it must not be called within the
// callback functions into this library.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
void ANT_Close(void)
{
   DSI_THREAD_IDNUM eThread = DSIThread_GetCurrentThreadIDNum();

   assert(eTheThread != eThread); // CANNOT CALL THIS FUNCTION FROM DLL THREAD (INSIDE DLL CALLBACK ROUTINES).

   if (!bInitialized)
      return;

   bInitialized = FALSE;

   DSIThread_MutexLock(&mutexTestDone);
   bGoThread = FALSE;

   UCHAR ucWaitResult = DSIThread_CondTimedWait(&condTestDone, &mutexTestDone, DSI_THREAD_INFINITE);
   assert(ucWaitResult == DSI_THREAD_ENONE);
   DSIThread_MutexUnlock(&mutexTestDone);

   //Destroy mutex and condition var
   DSIThread_MutexDestroy(&mutexTestDone);
   DSIThread_CondDestroy(&condTestDone);

   MemoryCleanup();

#if defined(DEBUG_FILE)
   DSIDebug::Close();
#endif
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to initialize the main callback funcation,
// the main callback funcation must be initialized before the application
// can receive any reponse messages.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
void ANT_AssignResponseFunction(RESPONSE_FUNC pfResponse_, UCHAR* pucResponseBuffer_)
{
   pfResponseFunc = pfResponse_;
   pucResponseBuffer = pucResponseBuffer_;
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to initialize the callback funcation and
// data buffers for a particular channel.  This must be done in order
// for a channel to function properly.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
void ANT_AssignChannelEventFunction(UCHAR ucLink, CHANNEL_EVENT_FUNC pfLinkEvent, UCHAR *pucRxBuffer)
{
   if(ucLink < MAX_CHANNELS)
   {
      sLink[ucLink].pfLinkEvent = pfLinkEvent;
      sLink[ucLink].pucRxBuffer = pucRxBuffer;
   }
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Unassigns response function. Important for memory management of
// higher layer applications to avoid this library calling invalid pointers
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
void ANT_UnassignAllResponseFunctions()
{
   pfResponseFunc = NULL;
   pucResponseBuffer = NULL;
   for(int i=0; i< MAX_CHANNELS; ++i)
   {
      sLink[i].pfLinkEvent = NULL;
      sLink[i].pucRxBuffer = NULL;
   }
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Returns a pointer to a string constant containing the core library
// version.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
const char* ANT_LibVersion(void)
{
   return(SW_VER);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to restart ANT on the module
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ResetSystem(void)
{
   if(pclMessageObject)
      return(pclMessageObject->ResetSystem());

   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the network address for a given
// module channel
//!! This is (should be) a private network function
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetNetworkKey(UCHAR ucNetNumber, UCHAR *pucKey)
{
   return ANT_SetNetworkKey_RTO(ucNetNumber, pucKey, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetNetworkKey_RTO(UCHAR ucNetNumber, UCHAR *pucKey, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetNetworkKey(ucNetNumber, pucKey, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to assign a channel
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_AssignChannel(UCHAR ucANTChannel, UCHAR ucChannelType_, UCHAR ucNetNumber)
{
   return ANT_AssignChannel_RTO(ucANTChannel, ucChannelType_, ucNetNumber, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_AssignChannel_RTO(UCHAR ucANTChannel, UCHAR ucChannelType_, UCHAR ucNetNumber, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->AssignChannel(ucANTChannel, ucChannelType_, ucNetNumber, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to assign a channel using extended assignment
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_AssignChannelExt(UCHAR ucANTChannel, UCHAR ucChannelType_, UCHAR ucNetNumber, UCHAR ucExtFlags_)
{
   return ANT_AssignChannelExt_RTO(ucANTChannel, ucChannelType_, ucNetNumber, ucExtFlags_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_AssignChannelExt_RTO(UCHAR ucANTChannel, UCHAR ucChannelType_, UCHAR ucNetNumber, UCHAR ucExtFlags_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      UCHAR aucChannelType[] = {ucChannelType_, ucExtFlags_};  // Channel Type + Extended Assignment Byte

      return(pclMessageObject->AssignChannelExt(ucANTChannel, aucChannelType, 2, ucNetNumber, ulResponseTime_));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to unassign a channel
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_UnAssignChannel(UCHAR ucANTChannel)
{
   return ANT_UnAssignChannel_RTO(ucANTChannel, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_UnAssignChannel_RTO(UCHAR ucANTChannel, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->UnAssignChannel(ucANTChannel, ulResponseTime_));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the channel ID
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelId(UCHAR ucANTChannel_, USHORT usDeviceNumber_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_)
{
   return ANT_SetChannelId_RTO(ucANTChannel_, usDeviceNumber_, ucDeviceType_, ucTransmissionType_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelId_RTO(UCHAR ucANTChannel_, USHORT usDeviceNumber_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetChannelID(ucANTChannel_, usDeviceNumber_, ucDeviceType_, ucTransmissionType_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the messaging period
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelPeriod(UCHAR ucANTChannel_, USHORT usMesgPeriod_)
{
   return ANT_SetChannelPeriod_RTO(ucANTChannel_, usMesgPeriod_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelPeriod_RTO(UCHAR ucANTChannel_, USHORT usMesgPeriod_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetChannelPeriod(ucANTChannel_, usMesgPeriod_, ulResponseTime_));
   }
   return(FALSE);

}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the messaging period
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_RSSI_SetSearchThreshold(UCHAR ucANTChannel_, UCHAR ucThreshold_)
{
   return ANT_RSSI_SetSearchThreshold_RTO(ucANTChannel_, ucThreshold_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_RSSI_SetSearchThreshold_RTO(UCHAR ucANTChannel_, UCHAR ucThreshold_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetRSSISearchThreshold(ucANTChannel_, ucThreshold_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to set Low Priority Search Timeout. Not available on AP1
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetLowPriorityChannelSearchTimeout(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_)
{
   return ANT_SetLowPriorityChannelSearchTimeout_RTO(ucANTChannel_, ucSearchTimeout_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetLowPriorityChannelSearchTimeout_RTO(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetLowPriorityChannelSearchTimeout(ucANTChannel_, ucSearchTimeout_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the search timeout for a particular
// channel on the module
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelSearchTimeout(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_)
{
   return ANT_SetChannelSearchTimeout_RTO(ucANTChannel_, ucSearchTimeout_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelSearchTimeout_RTO(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetChannelSearchTimeout(ucANTChannel_, ucSearchTimeout_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the RF frequency for a given channel
//!! This is (should be) a private network function
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelRFFreq(UCHAR ucANTChannel_, UCHAR ucRFFreq_)
{
   return ANT_SetChannelRFFreq_RTO(ucANTChannel_, ucRFFreq_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelRFFreq_RTO(UCHAR ucANTChannel_, UCHAR ucRFFreq_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetChannelRFFrequency(ucANTChannel_, ucRFFreq_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the transmit power for the module
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetTransmitPower(UCHAR ucTransmitPower_)
{
   return ANT_SetTransmitPower_RTO(ucTransmitPower_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetTransmitPower_RTO(UCHAR ucTransmitPower_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetAllChannelsTransmitPower(ucTransmitPower_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to configure advanced bursting
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigureAdvancedBurst(BOOL bEnable_, UCHAR ucMaxPacketLength_, ULONG ulRequiredFields_, ULONG ulOptionalFields_)
{
   return ANT_ConfigureAdvancedBurst_RTO(bEnable_, ucMaxPacketLength_, ulRequiredFields_, ulOptionalFields_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigureAdvancedBurst_RTO(BOOL bEnable_, UCHAR ucMaxPacketLength_, ULONG ulRequiredFields_, ULONG ulOptionalFields_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigAdvancedBurst(bEnable_, ucMaxPacketLength_, ulRequiredFields_, ulOptionalFields_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Stall count version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigureAdvancedBurst_ext(BOOL bEnable_, UCHAR ucMaxPacketLength_, ULONG ulRequiredFields_, ULONG ulOptionalFields_, USHORT usStallCount_, UCHAR ucRetryCount_)
{
   return ANT_ConfigureAdvancedBurst_ext_RTO(bEnable_, ucMaxPacketLength_, ulRequiredFields_, ulOptionalFields_, usStallCount_, ucRetryCount_, 0);
}

///////////////////////////////////////////////////////////////////////
// Stall count version with response timeout
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigureAdvancedBurst_ext_RTO(BOOL bEnable_, UCHAR ucMaxPacketLength_, ULONG ulRequiredFields_, ULONG ulOptionalFields_, USHORT usStallCount_, UCHAR ucRetryCount_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigAdvancedBurst_ext(bEnable_, ucMaxPacketLength_, ulRequiredFields_, ulOptionalFields_, usStallCount_, ucRetryCount_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the transmit power for the module
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelTxPower(UCHAR ucANTChannel_, UCHAR ucTransmitPower_)
{
   return ANT_SetChannelTxPower_RTO(ucANTChannel_, ucTransmitPower_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetChannelTxPower_RTO(UCHAR ucANTChannel_, UCHAR ucTransmitPower_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetChannelTransmitPower(ucANTChannel_, ucTransmitPower_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to request a generic message
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_RequestMessage(UCHAR ucANTChannel_, UCHAR ucMessageID_)
{
   if(pclMessageObject){
      ANT_MESSAGE_ITEM stResponse;
      return pclMessageObject->SendRequest(ucMessageID_, ucANTChannel_, &stResponse, 0);
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to send a generic message
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_WriteMessage(UCHAR ucMessageID, UCHAR* aucData, USHORT usMessageSize)
{
   if(pclMessageObject){
      ANT_MESSAGE pstTempANTMessage;
      pstTempANTMessage.ucMessageID = ucMessageID;
      memcpy(pstTempANTMessage.aucData, aucData, MIN(usMessageSize, MESG_MAX_SIZE_VALUE));
      return pclMessageObject->WriteMessage(&pstTempANTMessage, usMessageSize);
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to open an assigned channel
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_OpenChannel(UCHAR ucANTChannel_)
{
   return ANT_OpenChannel_RTO(ucANTChannel_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_OpenChannel_RTO(UCHAR ucANTChannel_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->OpenChannel(ucANTChannel_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to close an opend channel
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_CloseChannel(UCHAR ucANTChannel_)
{
   return ANT_CloseChannel_RTO(ucANTChannel_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_CloseChannel_RTO(UCHAR ucANTChannel_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->CloseChannel(ucANTChannel_, ulResponseTime_));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to construct and send a broadcast data message.
// This message will be broadcast on the next synchronous channel period.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendBroadcastData(UCHAR ucANTChannel_, UCHAR *pucData_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SendBroadcastData(ucANTChannel_, pucData_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to construct and send an acknowledged data
// mesg.  This message will be transmitted on the next synchronous channel
// period.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendAcknowledgedData(UCHAR ucANTChannel_, UCHAR *pucData_)
{
   return ANT_SendAcknowledgedData_RTO(ucANTChannel_, pucData_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendAcknowledgedData_RTO(UCHAR ucANTChannel_, UCHAR *pucData_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SendAcknowledgedData( ucANTChannel_, pucData_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Used to send burst data using a block of data.  Proper sequence number
// of packet is maintained by the function.  Useful for testing purposes.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendBurstTransfer(UCHAR ucANTChannel_, UCHAR *pucData_, USHORT usNumDataPackets_)
{
   return ANT_SendBurstTransfer_RTO(ucANTChannel_, pucData_, usNumDataPackets_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendBurstTransfer_RTO(UCHAR ucANTChannel_, UCHAR *pucData_, USHORT usNumDataPackets_, ULONG ulResponseTime_)
{
   ULONG ulSize = usNumDataPackets_*8;   // Pass the number of bytes.
   ANTFRAMER_RETURN eStatus;

   if(pclMessageObject)
   {
      eStatus = pclMessageObject->SendTransfer( ucANTChannel_, pucData_, ulSize, ulResponseTime_);

      if( eStatus == ANTFRAMER_PASS )
         return(TRUE);
   }
   return(FALSE);
}




//////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to configure and start CW test mode.
// There is no way to turn off CW mode other than to do a reset on the module.
/////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_InitCWTestMode(void)
{
   return ANT_InitCWTestMode_RTO(0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_InitCWTestMode_RTO(ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->InitCWTestMode(ulResponseTime_));
   }
   return(FALSE);
}


//////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to configure and start CW test mode.
// There is no way to turn off CW mode other than to do a reset on the module.
/////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetCWTestMode(UCHAR ucTransmitPower_, UCHAR ucRFChannel_)
{
   return ANT_SetCWTestMode_RTO(ucTransmitPower_, ucRFChannel_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetCWTestMode_RTO(UCHAR ucTransmitPower_, UCHAR ucRFChannel_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetCWTestMode(ucTransmitPower_, ucRFChannel_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Add a channel ID to a channel's include/exclude ID list
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_AddChannelID(UCHAR ucANTChannel_, USHORT usDeviceNumber_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_, UCHAR ucListIndex_)
{
   return ANT_AddChannelID_RTO(ucANTChannel_, usDeviceNumber_, ucDeviceType_, ucTransmissionType_, ucListIndex_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_AddChannelID_RTO(UCHAR ucANTChannel_, USHORT usDeviceNumber_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_, UCHAR ucListIndex_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->AddChannelID(ucANTChannel_, usDeviceNumber_, ucDeviceType_, ucTransmissionType_, ucListIndex_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure the size and type of a channel's include/exclude ID list
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigList(UCHAR ucANTChannel_, UCHAR ucListSize_, UCHAR ucExclude_)
{
   return ANT_ConfigList_RTO(ucANTChannel_, ucListSize_, ucExclude_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigList_RTO(UCHAR ucANTChannel_, UCHAR ucListSize_, UCHAR ucExclude_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigList(ucANTChannel_, ucListSize_, ucExclude_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Open Scan Mode
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_OpenRxScanMode()
{
   return ANT_OpenRxScanMode_RTO(0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_OpenRxScanMode_RTO(ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->OpenRxScanMode(ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure ANT Frequency Agility Functionality (not on AP1 or AT3)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigFrequencyAgility(UCHAR ucANTChannel_, UCHAR ucFreq1_, UCHAR ucFreq2_, UCHAR ucFreq3_)
{
   return(ANT_ConfigFrequencyAgility_RTO(ucANTChannel_, ucFreq1_, ucFreq2_, ucFreq3_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigFrequencyAgility_RTO(UCHAR ucANTChannel_, UCHAR ucFreq1_, UCHAR ucFreq2_, UCHAR ucFreq3_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigFrequencyAgility(ucANTChannel_, ucFreq1_, ucFreq2_, ucFreq3_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure proximity search (not on AP1 or AT3)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetProximitySearch(UCHAR ucANTChannel_, UCHAR ucSearchThreshold_)
{
   return(ANT_SetProximitySearch_RTO(ucANTChannel_, ucSearchThreshold_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetProximitySearch_RTO(UCHAR ucANTChannel_, UCHAR ucSearchThreshold_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetProximitySearch(ucANTChannel_, ucSearchThreshold_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure Event Filter (USBm and nRF5 only)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigEventFilter(USHORT usEventFilter_)
{
   return(ANT_ConfigEventFilter_RTO(usEventFilter_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigEventFilter_RTO(USHORT usEventFilter_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigEventFilter(usEventFilter_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure Event Buffer (USBm only)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigEventBuffer(UCHAR ucConfig_, USHORT usSize_, USHORT usTime_)
{
   return(ANT_ConfigEventBuffer_RTO(ucConfig_, usSize_, usTime_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigEventBuffer_RTO(UCHAR ucConfig_, USHORT usSize_, USHORT usTime_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigEventBuffer(ucConfig_, usSize_, usTime_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure High Duty Search (USBm only)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigHighDutySearch(UCHAR ucEnable_, UCHAR ucSuppressionCycles_)
{
   return(ANT_ConfigHighDutySearch_RTO(ucEnable_, ucSuppressionCycles_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigHighDutySearch_RTO(UCHAR ucEnable_, UCHAR ucSuppressionCycles_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigHighDutySearch(ucEnable_, ucSuppressionCycles_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure Selective Data Update (USBm only)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigSelectiveDataUpdate(UCHAR ucChannel_, UCHAR ucSduConfig_)
{
   return(ANT_ConfigSelectiveDataUpdate_RTO(ucChannel_, ucSduConfig_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigSelectiveDataUpdate_RTO(UCHAR ucChannel_, UCHAR ucSduConfig_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigSelectiveDataUpdate(ucChannel_, ucSduConfig_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Set Selective Data Update Mask (USBm only)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetSelectiveDataUpdateMask(UCHAR ucMaskNumber_, UCHAR* pucSduMask_)
{
   return(ANT_SetSelectiveDataUpdateMask_RTO(ucMaskNumber_, pucSduMask_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetSelectiveDataUpdateMask_RTO(UCHAR ucMaskNumber_, UCHAR* pucSduMask_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetSelectiveDataUpdateMask(ucMaskNumber_, pucSduMask_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Configure User NVM (USBm only)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigUserNVM(USHORT usAddress_, UCHAR* pucData_, UCHAR ucSize_)
{
   return(ANT_ConfigUserNVM_RTO(usAddress_, pucData_, ucSize_, 0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_ConfigUserNVM_RTO(USHORT usAddress_, UCHAR* pucData_, UCHAR ucSize_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ConfigUserNVM(usAddress_, pucData_, ucSize_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Message to put into DEEP SLEEP (not on AP1 or AT3)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SleepMessage()
{
   return(ANT_SleepMessage_RTO(0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SleepMessage_RTO(ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SleepMessage(ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Message to put into DEEP SLEEP (not on AP1 or AT3)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_CrystalEnable()
{
   return(ANT_CrystalEnable_RTO(0));
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_CrystalEnable_RTO(ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->CrystalEnable(ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to write NVM data
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Write(UCHAR ucSize_, UCHAR *pucData_)
{
   return ANT_NVM_Write_RTO(ucSize_, pucData_ , 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Write_RTO(UCHAR ucSize_, UCHAR *pucData_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ScriptWrite(ucSize_,pucData_, ulResponseTime_ ));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to clear NVM data
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Clear(UCHAR ucSectNumber_)
{
   return ANT_NVM_Clear_RTO(ucSectNumber_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Clear_RTO(UCHAR ucSectNumber_, ULONG ulResponseTime_)
//Sector number is useless here, but is still here for backwards compatibility
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ScriptClear(ulResponseTime_));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set default NVM sector
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_SetDefaultSector(UCHAR ucSectNumber_)
{
   return ANT_NVM_SetDefaultSector_RTO(ucSectNumber_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_SetDefaultSector_RTO(UCHAR ucSectNumber_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ScriptSetDefaultSector(ucSectNumber_, ulResponseTime_));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to end NVM sector
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_EndSector()
{
   return ANT_NVM_EndSector_RTO(0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_EndSector_RTO(ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ScriptEndSector(ulResponseTime_));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to dump the contents of the NVM
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Dump()
{
   return ANT_NVM_Dump_RTO(0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Dump_RTO(ULONG ulResponseTime_)
//Response time is useless here, but is kept for backwards compatibility
{
   if(pclMessageObject)
   {
      pclMessageObject->ScriptDump();
     return TRUE;
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to lock the contents of the NVM
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Lock()
{
   return ANT_NVM_Lock_RTO(0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_NVM_Lock_RTO(ULONG ulResponseTimeout_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->ScriptLock(ulResponseTimeout_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the state of the FE (FIT1e)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL FIT_SetFEState(UCHAR ucFEState_)
{
   return FIT_SetFEState_RTO(ucFEState_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL FIT_SetFEState_RTO(UCHAR ucFEState_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->FITSetFEState(ucFEState_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to set the pairing distance (FIT1e)
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL FIT_AdjustPairingSettings(UCHAR ucSearchLv_, UCHAR ucPairLv_, UCHAR ucTrackLv_)
{
   return FIT_AdjustPairingSettings_RTO(ucSearchLv_, ucPairLv_, ucTrackLv_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL FIT_AdjustPairingSettings_RTO(UCHAR ucSearchLv_, UCHAR ucPairLv_, UCHAR ucTrackLv_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->FITAdjustPairingSettings(ucSearchLv_, ucPairLv_, ucTrackLv_, ulResponseTime_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to construct and send an extended broadcast data message.
// This message will be broadcast on the next synchronous channel period.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendExtBroadcastData(UCHAR ucANTChannel_, UCHAR *pucData_)
{
   if(!pclMessageObject)
      return FALSE;
   return pclMessageObject->SendExtBroadcastData(ucANTChannel_, pucData_);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to construct and send an extended acknowledged data
// mesg.  This message will be transmitted on the next synchronous channel
// period.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendExtAcknowledgedData(UCHAR ucANTChannel_, UCHAR *pucData_)
{
   return ANT_SendExtAcknowledgedData_RTO(ucANTChannel_, pucData_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendExtAcknowledgedData_RTO(UCHAR ucANTChannel_, UCHAR *pucData_, ULONG ulResponseTime_)
{
   if(!pclMessageObject)
      return FALSE;

   return (ANTFRAMER_PASS == pclMessageObject->SendExtAcknowledgedData(ucANTChannel_, pucData_, ulResponseTime_));
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Called by the application to construct and send extended burst data
// mesg.
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to send extended burst data with individual packets.  Proper sequence number
// of packet is maintained by the application.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SendExtBurstTransferPacket(UCHAR ucANTChannelSeq_, UCHAR *pucData_)
{
   if(pclMessageObject)
   {
      ANT_MESSAGE stMessage;

      stMessage.ucMessageID = MESG_EXT_BURST_DATA_ID;
      stMessage.aucData[0] = ucANTChannelSeq_;
      memcpy(&stMessage.aucData[1],pucData_, MESG_EXT_DATA_SIZE-1);

      return pclMessageObject->WriteMessage(&stMessage, MESG_EXT_DATA_SIZE);
   }
   return(FALSE);
}
///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to send extended burst data using a block of data.  Proper sequence number
// of packet is maintained by the function.  Useful for testing purposes.
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
USHORT ANT_SendExtBurstTransfer(UCHAR ucANTChannel_, UCHAR *pucData_, USHORT usDataPackets_)
{
   return ANT_SendExtBurstTransfer_RTO(ucANTChannel_, pucData_, usDataPackets_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
USHORT ANT_SendExtBurstTransfer_RTO(UCHAR ucANTChannel_, UCHAR *pucData_, USHORT usDataPackets_, ULONG ulResponseTime_)
{
   if(!pclMessageObject)
      return FALSE;

   return (ANTFRAMER_PASS == pclMessageObject->SendExtBurstTransfer(ucANTChannel_, pucData_, usDataPackets_*8, ulResponseTime_));
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to force the module to use extended rx messages all the time
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_RxExtMesgsEnable(UCHAR ucEnable_)
{
   return ANT_RxExtMesgsEnable_RTO(ucEnable_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_RxExtMesgsEnable_RTO(UCHAR ucEnable_, ULONG ulResponseTimeout_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->RxExtMesgsEnable(ucEnable_, ulResponseTimeout_));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Used to set a channel device ID to the module serial number
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetSerialNumChannelId(UCHAR ucANTChannel_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_)
{
   return ANT_SetSerialNumChannelId_RTO(ucANTChannel_, ucDeviceType_, ucTransmissionType_, 0);
}


///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetSerialNumChannelId_RTO(UCHAR ucANTChannel_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->SetSerialNumChannelId(ucANTChannel_, ucDeviceType_, ucTransmissionType_, ulResponseTime_));
   }
   return(FALSE);
}


///////////////////////////////////////////////////////////////////////
// Priority: Any
//
// Enables the module LED to flash on RF activity
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_EnableLED(UCHAR ucEnable_)
{
   return ANT_EnableLED_RTO(ucEnable_, 0);
}

///////////////////////////////////////////////////////////////////////
// Response TimeOut Version
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_EnableLED_RTO(UCHAR ucEnable_, ULONG ulResponseTime_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->EnableLED(ucEnable_, ulResponseTime_));
   }
   return(FALSE);
}



///////////////////////////////////////////////////////////////////////
// Called by the application to get the product string and serial number string (four bytes) of a particular device
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_GetDeviceUSBInfo(UCHAR ucDeviceNum, UCHAR* pucProductString, UCHAR* pucSerialString)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->GetDeviceUSBInfo(ucDeviceNum, pucProductString, pucSerialString, USB_MAX_STRLEN));
   }
   return(FALSE);
}

///////////////////////////////////////////////////////////////////////
// Called by the application to get the USB PID
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_GetDeviceUSBPID(USHORT* pusPID_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->GetDeviceUSBPID(*pusPID_));
   }
   return (FALSE);
}

///////////////////////////////////////////////////////////////////////
// Called by the application to get the USB VID
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_GetDeviceUSBVID(USHORT* pusVID_)
{
   if(pclMessageObject)
   {
      return(pclMessageObject->GetDeviceUSBVID(*pusVID_));
   }
   return (FALSE);
}



////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
// The following are the Integrated ANTFS_Client functions
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

//Memory Device Commands/////////////
extern "C" EXPORT
BOOL ANTFS_InitEEPROMDevice(USHORT usPageSize_, UCHAR ucAddressConfig_)
{
   return (pclMessageObject->InitEEPROMDevice(usPageSize_, ucAddressConfig_, 3000));
}

//File System Commands//////////////
extern "C" EXPORT
BOOL ANTFS_InitFSMemory()
{
   return (pclMessageObject->InitFSMemory(3000));
}

extern "C" EXPORT
BOOL ANTFS_FormatFSMemory(USHORT usNumberOfSectors_, USHORT usPagesPerSector_)
{
   return (pclMessageObject->FormatFSMemory(usNumberOfSectors_, usPagesPerSector_, 3000));
}

extern "C" EXPORT
BOOL ANTFS_SaveDirectory()
{
   return (pclMessageObject->SaveDirectory(3000));
}

extern "C" EXPORT
BOOL ANTFS_DirectoryRebuild()
{
   return (pclMessageObject->DirectoryRebuild(3000));
}

extern "C" EXPORT
BOOL ANTFS_FileDelete(UCHAR ucFileHandle_)
{
   return (pclMessageObject->FileDelete(ucFileHandle_, 3000));
}

extern "C" EXPORT
BOOL ANTFS_FileClose(UCHAR ucFileHandle_)
{
   return (pclMessageObject->FileClose(ucFileHandle_, 3000));
}

extern "C" EXPORT
BOOL ANTFS_SetFileSpecificFlags(UCHAR ucFileHandle_, UCHAR ucFlags_)
{
   return (pclMessageObject->SetFileSpecificFlags(ucFileHandle_, ucFlags_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_DirectoryReadLock(BOOL bLock_)
{
   return (pclMessageObject->DirectoryReadLock(bLock_, 3000));
}

extern "C" EXPORT
BOOL ANTFS_SetSystemTime(ULONG ulTime_)
{
   return (pclMessageObject->SetSystemTime(ulTime_, 3000));
}



//File System Requests////////////
extern "C" EXPORT
ULONG ANTFS_GetUsedSpace()
{
   return (pclMessageObject->GetUsedSpace(3000));
}

extern "C" EXPORT
ULONG ANTFS_GetFreeSpace()
{
   return (pclMessageObject->GetFreeFSSpace(3000));
}

extern "C" EXPORT
USHORT ANTFS_FindFileIndex(UCHAR ucFileDataType_, UCHAR ucFileSubType_, USHORT usFileNumber_)
{
   return (pclMessageObject->FindFileIndex(ucFileDataType_, ucFileSubType_, usFileNumber_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_ReadDirectoryAbsolute(ULONG ulOffset_, UCHAR ucSize_, UCHAR* pucBuffer_)
{
   return (pclMessageObject->ReadDirectoryAbsolute(ulOffset_, ucSize_, pucBuffer_,3000));
}

extern "C" EXPORT
UCHAR ANTFS_DirectoryReadEntry (USHORT usFileIndex_, UCHAR* ucFileDirectoryBuffer_)
{
   return (pclMessageObject->DirectoryReadEntry (usFileIndex_, ucFileDirectoryBuffer_, 3000));
}

extern "C" EXPORT
ULONG  ANTFS_DirectoryGetSize()
{
   return (pclMessageObject->DirectoryGetSize(3000));
}

extern "C" EXPORT
USHORT ANTFS_FileCreate(USHORT usFileIndex_, UCHAR ucFileDataType_, ULONG ulFileIdentifier_, UCHAR ucFileDataTypeSpecificFlags_, UCHAR ucGeneralFlags)
{
   return (pclMessageObject->FileCreate(usFileIndex_, ucFileDataType_, ulFileIdentifier_, ucFileDataTypeSpecificFlags_, ucGeneralFlags, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_FileOpen(USHORT usFileIndex_, UCHAR ucOpenFlags_)
{
   return (pclMessageObject->FileOpen(usFileIndex_, ucOpenFlags_, 3000));
}


extern "C" EXPORT
UCHAR ANTFS_FileReadAbsolute(UCHAR ucFileHandle_, ULONG ulOffset_, UCHAR ucReadSize_, UCHAR* pucReadBuffer_)
{
   return (pclMessageObject->FileReadAbsolute(ucFileHandle_, ulOffset_, ucReadSize_, pucReadBuffer_, 3000));
}


extern "C" EXPORT
UCHAR ANTFS_FileReadRelative(UCHAR ucFileHandle_, UCHAR ucReadSize_, UCHAR* pucReadBuffer_)
{
   return (pclMessageObject->FileReadRelative(ucFileHandle_, ucReadSize_, pucReadBuffer_, 3000));
}


extern "C" EXPORT
UCHAR ANTFS_FileWriteAbsolute(UCHAR ucFileHandle_, ULONG ulFileOffset_, UCHAR ucWriteSize_, const UCHAR* pucWriteBuffer_, UCHAR* ucBytesWritten_)
{
   return (pclMessageObject->FileWriteAbsolute(ucFileHandle_, ulFileOffset_, ucWriteSize_, pucWriteBuffer_, ucBytesWritten_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_FileWriteRelative(UCHAR ucFileHandle_, UCHAR ucWriteSize_, const UCHAR* pucWriteBuffer_, UCHAR* ucBytesWritten_)
{
   return (pclMessageObject->FileWriteRelative(ucFileHandle_, ucWriteSize_, pucWriteBuffer_, ucBytesWritten_, 3000));
}

extern "C" EXPORT
ULONG ANTFS_FileGetSize(UCHAR ucFileHandle_)
{
   return (pclMessageObject->FileGetSize(ucFileHandle_, 3000));
}

extern "C" EXPORT
ULONG ANTFS_FileGetSizeInMem(UCHAR ucFileHandle_)
{
   return (pclMessageObject->FileGetSizeInMem(ucFileHandle_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_FileGetSpecificFlags(UCHAR ucFileHandle_)
{
   return (pclMessageObject->FileGetSpecificFlags(ucFileHandle_, 3000));
}

extern "C" EXPORT
ULONG ANTFS_FileGetSystemTime()
{
   return (pclMessageObject->FileGetSystemTime(3000));
}


//FS-Crypto Commands/////////////
extern "C" EXPORT
UCHAR ANTFS_CryptoAddUserKeyIndex(UCHAR ucIndex_,  UCHAR* pucKey_)
{
   return (pclMessageObject->CryptoAddUserKeyIndex(ucIndex_, pucKey_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_CryptoSetUserKeyIndex(UCHAR ucIndex_)
{
   return (pclMessageObject->CryptoSetUserKeyIndex(ucIndex_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_CryptoSetUserKeyVal(UCHAR* pucKey_)
{
   return (pclMessageObject->CryptoSetUserKeyVal(pucKey_, 3000));
}


//FIT Commands///////////////////////
extern "C" EXPORT
UCHAR ANTFS_FitFileIntegrityCheck(UCHAR ucFileHandle_)
{
   return (pclMessageObject->FitFileIntegrityCheck(ucFileHandle_, 3000));
}


//ANT-FS Commands////////////////////
extern "C" EXPORT
UCHAR ANTFS_OpenBeacon()
{
   return (pclMessageObject->OpenBeacon(3000));
}

extern "C" EXPORT
UCHAR ANTFS_CloseBeacon()
{
   return (pclMessageObject->CloseBeacon(3000));
}

extern "C" EXPORT
UCHAR ANTFS_ConfigBeacon(USHORT usDeviceType_, USHORT usManufacturer_, UCHAR ucAuthType_, UCHAR ucBeaconStatus_)
{
   return (pclMessageObject->ConfigBeacon(usDeviceType_, usManufacturer_, ucAuthType_, ucBeaconStatus_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_SetFriendlyName(UCHAR ucLength_, const UCHAR* pucString_)
{
   return (pclMessageObject->SetFriendlyName(ucLength_, pucString_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_SetPasskey(UCHAR ucLength_, const UCHAR* pucString_)
{
   return (pclMessageObject->SetPasskey(ucLength_, pucString_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_SetBeaconState(UCHAR ucBeaconStatus_)
{
   return (pclMessageObject->SetBeaconState(ucBeaconStatus_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_PairResponse(BOOL bAccept_)
{
   return (pclMessageObject->PairResponse(bAccept_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_SetLinkFrequency(UCHAR ucChannelNumber_, UCHAR ucFrequency_)
{
   return (pclMessageObject->SetLinkFrequency(ucChannelNumber_, ucFrequency_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_SetBeaconTimeout(UCHAR ucTimeout_)
{
   return (pclMessageObject->SetBeaconTimeout(ucTimeout_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_SetPairingTimeout(UCHAR ucTimeout_)
{
   return (pclMessageObject->SetPairingTimeout(ucTimeout_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_EnableRemoteFileCreate(BOOL bEnable_)
{
   return (pclMessageObject->EnableRemoteFileCreate(bEnable_, 3000));
}


//ANT-FS Responses////////////////////
extern "C" EXPORT
UCHAR ANTFS_GetCmdPipe(UCHAR ucOffset_, UCHAR ucReadSize_, UCHAR* pucReadBuffer_)
{
   return (pclMessageObject->GetCmdPipe(ucOffset_, ucReadSize_, pucReadBuffer_, 3000));
}

extern "C" EXPORT
UCHAR ANTFS_SetCmdPipe(UCHAR ucOffset_, UCHAR ucWriteSize_, const UCHAR* pucWriteBuffer_)
{
   return (pclMessageObject->SetCmdPipe(ucOffset_, ucWriteSize_, pucWriteBuffer_, 3000));
}


//GetFSResponse/////////////////////////
extern "C" EXPORT
UCHAR ANTFS_GetLastError()
{
   return (pclMessageObject->GetLastError());
}


///////////////////////////////////////////////////////////////////////
// Set the directory the log files are saved to
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
BOOL ANT_SetDebugLogDirectory(char* pcDirectory)
{
#if defined(DEBUG_FILE)
   return DSIDebug::SetDirectory(pcDirectory);
#else
   return false;
#endif
}


///////////////////////////////////////////////////////////////////////
// Put current thread to sleep for the specified number of milliseconds
///////////////////////////////////////////////////////////////////////
extern "C" EXPORT
void ANT_Nap(ULONG ulMilliseconds_)
{
   DSIThread_Sleep(ulMilliseconds_);
}

// Local functions ****************************************************//
static DSI_THREAD_RETURN MessageThread(void *pvParameter_)
{
   ANT_MESSAGE stMessage;
   USHORT usSize;

   eTheThread = DSIThread_GetCurrentThreadIDNum();

   while(bGoThread)
   {
      if(pclMessageObject->WaitForMessage(1000/*DSI_THREAD_INFINITE*/))
      {
         usSize = pclMessageObject->GetMessage(&stMessage);

         if(usSize == DSI_FRAMER_ERROR)
         {
            // Get the message to clear the error
            usSize = pclMessageObject->GetMessage(&stMessage, MESG_MAX_SIZE_VALUE);
            continue;
         }

         if(usSize != 0 && usSize != DSI_FRAMER_ERROR && usSize != DSI_FRAMER_TIMEDOUT)
         {
            SerialHaveMessage(stMessage, usSize);
         }
      }
   }

   DSIThread_MutexLock(&mutexTestDone);
   UCHAR ucCondResult = DSIThread_CondSignal(&condTestDone);
   assert(ucCondResult == DSI_THREAD_ENONE);
   DSIThread_MutexUnlock(&mutexTestDone);

   return(NULL);
}

//Called internally to delete objects from memory
static void MemoryCleanup(void)
{
   if(pclSerialObject)
   {
      //Close all stuff
      pclSerialObject->Close();
      delete pclSerialObject;
      pclSerialObject = NULL;
   }

   if(pclMessageObject)
   {
      delete pclMessageObject;
      pclMessageObject = NULL;
   }
}

///////////////////////////////////////////////////////////////////////
//
// Processes a received serial message.  This function is intended to be
// called by the serial message driver code, to be defined by the user,
// when a serial message is received from the ANT module.
///////////////////////////////////////////////////////////////////////
static void SerialHaveMessage(ANT_MESSAGE& stMessage_, USHORT usSize_)
{
   UCHAR ucANTChannel;

   //If no response function has been assigned, ignore the message and unlock
   //the receive buffer
   if (pfResponseFunc == NULL)
      return;


   ucANTChannel = stMessage_.aucData[MESG_CHANNEL_OFFSET] & CHANNEL_NUMBER_MASK;


   //Process the message to determine whether it is a response event or one
   //of the channel events and call the appropriate event function.
   switch (stMessage_.ucMessageID)
   {
      case MESG_RESPONSE_EVENT_ID:
      {
         if (stMessage_.aucData[MESG_EVENT_ID_OFFSET] != MESG_EVENT_ID) // this is a response
         {
            if (pucResponseBuffer)
            {
               memcpy(pucResponseBuffer, stMessage_.aucData, MESG_RESPONSE_EVENT_SIZE);
               pfResponseFunc(ucANTChannel, MESG_RESPONSE_EVENT_ID);
            }
         }
         else // this is an event
         {
            // If we are in auto transfer mode, stop sending packets
            if ((stMessage_.aucData[MESG_EVENT_CODE_OFFSET] == EVENT_TRANSFER_TX_FAILED) && (ucAutoTransferChannel == ucANTChannel))
               usNumDataPackets = 0;

            if (sLink[ucANTChannel].pfLinkEvent == NULL)
               break;

            memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, usSize_);
            sLink[ucANTChannel].pfLinkEvent(ucANTChannel, stMessage_.aucData[MESG_EVENT_CODE_OFFSET]); // pass through any events not handled here
         }
         break;
      }
      case MESG_BROADCAST_DATA_ID:
      {
         if (  sLink[ucANTChannel].pfLinkEvent == NULL ||
               sLink[ucANTChannel].pucRxBuffer == NULL)
         {
            break;
         }

         // If size is greater than the standard data message size, then assume
         // that this is a data message with a flag at the end. Set the event accordingly.
         if(usSize_ > MESG_DATA_SIZE)
         {
            //Call channel event function with Broadcast message code
            memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, usSize_);
            sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_FLAG_BROADCAST);                 // process the event
         }
         else
         {
            //Call channel event function with Broadcast message code
            memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, ANT_STANDARD_DATA_PAYLOAD_SIZE + MESG_CHANNEL_NUM_SIZE);
            sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_BROADCAST);                 // process the event
         }
         break;

      }

      case MESG_ACKNOWLEDGED_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;


         if(usSize_ > MESG_DATA_SIZE)
         {
            //Call channel event function with Broadcast message code
            memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, usSize_);
            sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_FLAG_ACKNOWLEDGED);                 // process the event
         }
         else
         {
            //Call channel event function with Acknowledged message code
            memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, ANT_STANDARD_DATA_PAYLOAD_SIZE + MESG_CHANNEL_NUM_SIZE);
            sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_ACKNOWLEDGED);                 // process the message
         }
         break;
      }
      case MESG_BURST_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;

         if(usSize_ > MESG_DATA_SIZE)
         {
            //Call channel event function with Broadcast message code
            memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, usSize_);
            sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_FLAG_BURST_PACKET);                 // process the event
         }
         else
         {
            //Call channel event function with Burst message code
            memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, ANT_STANDARD_DATA_PAYLOAD_SIZE + MESG_CHANNEL_NUM_SIZE);
            sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_BURST_PACKET);                 // process the message
         }
         break;
      }
      case MESG_EXT_BROADCAST_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;

         //Call channel event function with Broadcast message code
         memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, MESG_EXT_DATA_SIZE);
         sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_EXT_BROADCAST);                 // process the event
         break;
      }
      case MESG_EXT_ACKNOWLEDGED_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;

         //Call channel event function with Acknowledged message code
         memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, MESG_EXT_DATA_SIZE);
         sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_EXT_ACKNOWLEDGED);              // process the message
         break;
      }
      case MESG_EXT_BURST_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;

         //Call channel event function with Burst message code
         memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, MESG_EXT_DATA_SIZE);
         sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_EXT_BURST_PACKET);                 // process the message
         break;
      }
      case MESG_RSSI_BROADCAST_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;

         //Call channel event function with Broadcast message code
         memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, MESG_RSSI_DATA_SIZE);
         sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_RSSI_BROADCAST);                 // process the event
         break;
      }
      case MESG_RSSI_ACKNOWLEDGED_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;

         //Call channel event function with Acknowledged message code
         memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, MESG_RSSI_DATA_SIZE);
         sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_RSSI_ACKNOWLEDGED);                 // process the message
         break;
      }
      case MESG_RSSI_BURST_DATA_ID:
      {
         if (sLink[ucANTChannel].pfLinkEvent == NULL)
            break;

         //Call channel event function with Burst message code
         memcpy(sLink[ucANTChannel].pucRxBuffer, stMessage_.aucData, MESG_RSSI_DATA_SIZE);
         sLink[ucANTChannel].pfLinkEvent(ucANTChannel, EVENT_RX_RSSI_BURST_PACKET);                 // process the message
         break;
      }

      case MESG_SCRIPT_DATA_ID:
      {
         if (pucResponseBuffer)
         {
            memcpy(pucResponseBuffer, stMessage_.aucData, usSize_);
            pucResponseBuffer[10] = (UCHAR)usSize_;
            pfResponseFunc(ucANTChannel, MESG_SCRIPT_DATA_ID);
         }
         break;
      }
      case MESG_SCRIPT_CMD_ID:
      {
         if (pucResponseBuffer)
         {
            memcpy(pucResponseBuffer, stMessage_.aucData, MESG_SCRIPT_CMD_SIZE);
            pfResponseFunc(ucANTChannel, MESG_SCRIPT_CMD_ID);
         }
         break;
      }
      default:
      {
         if (pucResponseBuffer)                     // can we process this link
         {
            memcpy(pucResponseBuffer, stMessage_.aucData, usSize_);
            pfResponseFunc(ucANTChannel, stMessage_.ucMessageID );
         }
         break;
      }
   }

   return;
}

extern "C" EXPORT
void ANT_LibConfigCustom() {
   fprintf(stderr, "ERR: ANT_LibConfigCustom not implemented");
   exit(1);
}