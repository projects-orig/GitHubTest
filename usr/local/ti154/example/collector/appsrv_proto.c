/******************************************************************************
 @file appsrv_proto.c

 @brief TIMAC 2.0 API convert appsrv structures to wire line protocol

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

#include "malloc.h"
#include "compiler.h"
#include "api_mac.h"
#include "api_mac.pb-c.h"

#include "appsrv.h"
#include "log.h"

#include "llc.h"
#include "csf.h"
#include "csf_linux.h"
#include "csf_proto.h"
#include "llc.pb-c.h"
#include "llc_proto.h"
#include "smsgs.h"
#include "smsgs_proto.h"
#include "api_mac_proto.h"
#include "appsrv_proto.h"
#include "config.h"
#include "malloc.h"
#include "stdbool.h"

/* Local Functions */
CllcStates appsrv_getCllcProtoState(Cllc_states_t state);

void free_AppsrvTxDataCnf(AppsrvTxDataCnf *pThis)
{
    if(pThis)
    {
        free(pThis);
        pThis = NULL;
    }
}

AppsrvTxDataCnf *copy_AppsrvTxDataCnf(uint8_t status)
{
    AppsrvTxDataCnf *pAnswer;

    pAnswer = (AppsrvTxDataCnf *)calloc(1,sizeof(*pAnswer));
    if(pAnswer == NULL)
    {
        LOG_printf(LOG_ERROR, "No memory for: AppsrvTxDataCnf\n");
        return pAnswer;
    }

    appsrv_tx_data_cnf__init(pAnswer);
    pAnswer->status = status;

    return pAnswer;
}

void free_AppsrvDeviceDataRxInd(AppsrvDeviceDataRxInd *pThis)
{
    if(pThis)
    {
        if(pThis->sconfigmsg)
        {
            free_SmsgsConfigRspMsg(pThis->sconfigmsg);
            pThis->sconfigmsg = NULL;
        }
        if(pThis->sdatamsg)
        {
            free_SmsgsSensorMsg(pThis->sdatamsg);
            pThis->sdatamsg = NULL;
        }
        
        if(pThis->srcaddr)
        {
            free_ApiMacSAddr(pThis->srcaddr);
            pThis->srcaddr = NULL;
        }
        free(pThis);
        pThis = NULL;
    }
}

