/******************************************************************************

 @file cllc.c

 @brief Coordinator Logical Link Controller

 This module is the Coordinator Logical Link Controller for the application.

 Group: WCS LPC
 $Target Devices: Linux: AM335x, Embedded Devices: CC1310, CC1350$

 ******************************************************************************
 $License: BSD3 2016 $
  
   Copyright (c) 2015, Texas Instruments Incorporated
   All rights reserved.
  
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
  
   *  Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
  
   *  Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
  
   *  Neither the name of Texas Instruments Incorporated nor the names of
      its contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
  
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************
 $Release Name: TI-15.4Stack Linux x64 SDK$
 $Release Date: July 14, 2016 (2.00.00.30)$
 *****************************************************************************/

/******************************************************************************
 Includes
 *****************************************************************************/
#include <string.h>
#include "cllc.h"
#include "csf.h"
#include "csf_linux.h"
#include "collector.h"
#include "util.h"
#include "log.h"
/******************************************************************************
 Constants and definitions
 *****************************************************************************/
#define CLLC_CHAN_LOWEST             0
/* Returns if the specific bit in the scan channel map array is set */
#define CLLC_IS_CHANNEL_MASK_SET(a, c) \
                     (*((uint8_t*)(a) + ((c) - CLLC_CHAN_LOWEST) / 8) & \
                     ((uint8_t) 1 << (((c) - CLLC_CHAN_LOWEST) % 8)))
#define CLLC_DEFAULT_KEY_SOURCE {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33}
#define CLLC_INVALID_PAN         0xFFFF
#define CLLC_JOIN_PERMIT_ON      0xFFFFFFFF
#define CLLC_MAX_ENERGY          255
#define CLLC_PAN_NOT_FOUND       0x0000

/*! MPM Constants for start request */
#define CLLC_OFFSET_TIMESLOT     0
#define CLLC_EBEACONORDER        15
#define CLLC_NBPANEBEACONORDER   16383

/*! FH default PIB values */
/*! value for ApiMac_FHAttribute_routingCost PIB */
#define CLLC_FH_ROUTING_COST            0x00
/*! value for ApiMac_FHAttribute_routingMethod PIB */
#define CLLC_FH_ROUTING_METHOD          0x01
/*! value for ApiMac_FHAttribute_eapolReady PIB */
#define CLLC_FH_EAPOL_READY             0x01
/*! value for ApiMac_FHAttribute_fanTPSVersion PIB */
#define CLLC_FH_FANTPSVERSION           0x01
/*! value for ApiMac_FHAttribute_gtk0Hash PIB */
#define CLLC_FH_GTK0HASH                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
/*! value for ApiMac_FHAttribute_gtk1Hash PIB */
#define CLLC_FH_GTK1HASH                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
/*! value for ApiMac_FHAttribute_gtk2Hash PIB */
#define CLLC_FH_GTK2HASH                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
/*! value for ApiMac_FHAttribute_gtk3Hash PIB */
#define CLLC_FH_GTK3HASH                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03}
#define CLLC_FH_PANVERSION              0x0000
#define CLLC_FH_MAX_TRICKLE             CONFIG_TRICKLE_MAX_CLK_DURATION
#define CLLC_FH_MIN_TRICKLE             CONFIG_TRICKLE_MIN_CLK_DURATION

/*!
 Variable to start the assignment of short addresses by the coordinator
 to each the device that associates to it
 */
#define CLLC_ASSOC_DEVICE_STARTING_NUMBER 1

/******************************************************************************
 Security constants and definitions
 *****************************************************************************/

#define KEY_TABLE_ENTRIES 1
#define KEY_ID_LOOKUP_ENTRIES 1
#define KEY_DEVICE_TABLE_ENTRIES 8
#define KEY_USAGE_TABLE_ENTRIES 1
#define SECURITY_LEVEL_ENTRIES 1

#define MAC_FRAME_TYPE_DATA 1
#define MAC_DATA_REQ_FRAME 4

/******************************************************************************
 Structures
 *****************************************************************************/
/* building block for PAN descriptor linked list used to store PAN
 descriptors received during beacon and scan */
typedef struct
{
    void *pNext;
    ApiMac_panDesc_t panDescList;
} panDescList_t;

/* Coordinator information, used to store default parameters */
typedef struct
{
    uint8_t channel;
    uint16_t panID;
    uint16_t shortAddr;
    Cllc_states_t currentCllcState;
    Cllc_coord_states_t currentCoordState;
} coordInformation_t;

/******************************************************************************
 Global variables
 *****************************************************************************/
/* Task pending events */
uint16_t Cllc_events = 0;
/* Association table */
Cllc_associated_devices_t Cllc_associatedDevList[CONFIG_MAX_DEVICES];
Cllc_statistics_t Cllc_statistics;

/**
 * Variable to start the assignment of short addresses by the coordinator
 * to each the device that associates to it
 */
uint16_t Cllc_devShortAddr = 0x0001;
uint8_t Cllc_keySource[] = CLLC_DEFAULT_KEY_SOURCE;

/******************************************************************************
 Local variables
 *****************************************************************************/
/* Default variables structure for coordinator */
STATIC coordInformation_t coordInfoBlock;
/* default channel mask */
STATIC uint8_t chanMask[APIMAC_154G_CHANNEL_BITMAP_SIZ];
/* Linked list to store incoming PAN descriptors */
STATIC panDescList_t *pPANDesclist = NULL;
/* number of devices associated with the coordinator */
STATIC uint8_t Cllc_numOfDevices = 0;
/* copy of MAC API callbacks */
STATIC ApiMac_callbacks_t macCallbacksCopy = { 0 };
/* copy of CLLC callbacks */
STATIC Cllc_callbacks_t *pCllcCallbacksCopy = (Cllc_callbacks_t *) NULL;
//STATIC uint8_t fhNetname[32] = CONFIG_FH_NETNAME;
#define fhNetname ((uint8_t *)(&CONFIG_FH_NETNAME[0]))
STATIC uint8_t fhGtkHash0[] = CLLC_FH_GTK0HASH;
STATIC uint8_t fhGtkHash1[] = CLLC_FH_GTK1HASH;
STATIC uint8_t fhGtkHash2[] = CLLC_FH_GTK2HASH;
STATIC uint8_t fhGtkHash3[] = CLLC_FH_GTK3HASH;
STATIC uint32_t fhPAtrickleTime; /* linux: these come from the configuration file at startup */
STATIC uint32_t fhPCtrickleTime;

STATIC uint8_t Cllc_shortAddress = 0;

/******************************************************************************
 Local security variables
 *****************************************************************************/

static const ApiMac_keyIdLookupDescriptor_t keyIdLookupList[] =
    {
      {
        /* Key identity data */
        { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x03 },
        0x01 /* 9 octets */
      }
    };

/* Key device list can be modified at run time */
static ApiMac_keyDeviceDescriptor_t keyDeviceList[] =
    {
      { 0x00, false, false },
      { 0x00, false, false },
      { 0x00, false, false },
      { 0x00, false, false },
      { 0x00, false, false },
      { 0x00, false, false },
      { 0x00, false, false },
      { 0x00, false, false }
    };

static const ApiMac_keyUsageDescriptor_t keyUsageList[] =
    {
      { MAC_FRAME_TYPE_DATA, MAC_DATA_REQ_FRAME }
    };

