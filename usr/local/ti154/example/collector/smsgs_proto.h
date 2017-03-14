/******************************************************************************
 @file smsgs_proto.h

 @brief TIMAC 2.0 API TIMAC 2.0 - These convert smsgs.h structures to protobuf form

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

#if !defined(SMSGS_PROTO_H)
#define SMGS_PROTO_H

#include "smsgs.h"
#include "smsgs.pb-c.h"

/*!
 * @brief Copy sensor config response to protobuf form
 * @param pConfigRsp - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsConfigRspMsg *copy_Smsgs_configRspMsg(
    const Smsgs_configRspMsg_t *pConfigRspMsg);

/*!
 * @brief release memory for protobuf form of sensor config response.
 * @param pThis - data to free
 */
void free_SmsgsConfigRspMsg(SmsgsConfigRspMsg *pThis);

/*!
 * @brief Release memory for a sensor message
 * @param pThis - data to free
 */
void free_SmsgsSensorMsg(SmsgsSensorMsg *pThis);

/*!
 * @brief Convert a sensor message
 * @param pSensormsg - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsSensorMsg *copy_Smsgs_sensorMsg(const Smsgs_sensorMsg_t *pSensorMsg);

/*!
 * @brief Release memory for a config message
 * @param pThis - data to free
 */
void free_SmsgsConfigSettingsField(SmsgsConfigSettingsField *pThis);

/*!
 * @brief Release memory for a msg status message
 * @param pThis - data to free
 */
void free_SmsgsMsgStatsField(SmsgsMsgStatsField *pThis);

/*!
 * @brief convert a config setting message
 * @param pThis - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsConfigSettingsField *copy_Smsgs_configSettingsField(
    const Smsgs_configSettingsField_t *pThis);

/*!
 * @brief convert a config msg stats message
 * @param pThis - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsMsgStatsField *copy_Smsgs_msgStatsField(
    const Smsgs_msgStatsField_t *pThis);

/*! =========================================================================== */
/*!
 * @brief convert a config Remote Control message
 * @param pThis - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsEve003SensorField *copy_Smsgs_eve003SensorField(
    const Smsgs_eve003SensorField_t *pThis);
/*!
 * @brief Release memory for a remote control sensor message
 * @param pThis - data to free
 */
void free_SmsgsEve003SensorField(SmsgsEve003SensorField *pThis);

/*!
 * @brief convert a config Signal message
 * @param pThis - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsEve004SensorField *copy_Smsgs_eve004SensorField(
    const Smsgs_eve004SensorField_t *pThis);
/*!
 * @brief Release memory for a Signal sensor message
 * @param pThis - data to free
 */
void free_SmsgsEve004SensorField(SmsgsEve004SensorField *pThis);

/*!
 * @brief convert a config EVE005 message
 * @param pThis - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsEve005SensorField *copy_Smsgs_eve005SensorField(
    const Smsgs_eve005SensorField_t *pThis);
/*!
 * @brief Release memory for a EVE005 sensor message
 * @param pThis - data to free
 */
void free_SmsgsEve005SensorField(SmsgsEve005SensorField *pThis);

/*!
 * @brief convert a config PIR message
 * @param pThis - data to convert to the protobuf format
 * @returns data in protobuf format
 */
SmsgsEve006SensorField *copy_Smsgs_eve006SensorField(
    const Smsgs_eve006SensorField_t *pThis);
/*!
 * @brief Release memory for a PIR sensor message
 * @param pThis - data to free
 */
void free_SmsgsEve006SensorField(SmsgsEve006SensorField *pThis);

/*! =========================================================================== */
SmsgsAlarmConfigRspMsg *copy_Smsgs_alarmConfigRspMsg(
    const Smsgs_alarmConfigRspMsg_t *pConfigRsp);    
void free_SmsgsAlarmConfigRspMsg(SmsgsAlarmConfigRspMsg *pThis);

SmsgsGSensorConfigRspMsg *copy_Smsgs_gSensorConfigRspMsg(
    const Smsgs_gSensorConfigRspMsg_t *pConfigRsp);
void free_SmsgsGSensorConfigRspMsg(SmsgsGSensorConfigRspMsg *pThis);

SmsgsElectricLockConfigRspMsg *copy_Smsgs_electricLockConfigRspMsg(
    const Smsgs_electricLockConfigRspMsg_t *pConfigRsp);    
