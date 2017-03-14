/*implements an example application which starts the network,allows new devices to join network,configures the joining device on how often to report the sensor data , for sleepy devices configures how often to poll for buffered messages in case of non-beacon and frequency hopping mode of network operation,and tracks if connected devices are active on the network or not by periodically sending the tracking request message to which it expects the tracking response message.*/

/******************************************************************************
 Includes
 *****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "util.h"
#include "api_mac.h"
#include "cllc.h"
#include "csf.h"
#include "smsgs.h"
#include "collector.h"
#include "log.h"
/******************************************************************************
 Constants and definitions
 *****************************************************************************/

#if !defined(STATIC)
/* make local */
#define STATIC static
#endif

#if !defined(CONFIG_AUTO_START)
#if defined(AUTO_START)
#define CONFIG_AUTO_START 1
#else
#define CONFIG_AUTO_START 0
#endif
#endif

/* beacon order for non beacon network */
#define NON_BEACON_ORDER      15

/* MAC Indirect Persistent Timeout */
#define INDIRECT_PERSISTENT_TIME 750

/* default MSDU Handle rollover */
#define MSDU_HANDLE_MAX 0x3F

/* App marker in MSDU handle */
#define APP_MARKER_MSDU_HANDLE 0x80

/* App Config request marker for the MSDU handle */
#define APP_CONFIG_MSDU_HANDLE 0x40

/*! =================================================== */
/* Default configuration frame control */   
#define CONFIG_FRAME_CONTROL (Smsgs_dataFields_msgStats | \
                              Smsgs_dataFields_configSettings | \
                              Smsgs_dataFields_eve003Sensor | \
                              Smsgs_dataFields_eve004Sensor | \
                              Smsgs_dataFields_eve005Sensor | \
                              Smsgs_dataFields_eve006Sensor)

/* Delay for config request retry in busy network */
#define CONFIG_DELAY 1000
#define CONFIG_RESPONSE_DELAY 3*CONFIG_DELAY
/* Tracking timeouts */
#define TRACKING_CNF_DELAY_TIME 2000 /* in milliseconds */
#define TRACKING_DELAY_TIME 60000 /* in milliseconds */
#define TRACKING_TIMEOUT_TIME (CONFIG_POLLING_INTERVAL * 2) /*in milliseconds*/

/* Assoc Table (CLLC) status settings */
#define ASSOC_CONFIG_SENT       0x0100    /* Config Req sent */
#define ASSOC_CONFIG_RSP        0x0200    /* Config Rsp received */
#define ASSOC_CONFIG_MASK       0x0300    /* Config mask */
#define ASSOC_TRACKING_SENT     0x1000    /* Tracking Req sent */
#define ASSOC_TRACKING_RSP      0x2000    /* Tracking Rsp received */
#define ASSOC_TRACKING_RETRY    0x4000    /* Tracking Req retried */
#define ASSOC_TRACKING_ERROR    0x8000    /* Tracking Req error */
#define ASSOC_TRACKING_MASK     0xF000    /* Tracking mask  */
/******************************************************************************
 Global variables
 *****************************************************************************/

/* Task pending events */
uint16_t Collector_events = 0;

/*! Collector statistics */
Collector_statistics_t Collector_statistics;

/******************************************************************************
 Local variables
 *****************************************************************************/

static void *sem;

/*! true if the device was restarted */
static bool restarted = false;

/*! CLLC State */
STATIC Cllc_states_t cllcState = Cllc_states_initWaiting;

/*! Device's PAN ID */
STATIC uint16_t devicePanId = 0xFFFF;

/*! Device's Outgoing MSDU Handle values */
STATIC uint8_t deviceTxMsduHandle = 0;

STATIC bool fhEnabled = false;

/******************************************************************************
 Local function prototypes
 *****************************************************************************/
static void initializeClocks(void);
static void cllcStartedCB(Llc_netInfo_t *pStartedInfo);
static ApiMac_assocStatus_t cllcDeviceJoiningCB(
                ApiMac_deviceDescriptor_t *pDevInfo,
                ApiMac_capabilityInfo_t *pCapInfo);
static void cllcStateChangedCB(Cllc_states_t state);
static void dataCnfCB(ApiMac_mcpsDataCnf_t *pDataCnf);
static void dataIndCB(ApiMac_mcpsDataInd_t *pDataInd);
static void processStartEvent(void);
static void processConfigResponse(ApiMac_mcpsDataInd_t *pDataInd);
static void processTrackingResponse(ApiMac_mcpsDataInd_t *pDataInd);
static void processToggleLedResponse(ApiMac_mcpsDataInd_t *pDataInd);

/*! ========================================================================== */
static void processCustomConfigResponse(ApiMac_mcpsDataInd_t *pDataInd);

static int processAlarmConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf);
static int processGSensorConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf);
static int processElectricLockConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf);
static int processSignalConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf);
static int processTempConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf);
static int processLowBatteryConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf);
static int processDistanceConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf);
static int processMusicConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processIntervalConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processMotionConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processResistanceConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processMicrowaveConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processPirConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processSetUnsetConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processDisconnectResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);
static int processElectricLockActionResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf);

static void processSensorData(ApiMac_mcpsDataInd_t *pDataInd);
static Cllc_associated_devices_t *findDevice(ApiMac_sAddr_t *pAddr);
static Cllc_associated_devices_t *findDeviceStatusBit(uint16_t mask, uint16_t statusBit);
static uint8_t getMsduHandle(Smsgs_cmdIds_t msgType);
static void sendMsg(Smsgs_cmdIds_t type, uint16_t dstShortAddr, bool rxOnIdle,
                    uint16_t len,
                    uint8_t *pData);
static void generateConfigRequests(void);
static void generateTrackingRequests(void);
static void sendTrackingRequest(Cllc_associated_devices_t *pDev);
static void commStatusIndCB(ApiMac_mlmeCommStatusInd_t *pCommStatusInd);
static void pollIndCB(ApiMac_mlmePollInd_t *pPollInd);
static void processDataRetry(ApiMac_sAddr_t *pAddr);
static void processConfigRetry(void);
static void pairingIndCB(void);
static void antennaIndCB(ApiMac_antennaInd_t *pAntennaInd);

/******************************************************************************
 Callback tables
 *****************************************************************************/

/*! API MAC Callback table */
ApiMac_callbacks_t Collector_macCallbacks =
    {
      /*! Associate Indicated callback */
      NULL,
      /*! Associate Confirmation callback */
      NULL,
      /*! Disassociate Indication callback */
      NULL,
      /*! Disassociate Confirmation callback */
      NULL,
      /*! Beacon Notify Indication callback */
      NULL,
      /*! Orphan Indication callback */
      NULL,
      /*! Scan Confirmation callback */
      NULL,
      /*! Start Confirmation callback */
      NULL,
      /*! Sync Loss Indication callback */
      NULL,
      /*! Poll Confirm callback */
      NULL,
      /*! Comm Status Indication callback */
      commStatusIndCB,
      /*! Poll Indication Callback */
      pollIndCB,
      /*! Data Confirmation callback */
      dataCnfCB,
      /*! Data Indication callback */
      dataIndCB,
      /*! Purge Confirm callback */
      NULL,
      /*! WiSUN Async Indication callback */
      NULL,
      /*! WiSUN Async Confirmation callback */
      NULL,
      /*! Unprocessed message callback */
      NULL,
      /*! process pairing Indication callback */
      pairingIndCB,
      /*! process antenna Indication callback */
      antennaIndCB
    };

STATIC Cllc_callbacks_t cllcCallbacks =
    {
      /*! Coordinator Started Indication callback */
      cllcStartedCB,
      /*! Device joining callback */
      cllcDeviceJoiningCB,
      /*! The state has changed callback */
      cllcStateChangedCB
    };

/******************************************************************************
 Public Functions
 *****************************************************************************/

/*!
 Initialize this application.

 Public function defined in collector.h
 */
void Collector_init(void)
{
    /* Initialize the collector's statistics */
    memset(&Collector_statistics, 0, sizeof(Collector_statistics_t));

    /* Initialize the MAC */
    sem = ApiMac_init(CONFIG_FH_ENABLE);

    /* Initialize the Coordinator Logical Link Controller */
    Cllc_init(&Collector_macCallbacks, &cllcCallbacks);

    /* Register the MAC Callbacks */
    ApiMac_registerCallbacks(&Collector_macCallbacks);

    /* Initialize the platform specific functions */
    Csf_init(sem);

    /* Set the indirect persistent timeout */
    ApiMac_mlmeSetReqUint16(ApiMac_attribute_transactionPersistenceTime,
                            INDIRECT_PERSISTENT_TIME);
    ApiMac_mlmeSetReqUint8(ApiMac_attribute_phyTransmitPowerSigned,
                           (uint8_t)CONFIG_TRANSMIT_POWER);

    /* Initialize the app clocks */
    initializeClocks();

    if(CONFIG_AUTO_START)
    {
        /* Start the device */
        Util_setEvent(&Collector_events, COLLECTOR_START_EVT);
    }
}

