/******************************************************************************

 @file smsgs.h

 @brief Data Structures for the sensor messages sent over the air.

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
#ifndef SMGSS_H
#define SMGSS_H

/******************************************************************************
 Includes
 *****************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 \defgroup Sensor Over-the-air Messages
 <BR>
 This header file defines the over-the-air messages between the collector
 and the sensor applications.
 <BR>
 Each field in these message are formatted low byte first, so a 16 bit field
 with a value of 0x1516 will be sent as 0x16, 0x15.
 <BR>
 The first byte of each message (data portion) contains the command ID (@ref
 Smsgs_cmdIds).
 <BR>
 The <b>Configuration Request Message</b> is defined as:
     - Command ID - [Smsgs_cmdIds_configReq](@ref Smsgs_cmdIds) (1 byte)
     - Frame Control field - Smsgs_dataFields (16 bits) - tells the sensor
     what to report in the Sensor Data Message.
     - Reporting Interval - in millseconds (32 bits) - how often to report, 0
     means to turn off automated reporting, but will force the sensor device
     to send the Sensor Data message once.
     - Polling Interval - in millseconds (32 bits) - If the sensor device is
     a sleep device, this tells the device how often to poll its parent for
     data.
 <BR>
 The <b>Configuration Response Message</b> is defined as:
     - Command ID - [Smsgs_cmdIds_configRsp](@ref Smsgs_cmdIds) (1 byte)
     - Status field - Smsgs_statusValues (16 bits) - status of the
     configuration request.
     - Frame Control field - Smsgs_dataFields (16 bits) - tells the collector
     what fields are supported by this device (this only includes the bits set
     in the request message).
     - Reporting Interval - in millseconds (32 bits) - how often to report, 0
     means to turn off reporting.
     - Polling Interval - in millseconds (32 bits) - If the sensor device is
     a sleep device, this tells how often this device will poll its parent.
     A value of 0 means that the device doesn't sleep.
 <BR>
 The <b>Sensor Data Message</b> is defined as:
     - Command ID - [Smsgs_cmdIds_sensorData](@ref Smsgs_cmdIds) (1 byte)
     - Frame Control field - Smsgs_dataFields (16 bits) - tells the collector
     what fields are included in this message.
     - Data Fields - The length of this field is determined by what data fields
     are included.  The order of the data fields are determined by the bit
     position of the Frame Control field (low bit first).  For example, if the
     frame control field has Smsgs_dataFields_tempSensor and
     Smsgs_dataFields_lightSensor set, then the Temp Sensor field is first,
     followed by the light sensor field.
 <BR>
 The <b>Temp Sensor Field</b> is defined as:
    - Ambience Chip Temperature - (int16_t) - each value represents signed
      integer part of temperature in Deg C (-256 .. +255)
    - Object Temperature - (int16_t) -  each value represents signed
      integer part of temperature in Deg C (-256 .. +255)
 <BR>
 The <b>Light Sensor Field</b> is defined as:
    - Raw Sensor Data - (uint16_6) raw data read out of the OPT2001 light
    sensor.
 <BR>
 The <b>Humidity Sensor Field</b> is defined as:
    - Raw Temp Sensor Data - (uint16_t) - raw temperature data from the
    Texas Instruments HCD1000 humidity sensor.
    - Raw Humidity Sensor Data - (uint16_t) - raw humidity data from the
    Texas Instruments HCD1000 humidity sensor.
 <BR>
 The <b>Message Statistics Field</b> is defined as:
     - joinAttempts - uint16_t - total number of join attempts (associate
     request sent)
     - joinFails - uint16_t - total number of join attempts failed
     - msgsAttempted - uint16_t - total number of sensor data messages
     attempted.
     - msgsSent - uint16_t - total number of sensor data messages successfully
     sent (OTA).
     - trackingRequests - uint16_t - total number of tracking requests received.
     - trackingResponseAttempts - uint16_t - total number of tracking response
     attempted
     - trackingResponseSent - uint16_t - total number of tracking response
     success
     - configRequests - uint16_t - total number of config requests received.
     - configResponseAttempts - uint16_t - total number of config response
     attempted
     - configResponseSent - uint16_t - total number of config response
     success
     - channelAccessFailures - uint16_t - total number of Channel Access
     Failures.  These are indicated in MAC data confirms for MAC data requests.
     - macAckFailures - uint16_t - Total number of MAC ACK failures. These are
     indicated in MAC data confirms for MAC data requests.
     - otherDataRequestFailures - uint16_t - Total number of MAC data request
     failures, other than channel access failure or MAC ACK failures.
     - syncLossIndications - uint16_t - Total number of sync loss failures
     received for sleepy devices.
     - rxDecryptFailues - uint16_t - Total number of RX Decrypt failures.
     - txEncryptFailues - uint16_t - Total number of TX Encrypt failures.
     - resetCount - uint16_t - Total number of resets.
     - lastResetReason - uint16_t - 0 - no reason, 2 - HAL/ICALL,
     3 - MAC, 4 - TIRTOS
 <BR>
 The <b>Config Settings Field</b> is defined as:
     - Reporting Interval - in millseconds (32 bits) - how often to report, 0
     means reporting is off.
     - Polling Interval - in millseconds (32 bits) - If the sensor device is
     a sleep device, this states how often the device polls its parent for
     data. This field is 0 if the device doesn't sleep.
 */

