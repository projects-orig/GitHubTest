/******************************************************************************
 @file smsgs_proto.c

 @brief TIMAC 2.0 API Manage smsgs.h structures to protobuf conversion

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
#include "api_mac.h"
#include "api_mac.pb-c.h"

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

#include "malloc.h"

/* Release memory for a config settings field.
 * Public function defined in smsgs_proto.h
 */
void free_SmsgsConfigSettingsField(SmsgsConfigSettingsField *pThis)
{
    if(pThis)
    {
        free((void *)(pThis));
    }
}

/* Free memory for a msg stats field.
 * Public function defined in smsgs_proto.h
 */
void free_SmsgsMsgStatsField(SmsgsMsgStatsField *pThis)
{
    if(pThis)
    {
        free((void *)(pThis));
        pThis = NULL;
    }
}

/* Convert settings field to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsConfigSettingsField *copy_Smsgs_configSettingsField(
    const Smsgs_configSettingsField_t *pThis)
{
    SmsgsConfigSettingsField *pResult;

    pResult = calloc(1, sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsConfigSettingsField\n");
    }
    else
    {
        smsgs_config_settings_field__init(pResult);
        pResult->reportinginterval      = pThis->reportingInterval;
        pResult->pollinginterval        = pThis->pollingInterval;
    }
    return pResult;
}

/* Convert msg stats field to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsMsgStatsField *copy_Smsgs_msgStatsField(
    const Smsgs_msgStatsField_t *pThis)
{
    SmsgsMsgStatsField *pResult;

    pResult = calloc(1,sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsMsgStatsField\n");
    }
    else
    {
        smsgs_msg_stats_field__init(pResult);
        pResult->joinattempts       = pThis->joinAttempts;
        pResult->joinfails          = pThis->joinFails;
        pResult->msgsattempted      = pThis->msgsAttempted;
        pResult->msgssent           = pThis->msgsSent;
        pResult->trackingrequests       = pThis->trackingRequests;
        pResult->trackingresponseattempts   = pThis->trackingResponseAttempts;
        pResult->channelaccessfailures  = pThis->channelAccessFailures;
        pResult->macackfailures     = pThis->macAckFailures;
        pResult->otherdatarequestfailures   = pThis->otherDataRequestFailures;
        pResult->synclossindications    = pThis->syncLossIndications;
    }
    return pResult;
}

/* Convert a sensor message to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsSensorMsg *copy_Smsgs_sensorMsg(const Smsgs_sensorMsg_t *pSensorMsg)
{
    bool fail;
    SmsgsSensorMsg *pResult;

    pResult = (SmsgsSensorMsg *)calloc(1,sizeof(*pResult));
    if(pResult == NULL)
    {
        goto _fail;
    }

    smsgs_sensor_msg__init(pResult);

    fail = false;

    pResult->cmdid = pSensorMsg->cmdId;

    pResult->framecontrol = pSensorMsg->frameControl;

	/*! ======================================================================== */
    if(pSensorMsg->frameControl & Smsgs_dataFields_msgStats)
    {
        pResult->msgstats       =
            copy_Smsgs_msgStatsField(&(pSensorMsg->msgStats));
        if(pResult->msgstats == NULL)
        {
            fail = true;
        }
    }

    if(pSensorMsg->frameControl & Smsgs_dataFields_configSettings)
    {
        pResult->configsettings =
            copy_Smsgs_configSettingsField(&(pSensorMsg->configSettings));
        if(pResult->configsettings == NULL)
        {
            fail = true;
        }
    }
    
    if(pSensorMsg->frameControl & Smsgs_dataFields_eve003Sensor)
    {
        pResult->eve003sensor =
            copy_Smsgs_eve003SensorField(&(pSensorMsg->eve003Sensor));
		if(pResult->eve003sensor == NULL)
        {
            fail = true;
        }
    }
    if(pSensorMsg->frameControl & Smsgs_dataFields_eve004Sensor)
    {
        pResult->eve004sensor =
            copy_Smsgs_eve004SensorField(&(pSensorMsg->eve004Sensor));
		if(pResult->eve004sensor == NULL)
        {
            fail = true;
        }
    }
    if(pSensorMsg->frameControl & Smsgs_dataFields_eve005Sensor)
    {
        pResult->eve005sensor =
            copy_Smsgs_eve005SensorField(&(pSensorMsg->eve005Sensor));
		if(pResult->eve005sensor == NULL)
        {
            fail = true;
        }
    }
    if(pSensorMsg->frameControl & Smsgs_dataFields_eve006Sensor)
    {
        pResult->eve006sensor =
            copy_Smsgs_eve006SensorField(&(pSensorMsg->eve006Sensor));
		if(pResult->eve006sensor == NULL)
        {
            fail = true;
        }
    }
    
    if(fail)
    {
    _fail:
        LOG_printf(LOG_ERROR,"No memory for SmsgsSensorMsg\n");
        free_SmsgsSensorMsg(pResult);
        pResult = NULL;
    }
    return pResult;
}