STATIC ApiMac_keyDescriptor_t keyTable[] =
    {
      {
        (ApiMac_keyIdLookupDescriptor_t *)keyIdLookupList,
        KEY_ID_LOOKUP_ENTRIES,
        (ApiMac_keyDeviceDescriptor_t *)keyDeviceList,
        KEY_DEVICE_TABLE_ENTRIES,
        (ApiMac_keyUsageDescriptor_t *)keyUsageList,
        KEY_USAGE_TABLE_ENTRIES,
        KEY_TABLE_DEFAULT_KEY,
        0 /* frame counter */
      }
    };

STATIC const ApiMac_securityPibSecurityLevelEntry_t securityLevelEntry =
    {
      0,
      { MAC_FRAME_TYPE_DATA, MAC_DATA_REQ_FRAME, 0, false }
    };

STATIC ApiMac_secLevel_t secLevel = ApiMac_secLevel_encMic32;

STATIC ApiMac_keyIdMode_t secKeyIdMode = ApiMac_keyIdMode_8;
STATIC uint8_t secKeyIndex = 3; /* cant be zero for implicit key identifier */

// STATIC bool macSecurity
#define macSecurity  CONFIG_SECURE

/******************************************************************************
 Local Function Prototypes
 *****************************************************************************/
/* CLLC callbacks */
static void assocIndCb(ApiMac_mlmeAssociateInd_t *pData);
static void beaconNotifyIndCb(ApiMac_mlmeBeaconNotifyInd_t *pData);
static void scanCnfCb(ApiMac_mlmeScanCnf_t *pData);
static void startCnfCb(ApiMac_mlmeStartCnf_t *pData);
static void disassocIndCb(ApiMac_mlmeDisassociateInd_t *pData);
static void wsAsyncIndCb(ApiMac_mlmeWsAsyncInd_t *pData);
static void dataIndCb(ApiMac_mcpsDataInd_t *pData);
static void orphanIndCb(ApiMac_mlmeOrphanInd_t *pData);

static void switchState(Cllc_coord_states_t newState);
static void processState(Cllc_coord_states_t state);
static void maintainAssocTable(ApiMac_deviceDescriptor_t *pDevInfo,
                               ApiMac_capabilityInfo_t *pCapInfo,
                               uint32_t reporting,
                               uint32_t polling,
                               int8_t rssi,
                               uint16_t status,
                               bool mode);
static void configureStartParam(uint8_t channel);

/* PAN decriptor list management functions */
static void addToPANList(ApiMac_panDesc_t *pData);
static void clearPANList(void);
static bool findInList(ApiMac_panDesc_t *pData);

/* Scan results parsing */
static uint8_t findBestChannel(uint8_t *pResults);
static uint16_t findChannel(uint16_t panID,uint8_t channel);

static Cllc_associated_devices_t *findDevice(uint16_t shortAddr);
static uint16_t findNumDevice(uint16_t shortAddr);
static void updateState(Cllc_states_t state);
static void sendAsyncReq(uint8_t frameType);
static void joinPermitExpired(void);
static void sendStartReq(bool startFH);
static void sendScanReq(ApiMac_scantype_t type);

// static uint16_t getUnuseShortAddr(void);

/******************************************************************************
 Public Functions
 *****************************************************************************/
/*!
 Initialize this module.

 Public function defined in cllc.h
 */
void Cllc_init(ApiMac_callbacks_t *pMacCbs, Cllc_callbacks_t *pCllcCbs)
{
    uint16_t panId = CONFIG_PAN_ID;
    /* Linux: initialize these at startup */
    memcpy( chanMask         , linux_CONFIG_CHANNEL_MASK, sizeof(chanMask));
    fhPAtrickleTime = CLLC_FH_MIN_TRICKLE;
    fhPCtrickleTime = CLLC_FH_MIN_TRICKLE;

    /* Initialize coordinator information variables */
    if((panId != 0x0000) && (panId != CLLC_INVALID_PAN))
    {
        coordInfoBlock.panID = panId;
    }
    else
    {
        /* PAN ID in the config file is invalid so we assign a random value */
        coordInfoBlock.panID = 0x0001;
    }

    coordInfoBlock.channel = CLLC_CHAN_LOWEST;
    coordInfoBlock.shortAddr = CONFIG_COORD_SHORT_ADDR;
    coordInfoBlock.currentCllcState = Cllc_states_initWaiting;
    coordInfoBlock.currentCoordState = Cllc_coordStates_initialized;

    /* Save MAC API callback */
    memcpy(&macCallbacksCopy, pMacCbs, sizeof(ApiMac_callbacks_t));
    pCllcCallbacksCopy = pCllcCbs;

    /* overwrite callbacks with llc callbacks */
    pMacCbs->pAssocIndCb = assocIndCb;
    pMacCbs->pBeaconNotifyIndCb = beaconNotifyIndCb;
    pMacCbs->pScanCnfCb = scanCnfCb;
    pMacCbs->pStartCnfCb = startCnfCb;
    pMacCbs->pDisassociateIndCb = disassocIndCb;
    pMacCbs->pWsAsyncIndCb = wsAsyncIndCb;
    pMacCbs->pDataIndCb = dataIndCb;
    pMacCbs->pOrphanIndCb = orphanIndCb;

    /* initialize association table */
    memset(Cllc_associatedDevList, 0xFF,
           (sizeof(Cllc_associated_devices_t) * CONFIG_MAX_DEVICES));

    ApiMac_mlmeSetReqBool(ApiMac_attribute_RxOnWhenIdle,true);

    if(!CONFIG_FH_ENABLE)
    {
        /* set PIB items */
        /* setup short address */
        ApiMac_mlmeSetReqUint16(ApiMac_attribute_shortAddress,
                                coordInfoBlock.shortAddr);
        /* initialize join permit timer clock */
        Csf_initializeJoinPermitClock();
    }
    else
    {
        uint8_t excludeChannels[APIMAC_154G_CHANNEL_BITMAP_SIZ];
        uint8_t sizeOfChannelMask, idx;
        uint8_t configChannelMask[APIMAC_154G_CHANNEL_BITMAP_SIZ];
        memcpy( configChannelMask, linux_CONFIG_FH_CHANNEL_MASK, sizeof(configChannelMask) );

        /* Always set association permit to 1 for FH */
        ApiMac_mlmeSetReqBool(ApiMac_attribute_associatePermit, true);
        /* initialize app clocks */
        Csf_initializeTrickleClock();
        /* set PIB to FH coordinator */
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_unicastChannelFunction, 2);
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_broadcastChannelFunction,
                                 2);
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_unicastDwellInterval,
                                 CONFIG_DWELL_TIME);
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_broadcastDwellInterval,
                                 CONFIG_DWELL_TIME);

        /* set Exclude Channels */
        sizeOfChannelMask = sizeof(configChannelMask)/sizeof(uint8_t);
        if(sizeOfChannelMask > APIMAC_154G_CHANNEL_BITMAP_SIZ)
        {
            sizeOfChannelMask = APIMAC_154G_CHANNEL_BITMAP_SIZ;
        }
        memset(excludeChannels, 0, APIMAC_154G_CHANNEL_BITMAP_SIZ);
        for(idx = 0; idx < sizeOfChannelMask; idx++)
        {
            excludeChannels[idx] = ~configChannelMask[idx];
        }
        ApiMac_mlmeSetFhReqArray(ApiMac_FHAttribute_unicastExcludedChannels,
                                 excludeChannels);
        ApiMac_mlmeSetFhReqArray(ApiMac_FHAttribute_broadcastExcludedChannels,
                                 excludeChannels);
    }
}

