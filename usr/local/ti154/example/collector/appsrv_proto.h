/******************************************************************************
 @file appsrv_proto.h

 @brief TIMAC 2.0 API Convert the wire messages for the appsrv (web) interface

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

#if !defined(APPSRV2_PROTO_H)
#define APPSRV2_PROTO_H

#include "api_mac.h"
#include "smsgs.h"

#include "appsrv.pb-c.h"

/*!
 * @brief release memory for protobuf device data request.
 * @param pThis - item to be freed.
 */
void free_AppsrvTxDataCnf(AppsrvTxDataCnf *pThis);

/*!
 * @brief create protobuf message tx data confirm.
 * @param status status to put into the protobuf message
 * @returns protobuf structure to encode
 */
 AppsrvTxDataCnf *copy_AppsrvTxDataCnf(uint8_t status);

/*!
 * @brief Create protobuf device data indication
 * @param pAddr - address to insert into message
 * @param rssi  - rssi value to insert into message
 * @param pSensorMsg - sensor data to insert
 * @param pCfgRspMsg - the response message to insert
 * @returns protobuf structure to encode
 */
AppsrvDeviceDataRxInd *copy_AppsrvDeviceDataRxInd(
    ApiMac_sAddr_t *pAddr,
    int32_t rssi,
    Smsgs_sensorMsg_t *pSensorMsg,
    Smsgs_configRspMsg_t *pCfgRspMsg);

/*!
 * @brief release memory for protobuf device data indication.
 * @param pThis - item to be freed.
 */
void free_AppsrvDeviceDataRxInd(AppsrvDeviceDataRxInd *pThis);

/*! ======================================================================= */
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
                                                  Smsgs_electricLockActionRspMsg_t *pElectricActRspMsg);

void free_AppsrvDeviceResponseDataRxInd(AppsrvDeviceDataRxInd *pThis);

/*!
 * @brief Create protobuf version of app join confirm
 * @param pThis - item to be freed.
 */
void free_AppsrvSetJoinPermitCnf(AppsrvSetJoinPermitCnf *pThis);

/*!
 * @brief Release memory for protobuf version of app join confirm
 * @param status - status to encode in protobuf message
 * @returns protobuf structure to encode
 */
AppsrvSetJoinPermitCnf *copy_AppsrvSetJoinPermitCnf(int status);

/*!
 * @brief Free the device update indication
 * @param pThis - item to be freed.
 */
void free_AppsrvDeviceUpdateInd(AppsrvDeviceUpdateInd *pThis);

/*!
 * @brief Release memory for protobuf version of device updated indication
 * @param pDevListItem - device list istem to encode
 * @returns protobuf structure to encode
 */
AppsrvDeviceUpdateInd *copy_AppsrvDeviceUpdateInd(Llc_deviceListItem_t *pDevListItem);

/*!
 * @brief Release memory for protobuf version of device updated indication
 * @param networkinfo - data for the protobuf message
 * @returns protobuf structure to encode
 */
AppsrvNwkInfoUpdateInd *copy_AppsrvNwkInfoUpdateInd(Llc_netInfo_t *networkInfo);

/*!
 * @brief Release memory for protobuf version of network update indication
 * @param networkinfo - structure to free
 */
void free_AppsrvNwkInfoUpdateInd(AppsrvNwkInfoUpdateInd *networkInfo);

/*!
 * @brief Release memory for protobuf version of collector state change
 * @param s - state to encode
 * @returns protobuf structure to encode
 */
AppsrvCollectorStateCngUpdateInd *copy_AppsrvCollectorStateCngUpdateInd(Cllc_states_t s);

/*!
 * @brief Release memory for protobuf version of collector state change
 * @param pThis - item to be freed.
 */
void free_AppsrvCollectorStateCngUpdateInd(AppsrvCollectorStateCngUpdateInd *pThis);

/*!
 * @brief convert the GetNetworkInfo conf message
 * @returns protobuf structure to encode
 */
AppsrvGetNwkInfoCnf *copy_AppsrvGetNwkInfoCnf(void);

/*!
 * @brief free the GetNetworkInfo conf message
 * @param pThis - item to be freed.
 */
void free_AppsrvGetNwkInfoCnf(AppsrvGetNwkInfoCnf *pThis);

/*!
 * @brief convert the Get Device Array Conf message
 * @returns protobuf structure to encode
 */
AppsrvGetDeviceArrayCnf *copy_AppsrvGetDeviceArrayCnf(void);

/*!
 * @brief free the Get Device Array Conf message
 * @param pThis - item to be freed.
 */
void free_AppsrvGetDeviceArrayCnf(AppsrvGetDeviceArrayCnf *pThis);

/*!
 * @brief convert the device not active update indication message.
 * @param pDevInfo - data for the protobuf structure
 * @param timeout - data for the protobuf structure
 * @returns protobuf structure to encode
 */
AppsrvDeviceNotActiveUpdateInd *copy_AppsrvDeviceNotActiveUpdateInd(
    const ApiMac_deviceDescriptor_t *pDevInfo,
    bool timeout);

/*!
 * @brief free the Device Not Active ind message
 * @param pThis - item to be freed.
 */
void free_AppsrvDeviceNotActiveUpdateInd(AppsrvDeviceNotActiveUpdateInd *pThis);

/*!
 * @brief Create protobuf network parameter message
 * @param pNetInfo - data for the protobuf structure
 * @param securityenabled - data for the protobuf structure
 * @returns protobuf structure to encode
 */
AppsrvNwkParam *copy_AppsrvNwkParam(Llc_netInfo_t *pNetInfo, int32_t securityenabled);

/*!
 * @brief Release memory for protobuf network parameter message.
 * @param pThis - item to be freed.
 */
void free_AppsrvNwkParam(AppsrvNwkParam *pThis);

AppsrvPairingUpdateInd *copy_AppsrvPairingUpdateInd(uint8_t state);
void free_AppsrvPairingUpdateInd(AppsrvPairingUpdateInd *pThis);

AppsrvAntennaUpdateInd *copy_AppsrvAntennaUpdateInd(uint8_t state);
void free_AppsrvAntennaUpdateInd(AppsrvAntennaUpdateInd *pThis);

AppsrvSetIntervalUpdateInd *copy_AppsrvSetIntervalUpdateInd(ApiMac_sAddr_t *pAddr, uint32_t reporting, uint32_t polling);
void free_AppsrvSetIntervalUpdateInd(AppsrvSetIntervalUpdateInd *pThis);

#endif

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