/* release memory for a sensor message to protobuf form
 * Public function defined in smsgs_proto.h
 */
void free_SmsgsSensorMsg(SmsgsSensorMsg *pThis)
{
    if(pThis == NULL)
    {
        return;
    }

	/*! ======================================================================== */
    if(pThis->msgstats)
    {
        free_SmsgsMsgStatsField(pThis->msgstats);
        pThis->msgstats = NULL;
    }

    if(pThis->configsettings)
    {
        free_SmsgsConfigSettingsField(pThis->configsettings);
        pThis->configsettings = NULL;
    }
    
    if(pThis->eve003sensor)
    {
        free_SmsgsEve003SensorField(pThis->eve003sensor);
        pThis->eve003sensor = NULL;
    }
    if(pThis->eve004sensor)
    {
        free_SmsgsEve004SensorField(pThis->eve004sensor);
        pThis->eve004sensor = NULL;
    }
    if(pThis->eve005sensor)
    {
        free_SmsgsEve005SensorField(pThis->eve005sensor);
        pThis->eve005sensor = NULL;
    }
    if(pThis->eve006sensor)
    {
        free_SmsgsEve006SensorField(pThis->eve006sensor);
        pThis->eve006sensor = NULL;
    }
    
    free( (void *)(pThis) );
}

/* convert sensor config response message
 * Public function in smsgs_proto.h
 */
SmsgsConfigRspMsg *copy_Smsgs_configRspMsg(
    const Smsgs_configRspMsg_t *pConfigRspMsg)
{
    SmsgsConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(pDATA == NULL)
    {
        LOG_printf(LOG_ERROR, "no memory for SmsgsConfigRspMsg\n");
        return NULL;
    }

    smsgs_config_rsp_msg__init(pDATA);

    pDATA->cmdid = pConfigRspMsg->cmdId;
    pDATA->framecontrol = pConfigRspMsg->frameControl;
    pDATA->pollinginterval = pConfigRspMsg->pollingInterval;
    pDATA->treportinginterval = pConfigRspMsg->reportingInterval;
    pDATA->status = pConfigRspMsg->status;
    return pDATA;
}

/* Free sensor config response message
 * Public function in smsgs_proto.h
 */