/******************************************************************************
 Constants and definitions
 *****************************************************************************/
/*! Sensor Message Extended Address Length */
#define SMGS_SENSOR_EXTADDR_LEN 8

/*! Config Request message length (over-the-air length) */
#define SMSGS_CONFIG_REQUEST_MSG_LENGTH 11
/*! Config Response message length (over-the-air length) */
#define SMSGS_CONFIG_RESPONSE_MSG_LENGTH 13
/*! Tracking Request message length (over-the-air length) */
#define SMSGS_TRACKING_REQUEST_MSG_LENGTH 1
/*! Tracking Response message length (over-the-air length) */
#define SMSGS_TRACKING_RESPONSE_MSG_LENGTH 1

/*! Length of a sensor data message with no configured data fields */
#define SMSGS_BASIC_SENSOR_LEN (3 + SMGS_SENSOR_EXTADDR_LEN)
/*! Length of the messageStatistics portion of the sensor data message */
#define SMSGS_SENSOR_MSG_STATS_LEN 36
/*! Length of the configSettings portion of the sensor data message */
#define SMSGS_SENSOR_CONFIG_SETTINGS_LEN 8
/*! Toggle Led Request message length (over-the-air length) */
#define SMSGS_TOGGLE_LED_REQUEST_MSG_LEN 1
/*! Toggle Led Request message length (over-the-air length) */
#define SMSGS_TOGGLE_LED_RESPONSE_MSG_LEN 2

/*! ====================================================================== */
/*! Length of the Remote Control Sensor portion of the sensor data message */
#define SMSGS_SENSOR_EVE003_LEN 5
/*! Length of the Signal Sensor portion of the sensor data message */
#define SMSGS_SENSOR_EVE004_LEN 6
/*! Length of the EVE005 Sensor portion of the sensor data message */
#define SMSGS_SENSOR_EVE005_LEN 10
/*! Length of the PIR Sensor portion of the sensor data message */
#define SMSGS_SENSOR_EVE006_LEN 10