/*!
 Application task processing.

 Public function defined in collector.h
 */
void Collector_process(void)
{
    /* Start the collector device in the network */
    if(Collector_events & COLLECTOR_START_EVT)
    {
        if(cllcState == Cllc_states_initWaiting)
        {
            processStartEvent();
        }

        /* Clear the event */
        Util_clearEvent(&Collector_events, COLLECTOR_START_EVT);
    }

    /* Is it time to send the next tracking message? */
    if(Collector_events & COLLECTOR_TRACKING_TIMEOUT_EVT)
    {
        /* Process Tracking Event */
        generateTrackingRequests();

        /* Clear the event */
        Util_clearEvent(&Collector_events, COLLECTOR_TRACKING_TIMEOUT_EVT);
    }

    /*
     The generate a config request for all associated devices that need one
     */
    if(Collector_events & COLLECTOR_CONFIG_EVT)
    {
        generateConfigRequests();

        /* Clear the event */
        Util_clearEvent(&Collector_events, COLLECTOR_CONFIG_EVT);
    }

    /* Process LLC Events */
    Cllc_process();

    /* Allow the Specific functions to process */
    Csf_processEvents();

    /*
     Don't process ApiMac messages until all of the collector events
     are processed.
     */
    if(Collector_events == 0)
    {
        /* Wait for response message or events */
        ApiMac_processIncoming();
    }
}

/*!
 Build and send the configuration message to a device.

 Public function defined in collector.h
 */
Collector_status_t Collector_sendConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint32_t reportingInterval,
                                               uint32_t pollingInterval)
{
    Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_CONFIG_REQUEST_MSG_LENGTH];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_configReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = Util_breakUint32(reportingInterval, 0);
            *pBuf++ = Util_breakUint32(reportingInterval, 1);
            *pBuf++ = Util_breakUint32(reportingInterval, 2);
            *pBuf++ = Util_breakUint32(reportingInterval, 3);
            *pBuf++ = Util_breakUint32(pollingInterval, 0);
            *pBuf++ = Util_breakUint32(pollingInterval, 1);
            *pBuf++ = Util_breakUint32(pollingInterval, 2);
            *pBuf = Util_breakUint32(pollingInterval, 3);

            sendMsg(Smsgs_cmdIds_configReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_CONFIG_REQUEST_MSG_LENGTH),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

/*!
 Update the collector statistics

 Public function defined in collector.h
 */
void Collector_updateStats( void )
{
    /* update the stats from the MAC */
    ApiMac_mlmeGetReqUint32(ApiMac_attribute_diagRxSecureFail,
                            &Collector_statistics.rxDecryptFailures);

    ApiMac_mlmeGetReqUint32(ApiMac_attribute_diagTxSecureFail,
                            &Collector_statistics.txEncryptFailures);
}

/*!
 Build and send the toggle led message to a device.

 Public function defined in collector.h
 */
Collector_status_t Collector_sendToggleLedRequest(ApiMac_sAddr_t *pDstAddr)
{
    Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_TOGGLE_LED_REQUEST_MSG_LEN];

            /* Build the message */
            buffer[0] = (uint8_t)Smsgs_cmdIds_toggleLedReq;

            sendMsg(Smsgs_cmdIds_toggleLedReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    SMSGS_TOGGLE_LED_REQUEST_MSG_LEN,
                    buffer);

            status = Collector_status_success;
        }
        else
        {
            status = Collector_status_deviceNotFound;
        }
    }

    return(status);
}

Collector_status_t Collector_sendAntennaRequest(uint8_t state)
{
    Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        ApiMac_antennaReq_t dataReq;

        /* Fill the data request field */
        memset(&dataReq, 0, sizeof(ApiMac_antennaReq_t));

        dataReq.status = state;

        ApiMac_antennaReq(&dataReq);
        status = Collector_status_success;
    }

    return (status);
}

Collector_status_t Collector_setInterval(ApiMac_sAddr_t *pDstAddr, uint32_t reporting, uint32_t polling)
{
    Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            ApiMac_status_t result = Cllc_setInterval(&item.devInfo.extAddress, reporting, polling);
            if (result == ApiMac_status_success) {
                Cllc_associated_devices_t *pDev;
                pDev = findDevice(pDstAddr);
                if(pDev != NULL)
                {
                    pDev->status &= ~ASSOC_CONFIG_SENT;
                    pDev->status &= ~ASSOC_CONFIG_RSP;
                }
        
                Util_setEvent(&Collector_events, COLLECTOR_CONFIG_EVT);
                status = Collector_status_success;
            }
        }
        else
        {
            status = Collector_status_deviceNotFound;
        }
    }
    return(status);
}

/******************************************************************************
 Local Functions
 *****************************************************************************/

/*!
 * @brief       Initialize the clocks.
 */
static void initializeClocks(void)
{
    /* Initialize the tracking clock */
    Csf_initializeTrackingClock();
    Csf_initializeConfigClock();
}

/*!
 * @brief      CLLC Started callback.
 *
 * @param      pStartedInfo - pointer to network information
 */
static void cllcStartedCB(Llc_netInfo_t *pStartedInfo)
{
    devicePanId = pStartedInfo->devInfo.panID;
    if(pStartedInfo->fh == true)
    {
        fhEnabled = true;
    }

    /* updated the user */
    Csf_networkUpdate(restarted, pStartedInfo);

    /* Start the tracking clock */
    Csf_setTrackingClock(TRACKING_DELAY_TIME);
}

/*!
 * @brief      Device Joining callback from the CLLC module (ref.
 *             Cllc_deviceJoiningFp_t in cllc.h).  This function basically
 *             gives permission that the device can join with the return
 *             value.
 *
 * @param      pDevInfo - device information
 * @param      capInfo - device's capability information
 *
 * @return     ApiMac_assocStatus_t
 */
static ApiMac_assocStatus_t cllcDeviceJoiningCB(
                ApiMac_deviceDescriptor_t *pDevInfo,
                ApiMac_capabilityInfo_t *pCapInfo)
{
    ApiMac_assocStatus_t status;

    /* Make sure the device is in our PAN */
    if(pDevInfo->panID == devicePanId)
    {
        /* Update the user that a device is joining */
        status = Csf_deviceUpdate(pDevInfo, pCapInfo, CONFIG_REPORTING_INTERVAL, CONFIG_POLLING_INTERVAL);
        if(status==ApiMac_assocStatus_success)
        {
            /* Add device to security device table */
            Cllc_addSecDevice(pDevInfo->panID,
                              pDevInfo->shortAddress,
                              &pDevInfo->extAddress, 0);

            Util_setEvent(&Collector_events, COLLECTOR_CONFIG_EVT);
        }
    }
    else
    {
        status = ApiMac_assocStatus_panAccessDenied;
    }
    return (status);
}

/*!
 * @brief     CLLC State Changed callback.
 *
 * @param     state - CLLC new state
 */
static void cllcStateChangedCB(Cllc_states_t state)
{
    /* Save the state */
    cllcState = state;

    /* Notify the user interface */
    Csf_stateChangeUpdate(cllcState);
}

/*!
 * @brief      MAC Data Confirm callback.
 *
 * @param      pDataCnf - pointer to the data confirm information
 */