void free_SmsgsConfigRspMsg(SmsgsConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/*! ======================================================================== */
/* Convert remote control field to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsEve003SensorField *copy_Smsgs_eve003SensorField(
    const Smsgs_eve003SensorField_t *pThis)
{
    SmsgsEve003SensorField *pResult;

    pResult = calloc(1, sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsEve003SensorField\n");
    }
    else
    {
        smsgs_eve003_sensor_field__init(pResult);
        pResult->button = pThis->button;
    	pResult->dip = pThis->dip;
    	pResult->setting = pThis->setting;
    	pResult->battery = pThis->battery;
    }
    return pResult;
}
/* Free memory for a remote contorl sensor field.
 * Public function defined in smsgs_proto.h
 */
void free_SmsgsEve003SensorField(SmsgsEve003SensorField *pThis)
{
    if(pThis)
    {
        free((void *)(pThis));
        pThis = NULL;
    }
}

/* Convert Signal field to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsEve004SensorField *copy_Smsgs_eve004SensorField(
    const Smsgs_eve004SensorField_t *pThis)
{
    SmsgsEve004SensorField *pResult;

    pResult = calloc(1, sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsEve004SensorField\n");
    }
    else
    {
        smsgs_eve004_sensor_field__init(pResult);
        pResult->tempvalue = pThis->tempValue;
        pResult->state = pThis->state;
        pResult->alarm = pThis->alarm;
        pResult->dip = pThis->dip;
        pResult->battery = pThis->battery;
        pResult->rssi = pThis->rssi;
    }
    return pResult;
}

/* Free memory for a Signal sensor field.
 * Public function defined in smsgs_proto.h
 */
void free_SmsgsEve004SensorField(SmsgsEve004SensorField *pThis)
{
    if(pThis)
    {
        free((void *)(pThis));
        pThis = NULL;
    }
}

/* Convert EVE005 field to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsEve005SensorField *copy_Smsgs_eve005SensorField(
    const Smsgs_eve005SensorField_t *pThis)
{
    SmsgsEve005SensorField *pResult;

    pResult = calloc(1, sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsEve005SensorField\n");
    }
    else
    {
        smsgs_eve005_sensor_field__init(pResult);
        pResult->tmpvalue = pThis->tmpValue;
        pResult->batvalue = pThis->batValue;
        pResult->rssi = pThis->rssi;
        pResult->button = pThis->button;
        pResult->dip_id = pThis->dip_id;
        pResult->dip_control = pThis->dip_control;
        pResult->sensor_s = pThis->sensor_s;
        pResult->alarm = pThis->alarm;
        pResult->resist1 = pThis->resist1;
        pResult->resist2 = pThis->resist2;
    }
    return pResult;
}

/* Free memory for a EVE005 sensor field.
 * Public function defined in smsgs_proto.h
 */
void free_SmsgsEve005SensorField(SmsgsEve005SensorField *pThis)
{
    if(pThis)
    {
        free((void *)(pThis));
        pThis = NULL;
    }
}

/* Convert PIR field to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsEve006SensorField *copy_Smsgs_eve006SensorField(
    const Smsgs_eve006SensorField_t *pThis)
{
    SmsgsEve006SensorField *pResult;

    pResult = calloc(1, sizeof(*pResult));
    if(!pResult)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsEve006SensorField\n");
    }
    else
    {
        smsgs_eve006_sensor_field__init(pResult);
        pResult->button = pThis->button;
        pResult->state = pThis->state;
        pResult->abnormal = pThis->abnormal;
        pResult->dip = pThis->dip;
        pResult->dip_id = pThis->dip_id;
        pResult->temp = pThis->temp;
        pResult->humidity = pThis->humidity;
        pResult->battery = pThis->battery;
        pResult->resist1 = pThis->resist1;
        pResult->resist2 = pThis->resist2;
    }
    return pResult;
}

/* Free memory for a PIR sensor field.
 * Public function defined in smsgs_proto.h
 */
void free_SmsgsEve006SensorField(SmsgsEve006SensorField *pThis)
{
    if(pThis)
    {
        free((void *)(pThis));
        pThis = NULL;
    }
}

/*! ======================================================================== */
/* Convert Alarm Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsAlarmConfigRspMsg *copy_Smsgs_alarmConfigRspMsg(
    const Smsgs_alarmConfigRspMsg_t *pConfigRsp)
{
    SmsgsAlarmConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsAlarmConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_alarm_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->time = pConfigRsp->time;

    return pDATA;
}

/* Free Alarm config response message
 * Public function in smsgs_proto.h
 */
void free_SmsgsAlarmConfigRspMsg(SmsgsAlarmConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert G-Sensor Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsGSensorConfigRspMsg *copy_Smsgs_gSensorConfigRspMsg(
    const Smsgs_gSensorConfigRspMsg_t *pConfigRsp)
{
	SmsgsGSensorConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsGSensorConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_g_sensor_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->enable = pConfigRsp->enable;
	pDATA->sensitivity = pConfigRsp->sensitivity;
	
    return pDATA;
}

/* Free G-Sensor config response message
 * Public function in smsgs_proto.h
 */
void free_SmsgsGSensorConfigRspMsg(SmsgsGSensorConfigRspMsg *pThis)
{
	if(pThis)
    {
        free(pThis);
    }
}

/* Convert Electric Lock Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsElectricLockConfigRspMsg *copy_Smsgs_electricLockConfigRspMsg(
    const Smsgs_electricLockConfigRspMsg_t *pConfigRsp)
{
	SmsgsElectricLockConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsElectricLockConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_electric_lock_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->enable = pConfigRsp->enable;
	pDATA->time = pConfigRsp->time;
	
    return pDATA;
}

/* Free Electric Lock config response message
 * Public function in smsgs_proto.h
 */
void free_SmsgsElectricLockConfigRspMsg(SmsgsElectricLockConfigRspMsg *pThis)
{
	if(pThis)
    {
        free(pThis);
    }
}

/* Convert Signal Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsSignalConfigRspMsg *copy_Smsgs_signalConfigRspMsg(
    const Smsgs_signalConfigRspMsg_t *pConfigRsp)
{
	SmsgsSignalConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsSignalConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_signal_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->mode = pConfigRsp->mode;
	pDATA->value = pConfigRsp->value;
	pDATA->offset = pConfigRsp->offset;
	
    return pDATA;
}

/* Free Signal config response message
 * Public function in smsgs_proto.h
 */
void free_SmsgsSignalConfigRspMsg(SmsgsSignalConfigRspMsg *pThis)
{
	if(pThis)
    {
        free(pThis);
    }
}

/* Convert Temp Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsTempConfigRspMsg *copy_Smsgs_tempConfigRspMsg(
    const Smsgs_tempConfigRspMsg_t *pConfigRsp)
{
	SmsgsTempConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsTempConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_temp_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
	pDATA->value = pConfigRsp->value;
	pDATA->offset = pConfigRsp->offset;
	
    return pDATA;
}

/* Free Signal config response message
 * Public function in smsgs_proto.h
 */
void free_SmsgsTempConfigRspMsg(SmsgsTempConfigRspMsg *pThis)
 {
 	if(pThis)
    {
        free(pThis);
    }
 }
 
/* Convert Low battery Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsLowBatteryConfigRspMsg *copy_Smsgs_lowBatteryConfigRspMsg(
    const Smsgs_lowBatteryConfigRspMsg_t *pConfigRsp)
{
	SmsgsLowBatteryConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsLowBatteryConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_low_battery_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
	pDATA->value = pConfigRsp->value;
	pDATA->offset = pConfigRsp->offset;
	
    return pDATA;
}

/* Free Low battery config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsLowBatteryConfigRspMsg(SmsgsLowBatteryConfigRspMsg *pThis)
{
 	if(pThis)
    {
        free(pThis);
    }
}
/* Convert Distance Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsDistanceConfigRspMsg *copy_Smsgs_distanceConfigRspMsg(
    const Smsgs_distanceConfigRspMsg_t *pConfigRsp)
{
	SmsgsDistanceConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsDistanceConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_distance_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
	pDATA->mode = pConfigRsp->mode;
	pDATA->distance = pConfigRsp->distance;
	
    return pDATA;
} 

/* Free Distance config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsDistanceConfigRspMsg(SmsgsDistanceConfigRspMsg *pThis)
{
	if(pThis)
    {
        free(pThis);
    }
}
/* Convert Music Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsMusicConfigRspMsg *copy_Smsgs_musicConfigRspMsg(
    const Smsgs_musicConfigRspMsg_t *pConfigRsp)
{
    SmsgsMusicConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsMusicConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_music_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->mode = pConfigRsp->mode;
    pDATA->time = pConfigRsp->time;
    
    return pDATA;
}
/* Free Music config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsMusicConfigRspMsg(SmsgsMusicConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Interval Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsIntervalConfigRspMsg *copy_Smsgs_intervalConfigRspMsg(
    const Smsgs_intervalConfigRspMsg_t *pConfigRsp)
{
    SmsgsIntervalConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsIntervalConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_interval_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->mode = pConfigRsp->mode;
    pDATA->time = pConfigRsp->time;
    
    return pDATA;
} 
/* Free Interval config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsIntervalConfigRspMsg(SmsgsIntervalConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Motion Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsMotionConfigRspMsg *copy_Smsgs_motionConfigRspMsg(
    const Smsgs_motionConfigRspMsg_t *pConfigRsp)
{
    SmsgsMotionConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsMotionConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_motion_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->count = pConfigRsp->count;
    pDATA->time = pConfigRsp->time;
    
    return pDATA;
} 
/* Free Motion config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsMotionConfigRspMsg(SmsgsMotionConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Resistance Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsResistanceConfigRspMsg *copy_Smsgs_resistanceConfigRspMsg(
    const Smsgs_resistanceConfigRspMsg_t *pConfigRsp)
{
    SmsgsResistanceConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsResistanceConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_resistance_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->value = pConfigRsp->value;
    
    return pDATA;
} 
/* Free Resistance config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsResistanceConfigRspMsg(SmsgsResistanceConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Microwave Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsMicrowaveConfigRspMsg *copy_Smsgs_microwaveConfigRspMsg(
    const Smsgs_microwaveConfigRspMsg_t *pConfigRsp)
{
    SmsgsMicrowaveConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsMicrowaveConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_microwave_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->enable = pConfigRsp->enable;
    pDATA->sensitivity = pConfigRsp->sensitivity;
    
    return pDATA;
} 
/* Free Microwave config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsMicrowaveConfigRspMsg(SmsgsMicrowaveConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Pir Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsPirConfigRspMsg *copy_Smsgs_pirConfigRspMsg(
    const Smsgs_pirConfigRspMsg_t *pConfigRsp)
{
    SmsgsPirConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsPirConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_pir_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->enable = pConfigRsp->enable;
    
    return pDATA;
} 
/* Free Pir config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsPirConfigRspMsg(SmsgsPirConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Set Unset Config to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsSetUnsetConfigRspMsg *copy_Smsgs_setUnsetConfigRspMsg(
    const Smsgs_setUnsetConfigRspMsg_t *pConfigRsp)
{
    SmsgsSetUnsetConfigRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsSetUnsetConfigRspMsg\n");
        return NULL;
    }
    
    smsgs_set_unset_config_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->state = pConfigRsp->state;
    
    return pDATA;
} 
/* Free Set Unset config response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsSetUnsetConfigRspMsg(SmsgsSetUnsetConfigRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Disconnect to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsDisconnectRspMsg *copy_Smsgs_disconnectRspMsg(
    const Smsgs_disconnectRspMsg_t *pConfigRsp)
{
    SmsgsDisconnectRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsDisconnectRspMsg\n");
        return NULL;
    }
    
    smsgs_disconnect_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pConfigRsp->cmdId;
    pDATA->status = pConfigRsp->status;
    pDATA->framecontrol = pConfigRsp->frameControl;
    pDATA->time = pConfigRsp->time;
    
    return pDATA;
} 
/* Free Disconnect response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsDisconnectRspMsg(SmsgsDisconnectRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

/* Convert Electric Lock Action to protobuf form
 * Public function defined in smsgs_proto.h
 */
SmsgsElectricLockActionRspMsg *copy_Smsgs_electricLockActionRspMsg(
    const Smsgs_electricLockActionRspMsg_t *pActionRsp)
{
    SmsgsElectricLockActionRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsElectricLockActionRspMsg\n");
        return NULL;
    }
    
    smsgs_electric_lock_action_rsp_msg__init(pDATA);
    
    pDATA->cmdid = pActionRsp->cmdId;
    pDATA->status = pActionRsp->status;
    pDATA->framecontrol = pActionRsp->frameControl;
    pDATA->relay = pActionRsp->relay;
    
    return pDATA;
} 
/* Free Electric Lock Action response message
 * Public function in smsgs_proto.h
*/
void free_SmsgsElectricLockActionRspMsg(SmsgsElectricLockActionRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

SmsgsAntennaRspMsg *copy_Smsgs_antennaRspMsg(
    const Smsgs_antennaRspMsg_t *pRsp)
{
    SmsgsAntennaRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsAntennaRspMsg\n");
        return NULL;
    }
    
    smsgs_antenna_rsp_msg__init(pDATA);
    
    pDATA->state = pRsp->state;
    
    return pDATA;
} 

void free_SmsgsAntennaRspMsg(SmsgsAntennaRspMsg *pThis)
{
    if(pThis)
    {
        free(pThis);
    }
}

SmsgsSetIntervalRspMsg *copy_Smsgs_setIntervalRspMsg(
    const Smsgs_setIntervalRspMsg_t *pRsp)
{
    SmsgsSetIntervalRspMsg *pDATA;

    pDATA = calloc(1, sizeof(*pDATA));
    if(!pDATA)
    {
        LOG_printf(LOG_ERROR, "No memory for: SmsgsSetIntervalRspMsg\n");
        return NULL;
    }
    
    smsgs_set_interval_rsp_msg__init(pDATA);
    
    pDATA->reporting = pRsp->reporting;
    pDATA->polling = pRsp->polling;
    
    return pDATA;
} 

void free_SmsgsSetIntervalRspMsg(SmsgsSetIntervalRspMsg *pThis)
{
    if(pThis)
    {
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