/*! Set ALARM Sensor config Request message length (over-the-air length) */
#define SMSGS_ALARM_CONFIG_REQUEST_MSG_LEN 5
/*! Set ALARM Sensor config Response message length (over-the-air length) */
#define SMSGS_ALARM_CONFIG_RESPONSE_MSG_LENGTH 7
#define SMSGS_GSENSOR_CONFIG_REQUEST_MSG_LEN 5
#define SMSGS_GSENSOR_CONFIG_RESPONSE_MSG_LENGTH 7
#define SMSGS_ELECTRIC_LOCK_CONFIG_REQUEST_MSG_LEN 5
#define SMSGS_ELECTRIC_LOCK_CONFIG_RESPONSE_MSG_LENGTH 7
#define SMSGS_SIGNAL_CONFIG_REQUEST_MSG_LEN 6
#define SMSGS_SIGNAL_CONFIG_RESPONSE_MSG_LENGTH 8
#define SMSGS_TEMP_CONFIG_REQUEST_MSG_LEN 5
#define SMSGS_TEMP_CONFIG_RESPONSE_MSG_LENGTH 7
#define SMSGS_LOW_BATTERY_CONFIG_REQUEST_MSG_LEN 5
#define SMSGS_LOW_BATTERY_CONFIG_RESPONSE_MSG_LENGTH 7
#define SMSGS_DISTANCE_CONFIG_REQUEST_MSG_LEN 6
#define SMSGS_DISTANCE_CONFIG_RESPONSE_MSG_LENGTH 8
#define SMSGS_MUSIC_CONFIG_REQUEST_MSG_LEN 6
#define SMSGS_MUSIC_CONFIG_RESPONSE_MSG_LENGTH 8
#define SMSGS_INTERVAL_CONFIG_REQUEST_MSG_LEN 6
#define SMSGS_INTERVAL_CONFIG_RESPONSE_MSG_LENGTH 8
#define SMSGS_MOTION_CONFIG_REQUEST_MSG_LEN 6
#define SMSGS_MOTION_CONFIG_RESPONSE_MSG_LENGTH 8
#define SMSGS_RESISTANCE_CONFIG_REQUEST_MSG_LEN 4
#define SMSGS_RESISTANCE_CONFIG_RESPONSE_MSG_LENGTH 6
#define SMSGS_MICROWAVE_CONFIG_REQUEST_MSG_LEN 5
#define SMSGS_MICROWAVE_CONFIG_RESPONSE_MSG_LENGTH 7
#define SMSGS_PIR_CONFIG_REQUEST_MSG_LEN 4
#define SMSGS_PIR_CONFIG_RESPONSE_MSG_LENGTH 6
#define SMSGS_SET_UNSET_CONFIG_REQUEST_MSG_LEN 4
#define SMSGS_SET_UNSET_CONFIG_RESPONSE_MSG_LENGTH 6
#define SMSGS_DISCONNECT_REQUEST_MSG_LEN 5
#define SMSGS_DISCONNECT_RESPONSE_MSG_LENGTH 7
#define SMSGS_ELECTRIC_LOCK_ACTION_REQUEST_MSG_LEN 4
#define SMSGS_ELECTRIC_LOCK_ACTION_RESPONSE_MSG_LENGTH 6

/*!
 Message IDs for Sensor data messages.  When sent over-the-air in a message,
 this field is one byte.
 */
