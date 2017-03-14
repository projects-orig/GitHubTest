/******************************************************************************

 @file collector.h

 @brief TIMAC 2.0 Collector Example Application Header

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
#ifndef COLLECTOR_H
#define COLLECTOR_H

/******************************************************************************
 Includes
 *****************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "api_mac.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 Constants and definitions
 *****************************************************************************/

/*! Event ID - Start the device in the network */
#define COLLECTOR_START_EVT 0x0001
/*! Event ID - Tracking Timeout Event */
#define COLLECTOR_TRACKING_TIMEOUT_EVT 0x0002
/*! Event ID - Generate Configs Event */
#define COLLECTOR_CONFIG_EVT 0x0004

/* Default configuration reporting interval, in milliseconds */
#define CONFIG_REPORTING_INTERVAL 30000 //90000

/* Default configuration polling interval, in milliseconds */
#define CONFIG_POLLING_INTERVAL 2000

/*! Collector Status Values */
typedef enum
{
    /*! Success */
    Collector_status_success = 0,
    /*! Device Not Found */
    Collector_status_deviceNotFound = 1,
    /*! Collector isn't in the correct state to send a message */
    Collector_status_invalid_state = 2
} Collector_status_t;

/******************************************************************************
 Structures
 *****************************************************************************/

/*! Collector Statistics */
typedef struct
{
    /*!
     Total number of tracking request messages attempted
     */
    uint32_t trackingRequestAttempts;
    /*!
     Total number of tracking request messages sent
     */
    uint32_t trackingReqRequestSent;
    /*!
     Total number of tracking response messages received
     */
    uint32_t trackingResponseReceived;
    /*!
     Total number of config request messages attempted
     */
    uint32_t configRequestAttempts;
    /*!
     Total number of config request messages sent
     */
    uint32_t configReqRequestSent;
    /*!
     Total number of config response messages received
     */
    uint32_t configResponseReceived;
    /*!
     Total number of sensor messages received
     */
    uint32_t sensorMessagesReceived;
    /*!
     Total number of failed messages because of channel access failure
     */
    uint32_t channelAccessFailures;
    /*!
     Total number of failed messages because of ACKs not received
     */
    uint32_t ackFailures;
    /*!
     Total number of failed transmit messages that are not channel access
     failure and not ACK failures
     */
    uint32_t otherTxFailures;
    /*! Total number of RX Decrypt failures. */
    uint32_t rxDecryptFailures;
    /*! Total number of TX Encrypt failures. */
    uint32_t txEncryptFailures;
    /* Total Transaction Expired Count */
    uint32_t txTransactionExpired;
    /* Total transaction Overflow error */
    uint32_t txTransactionOverflow;
} Collector_statistics_t;

/******************************************************************************
 Global Variables
 *****************************************************************************/

/*! Collector events flags */
extern uint16_t Collector_events;

/*! Collector statistics */
extern Collector_statistics_t Collector_statistics;

extern ApiMac_callbacks_t Collector_macCallbacks;

/******************************************************************************
 Function Prototypes
 *****************************************************************************/

/*!
 * @brief Initialize this application.
 */
extern void Collector_init(void);

/*!
 * @brief Application task processing.
 */
extern void Collector_process(void);

/*!
 * @brief Build and send the configuration message to a device.
 *
 * @param pDstAddr - destination address of the device to send the message
 * @param frameControl - configure what to the device is to report back.
 *                       Ref. Smsgs_dataFields_t.
 * @param reportingInterval - in milliseconds- how often to report, 0
 *                            means to turn off automated reporting, but will
 *                            force the sensor device to send the Sensor Data
 *                            message once.
 * @param pollingInterval - in milliseconds- how often to the device is to
 *                          poll its parent for data (for sleeping devices
 *                          only.
 *
 * @return Collector_status_success, Collector_status_invalid_state
 *         or Collector_status_deviceNotFound
 */
extern Collector_status_t Collector_sendConfigRequest(ApiMac_sAddr_t *pDstAddr,
                uint16_t frameControl,
                uint32_t reportingInterval,
                uint32_t pollingInterval);

/*!
 * @brief Update the collector statistics
 */
extern void Collector_updateStats( void );

/*!
 * @brief Build and send the toggle led message to a device.
 *
 * @param pDstAddr - destination address of the device to send the message
 *
 * @return Collector_status_success, Collector_status_invalid_state
 *         or Collector_status_deviceNotFound
 */
extern Collector_status_t Collector_sendToggleLedRequest(
                ApiMac_sAddr_t *pDstAddr);

extern Collector_status_t Collector_sendAntennaRequest(uint8_t state);

extern Collector_status_t Collector_setInterval(ApiMac_sAddr_t *pDstAddr, uint32_t reporting, uint32_t polling);

/*! ===================================================================== */
/*!
 * @brief Build and send the Alarm Config message to a device.
 *
 * @param pDstAddr - destination address of the device to send the message
 * @param frameControl - configure what to the device is to report back.
 *                       Ref. Smsgs_dataFields_t.
 * @param time - time second
 *
 * @return Collector_status_success, Collector_status_invalid_state
 *         or Collector_status_deviceNotFound
 */
extern Collector_status_t Collector_sendAlarmConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint16_t time);

extern Collector_status_t Collector_sendGSensorConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable,
                                               uint8_t sensitivity);
                                               
extern Collector_status_t Collector_sendElectricLockConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable,
                                               uint8_t time);
                                               
extern Collector_status_t Collector_sendSignalConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint8_t value,
                                               uint8_t offset);
                                               
extern Collector_status_t Collector_sendTempConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t value,
                                               uint8_t offset);
                                               
extern Collector_status_t Collector_sendLowBatteryConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t value,
                                               uint8_t offset);
                                               
extern Collector_status_t Collector_sendDistanceConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint16_t distance);

extern Collector_status_t Collector_sendMusicConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint16_t music);

extern Collector_status_t Collector_sendIntervalConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint16_t time);

extern Collector_status_t Collector_sendMotionConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t count,
                                               uint16_t time);

extern Collector_status_t Collector_sendResistanceConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t value);

extern Collector_status_t Collector_sendMicrowaveConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable,
                                               uint8_t sensitivity);

extern Collector_status_t Collector_sendPirConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable);

extern Collector_status_t Collector_sendSetUnsetConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t state);

extern Collector_status_t Collector_sendDisconnectRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint16_t time);

extern Collector_status_t Collector_sendElectricLockActionRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t relay);

#ifdef __cplusplus
}
#endif

#endif /* COLLECTOR_H */

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