static void dataCnfCB(ApiMac_mcpsDataCnf_t *pDataCnf)
{
    /* Record statistics */
    if(pDataCnf->status == ApiMac_status_channelAccessFailure)
    {
        Collector_statistics.channelAccessFailures++;
    }
    else if(pDataCnf->status == ApiMac_status_noAck)
    {
        Collector_statistics.ackFailures++;
    }
    else if(pDataCnf->status == ApiMac_status_transactionExpired)
    {
        Collector_statistics.txTransactionExpired++;
    }
    else if(pDataCnf->status == ApiMac_status_transactionOverflow)
    {
        Collector_statistics.txTransactionOverflow++;
    }
    else if(pDataCnf->status == ApiMac_status_success)
    {
        Csf_updateFrameCounter(NULL, pDataCnf->frameCntr);
    }
    else if(pDataCnf->status != ApiMac_status_success)
    {
        Collector_statistics.otherTxFailures++;
    }

    /* Make sure the message came from the app */
    if(pDataCnf->msduHandle & APP_MARKER_MSDU_HANDLE)
    {
        /* What message type was the original request? */
        if(pDataCnf->msduHandle & APP_CONFIG_MSDU_HANDLE)
        {
            /* Config Request */
            Cllc_associated_devices_t *pDev;
            pDev = findDeviceStatusBit(ASSOC_CONFIG_MASK, ASSOC_CONFIG_SENT);
            if(pDev != NULL)
            {
                if(pDataCnf->status != ApiMac_status_success)
                {
                    /* Try to send again */
                    pDev->status &= ~ASSOC_CONFIG_SENT;
                    Csf_setConfigClock(CONFIG_DELAY);
                }
                else
                {
                    pDev->status |= ASSOC_CONFIG_SENT;
                    pDev->status |= ASSOC_CONFIG_RSP;
                    pDev->status |= CLLC_ASSOC_STATUS_ALIVE;
                    Csf_setConfigClock(CONFIG_RESPONSE_DELAY);
                }
            }

            /* Update stats */
            if(pDataCnf->status == ApiMac_status_success)
            {
                Collector_statistics.configReqRequestSent++;
            }
        }
        else
        {
            /* Tracking Request */
            Cllc_associated_devices_t *pDev;
            pDev = findDeviceStatusBit(ASSOC_TRACKING_SENT,
                                       ASSOC_TRACKING_SENT);
            if(pDev != NULL)
            {
                if(pDataCnf->status == ApiMac_status_success)
                {
                    /* Make sure the retry is clear */
                    pDev->status &= ~ASSOC_TRACKING_RETRY;
                }
                else
                {
                    if(pDev->status & ASSOC_TRACKING_RETRY)
                    {
                        /* We already tried to resend */
                        pDev->status &= ~ASSOC_TRACKING_RETRY;
                        pDev->status |= ASSOC_TRACKING_ERROR;
                    }
                    else
                    {
                        /* Go ahead and retry */
                        pDev->status |= ASSOC_TRACKING_RETRY;
                    }

                    pDev->status &= ~ASSOC_TRACKING_SENT;

                    /* Try to send again or another */
                    Csf_setTrackingClock(TRACKING_CNF_DELAY_TIME);
                }
            }

            /* Update stats */
            if(pDataCnf->status == ApiMac_status_success)
            {
                Collector_statistics.trackingReqRequestSent++;
            }
        }
    }
}

/*!
 * @brief      MAC Data Indication callback.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void dataIndCB(ApiMac_mcpsDataInd_t *pDataInd)
{
    if((pDataInd != NULL) && (pDataInd->msdu.p != NULL)
       && (pDataInd->msdu.len > 0))
    {
        Smsgs_cmdIds_t cmdId = (Smsgs_cmdIds_t)*(pDataInd->msdu.p);

        if(Cllc_securityCheck(&(pDataInd->sec)) == false)
        {
            /* reject the message */
            return;
        }

        if(pDataInd->srcAddr.addrMode == ApiMac_addrType_extended)
        {
            uint16_t shortAddr = Csf_getDeviceShort(
                            &pDataInd->srcAddr.addr.extAddr);
            if(shortAddr != CSF_INVALID_SHORT_ADDR)
            {
                /* Switch to the short address for internal tracking */
                pDataInd->srcAddr.addrMode = ApiMac_addrType_short;
                pDataInd->srcAddr.addr.shortAddr = shortAddr;
            }
            else
            {
                /* Can't accept the message - ignore it */
                return;
            }
        }

        switch(cmdId)
        {
            case Smsgs_cmdIds_configRsp:
                processConfigResponse(pDataInd);
                break;
            case Smsgs_cmdIds_trackingRsp:
                processTrackingResponse(pDataInd);
                break;
            case Smsgs_cmdIds_toggleLedRsp:
                processToggleLedResponse(pDataInd);
                break;
            case Smsgs_cmdIds_sensorData:
                processSensorData(pDataInd);
                break;
            case Smsgs_cmdIds_alarmConfigRsp:
            case Smsgs_cmdIds_gSensorConfigRsp:
            case Smsgs_cmdIds_electricLockConfigRsp:
            case Smsgs_cmdIds_signalConfigRsp:
            case Smsgs_cmdIds_tempConfigRsp:
            case Smsgs_cmdIds_lowBatteryConfigRsp:
            case Smsgs_cmdIds_distanceConfigRsp:
            case Smsgs_cmdIds_musicConfigRsp:
            case Smsgs_cmdIds_intervalConfigRsp:
            case Smsgs_cmdIds_motionConfigRsp:
            case Smsgs_cmdIds_resistanceConfigRsp:
            case Smsgs_cmdIds_microwaveConfigRsp:
            case Smsgs_cmdIds_pirConfigRsp:
            case Smsgs_cmdIds_setUnsetConfigRsp:
            case Smsgs_cmdIds_disconnectRsp:
            case Smsgs_cmdIds_electricLockActionRsp:
                processCustomConfigResponse(pDataInd);
                break;
            default:
                /* Should not receive other messages */
                LOG_printf(LOG_ALWAYS, "dataIndCB cmdId is not found: %d\n", cmdId);
                break;
        }
    }
}

/*!
 * @brief      Process the start event
 */
static void processStartEvent(void)
{
    Llc_netInfo_t netInfo;

    /* See if there is existing network information */
    if(Csf_getNetworkInformation(&netInfo))
    {
        Llc_deviceListItem_t *pDevList = NULL;
        uint16_t numDevices = 0;
        uint32_t frameCounter = 0;

        Csf_getFrameCounter(NULL, &frameCounter);

        /* Initialize the MAC Security */
        Cllc_securityInit(frameCounter);

        numDevices = Csf_getNumDeviceListEntries();
        if (numDevices > 0)
        {
            /* Allocate enough memory for all know devices */
            pDevList = (Llc_deviceListItem_t *)Csf_malloc(
                            sizeof(Llc_deviceListItem_t) * numDevices);
            if(pDevList)
            {
                uint8_t i = 0;

                /* Use a temp pointer to cycle through the list */
                Llc_deviceListItem_t *pItem = pDevList;
                for(i = 0; i < numDevices; i++, pItem++)
                {
                    Csf_getDeviceItem(i, pItem);

                    /* Add device to security device table */
                    Cllc_addSecDevice(pItem->devInfo.panID,
                                      pItem->devInfo.shortAddress,
                                      &pItem->devInfo.extAddress,
                                      pItem->rxFrameCounter);
                }
            }
            else
            {
                numDevices = 0;
            }
        }

        /* Restore with the network and device information */
        Cllc_restoreNetwork(&netInfo, (uint8_t)numDevices, pDevList);

        if (pDevList)
        {
            Csf_free(pDevList);
        }

        restarted = true;
    }
    else
    {
        restarted = false;

        /* Initialize the MAC Security */
        Cllc_securityInit(0);

        /* Start a new netork */
        Cllc_startNetwork();
    }
}

/*!
 * @brief      Process the Config Response message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processConfigResponse(ApiMac_mcpsDataInd_t *pDataInd)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len == SMSGS_CONFIG_RESPONSE_MSG_LENGTH)
    {
        Cllc_associated_devices_t *pDev;
        Smsgs_configRspMsg_t configRsp;
        uint8_t *pBuf = pDataInd->msdu.p;

        /* Parse the message */
        configRsp.cmdId = (Smsgs_cmdIds_t)*pBuf++;

        configRsp.status = (Smsgs_statusValues_t)Util_buildUint16(pBuf[0],
                                                                  pBuf[1]);
        pBuf += 2;

        configRsp.frameControl = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;

        configRsp.reportingInterval = Util_buildUint32(pBuf[0], pBuf[1],
                                                       pBuf[2],
                                                       pBuf[3]);
        pBuf += 4;

        configRsp.pollingInterval = Util_buildUint32(pBuf[0], pBuf[1], pBuf[2],
                                                     pBuf[3]);

        pDev = findDevice(&pDataInd->srcAddr);
        if(pDev != NULL)
        {
            /* Clear the sent flag and set the response flag */
            pDev->status &= ~ASSOC_CONFIG_SENT;
            pDev->status |= ASSOC_CONFIG_RSP;
        }

        /* report the config response */
        Csf_deviceConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi,
                               &configRsp);

        Util_setEvent(&Collector_events, COLLECTOR_CONFIG_EVT);

        Collector_statistics.configResponseReceived++;
    }
}