typedef enum
{
    /*! Configuration message, sent from the collector to the sensor */
    Smsgs_cmdIds_configReq = 1,
    /*! Configuration Response message, sent from the sensor to the collector */
    Smsgs_cmdIds_configRsp = 2,
    /*! Tracking request message, sent from the the collector to the sensor */
    Smsgs_cmdIds_trackingReq = 3,
     /*! Tracking response message, sent from the sensor to the collector */
    Smsgs_cmdIds_trackingRsp = 4,
    /*! Sensor data message, sent from the sensor to the collector */
    Smsgs_cmdIds_sensorData = 5,
    /* Toggle LED message, sent from the collector to the sensor */
    Smsgs_cmdIds_toggleLedReq = 6,
    /* Toggle LED response msg, sent from the sensor to the collector */
    Smsgs_cmdIds_toggleLedRsp = 7,

    /*! ====================================================================== */
    Smsgs_cmdIds_alarmConfigReq = 8,
	Smsgs_cmdIds_alarmConfigRsp = 9,
	Smsgs_cmdIds_gSensorConfigReq = 10,
	Smsgs_cmdIds_gSensorConfigRsp = 11,
	Smsgs_cmdIds_electricLockConfigReq = 12,
	Smsgs_cmdIds_electricLockConfigRsp = 13,
	Smsgs_cmdIds_signalConfigReq = 14,
	Smsgs_cmdIds_signalConfigRsp = 15,
	Smsgs_cmdIds_tempConfigReq = 16,
	Smsgs_cmdIds_tempConfigRsp = 17,
	Smsgs_cmdIds_lowBatteryConfigReq = 18,
	Smsgs_cmdIds_lowBatteryConfigRsp = 19,
	Smsgs_cmdIds_distanceConfigReq = 20,
	Smsgs_cmdIds_distanceConfigRsp = 21,
    Smsgs_cmdIds_musicConfigReq = 22,
    Smsgs_cmdIds_musicConfigRsp = 23,
    Smsgs_cmdIds_intervalConfigReq = 24,
    Smsgs_cmdIds_intervalConfigRsp = 25,
    Smsgs_cmdIds_motionConfigReq = 26,
    Smsgs_cmdIds_motionConfigRsp = 27,
    Smsgs_cmdIds_resistanceConfigReq = 28,
    Smsgs_cmdIds_resistanceConfigRsp = 29,
    Smsgs_cmdIds_microwaveConfigReq = 30,
    Smsgs_cmdIds_microwaveConfigRsp = 31,
    Smsgs_cmdIds_pirConfigReq = 32,
    Smsgs_cmdIds_pirConfigRsp = 33,
    Smsgs_cmdIds_setUnsetConfigReq = 34,
    Smsgs_cmdIds_setUnsetConfigRsp = 35,
    Smsgs_cmdIds_disconnectReq = 36,
    Smsgs_cmdIds_disconnectRsp = 37,
    Smsgs_cmdIds_electricLockActionReq = 38,
    Smsgs_cmdIds_electricLockActionRsp = 39,
    Smsgs_cmdIds_antennaReq = 40,
    Smsgs_cmdIds_antennaRsp = 41,
    Smsgs_cmdIds_setIntervalReq = 42,
    Smsgs_cmdIds_setIntervalRsp = 43,

    Smsgs_cmdIds_removeDevice = 100,
    Smsgs_cmdIds_clean = 101
 } Smsgs_cmdIds_t;

/*!
 Frame Control field states what data fields are included in reported
 sensor data, each value is a bit mask value so that they can be combined
 (OR'd together) in a control field.
 When sent over-the-air in a message this field is 2 bytes.
 */
typedef enum
{
    /*! Message Statistics */
    Smsgs_dataFields_msgStats = 0x0001,
    /*! Config Settings */
    Smsgs_dataFields_configSettings = 0x0002,
    
    /*! ====================================================================== */
    /*! Remote control Sensor */
    Smsgs_dataFields_eve003Sensor = 0x0008,
    /*! Signal Sensor */
    Smsgs_dataFields_eve004Sensor = 0x0010,
    /*! EVE005 Sensor */
    Smsgs_dataFields_eve005Sensor = 0x0020,
    /*! PIR Sensor */
    Smsgs_dataFields_eve006Sensor = 0x0040,
} Smsgs_dataFields_t;

/*!
 Status values for the over-the-air messages
 */
typedef enum
{
    /*! Success */
    Smsgs_statusValues_success = 0,
    /*! Message was invalid and ignored */
    Smsgs_statusValues_invalid = 1,
    /*!
     Config message was received but only some frame control fields
     can be sent or the reportingInterval or pollingInterval fail
     range checks.
     */
    Smsgs_statusValues_partialSuccess = 2,
} Smsgs_statusValues_t;

/******************************************************************************
 Structures - Building blocks for the over-the-air sensor messages
 *****************************************************************************/

/*!
 Configuration Request message: sent from controller to the sensor.
 */
typedef struct _Smsgs_configreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! Reporting Interval */
    uint32_t reportingInterval;
    /*! Polling Interval */
    uint32_t pollingInterval;
} Smsgs_configReqMsg_t;

/*!
 Configuration Response message: sent from the sensor to the collector
 in response to the Configuration Request message.
 */
typedef struct _Smsgs_configrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! Reporting Interval - 4 bytes */
    uint32_t reportingInterval;
    /*! Polling Interval - 4 bytes */
    uint32_t pollingInterval;
} Smsgs_configRspMsg_t;