AppsrvDeviceDataRxInd *copy_AppsrvDeviceDataRxInd(ApiMac_sAddr_t *pAddr,
                                                  int32_t rssi,
                                                  Smsgs_sensorMsg_t *pSensorMsg,
                                                  Smsgs_configRspMsg_t *pCfgRspMsg)
{
    AppsrvDeviceDataRxInd *pAnswer;

    pAnswer = (AppsrvDeviceDataRxInd *)calloc(1, sizeof(*pAnswer));
    if(pAnswer == NULL)
    {
        LOG_printf(LOG_ERROR, "No memory for: AppsrvDeviceDataRxInd\n");
        return pAnswer;
    }

    appsrv_device_data_rx_ind__init(pAnswer);

    pAnswer->rssi = rssi;
    LOG_printf(LOG_APPSRV_MSG_CONTENT,
               "RSSI copied for proto transfer is %i \n",
               pAnswer->rssi);
    pAnswer->srcaddr = copy_ApiMac_sAddr(pAddr);
    if(pAnswer->srcaddr == NULL)
    {
    fail:
        free_AppsrvDeviceDataRxInd(pAnswer);
        pAnswer = NULL;
        return pAnswer;
    }

    if(pCfgRspMsg)
    {
        pAnswer->sconfigmsg = copy_Smsgs_configRspMsg(pCfgRspMsg);
        if(pAnswer->sconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if(pSensorMsg)
    {
        pAnswer->sdatamsg = copy_Smsgs_sensorMsg(pSensorMsg);
        if(pAnswer->sdatamsg == NULL)
        {
            goto fail;
        }
    }
    
    return pAnswer;
}

/*! =============================================================================== */
void free_AppsrvDeviceResponseDataRxInd(AppsrvDeviceDataRxInd *pThis)
{
    if(pThis)
    {
        if(pThis->salarmconfigmsg)
        {
            free_SmsgsAlarmConfigRspMsg(pThis->salarmconfigmsg);
            pThis->salarmconfigmsg = NULL;
        }
        if(pThis->sgsensorconfigmsg)
        {
            free_SmsgsGSensorConfigRspMsg(pThis->sgsensorconfigmsg);
            pThis->sgsensorconfigmsg = NULL;
        }
        if(pThis->selectriclockconfigmsg)
        {
            free_SmsgsElectricLockConfigRspMsg(pThis->selectriclockconfigmsg);
            pThis->selectriclockconfigmsg = NULL;
        }
        if(pThis->ssignalconfigmsg)
        {
            free_SmsgsSignalConfigRspMsg(pThis->ssignalconfigmsg);
            pThis->ssignalconfigmsg = NULL;
        }
        if(pThis->stempconfigmsg)
        {
            free_SmsgsTempConfigRspMsg(pThis->stempconfigmsg);
            pThis->stempconfigmsg = NULL;
        }
        if(pThis->slowbatteryconfigmsg)
        {
            free_SmsgsLowBatteryConfigRspMsg(pThis->slowbatteryconfigmsg);
            pThis->slowbatteryconfigmsg = NULL;
        }
        if(pThis->sdistanceconfigmsg)
        {
            free_SmsgsDistanceConfigRspMsg(pThis->sdistanceconfigmsg);
            pThis->sdistanceconfigmsg = NULL;
        }
        if(pThis->smusicconfigmsg)
        {
            free_SmsgsMusicConfigRspMsg(pThis->smusicconfigmsg);
            pThis->smusicconfigmsg = NULL;
        }
        if(pThis->sintervalconfigmsg)
        {
            free_SmsgsIntervalConfigRspMsg(pThis->sintervalconfigmsg);
            pThis->sintervalconfigmsg = NULL;
        }
        if(pThis->smotionconfigmsg)
        {
            free_SmsgsMotionConfigRspMsg(pThis->smotionconfigmsg);
            pThis->smotionconfigmsg = NULL;
        }
        if(pThis->sresistanceconfigmsg)
        {
            free_SmsgsResistanceConfigRspMsg(pThis->sresistanceconfigmsg);
            pThis->sresistanceconfigmsg = NULL;
        }
        if(pThis->smicrowaveconfigmsg)
        {
            free_SmsgsMicrowaveConfigRspMsg(pThis->smicrowaveconfigmsg);
            pThis->smicrowaveconfigmsg = NULL;
        }
        if(pThis->spirconfigmsg)
        {
            free_SmsgsPirConfigRspMsg(pThis->spirconfigmsg);
            pThis->spirconfigmsg = NULL;
        }
        if(pThis->ssetunsetconfigmsg)
        {
            free_SmsgsSetUnsetConfigRspMsg(pThis->ssetunsetconfigmsg);
            pThis->ssetunsetconfigmsg = NULL;
        }
        if(pThis->sdisconnectmsg)
        {
            free_SmsgsDisconnectRspMsg(pThis->sdisconnectmsg);
            pThis->sdisconnectmsg = NULL;
        }
        if(pThis->selectriclockactionmsg)
        {
            free_SmsgsElectricLockActionRspMsg(pThis->selectriclockactionmsg);
            pThis->selectriclockactionmsg = NULL;
        }
        
        if(pThis->srcaddr)
        {
            free_ApiMacSAddr(pThis->srcaddr);
            pThis->srcaddr = NULL;
        }
        free(pThis);
        pThis = NULL;
    }
}

AppsrvDeviceDataRxInd *copy_AppsrvDeviceResponseDataRxInd(ApiMac_sAddr_t *pAddr,
                                                  int32_t rssi,
                                                  Smsgs_alarmConfigRspMsg_t *pAlarmCfgRspMsg,
                                                  Smsgs_gSensorConfigRspMsg_t *pGSensorCfgRspMsg,
                                                  Smsgs_electricLockConfigRspMsg_t *pElectricCfgRspMsg,
                                                  Smsgs_signalConfigRspMsg_t *pSignalCfgRspMsg,
                                                  Smsgs_tempConfigRspMsg_t *pTempCfgRspMsg,
                                                  Smsgs_lowBatteryConfigRspMsg_t *pLowBatteryCfgRspMsg,
                                                  Smsgs_distanceConfigRspMsg_t *pDistanceCfgRspMsg,
                                                  Smsgs_musicConfigRspMsg_t *pMusicCfgRspMsg,
                                                  Smsgs_intervalConfigRspMsg_t *pIntervalCfgRspMsg,
                                                  Smsgs_motionConfigRspMsg_t *pMotionCfgRspMsg,
                                                  Smsgs_resistanceConfigRspMsg_t *pResistanceCfgRspMsg,
                                                  Smsgs_microwaveConfigRspMsg_t *pMicrowaveCfgRspMsg,
                                                  Smsgs_pirConfigRspMsg_t *pPirCfgRspMsg,
                                                  Smsgs_setUnsetConfigRspMsg_t *pSetUnsetCfgRspMsg,
                                                  Smsgs_disconnectRspMsg_t *pDisconnectRspMsg,
                                                  Smsgs_electricLockActionRspMsg_t *pElectricActRspMsg)
{
    AppsrvDeviceDataRxInd *pAnswer;

    pAnswer = (AppsrvDeviceDataRxInd *)calloc(1, sizeof(*pAnswer));
    if(pAnswer == NULL)
    {
        LOG_printf(LOG_ERROR, "No memory for: AppsrvDeviceDataRxInd\n");
        return pAnswer;
    }

    appsrv_device_data_rx_ind__init(pAnswer);

    pAnswer->rssi = rssi;
    LOG_printf(LOG_APPSRV_MSG_CONTENT,
               "RSSI copied for proto transfer is %i \n",
               pAnswer->rssi);
    pAnswer->srcaddr = copy_ApiMac_sAddr(pAddr);
    if(pAnswer->srcaddr == NULL)
    {
    fail:
        free_AppsrvDeviceResponseDataRxInd(pAnswer);
        pAnswer = NULL;
        return pAnswer;
    }

    if (pAlarmCfgRspMsg)
    {
        pAnswer->salarmconfigmsg = copy_Smsgs_alarmConfigRspMsg(pAlarmCfgRspMsg);
        if(pAnswer->salarmconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pGSensorCfgRspMsg)
    {
        pAnswer->sgsensorconfigmsg = copy_Smsgs_gSensorConfigRspMsg(pGSensorCfgRspMsg);
        if(pAnswer->sgsensorconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pElectricCfgRspMsg)
    {
        pAnswer->selectriclockconfigmsg = copy_Smsgs_electricLockConfigRspMsg(pElectricCfgRspMsg);
        if(pAnswer->selectriclockconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pSignalCfgRspMsg)
    {
        pAnswer->ssignalconfigmsg = copy_Smsgs_signalConfigRspMsg(pSignalCfgRspMsg);
        if(pAnswer->ssignalconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pTempCfgRspMsg)
    {
        pAnswer->stempconfigmsg = copy_Smsgs_tempConfigRspMsg(pTempCfgRspMsg);
        if(pAnswer->stempconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pLowBatteryCfgRspMsg)
    {
        pAnswer->slowbatteryconfigmsg = copy_Smsgs_lowBatteryConfigRspMsg(pLowBatteryCfgRspMsg);
        if(pAnswer->slowbatteryconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pDistanceCfgRspMsg)
    {
        pAnswer->sdistanceconfigmsg = copy_Smsgs_distanceConfigRspMsg(pDistanceCfgRspMsg);
        if(pAnswer->sdistanceconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pMusicCfgRspMsg)
    {
        pAnswer->smusicconfigmsg = copy_Smsgs_musicConfigRspMsg(pMusicCfgRspMsg);
        if(pAnswer->smusicconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pIntervalCfgRspMsg)
    {
        pAnswer->sintervalconfigmsg = copy_Smsgs_intervalConfigRspMsg(pIntervalCfgRspMsg);
        if(pAnswer->sintervalconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pMotionCfgRspMsg)
    {
        pAnswer->smotionconfigmsg = copy_Smsgs_motionConfigRspMsg(pMotionCfgRspMsg);
        if(pAnswer->smotionconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pResistanceCfgRspMsg)
    {
        pAnswer->sresistanceconfigmsg = copy_Smsgs_resistanceConfigRspMsg(pResistanceCfgRspMsg);
        if(pAnswer->sresistanceconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pMicrowaveCfgRspMsg)
    {
        pAnswer->smicrowaveconfigmsg = copy_Smsgs_microwaveConfigRspMsg(pMicrowaveCfgRspMsg);
        if(pAnswer->smicrowaveconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pPirCfgRspMsg)
    {
        pAnswer->spirconfigmsg = copy_Smsgs_pirConfigRspMsg(pPirCfgRspMsg);
        if(pAnswer->spirconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pSetUnsetCfgRspMsg)
    {
        pAnswer->ssetunsetconfigmsg = copy_Smsgs_setUnsetConfigRspMsg(pSetUnsetCfgRspMsg);
        if(pAnswer->ssetunsetconfigmsg == NULL)
        {
            goto fail;
        }
    }
    if (pDisconnectRspMsg)
    {
        pAnswer->sdisconnectmsg = copy_Smsgs_disconnectRspMsg(pDisconnectRspMsg);
        if(pAnswer->sdisconnectmsg == NULL)
        {
            goto fail;
        }
    }
    if (pElectricActRspMsg)
    {
        pAnswer->selectriclockactionmsg = copy_Smsgs_electricLockActionRspMsg(pElectricActRspMsg);
        if(pAnswer->selectriclockactionmsg == NULL)
        {
            goto fail;
        }
    }
    
    return pAnswer;
}

CllcStates appsrv_getCllcProtoState(Cllc_states_t state)
{

    if(state == Cllc_states_initWaiting)
    {
        return CLLC_STATES__Cllc_states_initWaiting;
    }
    else if(state == Cllc_states_startingCoordinator)
    {
        return CLLC_STATES__Cllc_states_startingCoordinator;
    }
    else if(state == Cllc_states_initRestoringCoordinator)
    {
        return CLLC_STATES__Cllc_states_initRestoringCoordinator;
    }
    else if(state == Cllc_states_started)
    {
        return CLLC_STATES__Cllc_states_started;
    }
    else if(state == Cllc_states_restored)
    {
        return CLLC_STATES__Cllc_states_restored;
    }
    else if(state == Cllc_states_joiningAllowed)
    {
        return CLLC_STATES__Cllc_states_joiningAllowed;
    }
    else if(state == Cllc_states_joiningNotAllowed)
    {
        return CLLC_STATES__Cllc_states_joiningNotAllowed;
    }

    /* We should not reach here, as all states are handled above */
    LOG_printf(LOG_ERROR,
               "ERROR: Asked to translate illegate cllc state to proto state\n");
    return 0;
}

void free_AppsrvNwkParam(AppsrvNwkParam *pThis)
{
    if(pThis)
    {
        free_LlcNetInfo(pThis->nwkinfo);
        pThis->nwkinfo = NULL;
    }
}

AppsrvNwkParam *copy_AppsrvNwkParam(Llc_netInfo_t *pNetInfo,
                                    int32_t securityenabled)
{
    AppsrvNwkParam *pAnswer;
    bool b;
    Cllc_states_t state;

    pAnswer = calloc(1, sizeof(*pAnswer));
    if(pAnswer == NULL)
    {
        return pAnswer;
    }

    appsrv_nwk_param__init(pAnswer);
    b = CONFIG_FH_ENABLE;
    if(b == true)
    {
        pAnswer->networkmode = NWK_MODE__FREQUENCY_HOPPING;
    }
    else
    {
        int orderSF;
        int orderB;

        orderSF = CONFIG_MAC_SUPERFRAME_ORDER;
        orderB = CONFIG_MAC_BEACON_ORDER;
        if((orderSF == 15) && (orderB == 15))
        {
            pAnswer->networkmode = NWK_MODE__NON_BEACON;
        }
        else
        {
            pAnswer->networkmode = NWK_MODE__BEACON_ENABLED;
        }

    }
    // get current state
    state = Csf_getCllcState();

    pAnswer->state = appsrv_getCllcProtoState(state);

    pAnswer->nwkinfo = copy_Llc_netInfo(pNetInfo);
    pAnswer->securityenabled = securityenabled;
    return pAnswer;
}

void free_AppsrvSetJoinPermitCnf(AppsrvSetJoinPermitCnf *pThis)
{
    if(pThis)
    {
        free(pThis);
        pThis = NULL;
    }
}

AppsrvSetJoinPermitCnf *copy_AppsrvSetJoinPermitCnf(int status)
{
    AppsrvSetJoinPermitCnf *pCnf;

    pCnf = (AppsrvSetJoinPermitCnf *)calloc(1, sizeof(*pCnf));
    if(pCnf)
    {
        appsrv_set_join_permit_cnf__init(pCnf);
        pCnf->status = status;
    }
    return pCnf;
}

AppsrvDeviceUpdateInd *copy_AppsrvDeviceUpdateInd(Llc_deviceListItem_t *pDevListItem)
{
    AppsrvDeviceUpdateInd *pInd;

    pInd = (AppsrvDeviceUpdateInd *)calloc(1, sizeof(*pInd));
    if(pInd == NULL)
    {
        goto fail;
    }
    appsrv_device_update_ind__init(pInd);

    pInd->devdescriptor = copy_ApiMac_deviceDescriptor(&(pDevListItem->devInfo));
    pInd->devcapinfo = copy_ApiMac_capabilityInfo(&(pDevListItem->capInfo));

    if((pInd->devdescriptor == NULL) || (pInd->devcapinfo == NULL))
    {
    fail:
        LOG_printf(LOG_ERROR, "no memory for: AppsrvNwkInfoUpdateInd\n");
        if(pInd)
        {
            free_AppsrvDeviceUpdateInd(pInd);
            pInd = NULL;
        }
    }
    return pInd;
}

void free_AppsrvDeviceUpdateInd(AppsrvDeviceUpdateInd *pThis)
{
    if(pThis)
    {
        free_ApiMacDeviceDescriptor(pThis->devdescriptor);
        pThis->devdescriptor = NULL;
        free_ApiMacCapabilityInfo(pThis->devcapinfo);
        pThis->devcapinfo = NULL;
        free(pThis);
        pThis = NULL;
    }
}

AppsrvNwkInfoUpdateInd *copy_AppsrvNwkInfoUpdateInd(Llc_netInfo_t *networkInfo)
{
    AppsrvNwkInfoUpdateInd *pInd;

    pInd = (AppsrvNwkInfoUpdateInd *)calloc(1, sizeof(*pInd));
    if(pInd == NULL)
    {
        goto fail;
    }
    appsrv_nwk_info_update_ind__init(pInd);

    pInd->nwkinfo = copy_AppsrvNwkParam(networkInfo, CONFIG_SECURE);

    if(pInd->nwkinfo == NULL)
    {
    fail:
        LOG_printf(LOG_ERROR, "no memory for: AppsrvNwkInfoUpdateInd\n");
        if(pInd)
        {
            free_AppsrvNwkInfoUpdateInd(pInd);
            pInd = NULL;
        }
    }
    return pInd;
}

void free_AppsrvNwkInfoUpdateInd(AppsrvNwkInfoUpdateInd *pThis)
{
    if(pThis)
    {
        free_AppsrvNwkParam(pThis->nwkinfo);
        pThis->nwkinfo = NULL;
        free(pThis);
        pThis = NULL;
    }

}

AppsrvCollectorStateCngUpdateInd *copy_AppsrvCollectorStateCngUpdateInd(Cllc_states_t s)
{
    AppsrvCollectorStateCngUpdateInd *pAnswer;

    pAnswer = calloc(1, sizeof(*pAnswer));
    if(!pAnswer)
    {
        LOG_printf(LOG_ERROR, "No memory for AppsrvCollectorStateCngUpdateInd\n");
        return NULL;
    }
    appsrv_collector_state_cng_update_ind__init(pAnswer);
    /* get equivalent proto state */
    pAnswer->state = appsrv_getCllcProtoState(s);
    return pAnswer;

}
void free_AppsrvCollectorStateCngUpdateInd(AppsrvCollectorStateCngUpdateInd *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

AppsrvGetNwkInfoCnf *copy_AppsrvGetNwkInfoCnf(void)
{
    Llc_netInfo_t networkInfo;
    AppsrvGetNwkInfoCnf *pAnswer;

    pAnswer = calloc(1, sizeof(*pAnswer));
    if(!pAnswer)
    {
        LOG_printf(LOG_ERROR,"No memory for AppsrvGetNwkInfoCnf\n");
    }
    else
    {
        appsrv_get_nwk_info_cnf__init(pAnswer);

        pAnswer->status = Csf_getNetworkInformation(&networkInfo);
        if(pAnswer->status)
        {
            pAnswer->nwkinfo = copy_AppsrvNwkParam(&networkInfo, CONFIG_SECURE);
        }
    }

    return pAnswer;
}

void free_AppsrvGetNwkInfoCnf(AppsrvGetNwkInfoCnf *pThis)
{
    if(pThis)
    {
        if(pThis->nwkinfo)
        {
            free_AppsrvNwkParam(pThis->nwkinfo);
            pThis->nwkinfo = NULL;
        }
    }
    free((void *)(pThis));
}

AppsrvGetDeviceArrayCnf *copy_AppsrvGetDeviceArrayCnf(void)
{
    size_t x;
    size_t n;
    AppsrvGetDeviceArrayCnf *pAnswer;
    Csf_deviceInformation_t *pDeviceInfo;

    n = 0;
    pDeviceInfo = NULL;

    pAnswer = calloc(1, sizeof(*pAnswer));
    if(pAnswer == NULL)
    {
        goto fail;
    }
    appsrv_get_device_array_cnf__init(pAnswer);

    n = Csf_getDeviceInformationList(&pDeviceInfo);
    pAnswer->n_devinfo = n;
    pAnswer->devinfo   = calloc(pAnswer->n_devinfo+1,
                                sizeof(pAnswer->devinfo[0]));
    if(pAnswer->devinfo == NULL)
    {
        goto fail;
    }

    for(x = 0 ; x < pAnswer->n_devinfo ; x++)
    {
        pAnswer->devinfo[x] = copy_Csf_deviceInformation(pDeviceInfo+x);
        if(pAnswer->devinfo[x] == NULL)
        {
            goto fail;
        }
    }
    pAnswer->status = API_MAC_STATUS__ApiMac_status_success;

    if(pAnswer == NULL)
    {
    fail:
        LOG_printf(LOG_ERROR,"No memory for: AppsrvGetDeviceArrayCnf\n");
        free_AppsrvGetDeviceArrayCnf(pAnswer);
        pAnswer = NULL;
    }
    if(pDeviceInfo)
    {
        Csf_freeDeviceInformationList(n, pDeviceInfo);
    }

    return pAnswer;
}

void free_AppsrvGetDeviceArrayCnf(AppsrvGetDeviceArrayCnf *pThis)
{
    size_t x;

    if(pThis)
    {
        if(pThis->devinfo)
        {
            for(x = 0 ; x < pThis->n_devinfo ; x++)
            {
                free_CsfDeviceInformation(pThis->devinfo[x]);
                pThis->devinfo[x] = NULL;
            }
            free((void*)(pThis->devinfo));
            pThis->devinfo = NULL;
        }
        free(pThis);
        pThis = NULL;
    }
}

AppsrvDeviceNotActiveUpdateInd *copy_AppsrvDeviceNotActiveUpdateInd(
    const ApiMac_deviceDescriptor_t *pDevInfo,
    bool timeout)
{
    AppsrvDeviceNotActiveUpdateInd *pResult;

    pResult = calloc(1, sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR,
                   "No memory for: AppsrvDeviceNotActiveUpdateInd\n");
        return NULL;
    }
    appsrv_device_not_active_update_ind__init(pResult);
    pResult->timeout = timeout;

    pResult->devdescriptor = copy_ApiMac_deviceDescriptor(pDevInfo);
    if(pResult->devdescriptor == NULL)
    {
        free_AppsrvDeviceNotActiveUpdateInd(pResult);
        pResult = NULL;
    }
    return pResult;

}

void free_AppsrvDeviceNotActiveUpdateInd(AppsrvDeviceNotActiveUpdateInd *pThis)
{
    if(pThis)
    {
        free_ApiMacDeviceDescriptor(pThis->devdescriptor);
        pThis->devdescriptor = NULL;
        free((void *)(pThis));
    }
}

AppsrvPairingUpdateInd *copy_AppsrvPairingUpdateInd(uint8_t state)
{
    AppsrvPairingUpdateInd *pAnswer;

    pAnswer = calloc(1, sizeof(*pAnswer));
    if(!pAnswer)
    {
        LOG_printf(LOG_ERROR, "No memory for AppsrvPairingUpdateInd\n");
        return NULL;
    }
    appsrv_pairing_update_ind__init(pAnswer);
    /* get equivalent proto state */
    pAnswer->state = state;
    return pAnswer;
}

void free_AppsrvPairingUpdateInd(AppsrvPairingUpdateInd *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

AppsrvAntennaUpdateInd *copy_AppsrvAntennaUpdateInd(uint8_t state)
{
    AppsrvAntennaUpdateInd *pAnswer;

    pAnswer = calloc(1, sizeof(*pAnswer));
    if(!pAnswer)
    {
        LOG_printf(LOG_ERROR, "No memory for AppsrvAntennaUpdateInd\n");
        return NULL;
    }
    appsrv_antenna_update_ind__init(pAnswer);
    /* get equivalent proto state */
    pAnswer->state = state;
    return pAnswer;
}

void free_AppsrvAntennaUpdateInd(AppsrvAntennaUpdateInd *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

AppsrvSetIntervalUpdateInd *copy_AppsrvSetIntervalUpdateInd(ApiMac_sAddr_t *pAddr, uint32_t reporting, uint32_t polling)
{
    AppsrvSetIntervalUpdateInd *pAnswer;

    pAnswer = calloc(1, sizeof(*pAnswer));
    if(!pAnswer)
    {
        LOG_printf(LOG_ERROR, "No memory for AppsrvSetIntervalUpdateInd\n");
        return NULL;
    }
    appsrv_set_interval_update_ind__init(pAnswer);

    pAnswer->srcaddr = copy_ApiMac_sAddr(pAddr);
    if(pAnswer->srcaddr == NULL)
    {
        free_AppsrvSetIntervalUpdateInd(pAnswer);
        pAnswer = NULL;
        return pAnswer;
    }

    pAnswer->reporting = reporting;
    pAnswer->polling = polling;
    return pAnswer;
}

void free_AppsrvSetIntervalUpdateInd(AppsrvSetIntervalUpdateInd *pThis)
{
    if(pThis)
    {
        if(pThis->srcaddr)
        {
            free_ApiMacSAddr(pThis->srcaddr);
            pThis->srcaddr = NULL;
        }
        free(pThis);
    }
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