void free_SmsgsElectricLockConfigRspMsg(SmsgsElectricLockConfigRspMsg *pThis);

SmsgsSignalConfigRspMsg *copy_Smsgs_signalConfigRspMsg(
    const Smsgs_signalConfigRspMsg_t *pConfigRsp);    
void free_SmsgsSignalConfigRspMsg(SmsgsSignalConfigRspMsg *pThis);

SmsgsTempConfigRspMsg *copy_Smsgs_tempConfigRspMsg(
    const Smsgs_tempConfigRspMsg_t *pConfigRsp);    
void free_SmsgsTempConfigRspMsg(SmsgsTempConfigRspMsg *pThis);

SmsgsLowBatteryConfigRspMsg *copy_Smsgs_lowBatteryConfigRspMsg(
    const Smsgs_lowBatteryConfigRspMsg_t *pConfigRsp);    
void free_SmsgsLowBatteryConfigRspMsg(SmsgsLowBatteryConfigRspMsg *pThis);

SmsgsDistanceConfigRspMsg *copy_Smsgs_distanceConfigRspMsg(
    const Smsgs_distanceConfigRspMsg_t *pConfigRsp);    
void free_SmsgsDistanceConfigRspMsg(SmsgsDistanceConfigRspMsg *pThis);

SmsgsMusicConfigRspMsg *copy_Smsgs_musicConfigRspMsg(
    const Smsgs_musicConfigRspMsg_t *pConfigRsp);    
void free_SmsgsMusicConfigRspMsg(SmsgsMusicConfigRspMsg *pThis);

SmsgsIntervalConfigRspMsg *copy_Smsgs_intervalConfigRspMsg(
    const Smsgs_intervalConfigRspMsg_t *pConfigRsp);    
void free_SmsgsIntervalConfigRspMsg(SmsgsIntervalConfigRspMsg *pThis);

SmsgsMotionConfigRspMsg *copy_Smsgs_motionConfigRspMsg(
    const Smsgs_motionConfigRspMsg_t *pConfigRsp);    
void free_SmsgsMotionConfigRspMsg(SmsgsMotionConfigRspMsg *pThis);

SmsgsResistanceConfigRspMsg *copy_Smsgs_resistanceConfigRspMsg(
    const Smsgs_resistanceConfigRspMsg_t *pConfigRsp);    
void free_SmsgsResistanceConfigRspMsg(SmsgsResistanceConfigRspMsg *pThis);

SmsgsMicrowaveConfigRspMsg *copy_Smsgs_microwaveConfigRspMsg(
    const Smsgs_microwaveConfigRspMsg_t *pConfigRsp);    
void free_SmsgsMicrowaveConfigRspMsg(SmsgsMicrowaveConfigRspMsg *pThis);

SmsgsPirConfigRspMsg *copy_Smsgs_pirConfigRspMsg(
    const Smsgs_pirConfigRspMsg_t *pConfigRsp);    
void free_SmsgsPirConfigRspMsg(SmsgsPirConfigRspMsg *pThis);

SmsgsSetUnsetConfigRspMsg *copy_Smsgs_setUnsetConfigRspMsg(
    const Smsgs_setUnsetConfigRspMsg_t *pConfigRsp);    
void free_SmsgsSetUnsetConfigRspMsg(SmsgsSetUnsetConfigRspMsg *pThis);

SmsgsDisconnectRspMsg *copy_Smsgs_disconnectRspMsg(
    const Smsgs_disconnectRspMsg_t *pConfigRsp);    
void free_SmsgsDisconnectRspMsg(SmsgsDisconnectRspMsg *pThis);

SmsgsElectricLockActionRspMsg *copy_Smsgs_electricLockActionRspMsg(
    const Smsgs_electricLockActionRspMsg_t *pActionRsp);    
void free_SmsgsElectricLockActionRspMsg(SmsgsElectricLockActionRspMsg *pThis);

SmsgsAntennaRspMsg *copy_Smsgs_antennaRspMsg(
    const Smsgs_antennaRspMsg_t *pRsp);
void free_SmsgsAntennaRspMsg(SmsgsAntennaRspMsg *pThis);

SmsgsSetIntervalRspMsg *copy_Smsgs_setIntervalRspMsg(
    const Smsgs_setIntervalRspMsg_t *pRsp);
void free_SmsgsSetIntervalRspMsg(SmsgsSetIntervalRspMsg *pThis);

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