/*!
 Tracking Request message: sent from controller to the sensor.
 */
typedef struct _Smsgs_trackingreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
} Smsgs_trackingReqMsg_t;

/*!
 Tracking Response message: sent from the sensor to the collector
 in response to the Tracking Request message.
 */
typedef struct _Smsgs_trackingrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
} Smsgs_trackingRspMsg_t;

/*!
 Toggle LED Request message: sent from controller to the sensor.
 */
typedef struct _Smsgs_toggleledreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
} Smsgs_toggleLedReqMsg_t;

/*!
 Toggle LED Response message: sent from the sensor to the collector
 in response to the Toggle LED Request message.
 */
typedef struct _Smsgs_toggleledrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! LED State - 0 is off, 1 is on - 1 byte */
    uint8_t ledState;
} Smsgs_toggleLedRspMsg_t;

/*!
 Message Statistics Field
 */
typedef struct _Smsgs_msgstatsfield_t
{
    /*! total number of join attempts (associate request sent) */
    uint16_t joinAttempts;
    /*! total number of join attempts failed */
    uint16_t joinFails;
    /*! total number of sensor data messages attempted. */
    uint16_t msgsAttempted;
    /*! total number of sensor data messages sent. */
    uint16_t msgsSent;
    /*! total number of tracking requests received */
    uint16_t trackingRequests;
    /*! total number of tracking response attempted */
    uint16_t trackingResponseAttempts;
    /*! total number of tracking response success */
    uint16_t trackingResponseSent;
    /*! total number of config requests received */
    uint16_t configRequests;
    /*! total number of config response attempted */
    uint16_t configResponseAttempts;
    /*! total number of config response success */
    uint16_t configResponseSent;
    /*!
     Total number of Channel Access Failures.  These are indicated in MAC data
     confirms for MAC data requests.
     */
    uint16_t channelAccessFailures;
    /*!
     Total number of MAC ACK failures. These are indicated in MAC data
     confirms for MAC data requests.
     */
    uint16_t macAckFailures;
    /*!
     Total number of MAC data request failures, other than channel access
     failure or MAC ACK failures.
     */
    uint16_t otherDataRequestFailures;
    /*! Total number of sync loss failures received for sleepy devices. */
    uint16_t syncLossIndications;
    /*! Total number of RX Decrypt failures. */
    uint16_t rxDecryptFailures;
    /*! Total number of TX Encrypt failures. */
    uint16_t txEncryptFailures;
    /*! Total number of resets. */
    uint16_t resetCount;
    /*!
     Assert reason for the last reset -  0 - no reason, 2 - HAL/ICALL,
     3 - MAC, 4 - TIRTOS
     */
    uint16_t lastResetReason;
} Smsgs_msgStatsField_t;

/*!
 Message Statistics Field
 */
typedef struct _Smsgs_configsettingsfield_t
{
    /*!
     Reporting Interval - in millseconds, how often to report, 0
     means reporting is off
     */
    uint32_t reportingInterval;
    /*!
     Polling Interval - in millseconds (32 bits) - If the sensor device is
     a sleep device, this states how often the device polls its parent for
     data. This field is 0 if the device doesn't sleep.
     */
    uint32_t pollingInterval;
} Smsgs_configSettingsField_t;

/*! ====================================================================== */
/*!
 Remote control Sensor Field
 */
typedef struct _Smsgs_eve003sensorfield_t
{
    /*! 
    	bit0: button1
    	bit1: button2
    	bit2: button3
    	bit3: button state, 0: short, 1: long
     */
    uint8_t button;
    /*! 
    	bit0: eve003 type, 0 = 客戶, 1 = 巡查
    	bit1: alarm enable
    	bit2: button3 type, 0 = 分區, 1 = 電鎖
     */
    uint8_t dip;
    /*! 
    	bit0: reset
    	bit1: pairing
        bit2: clean
     */
    uint8_t setting;
    /*! battery Data read out of the remote control sensor */
    uint16_t battery;
} Smsgs_eve003SensorField_t;
/*!
 Signal Sensor Field
 */