/*!
 Cllc task processing.

 Public function defined in cllc.h
 */
void Cllc_process(void)
{
    /* The LLC has an event */
    if(Cllc_events & CLLC_PA_EVT)
    {
        if(CONFIG_DOUBLE_TRICKLE_TIMER)
        {
            if((2 * fhPAtrickleTime) < CLLC_FH_MAX_TRICKLE)
            {
                fhPAtrickleTime = 2 * fhPAtrickleTime;
            }
            else if(((2 * fhPAtrickleTime) == CLLC_FH_MAX_TRICKLE)
                  || ((2 * fhPAtrickleTime) > CLLC_FH_MAX_TRICKLE))
            {
                fhPAtrickleTime = CLLC_FH_MAX_TRICKLE;
            }
            /* Reset trickle timer */
            Csf_setTrickleClock(fhPAtrickleTime, ApiMac_wisunAsyncFrame_advertisement);
        }
        else
        {
            fhPAtrickleTime = 0;
        }
        /* Process LLC Event */
        sendAsyncReq(ApiMac_wisunAsyncFrame_advertisement);

        /* Clear the event */
        Util_clearEvent(&Cllc_events, CLLC_PA_EVT);
    }

    /* The LLC has an PC event */
    if(Cllc_events & CLLC_PC_EVT)
    {
        if(CONFIG_DOUBLE_TRICKLE_TIMER)
        {
            if(2 * fhPCtrickleTime < CLLC_FH_MAX_TRICKLE)
            {
                fhPCtrickleTime = 2 * fhPCtrickleTime;
            }
            else if(((2 * fhPCtrickleTime) == CLLC_FH_MAX_TRICKLE)
                  || ((2 * fhPCtrickleTime) > CLLC_FH_MAX_TRICKLE))
            {
                fhPCtrickleTime = CLLC_FH_MAX_TRICKLE;
            }
            /* Reset trickle timer */
            Csf_setTrickleClock(fhPCtrickleTime,
                                ApiMac_wisunAsyncFrame_config);
        }
        else
        {
            fhPCtrickleTime = 0;
        }
        /* Process LLC Event */
        sendAsyncReq(ApiMac_wisunAsyncFrame_config);

        /* Clear the event */
        Util_clearEvent(&Cllc_events, CLLC_PC_EVT);
    }

    /* Process state change event */
    if(Cllc_events & CLLC_STATE_CHANGE_EVT)
    {
        /* Process LLC Event */
        processState(coordInfoBlock.currentCoordState);

        /* Clear the event */
        Util_clearEvent(&Cllc_events, CLLC_STATE_CHANGE_EVT);
    }

    /* Process join permit event */
    if(Cllc_events & CLLC_JOIN_EVT)
    {
        joinPermitExpired();

        /* Clear the event */
        Util_clearEvent(&Cllc_events, CLLC_JOIN_EVT);
    }
}

/*!
 Start network

 Public function defined in cllc.h
 */
void Cllc_startNetwork(void)
{
    /* update state */
    updateState(Cllc_states_startingCoordinator);

    if(!CONFIG_FH_ENABLE)
    {
        /*
         Start active scan request to determine channel and PAN ID
         for coordinator
         */
        switchState(Cllc_coordStates_scanActive);
    }
    else
    {
        /* start energy scan to determine channel for start req */
        switchState(Cllc_coordStates_scanActiveCnf);
    }
}

/*!
 Restore network

 Public function defined in cllc.h
 */
void Cllc_restoreNetwork(Llc_netInfo_t *pNetworkInfo, uint8_t numDevices,
                         Llc_deviceListItem_t *pDevList)
{
    uint8_t i = 0;

    /* set state */
    updateState(Cllc_states_initRestoringCoordinator);

    coordInfoBlock.panID = pNetworkInfo->devInfo.panID;

    /* Populate network info according to type of network */
    if(pNetworkInfo->fh == true)
    {
        ApiMac_mlmeSetReqArray(ApiMac_attribute_extendedAddress,
                               (uint8_t*)(pNetworkInfo->devInfo.extAddress));
    	ApiMac_mlmeSetReqUint16(ApiMac_attribute_shortAddress,
                                pNetworkInfo->devInfo.shortAddress);
    }
    else
    {
        coordInfoBlock.channel = pNetworkInfo->channel;

        ApiMac_mlmeSetReqUint16(ApiMac_attribute_shortAddress,
                                 pNetworkInfo->devInfo.shortAddress);
    }

    sendStartReq(pNetworkInfo->fh);

    /* repopulate association table */
    for(i = 0; i < numDevices; i++, pDevList++)
    {
        /* Add to association table */
        maintainAssocTable(&pDevList->devInfo, &pDevList->capInfo,
                            pDevList->reporting, pDevList->polling,
                            1, 0, (false));
    }
}

/*!
 Set Join Permit On or Off

 Public function defined in cllc.h
 */
ApiMac_status_t Cllc_setJoinPermit(uint32_t duration)
{
    uint32_t joinDuration = 0;
    if(duration > 0)
    {
        /*  set join permit */
        ApiMac_mlmeSetReqBool(ApiMac_attribute_associatePermit, true);
        updateState(Cllc_states_joiningAllowed);
        if(duration != CLLC_JOIN_PERMIT_ON)
        {
            /* set timer for duration */
            joinDuration = duration;
        }
    }
    else
    {
        /*  set join permit */
        ApiMac_mlmeSetReqBool(ApiMac_attribute_associatePermit, false);
        updateState(Cllc_states_joiningNotAllowed);
    }

    Csf_setJoinPermitClock(joinDuration);
    return (ApiMac_status_success);
}

ApiMac_status_t Cllc_setInterval(ApiMac_sAddrExt_t *pExtAddr, uint32_t reporting, uint32_t polling)
{
    uint16_t shortAddr = Csf_getDeviceShort(pExtAddr);
    if(shortAddr != CSF_INVALID_SHORT_ADDR)
    {
        Cllc_associated_devices_t *pItem;

        pItem = findDevice(shortAddr);
        if(pItem)
        {
            pItem->reporting = reporting;
            pItem->polling = polling;
            return (ApiMac_status_success);
        }
    }
    return (ApiMac_status_invalidAddress);
}

/*!
 Remove Device

 Public function defined in cllc.h
 */
void Cllc_removeDevice(ApiMac_sAddrExt_t *pExtAddr)
{
    int8_t i = 0;
    uint16_t shortAddr = Csf_getDeviceShort(pExtAddr);

    if(shortAddr != CSF_INVALID_SHORT_ADDR)
    {
        Cllc_associated_devices_t *pItem;

        pItem = findDevice(shortAddr);
        if(pItem)
        {
            for(i = 0; i < CONFIG_MAX_DEVICES; i++)
            {
                if(Cllc_associatedDevList[i].shortAddr == pItem->shortAddr)
                {
                    /* Clear the entry - delete */
                    memset(&Cllc_associatedDevList[i], 0xFF,
                           sizeof(Cllc_associated_devices_t));
                    /* remove from NV */
                    Csf_removeDeviceListItem(pExtAddr);
                }
            }
        }
    }
}