/*!
 * @brief      Process the Tracking Response message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processTrackingResponse(ApiMac_mcpsDataInd_t *pDataInd)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len == SMSGS_TRACKING_RESPONSE_MSG_LENGTH)
    {
        Cllc_associated_devices_t *pDev;

        pDev = findDevice(&pDataInd->srcAddr);
        if(pDev != NULL)
        {
            if(pDev->status & ASSOC_TRACKING_SENT)
            {
                pDev->status &= ~ASSOC_TRACKING_SENT;
                pDev->status |= ASSOC_TRACKING_RSP;

                /* Setup for next tracking */
                Csf_setTrackingClock( TRACKING_DELAY_TIME);

                /* retry config request */
                processConfigRetry();
            }
        }

        /* Update stats */
        Collector_statistics.trackingResponseReceived++;
    }
}

/*!
 * @brief      Process the Toggle Led Response message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processToggleLedResponse(ApiMac_mcpsDataInd_t *pDataInd)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len == SMSGS_TOGGLE_LED_RESPONSE_MSG_LEN)
    {
        bool ledState;
        uint8_t *pBuf = pDataInd->msdu.p;

        /* Skip past the command ID */
        pBuf++;

        ledState = (bool)*pBuf;

        /* Notify the user */
        Csf_toggleResponseReceived(&pDataInd->srcAddr, ledState);
    }
}