typedef struct _Smsgs_eve004sensorfield_t
{
	/*! 
      bit0~6: temp value
      bit7:   0(+) 1(-)
    */
    uint8_t tempValue;//
    /* 0:未觸發, 1:觸發
       bit0:G-Sensor state
       bit1:巡察鈕
       bit2:reset
       bit3:pairing
       bit4:relay1 state
       bit5:relay2 state
       bit6:clean
    */
  	uint8_t state;
  	/*!
  		bit0:temp alarm(5%)
  		bit1:temp abnormal
  		bit2:Low battery alarm(5%)
  		bit3:Low battery abnormal
  	*/
  	uint8_t alarm;
  	/*! 
  		bit0:12V output enable
  		bit1:electric lock enable
  		bit2:reserved
  		bit3:reserved
  		bit4:id0
  		bit5:id1
  		bit6:id2
  		bit7:electric lock , 0:NC->NO, 1:NO->NC
  	*/
  	uint8_t dip;
  	uint8_t battery;
  	uint8_t rssi;
} Smsgs_eve004SensorField_t;
/*!
 EVE005 Sensor Field
 */
typedef struct _Smsgs_eve005sensorfield_t
{
    /*! 
      bit0~6: temp value
      bit7:   0(+) 1(-)
    */
    uint8_t tmpValue;
    /*!
  		battery value
  	*/
    uint8_t batValue;
    uint8_t rssi;
    /*!
  		bit0:reset
  		bit1:paring
        bit2:clean
  	*/
    uint8_t button;
    uint8_t dip_id;
    /*!
  		bit0:Z1 enable
  		bit1:Z2 enable
  		bit2:12V enable
  		bit3:磁簧 enable
  	*/
  	uint8_t dip_control;
    /*!
  		bit0:磁簧 0=close, 1= open
  		bit1:Z1 2k
  		bit2:Z2 2k
  		bit3:Z1
  		bit4:Z2
  		bit5:G-Sensor
  	*/
    uint8_t sensor_s;
    /*!
        bit0:temp alarm(5%)
        bit1:temp abnormal
        bit2:Low battery alarm(5%)
        bit3:Low battery abnormal
    */
    uint8_t alarm;
    /*!
        0k - 4.4k = (0 - 44)
    */
    uint8_t resist1;
    uint8_t resist2;
} Smsgs_eve005SensorField_t;
/*!
 PIR Sensor Field
 */
typedef struct _Smsgs_eve006sensorfield_t
{
    /*! 
      bit0: reset
      bit1: pairing
      bit2: clean
    */
    uint8_t button;
    /*!
      bit0: pir status
      bit1: mircrowave state
      bit2: 2.2k IO state1
      bit3: 2.2k IO loop1
      bit4: 2.2k IO state2
      bit5: 2.2k IO loop2
    */
    uint8_t state;
    /*! 
      bit0: G-Sensor abnormal 
      bit1: Temp alarm
      bit2: Temp abnormal
      bit3: battery alarm
      bit4: battery abnormal
      bit5: pir abnormal
    */
    uint8_t abnormal;
    /*!
      bit0: IO1 enable
      bit1: IO2 enable
      bit2: 12V enable
      bit3: Reserved
    */
    uint8_t dip;
    uint8_t dip_id;
    /*! 
      bit0~6: temp value
      bit7:   0(+) 1(-)
    */
    uint8_t temp;
    uint8_t humidity;
    uint8_t battery;
    /*!
        0k - 4.4k = (0 - 44)
    */
    uint8_t resist1;
    uint8_t resist2;
} Smsgs_eve006SensorField_t;

/*!
 Sensor Data message: sent from the sensor to the collector
 */