/*!
 Send disassociation request.

 Public function defined in cllc.h
 */
void Cllc_sendDisassociationRequest(void)
{
    ApiMac_mlmeDisassociateReq_t disassocReq;
    memset(&disassocReq, 0, sizeof(ApiMac_mlmeDisassociateReq_t));
    disassocReq.deviceAddress.addrMode = ApiMac_addrType_short;
    disassocReq.deviceAddress.addr.shortAddr = coordInfoBlock.shortAddr;
    disassocReq.devicePanId = coordInfoBlock.panID;
    disassocReq.disassociateReason = ApiMac_disassocateReason_coord;
    disassocReq.txIndirect = false;
    ApiMac_mlmeDisassociateReq(&disassocReq);
}

/*!
 Initialize the MAC Security

 Public function defined in cllc.h
 */
void Cllc_securityInit(uint32_t frameCounter)
{
    if(macSecurity == true)
    {
        ApiMac_secAddKeyInitFrameCounter_t secInfo;

        memset(&secInfo, 0, sizeof(ApiMac_secAddKeyInitFrameCounter_t));

        memcpy(secInfo.key, keyTable[0].key, APIMAC_KEY_MAX_LEN);
        secInfo.frameCounter = frameCounter;
        secInfo.replaceKeyIndex = 0;
        secInfo.newKeyFlag = true;
        secInfo.lookupDataSize = APIMAC_KEY_LOOKUP_LONG_LEN;
        memcpy(secInfo.lookupData, keyIdLookupList[0].lookupData,
               (APIMAC_MAX_KEY_LOOKUP_LEN));

        ApiMac_secAddKeyInitFrameCounter(&secInfo);
        ApiMac_mlmeSetSecurityReqArray(
                        ApiMac_securityAttribute_defaultKeySource,
                        (void *) Cllc_keySource);
        ApiMac_mlmeSetSecurityReqStruct(ApiMac_securityAttribute_keyTable,
                                        (void *) NULL);

        /* Write a security level entry to PIB */
        ApiMac_mlmeSetSecurityReqStruct(
                        ApiMac_securityAttribute_securityLevelEntry,
                        (void *)&securityLevelEntry);

        /* Set the MAC security */
        ApiMac_mlmeSetReqBool(ApiMac_attribute_securityEnabled, macSecurity);
    }
}

/*!
 Fill in the security structure

 Public function defined in cllc.h
 */
void Cllc_securityFill(ApiMac_sec_t *pSec)
{
    if(pSec)
    {
        memset(pSec, 0, sizeof(ApiMac_sec_t));

        if(macSecurity == true)
        {
            memcpy(pSec->keySource, keyIdLookupList[0].lookupData,
                   (APIMAC_KEY_SOURCE_MAX_LEN));
            pSec->securityLevel = secLevel;
            pSec->keyIdMode = secKeyIdMode;
            pSec->keyIndex = secKeyIndex;
        }
    }
}

/*!
 Check the security level against expected level

 Public function defined in cllc.h
 */
bool Cllc_securityCheck(ApiMac_sec_t *pSec)
{
    bool ret = false;

    if(macSecurity == true)
    {
        if(pSec)
        {
            if(pSec->securityLevel == secLevel)
            {
                ret = true;
            }
        }
    }
    else
    {
       ret = true;
    }

    return(ret);
}


/*!
 Add a device to the MAC security device table.

 Public function defined in cllc.h
 */
ApiMac_status_t Cllc_addSecDevice(uint16_t panID, uint16_t shortAddr,
                         ApiMac_sAddrExt_t *pExtAddr, uint32_t frameCounter)
{
    if(macSecurity == true)
    {
        ApiMac_secAddDevice_t device;
        uint8_t keyIndex = 0;

        memset(&device, 0, sizeof(ApiMac_secAddDevice_t));

        device.panID = panID;
        device.shortAddr = shortAddr;
        memcpy(device.extAddr, pExtAddr, sizeof(ApiMac_sAddrExt_t));
        device.frameCounter = frameCounter;

        device.exempt = false;

        /* get the key lookup information from the initial loaded key */
        device.keyIdLookupDataSize = keyIdLookupList[keyIndex].lookupDataSize;
        memcpy(device.keyIdLookupData, keyIdLookupList[keyIndex].lookupData,
               (APIMAC_MAX_KEY_LOOKUP_LEN));

        device.uniqueDevice = false;
        device.duplicateDevFlag = false;

        return(ApiMac_secAddDevice(&device));
    }
    else
    {
        return(ApiMac_status_success);
    }
}


/******************************************************************************
 Local Functions
 *****************************************************************************/
/*!
 * @brief       Switch to the new  coordinator state and set the event bit.
 *
 * @param       newState - next state of coordinator
 */
static void switchState(Cllc_coord_states_t newState)
{
    coordInfoBlock.currentCoordState = newState;
    Util_setEvent(&Cllc_events, CLLC_STATE_CHANGE_EVT);
}

/*!
 * @brief       Function to transition various states involved with scan request
 *               and start request before the coordinator is started.
 *
 * @param       state - current startup state of coordinator
 */
static void processState(Cllc_coord_states_t state)
{

    switch(state)
    {
        case Cllc_coordStates_scanActive:
            /* Active scan */
            sendScanReq(ApiMac_scantype_active);
            break;

        case Cllc_coordStates_scanActiveCnf:
            /* Energy detect scan */
            sendScanReq(ApiMac_scantype_energyDetect);
            break;

        case Cllc_coordStates_scanEdCnf:

            if(!CONFIG_FH_ENABLE)
            {
                /* check for duplicate PAN ID */
                configureStartParam(coordInfoBlock.channel);

                /* setup short address */
                ApiMac_mlmeSetReqUint16(ApiMac_attribute_shortAddress,
                                        coordInfoBlock.shortAddr);
            }
            sendStartReq(CONFIG_FH_ENABLE);
            break;

        case Cllc_coordStates_startCnf:
        {
            Llc_netInfo_t networkInfo = {{0}};

            /*
             device has started hence we do not need the
             saved PAN descriptors
             */
            clearPANList();

            memset(&networkInfo, 0, sizeof(Llc_netInfo_t));

            networkInfo.fh = CONFIG_FH_ENABLE;
            /* Setup basics */
            ApiMac_mlmeGetReqUint8(ApiMac_attribute_logicalChannel,
                                   &networkInfo.channel);
            ApiMac_mlmeGetReqUint16(ApiMac_attribute_panId,
                                    &networkInfo.devInfo.panID);
            ApiMac_mlmeGetReqArray(ApiMac_attribute_extendedAddress,
                                   (uint8_t*)&networkInfo.devInfo.extAddress);
            ApiMac_mlmeGetReqUint16(ApiMac_attribute_shortAddress,
                                    &networkInfo.devInfo.shortAddress);

            if(CONFIG_FH_ENABLE)
            {
                ApiMac_startFH();
                /* start trickle timer for PA */
                Csf_setTrickleClock(CLLC_FH_MIN_TRICKLE,
                                    ApiMac_wisunAsyncFrame_advertisement);
                /* start trickle timer for PC */
                Csf_setTrickleClock(CLLC_FH_MIN_TRICKLE + (CLLC_FH_MIN_TRICKLE/2),
                                    ApiMac_wisunAsyncFrame_config);
            }
            else
            {
                ApiMac_mlmeGetReqUint8(ApiMac_attribute_logicalChannel,
                                        &networkInfo.channel);
            }

            /* Inform the application of a start */
            if(pCllcCallbacksCopy && pCllcCallbacksCopy->pStartedCb)
            {
                pCllcCallbacksCopy->pStartedCb(&networkInfo);
            }

            /*  coordinator started , callback for start indication*/
            if(coordInfoBlock.currentCllcState ==
                            Cllc_states_initRestoringCoordinator)
            {
                updateState(Cllc_states_restored);
            }
            else
            {
                updateState(Cllc_states_started);
            }
        }
            break;

        default:
            break;
    }
}