/*!
 * @brief      Process the Sensor Data message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processSensorData(ApiMac_mcpsDataInd_t *pDataInd)
{
    Smsgs_sensorMsg_t sensorData;
    uint8_t *pBuf = pDataInd->msdu.p;

    memset(&sensorData, 0, sizeof(Smsgs_sensorMsg_t));

    /* Parse the message */
    sensorData.cmdId = (Smsgs_cmdIds_t)*pBuf++;

    memcpy(sensorData.extAddress, pBuf, SMGS_SENSOR_EXTADDR_LEN);
    pBuf += SMGS_SENSOR_EXTADDR_LEN;

    sensorData.frameControl = Util_buildUint16(pBuf[0], pBuf[1]);
    pBuf += 2;

    if(sensorData.frameControl & Smsgs_dataFields_msgStats)
    {
        sensorData.msgStats.joinAttempts = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.joinFails = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.msgsAttempted = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.msgsSent = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.trackingRequests = Util_buildUint16(pBuf[0],
                                                                pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.trackingResponseAttempts = Util_buildUint16(
                        pBuf[0],
                        pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.trackingResponseSent = Util_buildUint16(pBuf[0],
                                                                    pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.configRequests = Util_buildUint16(pBuf[0],
                                                              pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.configResponseAttempts = Util_buildUint16(
                        pBuf[0],
                        pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.configResponseSent = Util_buildUint16(pBuf[0],
                                                                  pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.channelAccessFailures = Util_buildUint16(pBuf[0],
                                                                     pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.macAckFailures = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.otherDataRequestFailures = Util_buildUint16(
                        pBuf[0],
                        pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.syncLossIndications = Util_buildUint16(pBuf[0],
                                                                   pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.rxDecryptFailures = Util_buildUint16(pBuf[0],
                                                                 pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.txEncryptFailures = Util_buildUint16(pBuf[0],
                                                                 pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.resetCount = Util_buildUint16(pBuf[0],
                                                          pBuf[1]);
        pBuf += 2;
        sensorData.msgStats.lastResetReason = Util_buildUint16(pBuf[0],
                                                               pBuf[1]);
        pBuf += 2;
    }

    if(sensorData.frameControl & Smsgs_dataFields_configSettings)
    {
        sensorData.configSettings.reportingInterval = Util_buildUint32(pBuf[0],
                                                                       pBuf[1],
                                                                       pBuf[2],
                                                                       pBuf[3]);
        pBuf += 4;
        sensorData.configSettings.pollingInterval = Util_buildUint32(pBuf[0],
                                                                     pBuf[1],
                                                                     pBuf[2],
                                                                     pBuf[3]);
        pBuf += 4;
    }
    
    /*! ======================================================================= */
    if(sensorData.frameControl & Smsgs_dataFields_eve003Sensor)
    {
        sensorData.eve003Sensor.button = *pBuf++;
        sensorData.eve003Sensor.dip = *pBuf++;
        sensorData.eve003Sensor.setting = *pBuf++;
        sensorData.eve003Sensor.battery = Util_buildUint16(pBuf[0], pBuf[1]);
        pBuf += 2;
    }
    if(sensorData.frameControl & Smsgs_dataFields_eve004Sensor)
    {
        sensorData.eve004Sensor.tempValue = *pBuf++;
        sensorData.eve004Sensor.state = *pBuf++;
        sensorData.eve004Sensor.alarm = *pBuf++;
        sensorData.eve004Sensor.dip = *pBuf++;
        sensorData.eve004Sensor.battery = *pBuf++;
        sensorData.eve004Sensor.rssi = *pBuf++;
    }
    if(sensorData.frameControl & Smsgs_dataFields_eve005Sensor)
    {
        sensorData.eve005Sensor.tmpValue = *pBuf++;
        sensorData.eve005Sensor.batValue = *pBuf++;
        sensorData.eve005Sensor.rssi = *pBuf++;
		    sensorData.eve005Sensor.button = *pBuf++;
		    sensorData.eve005Sensor.dip_id = *pBuf++;
		    sensorData.eve005Sensor.dip_control = *pBuf++;
		    sensorData.eve005Sensor.sensor_s = *pBuf++;
        sensorData.eve005Sensor.alarm = *pBuf++;
        sensorData.eve005Sensor.resist1 = *pBuf++;
        sensorData.eve005Sensor.resist2 = *pBuf++;
    }
    if(sensorData.frameControl & Smsgs_dataFields_eve006Sensor)
    {
        sensorData.eve006Sensor.button = *pBuf++;
        sensorData.eve006Sensor.state = *pBuf++;
        sensorData.eve006Sensor.abnormal = *pBuf++;
        sensorData.eve006Sensor.dip = *pBuf++;
        sensorData.eve006Sensor.dip_id = *pBuf++;
        sensorData.eve006Sensor.temp = *pBuf++;
        sensorData.eve006Sensor.humidity = *pBuf++;
        sensorData.eve006Sensor.battery = *pBuf++;
        sensorData.eve006Sensor.resist1 = *pBuf++;
        sensorData.eve006Sensor.resist2 = *pBuf++;
    }

    Collector_statistics.sensorMessagesReceived++;

    /* Report the sensor data */
    Csf_deviceSensorDataUpdate(&pDataInd->srcAddr, pDataInd->rssi,
                               &sensorData);

    processDataRetry(&(pDataInd->srcAddr));
}

/*! ======================================================================= */
/*!
 * @brief      Process the Alarm Config Response message.
 *
 * @param      pDataInd - pointer to the data indication information
 */
static void processCustomConfigResponse(ApiMac_mcpsDataInd_t *pDataInd)
{
    Cllc_associated_devices_t *pDev;
    uint8_t *pBuf = pDataInd->msdu.p;

    Smsgs_cmdIds_t cmdId = (Smsgs_cmdIds_t)*pBuf++;
    Smsgs_statusValues_t status = (Smsgs_statusValues_t)Util_buildUint16(pBuf[0], pBuf[1]);
    pBuf += 2;
	
    uint16_t frameControl = Util_buildUint16(pBuf[0], pBuf[1]);
    pBuf += 2;
	
    int result = -1;
    switch(cmdId)
    {
      case Smsgs_cmdIds_alarmConfigRsp:
    		  result = processAlarmConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
    		  break;
      case Smsgs_cmdIds_gSensorConfigRsp:
          result = processGSensorConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
    		  break;
      case Smsgs_cmdIds_electricLockConfigRsp:
          result = processElectricLockConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
    		  break;
      case Smsgs_cmdIds_signalConfigRsp:
          result = processSignalConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
    		  break;
      case Smsgs_cmdIds_tempConfigRsp:
    		  result = processTempConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
    		  break;
      case Smsgs_cmdIds_lowBatteryConfigRsp:
          result = processLowBatteryConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
    		  break;
      case Smsgs_cmdIds_distanceConfigRsp:
          result = processDistanceConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
    		  break;
      case Smsgs_cmdIds_musicConfigRsp:
          result = processMusicConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_intervalConfigRsp:
          result = processIntervalConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_motionConfigRsp:
          result = processMotionConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_resistanceConfigRsp:
          result = processResistanceConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_microwaveConfigRsp:
          result = processMicrowaveConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_pirConfigRsp:
          result = processPirConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_setUnsetConfigRsp:
          result = processSetUnsetConfigResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_disconnectRsp:
          result = processDisconnectResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
      case Smsgs_cmdIds_electricLockActionRsp:
          result = processElectricLockActionResponse(pDataInd, cmdId, status, frameControl, pBuf);
          break;
    	default:
    		  result = -1;
    		  break;
    }
	
    if (result < 0)
    {
        LOG_printf(LOG_ALWAYS, "processCustomConfigResponse cmdId is not found: %d\n", cmdId);
		    return;
    }
	
    pDev = findDevice(&pDataInd->srcAddr);
    if(pDev != NULL)
    {
        /* Clear the sent flag and set the response flag */
        pDev->status &= ~ASSOC_CONFIG_SENT;
        pDev->status |= ASSOC_CONFIG_RSP;
    }
     
    // Util_setEvent(&Collector_events, COLLECTOR_CONFIG_EVT);

    Collector_statistics.configResponseReceived++;
}

static int processAlarmConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_ALARM_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processAlarmConfigResponse length error!!!\n");
        return -1;
    }
	
    Smsgs_alarmConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.time = Util_buildUint16(pBuf[0], pBuf[1]);
    pBuf += 2;

    LOG_printf(LOG_ALWAYS, "processAlarmConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "time is %d\n", configRsp.time);
	
    /* report the config response */
    Csf_deviceAlarmConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processGSensorConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_GSENSOR_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processGSensorConfigResponse length error!!!\n");
        return -1;
    }
	
    Smsgs_gSensorConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.enable = *pBuf++;
    configRsp.sensitivity = *pBuf++;

    LOG_printf(LOG_ALWAYS, "processGSensorConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "enable is %d\n", configRsp.enable);
    LOG_printf(LOG_ALWAYS, "sensitivity is %d\n", configRsp.sensitivity);
	
    /* report the config response */
    Csf_deviceGSensorConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processElectricLockConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_ELECTRIC_LOCK_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processElectricLockConfigResponse length error!!!\n");
        return -1;
    }
	
    Smsgs_electricLockConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.enable = *pBuf++;
    configRsp.time = *pBuf++;

    LOG_printf(LOG_ALWAYS, "processElectricLockConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "enable is %d\n", configRsp.enable);
    LOG_printf(LOG_ALWAYS, "time is %d\n", configRsp.time);
	
    /* report the config response */
    Csf_deviceElectricLockConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processSignalConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_SIGNAL_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processSignalConfigResponse length error!!!\n");
        return -1;
    }
	
    Smsgs_signalConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.mode = *pBuf++;
    configRsp.value = *pBuf++;
    configRsp.offset = *pBuf++;

    LOG_printf(LOG_ALWAYS, "processSignalConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "mode is %d\n", configRsp.mode);
    LOG_printf(LOG_ALWAYS, "value is %d\n", configRsp.value);
    LOG_printf(LOG_ALWAYS, "offset is %d\n", configRsp.offset);
	
    /* report the config response */
    Csf_deviceSignalConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processTempConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_TEMP_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processTempConfigResponse length error!!!\n");
        return -1;
    }
	
    Smsgs_tempConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.value = *pBuf++;
    configRsp.offset = *pBuf++;

    LOG_printf(LOG_ALWAYS, "processTempConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "value is %d\n", configRsp.value);
    LOG_printf(LOG_ALWAYS, "offset is %d\n", configRsp.offset);
	
    /* report the config response */
    Csf_deviceTempConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processLowBatteryConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_LOW_BATTERY_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processLowBatteryConfigResponse length error!!!\n");
        return -1;
    }
	
    Smsgs_lowBatteryConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.value = *pBuf++;
    configRsp.offset = *pBuf++;

    LOG_printf(LOG_ALWAYS, "processLowBatteryConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "value is %d\n", configRsp.value);
    LOG_printf(LOG_ALWAYS, "offset is %d\n", configRsp.offset);
	
    /* report the config response */
    Csf_deviceLowBatteryConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processDistanceConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
										Smsgs_cmdIds_t cmdId,
										Smsgs_statusValues_t status,
										uint16_t frameControl,
										uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_DISTANCE_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processDistanceConfigResponse length error!!!\n");
        return -1;
    }
	
    Smsgs_distanceConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.mode = *pBuf++;
    configRsp.distance = Util_buildUint16(pBuf[0], pBuf[1]);

    LOG_printf(LOG_ALWAYS, "processDistanceConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "mode is %d\n", configRsp.mode);
    LOG_printf(LOG_ALWAYS, "distance is %d\n", configRsp.distance);
	
	 /* report the config response */
    Csf_deviceDistanceConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processMusicConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_MUSIC_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processMusicConfigResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_musicConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.mode = *pBuf++;
    configRsp.time = Util_buildUint16(pBuf[0], pBuf[1]);

    LOG_printf(LOG_ALWAYS, "processMusicConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "mode is %d\n", configRsp.mode);
    LOG_printf(LOG_ALWAYS, "time is %d\n", configRsp.time);
  
    /* report the config response */
    Csf_deviceMusicConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processIntervalConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_INTERVAL_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processIntervalConfigResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_intervalConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.mode = *pBuf++;
    configRsp.time = Util_buildUint16(pBuf[0], pBuf[1]);

    LOG_printf(LOG_ALWAYS, "processIntervalConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "mode is %d\n", configRsp.mode);
    LOG_printf(LOG_ALWAYS, "time is %d\n", configRsp.time);
  
    /* report the config response */
    Csf_deviceIntervalConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processMotionConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_MOTION_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processMotionConfigResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_motionConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.count = *pBuf++;
    configRsp.time = Util_buildUint16(pBuf[0], pBuf[1]);

    LOG_printf(LOG_ALWAYS, "processMotionConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "count is %d\n", configRsp.count);
    LOG_printf(LOG_ALWAYS, "time is %d\n", configRsp.time);
  
    /* report the config response */
    Csf_deviceMotionConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processResistanceConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_RESISTANCE_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processResistanceConfigResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_resistanceConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.value = *pBuf++;
    
    LOG_printf(LOG_ALWAYS, "processResistanceConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "value is %d\n", configRsp.value);
    
    /* report the config response */
    Csf_deviceResistanceConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processMicrowaveConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_MICROWAVE_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processMicrowaveConfigResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_microwaveConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.enable = *pBuf++;
    configRsp.sensitivity = *pBuf++;

    LOG_printf(LOG_ALWAYS, "processMicrowaveConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "enable is %d\n", configRsp.enable);
    LOG_printf(LOG_ALWAYS, "sensitivity is %d\n", configRsp.sensitivity);
  
    /* report the config response */
    Csf_deviceMicrowaveConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processPirConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_PIR_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processPirConfigResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_pirConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.enable = *pBuf++;
    
    LOG_printf(LOG_ALWAYS, "processPirConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "enable is %d\n", configRsp.enable);
    
    /* report the config response */
    Csf_devicePirConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processSetUnsetConfigResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_SET_UNSET_CONFIG_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processSetUnsetConfigResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_setUnsetConfigRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.state = *pBuf++;
    
    LOG_printf(LOG_ALWAYS, "processSetUnsetConfigResponse\n");
    LOG_printf(LOG_ALWAYS, "state is %d\n", configRsp.state);
    
    /* report the config response */
    Csf_deviceSetUnsetConfigUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processDisconnectResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_DISCONNECT_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processDisconnectResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_disconnectRspMsg_t configRsp;
    /* Parse the message */
    configRsp.cmdId = cmdId;
    configRsp.status = status;
    configRsp.frameControl = frameControl;

    configRsp.time = Util_buildUint16(pBuf[0], pBuf[1]);
    
    LOG_printf(LOG_ALWAYS, "processDisconnectResponse\n");
    LOG_printf(LOG_ALWAYS, "time is %d\n", configRsp.time);
    
    /* report the config response */
    Csf_deviceDisconnectUpdate(&pDataInd->srcAddr, pDataInd->rssi, &configRsp);
    return 1;
}

static int processElectricLockActionResponse(ApiMac_mcpsDataInd_t *pDataInd,
                    Smsgs_cmdIds_t cmdId,
                    Smsgs_statusValues_t status,
                    uint16_t frameControl,
                    uint8_t *pBuf)
{
    /* Make sure the message is the correct size */
    if(pDataInd->msdu.len != SMSGS_ELECTRIC_LOCK_ACTION_RESPONSE_MSG_LENGTH)
    {
        LOG_printf(LOG_ALWAYS, "processElectricLockActionResponse length error!!!\n");
        return -1;
    }
  
    Smsgs_electricLockActionRspMsg_t actionRsp;
    /* Parse the message */
    actionRsp.cmdId = cmdId;
    actionRsp.status = status;
    actionRsp.frameControl = frameControl;

    actionRsp.relay = *pBuf++;
    
    LOG_printf(LOG_ALWAYS, "processElectricLockActionResponse\n");
    LOG_printf(LOG_ALWAYS, "relay is %d\n", actionRsp.relay);
    
    /* report the config response */
    Csf_deviceElectricLockActionUpdate(&pDataInd->srcAddr, pDataInd->rssi, &actionRsp);
    return 1;
}


/*!
 Build and send the Alarm Config message to a device.

 Public function defined in collector.h
 */
Collector_status_t Collector_sendAlarmConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint16_t time)
{
	Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_ALARM_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_alarmConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = Util_loUint16(time);
            *pBuf++ = Util_hiUint16(time);

			      LOG_printf(LOG_ALWAYS, "Collector_sendAlarmConfigRequest\n");
			      LOG_printf(LOG_ALWAYS, "time is %d\n", time);
    	
            sendMsg(Smsgs_cmdIds_alarmConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_ALARM_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendGSensorConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable,
                                               uint8_t sensitivity)
{
	Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_GSENSOR_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_gSensorConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = enable;
            *pBuf++ = sensitivity;

			      LOG_printf(LOG_ALWAYS, "Collector_sendGSensorConfigRequest\n");
			      LOG_printf(LOG_ALWAYS, "enable is %d\n", enable);
			      LOG_printf(LOG_ALWAYS, "sensitivity is %d\n", sensitivity);
    	
            sendMsg(Smsgs_cmdIds_gSensorConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_GSENSOR_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendElectricLockConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable,
                                               uint8_t time)
{
	Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_ELECTRIC_LOCK_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_electricLockConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = enable;
            *pBuf++ = time;

			      LOG_printf(LOG_ALWAYS, "Collector_sendElectricLockConfigRequest\n");
			      LOG_printf(LOG_ALWAYS, "enable is %d\n", enable);
			      LOG_printf(LOG_ALWAYS, "time is %d\n", time);
    	
            sendMsg(Smsgs_cmdIds_electricLockConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_ELECTRIC_LOCK_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendSignalConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint8_t value,
                                               uint8_t offset)
{
	Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_SIGNAL_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_signalConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = mode;
            *pBuf++ = value;
            *pBuf++ = offset;

			      LOG_printf(LOG_ALWAYS, "Collector_sendSignalConfigRequest\n");
			      LOG_printf(LOG_ALWAYS, "mode is %d\n", mode);
			      LOG_printf(LOG_ALWAYS, "value is %d\n", value);
			      LOG_printf(LOG_ALWAYS, "offset is %d\n", offset);
    	
            sendMsg(Smsgs_cmdIds_signalConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_SIGNAL_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendTempConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t value,
                                               uint8_t offset)
{
	Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_TEMP_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_tempConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = value;
            *pBuf++ = offset;

			      LOG_printf(LOG_ALWAYS, "Collector_sendTempConfigRequest\n");
			      LOG_printf(LOG_ALWAYS, "value is %d\n", value);
			      LOG_printf(LOG_ALWAYS, "offset is %d\n", offset);
    	
            sendMsg(Smsgs_cmdIds_tempConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_TEMP_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendLowBatteryConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t value,
                                               uint8_t offset)
{
	Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_LOW_BATTERY_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_lowBatteryConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = value;
            *pBuf++ = offset;

			      LOG_printf(LOG_ALWAYS, "Collector_sendLowBatteryConfigRequest\n");
			      LOG_printf(LOG_ALWAYS, "value is %d\n", value);
			      LOG_printf(LOG_ALWAYS, "offset is %d\n", offset);
    	
            sendMsg(Smsgs_cmdIds_lowBatteryConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_LOW_BATTERY_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendDistanceConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint16_t distance)
{
	Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_DISTANCE_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_distanceConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = mode;
            *pBuf++ = Util_loUint16(distance);
            *pBuf++ = Util_hiUint16(distance);

			      LOG_printf(LOG_ALWAYS, "Collector_sendDistanceConfigRequest\n");
			      LOG_printf(LOG_ALWAYS, "mode is %d\n", mode);
			      LOG_printf(LOG_ALWAYS, "distance is %d\n", distance);
    	
            sendMsg(Smsgs_cmdIds_distanceConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_DISTANCE_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendMusicConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint16_t time)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_MUSIC_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_musicConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = mode;
            *pBuf++ = Util_loUint16(time);
            *pBuf++ = Util_hiUint16(time);

            LOG_printf(LOG_ALWAYS, "Collector_sendMusicConfigRequest\n");
            LOG_printf(LOG_ALWAYS, "mode is %d\n", mode);
            LOG_printf(LOG_ALWAYS, "time is %d\n", time);
      
            sendMsg(Smsgs_cmdIds_musicConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_MUSIC_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendIntervalConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t mode,
                                               uint16_t time)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_INTERVAL_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_intervalConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = mode;
            *pBuf++ = Util_loUint16(time);
            *pBuf++ = Util_hiUint16(time);

            LOG_printf(LOG_ALWAYS, "Collector_sendIntervalConfigRequest\n");
            LOG_printf(LOG_ALWAYS, "mode is %d\n", mode);
            LOG_printf(LOG_ALWAYS, "time is %d\n", time);
      
            sendMsg(Smsgs_cmdIds_intervalConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_INTERVAL_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendMotionConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t count,
                                               uint16_t time)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_MOTION_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_motionConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = count;
            *pBuf++ = Util_loUint16(time);
            *pBuf++ = Util_hiUint16(time);

            LOG_printf(LOG_ALWAYS, "Collector_sendMotionConfigRequest\n");
            LOG_printf(LOG_ALWAYS, "count is %d\n", count);
            LOG_printf(LOG_ALWAYS, "time is %d\n", time);
      
            sendMsg(Smsgs_cmdIds_motionConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_MOTION_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendResistanceConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t value)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_RESISTANCE_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_resistanceConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = value;
            
            LOG_printf(LOG_ALWAYS, "Collector_sendResistanceConfigRequest\n");
            LOG_printf(LOG_ALWAYS, "value is %d\n", value);
      
            sendMsg(Smsgs_cmdIds_resistanceConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_RESISTANCE_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendMicrowaveConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable,
                                               uint8_t sensitivity)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_MICROWAVE_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_microwaveConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = enable;
            *pBuf++ = sensitivity;
            
            LOG_printf(LOG_ALWAYS, "Collector_sendMicrowaveConfigRequest\n");
            LOG_printf(LOG_ALWAYS, "enable is %d\n", enable);
            LOG_printf(LOG_ALWAYS, "sensitivity is %d\n", sensitivity);
      
            sendMsg(Smsgs_cmdIds_microwaveConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_MICROWAVE_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendPirConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t enable)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_PIR_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_pirConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = enable;
            
            LOG_printf(LOG_ALWAYS, "Collector_sendPirConfigRequest\n");
            LOG_printf(LOG_ALWAYS, "enable is %d\n", enable);
      
            sendMsg(Smsgs_cmdIds_pirConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_PIR_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendSetUnsetConfigRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t state)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_SET_UNSET_CONFIG_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_setUnsetConfigReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = state;
            
            LOG_printf(LOG_ALWAYS, "Collector_sendSetUnsetConfigRequest\n");
            LOG_printf(LOG_ALWAYS, "state is %d\n", state);
      
            sendMsg(Smsgs_cmdIds_setUnsetConfigReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_SET_UNSET_CONFIG_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

Collector_status_t Collector_sendDisconnectRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint16_t time)
{
    Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_DISCONNECT_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;
            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_disconnectReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = Util_loUint16(time);
            *pBuf++ = Util_hiUint16(time);

            sendMsg(Smsgs_cmdIds_disconnectReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    SMSGS_DISCONNECT_REQUEST_MSG_LEN,
                    buffer);

            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return(status);
}

Collector_status_t Collector_sendElectricLockActionRequest(ApiMac_sAddr_t *pDstAddr,
                                               uint16_t frameControl,
                                               uint8_t relay)
{
  Collector_status_t status = Collector_status_invalid_state;

    /* Are we in the right state? */
    if(cllcState >= Cllc_states_started)
    {
        Llc_deviceListItem_t item;

        /* Is the device a known device? */
        if(Csf_getDevice(pDstAddr, &item))
        {
            uint8_t buffer[SMSGS_ELECTRIC_LOCK_ACTION_REQUEST_MSG_LEN];
            uint8_t *pBuf = buffer;

            /* Build the message */
            *pBuf++ = (uint8_t)Smsgs_cmdIds_electricLockActionReq;
            *pBuf++ = Util_loUint16(frameControl);
            *pBuf++ = Util_hiUint16(frameControl);
            *pBuf++ = relay;
            
            LOG_printf(LOG_ALWAYS, "Collector_sendElectricLockActionRequest\n");
            LOG_printf(LOG_ALWAYS, "relay is %d\n", relay);
      
            sendMsg(Smsgs_cmdIds_electricLockActionReq, item.devInfo.shortAddress,
                    item.capInfo.rxOnWhenIdle,
                    (SMSGS_ELECTRIC_LOCK_ACTION_REQUEST_MSG_LEN),
                    buffer);
            status = Collector_status_success;
            Collector_statistics.configRequestAttempts++;
            /* set timer for retry in case response is not received */
            Csf_setConfigClock(CONFIG_DELAY);
        }
    }

    return (status);
}

/*!
 * @brief      Find the associated device table entry matching pAddr.
 *
 * @param      pAddr - pointer to device's address
 *
 * @return     pointer to the associated device table entry,
 *             NULL if not found.
 */
static Cllc_associated_devices_t *findDevice(ApiMac_sAddr_t *pAddr)
{
    int x;
    Cllc_associated_devices_t *pItem = NULL;

    /* Check for invalid parameters */
    if((pAddr == NULL) || (pAddr->addrMode == ApiMac_addrType_none))
    {
        return (NULL);
    }

    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        /* Make sure the entry is valid. */
        if(Cllc_associatedDevList[x].shortAddr != CSF_INVALID_SHORT_ADDR)
        {
            if(pAddr->addrMode == ApiMac_addrType_short)
            {
                if(pAddr->addr.shortAddr == Cllc_associatedDevList[x].shortAddr)
                {
                    pItem = &Cllc_associatedDevList[x];
                    break;
                }
            }
        }
    }

    return (pItem);
}

/*!
 * @brief      Find the associated device table entry matching status bit.
 *
 * @param      statusBit - what status bit to find
 *
 * @return     pointer to the associated device table entry,
 *             NULL if not found.
 */
static Cllc_associated_devices_t *findDeviceStatusBit(uint16_t mask, uint16_t statusBit)
{
    int x;
    Cllc_associated_devices_t *pItem = NULL;

    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        /* Make sure the entry is valid. */
        if(Cllc_associatedDevList[x].shortAddr != CSF_INVALID_SHORT_ADDR)
        {
            if((Cllc_associatedDevList[x].status & mask) == statusBit)
            {
                pItem = &Cllc_associatedDevList[x];
                break;
            }
        }
    }

    return (pItem);
}

/*!
 * @brief      Get the next MSDU Handle
 *             <BR>
 *             The MSDU handle has 3 parts:<BR>
 *             - The MSBit(7), when set means the the application sent the message
 *             - Bit 6, when set means that the app message is a config request
 *             - Bits 0-5, used as a message counter that rolls over.
 *
 * @param      msgType - message command id needed
 *
 * @return     msdu Handle
 */
static uint8_t getMsduHandle(Smsgs_cmdIds_t msgType)
{
    uint8_t msduHandle = deviceTxMsduHandle;

    /* Increment for the next msdu handle, or roll over */
    if(deviceTxMsduHandle >= MSDU_HANDLE_MAX)
    {
        deviceTxMsduHandle = 0;
    }
    else
    {
        deviceTxMsduHandle++;
    }

    /* Add the App specific bit */
    msduHandle |= APP_MARKER_MSDU_HANDLE;

    /* Add the message type bit */
    if(msgType == Smsgs_cmdIds_configReq)
    {
        msduHandle |= APP_CONFIG_MSDU_HANDLE;
    }

    return (msduHandle);
}

/*!
 * @brief      Send MAC data request
 *
 * @param      type - message type
 * @param      dstShortAddr - destination short address
 * @param      rxOnIdle - true if not a sleepy device
 * @param      len - length of payload
 * @param      pData - pointer to the buffer
 */
static void sendMsg(Smsgs_cmdIds_t type, uint16_t dstShortAddr, bool rxOnIdle,
                    uint16_t len,
                    uint8_t *pData)
{
    ApiMac_mcpsDataReq_t dataReq;

    /* Fill the data request field */
    memset(&dataReq, 0, sizeof(ApiMac_mcpsDataReq_t));

    dataReq.dstAddr.addrMode = ApiMac_addrType_short;
    dataReq.dstAddr.addr.shortAddr = dstShortAddr;
    dataReq.srcAddrMode = ApiMac_addrType_short;

    if(fhEnabled)
    {
        Llc_deviceListItem_t item;

        if(Csf_getDevice(&(dataReq.dstAddr), &item))
        {
            /* Switch to the long address */
            dataReq.dstAddr.addrMode = ApiMac_addrType_extended;
            memcpy(&dataReq.dstAddr.addr.extAddr, &item.devInfo.extAddress,
                   (APIMAC_SADDR_EXT_LEN));
            dataReq.srcAddrMode = ApiMac_addrType_extended;
        }
        else
        {
            /* Can't send the message */
            return;
        }
    }

    dataReq.dstPanId = devicePanId;

    dataReq.msduHandle = getMsduHandle(type);

    dataReq.txOptions.ack = true;
    if(rxOnIdle == false)
    {
        dataReq.txOptions.indirect = true;
    }

    dataReq.msdu.len = len;
    dataReq.msdu.p = pData;

    /* Fill in the appropriate security fields */
    Cllc_securityFill(&dataReq.sec);

    /* Send the message */
    ApiMac_mcpsDataReq(&dataReq);
}

/*!
 * @brief      Generate Config Requests for all associate devices
 *             that need one.
 */
static void generateConfigRequests(void)
{
    int x;

    /* Clear any timed out transactions */
    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        if((Cllc_associatedDevList[x].shortAddr != CSF_INVALID_SHORT_ADDR)
           && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
        {
            if((Cllc_associatedDevList[x].status &
               (ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP))
               == (ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP))
            {
                Cllc_associatedDevList[x].status &= ~(ASSOC_CONFIG_SENT
                                | ASSOC_CONFIG_RSP);
            }
        }
    }

    /* Make sure we are only sending one config request at a time */
    if(findDeviceStatusBit(ASSOC_CONFIG_MASK, ASSOC_CONFIG_SENT) == NULL)
    {
        /* Run through all of the devices */
        for(x = 0; x < CONFIG_MAX_DEVICES; x++)
        {
            /* Make sure the entry is valid. */
            if((Cllc_associatedDevList[x].shortAddr != CSF_INVALID_SHORT_ADDR)
               && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
            {
                uint16_t status = Cllc_associatedDevList[x].status;

                /*
                 Has the device been sent or already received a config request?
                 */
                if(((status & (ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP)) == 0))
                {
                    ApiMac_sAddr_t dstAddr;
                    Collector_status_t stat;

                    /* Set up the destination address */
                    dstAddr.addrMode = ApiMac_addrType_short;
                    dstAddr.addr.shortAddr =
                        Cllc_associatedDevList[x].shortAddr;

                    /* Send the Config Request */
                    stat = Collector_sendConfigRequest(
                                    &dstAddr, (CONFIG_FRAME_CONTROL),
                                    (Cllc_associatedDevList[x].reporting),
                                    (Cllc_associatedDevList[x].polling));
                    if(stat == Collector_status_success)
                    {
                        /*
                         Mark as the message has been sent and expecting a response
                         */
                        Cllc_associatedDevList[x].status |= ASSOC_CONFIG_SENT;
                        Cllc_associatedDevList[x].status &= ~ASSOC_CONFIG_RSP;
                    }

                    /* Only do one at a time */
                    break;
                }
            }
        }
    }
}


/*!
 * @brief      Generate Config Requests for all associate devices
 *             that need one.
 */
static void generateTrackingRequests(void)
{
    int x;

    /* Run through all of the devices, looking for previous activity */
    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        /* Make sure the entry is valid. */
        if((Cllc_associatedDevList[x].shortAddr != CSF_INVALID_SHORT_ADDR)
             && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
        {
            uint16_t status = Cllc_associatedDevList[x].status;

            /*
             Has the device been sent a tracking request or received a
             tracking response?
             */
            if(status & ASSOC_TRACKING_RETRY)
            {
                sendTrackingRequest(&Cllc_associatedDevList[x]);
                return;
            }
            else if((status & (ASSOC_TRACKING_SENT | ASSOC_TRACKING_RSP
                               | ASSOC_TRACKING_ERROR)))
            {
                Cllc_associated_devices_t *pDev = NULL;
                int y;

                if(status & (ASSOC_TRACKING_SENT | ASSOC_TRACKING_ERROR))
                {
                    ApiMac_deviceDescriptor_t devInfo;
                    Llc_deviceListItem_t item;
                    ApiMac_sAddr_t devAddr;

                    /*
                     Timeout occured, notify the user that the tracking
                     failed.
                     */
                    memset(&devInfo, 0, sizeof(ApiMac_deviceDescriptor_t));

                    devAddr.addrMode = ApiMac_addrType_short;
                    devAddr.addr.shortAddr =
                        Cllc_associatedDevList[x].shortAddr;

                    if(Csf_getDevice(&devAddr, &item))
                    {
                        memcpy(&devInfo.extAddress,
                               &item.devInfo.extAddress,
                               sizeof(ApiMac_sAddrExt_t));
                    }
                    devInfo.shortAddress = Cllc_associatedDevList[x].shortAddr;
                    devInfo.panID = devicePanId;
                    Csf_deviceNotActiveUpdate(&devInfo,
                        ((status & ASSOC_TRACKING_SENT) ? true : false));

                    /* Not responding, so remove the alive marker */
                    Cllc_associatedDevList[x].status
                            &= ~(CLLC_ASSOC_STATUS_ALIVE
                                | ASSOC_CONFIG_SENT | ASSOC_CONFIG_RSP);
                }

                /* Clear the tracking bits */
                Cllc_associatedDevList[x].status  &= ~(ASSOC_TRACKING_ERROR
                                | ASSOC_TRACKING_SENT | ASSOC_TRACKING_RSP);

                /* Find the next valid device */
                y = x;
                while(pDev == NULL)
                {
                    /* Check for rollover */
                    if(y == (CONFIG_MAX_DEVICES-1))
                    {
                        /* Move to the beginning */
                        y = 0;
                    }
                    else
                    {
                        /* Move the the next device */
                        y++;
                    }

                    if(y == x)
                    {
                        /* We've come back around */
                        break;
                    }

                    /*
                     Is the entry valid and active */
                    if((Cllc_associatedDevList[y].shortAddr
                                    != CSF_INVALID_SHORT_ADDR)
                         && (Cllc_associatedDevList[y].status
                                   & CLLC_ASSOC_STATUS_ALIVE))
                    {
                        pDev = &Cllc_associatedDevList[y];
                    }
                }

                /* Was device found? */
                if(pDev)
                {
                    sendTrackingRequest(pDev);
                }
                else
                {
                    /* No device found, Setup delay for next tracking message */
                    Csf_setTrackingClock(TRACKING_DELAY_TIME);
                }

                /* Only do one at a time */
                return;
            }
        }
    }

    /* if no activity found, find the first active device */
    for(x = 0; x < CONFIG_MAX_DEVICES; x++)
    {
        /* Make sure the entry is valid. */
        if((Cllc_associatedDevList[x].shortAddr != CSF_INVALID_SHORT_ADDR)
              && (Cllc_associatedDevList[x].status & CLLC_ASSOC_STATUS_ALIVE))
        {
            sendTrackingRequest(&Cllc_associatedDevList[x]);
            break;
        }
    }

    if(x == CONFIG_MAX_DEVICES)
    {
        /* No device found, Setup delay for next tracking message */
        Csf_setTrackingClock(TRACKING_DELAY_TIME);
    }
}

/*!
 * @brief      Generate Tracking Requests for a device
 *
 * @param      pDev - pointer to the device's associate device table entry
 */
static void sendTrackingRequest(Cllc_associated_devices_t *pDev)
{
    uint8_t cmdId = Smsgs_cmdIds_trackingReq;

    /* Send the Tracking Request */
    sendMsg(Smsgs_cmdIds_trackingReq, pDev->shortAddr,
            pDev->capInfo.rxOnWhenIdle,
            (SMSGS_TRACKING_REQUEST_MSG_LENGTH),
            &cmdId);

    /* Mark as Tracking Request sent */
    pDev->status |= ASSOC_TRACKING_SENT;

    /* Setup Timeout for response */
    Csf_setTrackingClock(TRACKING_TIMEOUT_TIME);

    /* Update stats */
    Collector_statistics.trackingRequestAttempts++;
}

/*!
 * @brief      Process the MAC Comm Status Indication Callback
 *
 * @param      pCommStatusInd - Comm Status indication
 */
static void commStatusIndCB(ApiMac_mlmeCommStatusInd_t *pCommStatusInd)
{
    if(pCommStatusInd->reason == ApiMac_commStatusReason_assocRsp)
    {
        if(pCommStatusInd->status != ApiMac_status_success)
        {
            Cllc_associated_devices_t *pDev;

            pDev = findDevice(&pCommStatusInd->dstAddr);
            if(pDev)
            {
                /* Mark as inactive and clear config and tracking states */
                pDev->status = 0;
            }
        }
    }
}

/*!
 * @brief      Process the MAC Poll Indication Callback
 *
 * @param      pPollInd - poll indication
 */
static void pollIndCB(ApiMac_mlmePollInd_t *pPollInd)
{
    ApiMac_sAddr_t addr;

    addr.addrMode = ApiMac_addrType_short;
    if (pPollInd->srcAddr.addrMode == ApiMac_addrType_short)
    {
        addr.addr.shortAddr = pPollInd->srcAddr.addr.shortAddr;
    }
    else
    {
        addr.addr.shortAddr = Csf_getDeviceShort(
                        &pPollInd->srcAddr.addr.extAddr);
    }

    processDataRetry(&addr);
}

/*!
 * @brief      Process retries for config and tracking messages
 *
 * @param      addr - MAC address structure */
static void processDataRetry(ApiMac_sAddr_t *pAddr)
{
    if(pAddr->addr.shortAddr != CSF_INVALID_SHORT_ADDR)
    {
        Cllc_associated_devices_t *pItem;
        pItem = findDevice(pAddr);
        if(pItem)
        {
            if((pItem->status & CLLC_ASSOC_STATUS_ALIVE) == 0)
            {
                /* It was missing */
                pItem->status |= CLLC_ASSOC_STATUS_ALIVE;

                /* Check to see if we need to send it a config */
                if((pItem->status & (ASSOC_CONFIG_RSP | ASSOC_CONFIG_SENT
                   | ASSOC_TRACKING_SENT| ASSOC_TRACKING_RETRY)) == 0)
                {
                    if(CONFIG_MAC_BEACON_ORDER != NON_BEACON_ORDER)
                    {
                        /* Make sure we aren't already doing a tracking message */
                        if(((Collector_events & ASSOC_TRACKING_SENT) == 0)
                              && (findDeviceStatusBit(ASSOC_TRACKING_MASK,
                              ASSOC_TRACKING_SENT) == NULL))
                        {
                            /* Setup for next tracking */
                            Csf_setTrackingClock( TRACKING_DELAY_TIME);
                        }
                    }
                    processConfigRetry();
                }
            }
        }
    }
}

/*!
 * @brief      Process retries for config messages
 */
static void processConfigRetry(void)
{
    /* retry config request if not already sent */
    if(((Collector_events & COLLECTOR_CONFIG_EVT) == 0)
        && (Csf_isConfigTimerActive() == false))
    {
        /* Set config event */
        Csf_setConfigClock(CONFIG_DELAY);
    }
}

static void pairingIndCB(void)
{
    LOG_printf(LOG_ALWAYS, "Collector_pairingIndCB\n");
    Csf_pairingUpdate(1);
}

static void antennaIndCB(ApiMac_antennaInd_t *pAntennaInd)
{
    LOG_printf(LOG_ALWAYS, "Collector_antennaIndCB: %d\n", pAntennaInd->status);
    Csf_antennaUpdate(pAntennaInd->status);
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