typedef struct _Smsgs_sensormsg_t
{
    /*! Command ID */
    Smsgs_cmdIds_t cmdId;
    /*! Extended Address */
    uint8_t extAddress[SMGS_SENSOR_EXTADDR_LEN];
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*!
     Message Statistics field - valid only if Smsgs_dataFields_msgStats
     is set in frameControl.
     */
    Smsgs_msgStatsField_t msgStats;
    /*!
     Configuration Settings field - valid only if
     Smsgs_dataFields_configSettings is set in frameControl.
     */
    Smsgs_configSettingsField_t configSettings;
    /*! ====================================================================== */
    /*!
     Remote control Sensor field - valid only if Smsgs_dataFields_eve003Sensor
     is set in frameControl.
     */
    Smsgs_eve003SensorField_t eve003Sensor;
    /*!
     Signal Sensor field - valid only if Smsgs_dataFields_eve004Sensor
     is set in frameControl.
     */
    Smsgs_eve004SensorField_t eve004Sensor;
    /*!
     EVE005 Sensor field - valid only if Smsgs_dataFields_eve005Sensor
     is set in frameControl.
     */
    Smsgs_eve005SensorField_t eve005Sensor;
    /*!
     PIR Sensor field - valid only if Smsgs_dataFields_eve006Sensor
     is set in frameControl.
     */
    Smsgs_eve006SensorField_t eve006Sensor;
    
} Smsgs_sensorMsg_t;

/*! ====================================================================== */
typedef struct _Smsgs_alarmconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! time, second */
    uint16_t time;
} Smsgs_alarmConfigReqMsg_t;

typedef struct _Smsgs_alarmconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! time, second */
    uint16_t time;
} Smsgs_alarmConfigRspMsg_t;

typedef struct _Smsgs_gsensorconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* bit0:G-Sensor ,bit1:speaker */
    uint8_t enable;
    uint8_t sensitivity;
} Smsgs_gSensorConfigReqMsg_t;

typedef struct _Smsgs_gsensorconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* bit0:G-Sensor ,bit1:speaker */
    uint8_t enable;
    uint8_t sensitivity;
} Smsgs_gSensorConfigRspMsg_t;

typedef struct _Smsgs_electriclockconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* enable */
    uint8_t enable;
    /* time, second */
    uint8_t time;
} Smsgs_electricLockConfigReqMsg_t;

typedef struct _Smsgs_electriclockconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* enable */
    uint8_t enable;
    /* time, second */
    uint8_t time;
} Smsgs_electricLockConfigRspMsg_t;

typedef struct _Smsgs_signalconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* bit0: manual, bit1: auto */
    uint8_t mode;
    /* value */
    uint8_t value;
    /* offset */
    uint8_t offset;
} Smsgs_signalConfigReqMsg_t;

typedef struct _Smsgs_signalconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* bit0: manual, bit1: auto */
    uint8_t mode;
    /* value */
    uint8_t value;
    /* offset */
    uint8_t offset;
} Smsgs_signalConfigRspMsg_t;

typedef struct _Smsgs_tempconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* value */
    uint8_t value;
    /* offset */
    uint8_t offset;
} Smsgs_tempConfigReqMsg_t;

typedef struct _Smsgs_tempconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* value */
    uint8_t value;
    /* offset */
    uint8_t offset;
} Smsgs_tempConfigRspMsg_t;

typedef struct _Smsgs_lowbatteryconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* value */
    uint8_t value;
    /* offset */
    uint8_t offset;
} Smsgs_lowBatteryConfigReqMsg_t;

typedef struct _Smsgs_lowbatteryconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /* value */
    uint8_t value;
    /* offset */
    uint8_t offset;
} Smsgs_lowBatteryConfigRspMsg_t;

typedef struct _Smsgs_distanceconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! 1:auto  0:manual */
    uint8_t mode;
    //*! distance, meter */
    uint16_t distance;
} Smsgs_distanceConfigReqMsg_t;

typedef struct _Smsgs_distanceconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! 1:auto  0:manual */
    uint8_t mode;
    //*! distance, meter */
    uint16_t distance;
} Smsgs_distanceConfigRspMsg_t;

typedef struct _Smsgs_musicconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! music mode
        bit0~3: music mode
        bit4: type, 0 = use bit 0~3, 1 = use bit 5
        bit5: 0 = disable, 1 = enable
     */
    uint8_t mode;
    //*! time, second */
    uint16_t time;
} Smsgs_musicConfigReqMsg_t;