/*!
 * @brief       Process  Beacon Notification callback.
 *
 * @param       pData - pointer MAC Beacon indication info
 */
static void beaconNotifyIndCb(ApiMac_mlmeBeaconNotifyInd_t *pData)
{
    /* check beacon type */
    if(pData->beaconType == ApiMac_beaconType_normal)
    {
        /* add received PAN descriptor to list */
        addToPANList(&pData->panDesc);
    }

    /* Callback to MAC API */
    if(macCallbacksCopy.pBeaconNotifyIndCb != NULL)
    {
        macCallbacksCopy.pBeaconNotifyIndCb(pData);
    }
}

/*!
 * @brief       Determine parameters to be used to send Start request from
 *              results of energy scan and active scan
 *
 * @param       channel - best channel to start the coordinator
 */
static void configureStartParam(uint8_t channel)
{
    /* parse incoming beacons to search if another
     *  coordinator exists with this PAN ID */
    while(coordInfoBlock.panID == (findChannel(coordInfoBlock.panID, channel)))
    {
        /* another coordinator with this PAN ID exists,
         *  so we change our PAN ID*/
        coordInfoBlock.panID = coordInfoBlock.panID + 1;
    }
}

/*!
 * @brief       Process  Scan Confirm  callback.
 *
 * @param       pData - pointer to Scan Confirm
 */
static void scanCnfCb(ApiMac_mlmeScanCnf_t *pData)
{
    if((pData->status == ApiMac_status_success) || (pData->status
                                                    == ApiMac_status_noBeacon))
    {
        if(pData->scanType == ApiMac_scantype_active)
        {
            switchState(Cllc_coordStates_scanActiveCnf);
        }
        else if(pData->scanType == ApiMac_scantype_energyDetect)
        {
            uint8_t *pEnergyRes = Csf_malloc(pData->resultListSize);
            if(pEnergyRes)
            {
                memcpy(pEnergyRes, pData->result.pEnergyDetect,
                       pData->resultListSize);
                coordInfoBlock.channel = findBestChannel(pEnergyRes);
                switchState(Cllc_coordStates_scanEdCnf);
                Csf_free(pEnergyRes);
            }
        }
    }
    else
    {
        switchState(Cllc_coordStates_scanActive);
    }

    if(macCallbacksCopy.pScanCnfCb != NULL)
    {
        macCallbacksCopy.pScanCnfCb(pData);
    }
}

/*!
 * @brief       Determine channels set in the channel mask
 *
 * @param       pData - Pointer to MAC start confirm
 */
static void startCnfCb(ApiMac_mlmeStartCnf_t *pData)
{
    if(pData->status == ApiMac_status_success)
    {
        switchState(Cllc_coordStates_startCnf);
    }
    else
    {
        switchState(Cllc_coordStates_scanActive);
    }

    if(macCallbacksCopy.pStartCnfCb)
    {
        macCallbacksCopy.pStartCnfCb(pData);
    }
}

/*!
 * @brief       Parse the energy scan results to determine the best channel
 *              for the coordinator
 *
 * @param       pResults -  pointer to energy scan results
 */
static uint8_t findBestChannel(uint8_t *pResults)
{
    uint8_t chan;
    uint8_t currEnergy;
    uint8_t lastEnergy = CLLC_MAX_ENERGY;
    uint8_t coordStartChan = 0;

    for(chan = 0; chan < APIMAC_154G_MAX_NUM_CHANNEL; chan++)
    {
        if(CLLC_IS_CHANNEL_MASK_SET(chanMask, chan))
        {
            currEnergy = pResults[chan];
            if(currEnergy < lastEnergy)
            {
                coordStartChan = chan;
                lastEnergy = currEnergy;
            }
        }
    }
    return (coordStartChan);
}

/*!
 * @brief       Add PAN descriptor to linked list
 *
 * @param       pData - pointer to PAN descriptor
 */
static void addToPANList(ApiMac_panDesc_t *pData)
{
    /* check for duplicate PAN ID before adding */
    if(findInList(pData) == false)
    {
        panDescList_t *pItem = Csf_malloc(sizeof(panDescList_t));
        if(pItem)
        {
            panDescList_t *pList = pPANDesclist;

            memcpy(&(pItem->panDescList), pData, sizeof(ApiMac_panDesc_t));
            pItem->pNext = NULL;

            if(pList == NULL)
            {
                /* list is null initially, add first entry */
                pPANDesclist = pItem;
            }
            else
            {
                /* find the end */
                while(pList->pNext != NULL)
                {
                    pList = pList->pNext;
                }

                /* add new item to the end of the list */
                pList->pNext = pItem;
            }
        }
    }
}

/*!
 * @brief       Remove all entries in PAN decriptor linked list
 */
static void clearPANList(void)
{
    panDescList_t* pCurrent = pPANDesclist;
    panDescList_t* pNext;

    while(pCurrent != NULL)
    {
        pNext = pCurrent->pNext;
        Csf_free(pCurrent);
        pCurrent = pNext;
    }
    pPANDesclist = NULL;
}

/*!
 * @brief       Check for duplicate PAN descriptor
 *
 * @param       pData - pointer to PAN descriptor structure
 *
 * @return      true if found, else false
 */
static bool findInList(ApiMac_panDesc_t *pData)
{
    /* search in list for duplicates */
    panDescList_t *pItem = pPANDesclist;
    while(pItem != NULL)
    {
        if((pItem->panDescList.logicalChannel == pData->logicalChannel)
           && (pItem->panDescList.coordPanId == pData->coordPanId))
        {
            return (true);
        }
        else
        {
            pItem = pItem->pNext;
        }
    }

    return (false);
}

/*!
 * @brief       Find the PAN descriptor corresponding to a given channel
 *              in the PAN descriptor list
 *
 * @param       panID - pan ID to search
 * @param       channel - channel to search
 *
 * @return      PAN ID for that channel
 */
static uint16_t findChannel(uint16_t panID, uint8_t channel)
{
    /* search in PAN descriptors list for that channel */
    panDescList_t* pItem = pPANDesclist;

    while(pItem != NULL)
    {
        if(pItem->panDescList.logicalChannel == channel &&
                    pItem->panDescList.coordPanId == panID)
        {
           return (pItem->panDescList.coordPanId);
        }

        pItem = pItem->pNext;
    }
    return (CLLC_PAN_NOT_FOUND);
}

/*!
 * @brief       Handle Cllc associate indication callback
 *
 * @param       pData - pointer to Associate Indication structure
 */
static void assocIndCb(ApiMac_mlmeAssociateInd_t *pData)
{
    /* Device joining callback */
    ApiMac_deviceDescriptor_t devInfo;
    ApiMac_mlmeAssociateRsp_t assocRsp;

    /* Setup the device information structure */
    memcpy(&devInfo.extAddress, &pData->deviceAddress, (APIMAC_SADDR_EXT_LEN));
    devInfo.panID = coordInfoBlock.panID;

    /* Check to see if the device exists */
    devInfo.shortAddress = Csf_getDeviceShort(&pData->deviceAddress);
    if(devInfo.shortAddress == CSF_INVALID_SHORT_ADDR)
    {
        /* New device, make a new short address */
        assocRsp.status = ApiMac_assocStatus_panAccessDenied;
#if 1
        Cllc_numOfDevices = findNumDevice(CSF_INVALID_SHORT_ADDR);

        bool error = false;
        if (Cllc_numOfDevices == CONFIG_MAX_DEVICES)
        {
            /* not found */
            error = true;
        }else{
            uint8_t shortAddr = Cllc_shortAddress + 1;
            int i;
            for (i = 1; i < (CONFIG_MAX_DEVICES + 1); i++)
            {
                if (shortAddr > CONFIG_MAX_DEVICES) {
                    shortAddr = 1;
                }

                Cllc_associated_devices_t *pItem;
                pItem = findDevice(shortAddr);
                if (pItem == NULL)
                {
                    devInfo.shortAddress = shortAddr;
                    Cllc_shortAddress = shortAddr;
                    break;
                }
                shortAddr++;
            }

            if(i == (CONFIG_MAX_DEVICES + 1))
            {
                /* not found */
                error = true;
            }
        }

#else
        devInfo.shortAddress = (uint16_t) (Cllc_numOfDevices
                        + CLLC_ASSOC_DEVICE_STARTING_NUMBER);
#endif
        if (error == true)
        {
            Cllc_numOfDevices = 0;
            devInfo.shortAddress = 0;
            assocRsp.status = ApiMac_assocStatus_panAtCapacity;
        } else {
            if(pCllcCallbacksCopy && pCllcCallbacksCopy->pDeviceJoiningCb)
            {
                /* callback for device joining */
                assocRsp.status = pCllcCallbacksCopy->pDeviceJoiningCb(&devInfo,
                                                 &pData->capabilityInformation);
            }

            if(assocRsp.status == ApiMac_assocStatus_success)
            {
                /* add to association table */
                maintainAssocTable(&devInfo, &pData->capabilityInformation, 
                                    CONFIG_REPORTING_INTERVAL, CONFIG_POLLING_INTERVAL,
                                    1, 0, (false));
            }
        }
    }
    else
    {
        /* Device already exists use the old short address */
        assocRsp.status = ApiMac_assocStatus_success;
    }

    /* Fill assoc rsp fields */
    memset(&assocRsp.sec, 0, sizeof(ApiMac_sec_t));
    memcpy(&assocRsp.deviceAddress, &devInfo.extAddress,
           (APIMAC_SADDR_EXT_LEN));
    assocRsp.assocShortAddress = devInfo.shortAddress;

    /* Send response back to the device */
    ApiMac_mlmeAssociateRsp(&assocRsp);

    if(macCallbacksCopy.pAssocIndCb != NULL)
    {
        /* pass back to MAC API */
        macCallbacksCopy.pAssocIndCb(pData);
    }
}

/*!
 * @brief       Handle Disassociate Confirm callback
 *
 * @param       pData - pointer to Disassociate Confirm structure
 */
static void disassocIndCb(ApiMac_mlmeDisassociateInd_t *pData)
{
    if(pData->disassociateReason == ApiMac_disassocateReason_device)
    {
        /* remove device from association table */
        Cllc_removeDevice(&pData->deviceAddress);
    }

    if(macCallbacksCopy.pDisassociateIndCb)
    {
        macCallbacksCopy.pDisassociateIndCb(pData);
    }
}

/*!
 * @brief       Function to add or update a device in association table
 *
 * @param       pDevInfo - pointer to device descriptor information structure
 * @param       pCapInfo - pointer to capability information of the device
 * @param       rssi     - RSSI value
 * @param       status   - status to indicate if the device is still
 *                         communicating with the coordinator
 * @param       mode     - mode false is add, mode true is update
 */
static void maintainAssocTable(ApiMac_deviceDescriptor_t *pDevInfo,
                               ApiMac_capabilityInfo_t *pCapInfo,
                               uint32_t reporting,
                               uint32_t polling,
                               int8_t rssi,
                               uint16_t status,
                               bool mode)
{
    if(mode == false)
    {
        /* table is not filled yet */
        if(Cllc_numOfDevices < CONFIG_MAX_DEVICES)
        {
            memset(&Cllc_associatedDevList[Cllc_numOfDevices], 0x00,
                   sizeof(Cllc_associated_devices_t));
            Cllc_associatedDevList[Cllc_numOfDevices].shortAddr = pDevInfo
                            ->shortAddress;
            memcpy(&Cllc_associatedDevList[Cllc_numOfDevices].capInfo, pCapInfo,
                   sizeof(ApiMac_capabilityInfo_t));
            Cllc_associatedDevList[Cllc_numOfDevices].rssi = rssi;
            Cllc_associatedDevList[Cllc_numOfDevices].status |= status;
            Cllc_associatedDevList[Cllc_numOfDevices].reporting = reporting;
            Cllc_associatedDevList[Cllc_numOfDevices].polling = polling;

            Cllc_numOfDevices++;

            LOG_printf(LOG_ALWAYS, "maintainAssocTable setInterval shortAddr:%d, reporting:%d, polling:%d\n", 
                                          pDevInfo->shortAddress, reporting, polling);
        }
        else
        {
            /* find a blank spot from a removed device in the table */
            Cllc_associated_devices_t *pItem;

            /* look for an empty slot */
            pItem = findDevice(CSF_INVALID_SHORT_ADDR);

            if(pItem != NULL)
            {
                memset(pItem, 0, sizeof(Cllc_associated_devices_t));

                /* insert one of the blank spaces in the table */
                pItem->shortAddr = pDevInfo->shortAddress;
                memcpy(&pItem->capInfo, pCapInfo,
                       sizeof(ApiMac_capabilityInfo_t));
                pItem->rssi = rssi;
                pItem->status |= status;
            }
        }
    }
    else if(mode == true)
    {
        uint16_t shortAddr = Csf_getDeviceShort(&pDevInfo->extAddress);
        if(shortAddr != CSF_INVALID_SHORT_ADDR)
        {
            static Cllc_associated_devices_t *pItem;

            pItem = findDevice(shortAddr);
            if(pItem != NULL)
            {
                pItem->rssi = rssi;
                pItem->status |= status;
            }
        }
    }
}

/*!
 * @brief       callback for Async indication
 *
 * @param       pData - pointer to Async indication structure
 */