typedef struct _Smsgs_musicconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! music mode */
    uint8_t mode;
    //*! time, second */
    uint16_t time;
} Smsgs_musicConfigRspMsg_t;

typedef struct _Smsgs_intervalconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! 1:auto  0:manual */
    uint8_t mode;
    //*! time sec  */
    uint16_t time;
} Smsgs_intervalConfigReqMsg_t;

typedef struct _Smsgs_intervalconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! 1:auto  0:manual */
    uint8_t mode;
    //*! time sec */
    uint16_t time;
} Smsgs_intervalConfigRspMsg_t;

typedef struct _Smsgs_motionconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! count */
    uint8_t count;
    //*! time sec  */
    uint16_t time;
} Smsgs_motionConfigReqMsg_t;

typedef struct _Smsgs_motionconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! count */
    uint8_t count;
    //*! time sec  */
    uint16_t time;
} Smsgs_motionConfigRspMsg_t;

typedef struct _Smsgs_resistanceconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! value 0~% */
    uint8_t value;
} Smsgs_resistanceConfigReqMsg_t;

typedef struct _Smsgs_resistanceconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! value 0~% */
    uint8_t value;
} Smsgs_resistanceConfigRspMsg_t;

typedef struct _Smsgs_microwaveconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! enable */
    uint8_t enable;
    //*! sensitivity  */
    uint8_t sensitivity;
} Smsgs_microwaveConfigReqMsg_t;

typedef struct _Smsgs_microwaveconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! enable */
    uint8_t enable;
    //*! sensitivity  */
    uint8_t sensitivity;
} Smsgs_microwaveConfigRspMsg_t;

typedef struct _Smsgs_pirconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! enable */
    uint8_t enable;
} Smsgs_pirConfigReqMsg_t;

typedef struct _Smsgs_pirconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! enable */
    uint8_t enable;
} Smsgs_pirConfigRspMsg_t;

typedef struct _Smsgs_setunsetconfigreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! state, 1 = set, 0 = unset */
    uint8_t state;
} Smsgs_setUnsetConfigReqMsg_t;

typedef struct _Smsgs_setunsetconfigrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! state, 1 = set, 0 = unset */
    uint8_t state;
} Smsgs_setUnsetConfigRspMsg_t;

typedef struct _Smsgs_disconnectreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! delay time, second, if time is zero then immediately discoonected. */
    uint16_t time;
} Smsgs_disconnectReqMsg_t;

typedef struct _Smsgs_disconnectrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! delay time, second, if time is zero then immediately discoonected. */
    uint16_t time;
} Smsgs_disconnectRspMsg_t;

typedef struct _Smsgs_electriclockactionreqmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Frame Control field - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! bit0:relay1 action
        bit1:relay2 action */
    uint8_t relay;
} Smsgs_electricLockActionReqMsg_t;

typedef struct _Smsgs_electriclockactionrspmsg_t
{
    /*! Command ID - 1 byte */
    Smsgs_cmdIds_t cmdId;
    /*! Response Status - 2 bytes */
    Smsgs_statusValues_t status;
    /*! Frame Control field - 2 bytes - bit mask of Smsgs_dataFields */
    uint16_t frameControl;
    /*! bit0:relay1 action
        bit1:relay2 action */
    uint8_t relay;
} Smsgs_electricLockActionRspMsg_t;

typedef struct _Smsgs_antennareqmsg_t
{
    uint8_t state;
} Smsgs_antennaReqMsg_t;

typedef struct _Smsgs_antennarspmsg_t
{
    uint8_t state;
} Smsgs_antennaRspMsg_t;

typedef struct _Smsgs_setintervalreqmsg_t
{
    uint32_t reporting;
    uint32_t polling;
} Smsgs_setIntervalReqMsg_t;

typedef struct _Smsgs_setintervalrspmsg_t
{
    uint32_t reporting;
    uint32_t polling;
} Smsgs_setIntervalRspMsg_t;


#ifdef __cplusplus
}
#endif

#endif /* SMGSS_H */

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