static void wsAsyncIndCb(ApiMac_mlmeWsAsyncInd_t *pData)
{
    ApiMac_status_t status;
    ApiMac_payloadIeRec_t *pPayloadGroupRec = NULL;
    uint8_t netname[32];
    bool netNameIEFound = false;

        /* Parse group IEs */
        status = ApiMac_parsePayloadGroupIEs(pData->pPayloadIE, pData->payloadIeLen,
                                             &pPayloadGroupRec);

        if((status == ApiMac_status_success) && (pPayloadGroupRec != NULL))
        {
            ApiMac_payloadIeRec_t *pGroup = pPayloadGroupRec;

            while(pGroup != NULL)
            {
                if(pGroup->item.ieId == ApiMac_payloadIEGroup_WiSUN)
                {
                    ApiMac_payloadIeRec_t *pPayloadSubRec = NULL;

                    status = ApiMac_parsePayloadSubIEs(pGroup->item.pIEContent,
                                                       pGroup->item.ieContentLen,
                                                       &pPayloadSubRec);
                    if((status == ApiMac_status_success) && (pPayloadSubRec!= NULL))
                    {
                        ApiMac_payloadIeRec_t *pSubGroup = pPayloadSubRec;

                        while(pSubGroup != NULL)
                        {
                            uint8_t *pIEContent = pSubGroup->item.pIEContent;

                            switch(pSubGroup->item.ieId)
                            {
                                case ApiMac_wisunSubIE_netNameIE:
                                    if(pSubGroup->item.ieContentLen <=
                                        APIMAC_FH_NET_NAME_SIZE_MAX)
                                    {
                                        memset(&netname, 0, APIMAC_FH_NET_NAME_SIZE_MAX);
                                        memcpy(&netname, pIEContent, pSubGroup->item.ieContentLen);
                                        netNameIEFound = true;
                                    }
                                    break;
                                default:
                                    break;
                            }

                            if(netNameIEFound)
                            {
                                break;
                            }
                            /* move to the next item*/
                            pSubGroup = pSubGroup->pNext;
                        }

                      /* Free the IE List allocated by ApiMac_parsePayloadSubIEs() */
                        ApiMac_freeIEList(pPayloadSubRec);
                    }
                }

                /* Move to next item*/
                pGroup = pGroup->pNext;
            }

             /* Free the IE List allocated by ApiMac_parsePayloadGroupIEs() */
            ApiMac_freeIEList(pPayloadGroupRec);
        }

    if((!netNameIEFound) ||
       (memcmp(netname, fhNetname, APIMAC_FH_NET_NAME_SIZE_MAX) != 0))
    {
        /* Drop PAS and PCS from other networks */
        return;
    }

    if(pData->fhFrameType == ApiMac_fhFrameType_panAdvertSolicit)
    {
        /* PAS is received , increment statistics */
        Cllc_statistics.fhNumPASolicitReceived++;
        if (fhPAtrickleTime != CLLC_FH_MIN_TRICKLE)
        {
            fhPAtrickleTime = CLLC_FH_MIN_TRICKLE;
            /* reset trickle timer */
            Csf_setTrickleClock(fhPAtrickleTime,
                                ApiMac_wisunAsyncFrame_advertisement);
        }
        /* Add EUI to security device table */
        Cllc_addSecDevice(pData->srcPanId, pData->srcAddr.addr.shortAddr,
                          &(pData->srcAddr.addr.extAddr), pData->frameCntr);
    }
    else if(pData->fhFrameType == ApiMac_fhFrameType_configSolicit)
    {
        /* PCS is received , increment statistics */
        Cllc_statistics.fhNumPANConfigSolicitsReceived++;
        if (fhPCtrickleTime != CLLC_FH_MIN_TRICKLE)
        {
            fhPCtrickleTime = CLLC_FH_MIN_TRICKLE;
            /* reset trickle timer */
            Csf_setTrickleClock(fhPCtrickleTime,
                                ApiMac_wisunAsyncFrame_config);
        }
        /* Add EUI to security device table */
        Cllc_addSecDevice(pData->srcPanId, pData->srcAddr.addr.shortAddr,
                          &(pData->srcAddr.addr.extAddr), pData->frameCntr);
    }

    if(macCallbacksCopy.pWsAsyncIndCb != NULL)
    {
        macCallbacksCopy.pWsAsyncIndCb(pData);
    }
}

/*!
 * @brief       Process Data indication callback
 *
 * @param       pData - pointer to data indication callback structure
 */

static void dataIndCb(ApiMac_mcpsDataInd_t *pData)
{
    Csf_updateFrameCounter(&pData->srcAddr, pData->frameCntr);

    if(macCallbacksCopy.pDataIndCb != NULL)
    {
        macCallbacksCopy.pDataIndCb(pData);
    }
}

/*!
 * @brief      Find the associated device table entry matching an
 *             extended address.
 *
 * @param      shortAddr - device's short address
 *
 * @return     pointer to the associated device table entry,
 *             NULL if not found.
 */
static Cllc_associated_devices_t *findDevice(uint16_t shortAddr)
{
    int x;

    for(x = 0; (x < CONFIG_MAX_DEVICES); x++)
    {
        /* Make sure the entry is valid. */
        if(shortAddr == Cllc_associatedDevList[x].shortAddr)
        {
            return (&Cllc_associatedDevList[x]);
        }
    }
    return (NULL);
}

static uint16_t findNumDevice(uint16_t shortAddr)
{
    int x;

    for(x = 0; (x < CONFIG_MAX_DEVICES); x++)
    {
        /* Make sure the entry is valid. */
        if(shortAddr == Cllc_associatedDevList[x].shortAddr)
        {
            return x;
        }
    }
    return (CONFIG_MAX_DEVICES);
}

// static uint16_t getUnuseShortAddr(void)
// {
//     int find;
//     int shortAddr = 1;
//     int x;
//     for(x = 0; (x < CONFIG_MAX_DEVICES); x++)
//     {
//         find = false;

//         for(x = 0; (x < CONFIG_MAX_DEVICES); x++)
//         {
//              Make sure the entry is valid. 
//             if(shortAddr == Cllc_associatedDevList[x].shortAddr)
//             {
//                 find = true;
//                 break;
//             }
//         }

//         if (find == false) {
//             return shortAddr;
//         }
//         shortAddr++;
//     }
//     return 0;
// }

/*!
 * @brief       Process Orphan indication callback
 *
 * @param       pData - pointer to orphan indication callback structure
 */
static void orphanIndCb(ApiMac_mlmeOrphanInd_t *pData)
{
    Llc_deviceListItem_t item;
    ApiMac_sAddr_t devAddr;
    devAddr.addrMode = ApiMac_addrType_extended;
    memcpy(&devAddr.addr.extAddr, &pData->orphanAddress,
           sizeof(ApiMac_sAddrExt_t));

    if(Csf_getDevice(&devAddr, &item))
    {
        ApiMac_mlmeOrphanRsp_t orphanRsp;

        /* send orphan response */
        memcpy(&orphanRsp.orphanAddress, &item.devInfo.extAddress,
               (APIMAC_SADDR_EXT_LEN));
        memset(&orphanRsp.sec, 0, sizeof(ApiMac_sec_t));
        orphanRsp.associatedMember = true;
        orphanRsp.shortAddress = item.devInfo.shortAddress;
        ApiMac_mlmeOrphanRsp(&orphanRsp);
        maintainAssocTable(&item.devInfo, &item.capInfo, 
                            item.reporting, item.polling,
                            1, 0, (true));
    }

    if(macCallbacksCopy.pOrphanIndCb != NULL)
    {
        macCallbacksCopy.pOrphanIndCb(pData);
    }
}

/*!
 * @brief       Update Cllc state
 *
 * @param       state - new state
 */
static void updateState(Cllc_states_t state)
{
    coordInfoBlock.currentCllcState = state;
    if(pCllcCallbacksCopy && pCllcCallbacksCopy->pStateChangeCb)
    {
        /* state change callback */
        pCllcCallbacksCopy->pStateChangeCb(coordInfoBlock.currentCllcState);
    }
}

/*!
 * @brief       Send Async Frame for trickle timer
 *
 * @param       frameType - type of frame to send
 */
static void sendAsyncReq(uint8_t frameType)
{
    ApiMac_mlmeWSAsyncReq_t asyncReq;
    uint8_t sizeOfChannelMask;
    uint8_t asyncChannelMask[APIMAC_154G_CHANNEL_BITMAP_SIZ];
    memcpy( asyncChannelMask, linux_FH_ASYNC_CHANNEL_MASK, sizeof(asyncChannelMask));

    /* set of Exclude Channels */
    sizeOfChannelMask = sizeof(asyncChannelMask)/sizeof(uint8_t);
    if(sizeOfChannelMask > APIMAC_154G_CHANNEL_BITMAP_SIZ)
    {
        sizeOfChannelMask = APIMAC_154G_CHANNEL_BITMAP_SIZ;
    }
    memset(asyncReq.channels, 0, (APIMAC_154G_CHANNEL_BITMAP_SIZ));

    asyncReq.operation = ApiMac_wisunAsycnOperation_start;
    memcpy(asyncReq.channels, asyncChannelMask, sizeOfChannelMask);

    /* send PA or PC according to frame type */
    if(frameType == ApiMac_wisunAsyncFrame_advertisement)
    {
        /* set necessary PIB values for PA */
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_routingCost,
                                 (CLLC_FH_ROUTING_COST));
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_routingMethod,
                                 (CLLC_FH_ROUTING_METHOD));
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_eapolReady,
                                 (CLLC_FH_EAPOL_READY));
        ApiMac_mlmeSetFhReqUint8(ApiMac_FHAttribute_fanTPSVersion,
                                 (CLLC_FH_FANTPSVERSION));
        ApiMac_mlmeSetFhReqArray(ApiMac_FHAttribute_netName,&fhNetname[0]);
        ApiMac_mlmeSetFhReqUint16(ApiMac_FHAttribute_panSize,
                                  (CONFIG_FH_PAN_SIZE));

        /* Fill in the information for async request */
        asyncReq.frameType = ApiMac_wisunAsyncFrame_advertisement;

        /* no security for PA */
        memset(&asyncReq.sec, 0, sizeof(ApiMac_sec_t));

        Cllc_statistics.fhNumPASent++;

    }
    else if(frameType == ApiMac_wisunAsyncFrame_config)
    {
        /* set necessary PIB values for PA*/
        ApiMac_mlmeSetFhReqUint16(ApiMac_FHAttribute_panVersion,
                                  (CLLC_FH_PANVERSION));
        ApiMac_mlmeSetFhReqArray(ApiMac_FHAttribute_gtk0Hash,
                                 fhGtkHash0);
        ApiMac_mlmeSetFhReqArray(ApiMac_FHAttribute_gtk1Hash,
                                 fhGtkHash1);
        ApiMac_mlmeSetFhReqArray(ApiMac_FHAttribute_gtk2Hash,
                                 fhGtkHash2);
        ApiMac_mlmeSetFhReqArray(ApiMac_FHAttribute_gtk3Hash,
                                 fhGtkHash3);

        /* Fill in the information for async request */
        asyncReq.operation = ApiMac_wisunAsycnOperation_start;
        asyncReq.frameType = ApiMac_wisunAsyncFrame_config;

        /* security for PC */
        Cllc_securityFill(&asyncReq.sec);

        Cllc_statistics.fhNumPANConfigSent++;

    }
    ApiMac_mlmeWSAsyncReq(&asyncReq);
}

/*!
 * @brief       Handle join permit duration expiry
 */
static void joinPermitExpired(void)
{
    /* set join permit to false */
    ApiMac_mlmeSetReqBool(ApiMac_attribute_associatePermit, false);
    updateState(Cllc_states_joiningNotAllowed);
}

/*!
 * @brief       Send Start Request
 *
 * @param       startFH - true if FH enable else false
 */
static void sendStartReq(bool startFH)
{
    ApiMac_mlmeStartReq_t startReq;
    memset(&startReq, 0, sizeof(ApiMac_mlmeStartReq_t));

    /* read network parameters fill them in start req */
    startReq.startTime = 0;
    startReq.panId = coordInfoBlock.panID;
    startReq.logicalChannel = coordInfoBlock.channel;
    startReq.channelPage = CONFIG_CHANNEL_PAGE;
    startReq.phyID = CONFIG_PHY_ID;
    startReq.beaconOrder = CONFIG_MAC_BEACON_ORDER;
    startReq.superframeOrder = CONFIG_MAC_SUPERFRAME_ORDER;
    startReq.panCoordinator = true;
    startReq.batteryLifeExt = false;
    startReq.coordRealignment = false;
    startReq.realignSec.securityLevel = false;
    startReq.startFH = startFH;
    startReq.mpmParams.offsetTimeSlot = CLLC_OFFSET_TIMESLOT;
    startReq.mpmParams.eBeaconOrder = CLLC_EBEACONORDER;
    startReq.mpmParams.NBPANEBeaconOrder = CLLC_NBPANEBEACONORDER;
    startReq.mpmParams.pIEIDs = NULL;

    /* send Start Req to MAC API */
    ApiMac_mlmeStartReq(&startReq);
}

/*!
 * @brief       Send scan request
 *
 * @param       type - type of scan: active ,passive or orphan
 */
static void sendScanReq(ApiMac_scantype_t type)
{
    ApiMac_mlmeScanReq_t scanReq;
    /* set common parameters for all scans */
    memset(&scanReq, 0, sizeof(ApiMac_mlmeScanReq_t));
    /* set scan channels from channel mask*/
    memcpy(scanReq.scanChannels, chanMask,
    APIMAC_154G_CHANNEL_BITMAP_SIZ);
    scanReq.scanType = type;
    scanReq.scanDuration = CONFIG_SCAN_DURATION;
    scanReq.maxResults = 0;/* Expecting beacon notifications */
    scanReq.permitJoining = false;
    scanReq.linkQuality = CONFIG_LINKQUALITY;
    scanReq.percentFilter = CONFIG_PERCENTFILTER;
    scanReq.channelPage = CONFIG_CHANNEL_PAGE;
    scanReq.phyID = CONFIG_PHY_ID;
    /* using no security for scan request command */
    memset(&scanReq.sec, 0, sizeof(ApiMac_sec_t));
    /* send scan Req */
    ApiMac_mlmeScanReq(&scanReq);
}

/*
 *  ========================================
 *  Texas Instruments Micro Controller Style
 *  ========================================
 *  Local Variables:
 *  mode: c
 *  c-file-style: "bsd"
 *  tab-width: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 *  End:
 *  vim:set  filetype=c tabstop=4 shiftwidth=4 expandtab=true
 */

