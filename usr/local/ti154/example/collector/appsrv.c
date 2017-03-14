/******************************************************************************
 @file appsrv.c

 @brief TIMAC 2.0 API User Interface Collector API

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
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>

#include "appsrv.pb-c.h"

#include "debug_helpers.h"

#include "collector.h"

#include "api_mac.h"
#include "api_mac_linux.h"
#include "llc.h"
#include "cllc.h"
#include "smsgs.h"
#include "log.h"
#include "fatal.h"

#include "appsrv.h"
#include "csf_linux.h"
#include "mutex.h"
#include "threads.h"
#include "timer.h"

#include "stream.h"
#include "stream_socket.h"
#include "stream_uart.h"

#include "smsgs_proto.h"
#include "api_mac_proto.h"
#include "appsrv_proto.h"

/******************************************************************************
 Typedefs
*****************************************************************************/


struct appsrv_connection {
    /*! is this item busy (broadcasting) */
    bool is_busy;
    /*! has something gone wrong this is set to true */
    bool is_dead;
    /*! name for us in debug logs */
    char *dbg_name;

    /* what connection number is this? */
    int  connection_id;

    /*! The socket interface to the gateway */
    struct mt_msg_interface socket_interface;

    /*! Thread id for the socket interface */
    intptr_t thread_id_s2appsrv;

    /*! Next connection in the list */
    struct appsrv_connection *pNext;
};

/******************************************************************************
 GLOBAL Variables
*****************************************************************************/
/*! The interface the API mac uses, points to either the socket or the uart */
struct mt_msg_interface *API_MAC_msg_interface;
/*! generic template for all gateway connections */
struct socket_cfg appClient_socket_cfg;
/*! configuration for apimac if using a socket (ie: npi server) */
struct socket_cfg npi_socket_cfg;
/*! uart configuration for apimac if talking to UART instead of npi */
struct uart_cfg   uart_cfg;

/*! malloc for protobuf */
static void *my_allocator(void *notused, size_t n)
{
    (void)notused;
    return calloc(1, n);
}

/*! free for protobuf */
static void my_free(void *notused, void *pBuf)
{
    (void)(notused);
    free(pBuf);
}

/*! we control memory allocation for the protobuf code via this */
static ProtobufCAllocator protobuf_allocator = {
    .alloc = my_allocator,
    .free = my_free,
    .allocator_data = NULL
};

/*! Generic template for all gateway interfaces
  Note: These parameters can be modified via the ini file
*/
struct mt_msg_interface appClient_mt_interface_template = {
    .dbg_name                  = "appClient",
    .is_NPI                    = false,
    .frame_sync                = false,
    .include_chksum            = false,
    .hndl                      = 0,
    .s_cfg                     = NULL,
    .u_cfg                     = NULL,
    .rx_thread                 = 0,
    .tx_frag_size              = 0,
    .retry_max                 = 0,
    .frag_timeout_mSecs        = 10000,
    .intermsg_timeout_mSecs    = 10000,
    .intersymbol_timeout_mSecs = 100,
    .srsp_timeout_mSecs        = 300,
    .stack_id                  = 0,
    .len_2bytes                = true,
    .rx_handler_cookie         = 0,
    .is_dead                   = false,
    .flush_timeout_mSecs       = 100
};

/*! Generic template for uart interface
  Note: These parameters can be modified via the ini file
*/
struct mt_msg_interface uart_mt_interface = {
    .dbg_name                  = "uart",
    .is_NPI                    = false,
    .frame_sync                = true,
    .include_chksum            = true,
    .hndl                      = 0,
    .s_cfg                     = NULL,
    .u_cfg                     = &uart_cfg,
    .rx_thread                 = 0,
    .tx_frag_size              = 0,
    .retry_max                 = 0,
    .frag_timeout_mSecs        = 10000,
    .intermsg_timeout_mSecs    = 10000,
    .intersymbol_timeout_mSecs = 100,
    .srsp_timeout_mSecs        = 300,
    .stack_id                  = 0,
    .len_2bytes                = true,
    .rx_handler_cookie         = 0,
    .is_dead                   = false,
    .flush_timeout_mSecs       = 100
};

/*! Template for apimac connection to npi server
  Note: These parameters can be modified via the ini file
*/
struct mt_msg_interface npi_mt_interface = {
    .dbg_name                  = "npi",
    .is_NPI                    = false,
    .frame_sync                = true,
    .include_chksum            = true,
    .hndl                      = 0,
    .s_cfg                     = &npi_socket_cfg,
    .u_cfg                     = NULL,
    .rx_thread                 = 0,
    .tx_frag_size              = 0,
    .retry_max                 = 0,
    .frag_timeout_mSecs        = 10000,
    .intermsg_timeout_mSecs    = 10000,
    .intersymbol_timeout_mSecs = 100,
    .srsp_timeout_mSecs        = 300,
    .stack_id                  = 0,
    .len_2bytes                = true,
    .rx_handler_cookie         = 0,
    .is_dead                   = false,
    .flush_timeout_mSecs       = 100
};

/*******************************************************************
 * LOCAL VARIABLES
 ********************************************************************/

static intptr_t all_connections_mutex;
static struct appsrv_connection *all_connections;
static intptr_t all_connections_mutex;

/*******************************************************************
 * LOCAL FUNCTIONS
 ********************************************************************/

static void appsrv_processGetDeviceArrayReq(struct appsrv_connection *pCONN);

/*! Lock the list of gateway connections
  Often used when modifying the list
*/
static void lock_connection_list(void)
{
    MUTEX_lock(all_connections_mutex, -1);
}

/* See lock() above */
static void unlock_connection_list(void)
{
    MUTEX_unLock(all_connections_mutex);
}

/*!
 * @brief send a data confirm to the gateway
 * @param status - the status value to send
 */
static void send_AppsrvTxDataCnf(int status)
{
    AppsrvTxDataCnf *pCNF;
    struct mt_msg *pMsg;
    size_t len;

    pMsg = NULL;
    pCNF = NULL;

    pCNF = copy_AppsrvTxDataCnf( (uint8_t)status);
    if(!pCNF)
    {
        goto fail;
    }

    len = appsrv_tx_data_cnf__get_packed_size(pCNF);
    pMsg = MT_MSG_alloc((int)len,
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_TX_DATA_CNF);
    if(pMsg == NULL)
    {
        goto fail;
    }
    pMsg->pLogPrefix = "tx-data-cnf";

    /* set it to the template */
    MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
    appsrv_tx_data_cnf__pack(pCNF, &(pMsg->iobuf[pMsg->iobuf_idx]));
    MT_MSG_wrBuf(pMsg, NULL, len);

    appsrv_broadcast(pMsg);
fail:
    /* nothing to do here... */
    if(pMsg)
    {
        MT_MSG_free(pMsg);
        pMsg = NULL;
    }
    if(pCNF)
    {
        free_AppsrvTxDataCnf(pCNF);
    }
}


/*!
 * @brief send a join confirm to the gateway
 */
static void send_AppSrvJoinPermitCnf(int status)
{
    AppsrvSetJoinPermitCnf *pCNF;
    struct mt_msg *pMsg;
    size_t len;

    pMsg = NULL;
    pCNF = NULL;

    pCNF = copy_AppsrvSetJoinPermitCnf(status);
    if(!pCNF)
    {
        goto fail;
    }

    len = appsrv_set_join_permit_cnf__get_packed_size(pCNF);
    pMsg = MT_MSG_alloc((int)len,
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_SET_JOIN_PERMIT_CNF);
    if(pMsg == NULL)
    {
        goto fail;
    }
    pMsg->pLogPrefix = "join-permit-cnf";


    /* set it to the template */
    MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
    appsrv_set_join_permit_cnf__pack(pCNF, &(pMsg->iobuf[pMsg->iobuf_idx]));
    MT_MSG_wrBuf(pMsg, NULL, len);

    appsrv_broadcast(pMsg);
fail:
    /* nothing to do here... */
    if(pMsg)
    {
        MT_MSG_free(pMsg);
        pMsg = NULL;
    }
    if(pCNF)
    {
        free_AppsrvSetJoinPermitCnf(pCNF);
    }
}

/*!
 * @brief handle a data request from the gateway
 * @param pCONN - where the request came from
 * @param pIncommingMsg - the msg from the gateway
 */
static void appsrv_processTxDataReq(struct appsrv_connection *pCONN,
                                    struct mt_msg *pIncomingMsg)
{
    int status;

    AppsrvTxDataReq *msgTxDataReq;
    (void)pCONN;
    /* assume total failure */
    status = ApiMac_status_invalidParameter;

    /* de-serialize the incoming message */
    msgTxDataReq = appsrv_tx_data_req__unpack(&protobuf_allocator,
                                              pIncomingMsg->expected_len,
                                              &pIncomingMsg->iobuf[
                                                  pIncomingMsg->iobuf_idx]);
    if(msgTxDataReq == NULL)
    {
        LOG_printf(LOG_ERROR, "Error parsing appsrv_processTxDataReq\n");
        goto fail;
    }
  
    MT_MSG_rdBuf(pIncomingMsg, NULL, pIncomingMsg->expected_len);
    MT_MSG_parseComplete(pIncomingMsg);
    if(pIncomingMsg->is_error)
    {
        goto fail;
    }
  
    LOG_printf(LOG_APPSRV_MSG_CONTENT,
               " TxDataReq (id: %d, panid: 0x%04x)\n",
               msgTxDataReq->msgid,
               msgTxDataReq->devdescriptor->panid);
    
    /* Parse the device descriptor */
    ApiMac_sAddr_t pDstAddr;
    pDstAddr.addrMode = ApiMac_addrType_short;
    pDstAddr.addr.shortAddr =
        (uint16_t)msgTxDataReq->devdescriptor->shortaddress;

    /* Config Request */
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_configReq)
    {
        LOG_printf(LOG_APPSRV_MSG_CONTENT, " Config-req \n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, " framecontrol is %d \n",
                   msgTxDataReq->configreqmsg->framecontrol );
        LOG_printf(LOG_APPSRV_MSG_CONTENT, " reporting interval is %d \n",
                   msgTxDataReq->configreqmsg->reportinginterval );
        LOG_printf(LOG_APPSRV_MSG_CONTENT, " polling interval is 0x%x\n",
                   msgTxDataReq->configreqmsg->pollinginterval);


        Csf_sendConfigRequest(&pDstAddr,
                              (uint16_t)msgTxDataReq->configreqmsg->framecontrol,
                              msgTxDataReq->configreqmsg->reportinginterval,
                              msgTxDataReq->configreqmsg->pollinginterval);
    }

    /* Toggle led Request */
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_toggleLedReq)
    {
        LOG_printf(LOG_APPSRV_MSG_CONTENT, " Toggle-req \n");
        Csf_sendToggleLedRequest(&pDstAddr);
    }

    /*! ======================================================================= */
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_alarmConfigReq)
    {
    	uint16_t framecontrol = (uint16_t)msgTxDataReq->alarmconfigreqmsg->framecontrol;
        uint16_t time = (uint16_t)msgTxDataReq->alarmconfigreqmsg->time;
    	
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "Alarm Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "time is %d\n", time);
    		
        Csf_sendAlarmConfigRequest(&pDstAddr,
        							framecontrol,
        							time);
    }
    
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_gSensorConfigReq)
    {
    	uint16_t framecontrol = (uint16_t)msgTxDataReq->gsensorconfigreqmsg->framecontrol;
        uint8_t enable = (uint8_t)msgTxDataReq->gsensorconfigreqmsg->enable;
        uint8_t sensitivity = (uint8_t)msgTxDataReq->gsensorconfigreqmsg->sensitivity;
    	
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "G-Sensor Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "enable is %d\n", enable);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "sensitivity is %d\n", sensitivity);
    		
        Csf_sendGSensorConfigRequest(&pDstAddr,
        							framecontrol,
        							enable,
        							sensitivity);
    }
    
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_electricLockConfigReq)
    {
    	uint16_t framecontrol = (uint16_t)msgTxDataReq->electriclockconfigreqmsg->framecontrol;
        uint8_t enable = (uint8_t)msgTxDataReq->electriclockconfigreqmsg->enable;
        uint8_t time = (uint8_t)msgTxDataReq->electriclockconfigreqmsg->time;
    	
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "Electric Lock Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "enable is %d\n", enable);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "time is %d\n", time);
    		
        Csf_sendElectricLockConfigRequest(&pDstAddr,
        							framecontrol,
        							enable,
        							time);
    }
    
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_signalConfigReq)
    {
    	uint16_t framecontrol = (uint16_t)msgTxDataReq->signalconfigreqmsg->framecontrol;
        uint8_t mode = (uint8_t)msgTxDataReq->signalconfigreqmsg->mode;
        uint8_t value = (uint8_t)msgTxDataReq->signalconfigreqmsg->value;
        uint8_t offset = (uint8_t)msgTxDataReq->signalconfigreqmsg->offset;
    	
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "Signal Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "mode is %d\n", mode);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "value is %d\n", value);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "offset is %d\n", offset);
    		
        Csf_sendSignalConfigRequest(&pDstAddr,
        							framecontrol,
        							mode,
        							value,
        							offset);
    }
    
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_tempConfigReq)
    {
    	uint16_t framecontrol = (uint16_t)msgTxDataReq->tempconfigreqmsg->framecontrol;
        uint8_t value = (uint8_t)msgTxDataReq->tempconfigreqmsg->value;
        uint8_t offset = (uint8_t)msgTxDataReq->tempconfigreqmsg->offset;
    	
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "Temp Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "value is %d\n", value);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "offset is %d\n", offset);
    	
        Csf_sendTempConfigRequest(&pDstAddr,
        							framecontrol,
        							value,
        							offset);
    }
    
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_lowBatteryConfigReq)
    {
    	uint16_t framecontrol = (uint16_t)msgTxDataReq->lowbatteryconfigreqmsg->framecontrol;
        uint8_t value = (uint8_t)msgTxDataReq->lowbatteryconfigreqmsg->value;
        uint8_t offset = (uint8_t)msgTxDataReq->lowbatteryconfigreqmsg->offset;
    	
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "Low Battery Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "value is %d\n", value);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "offset is %d\n", offset);
    	
        Csf_sendLowBatteryConfigRequest(&pDstAddr,
        							framecontrol,
        							value,
        							offset);
    }
    
    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_distanceConfigReq)
    {
    	uint16_t framecontrol = (uint16_t)msgTxDataReq->distanceconfigreqmsg->framecontrol;
        uint8_t mode = (uint8_t)msgTxDataReq->distanceconfigreqmsg->mode;
        uint16_t distance = (uint16_t)msgTxDataReq->distanceconfigreqmsg->distance;
    	
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "Distance Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "mode is %d\n", mode);
    	LOG_printf(LOG_APPSRV_MSG_CONTENT, "distance is %d\n", distance);
    	
        Csf_sendDistanceConfigRequest(&pDstAddr,
        							framecontrol,
        							mode,
        							distance);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_musicConfigReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->musicconfigreqmsg->framecontrol;
        uint8_t mode = (uint8_t)msgTxDataReq->musicconfigreqmsg->mode;
        uint16_t time = (uint16_t)msgTxDataReq->musicconfigreqmsg->time;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Music Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "mode is %d\n", mode);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "time is %d\n", time);
        
        Csf_sendMusicConfigRequest(&pDstAddr,
                                    framecontrol,
                                    mode,
                                    time);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_intervalConfigReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->intervalconfigreqmsg->framecontrol;
        uint8_t mode = (uint8_t)msgTxDataReq->intervalconfigreqmsg->mode;
        uint16_t time = (uint16_t)msgTxDataReq->intervalconfigreqmsg->time;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Interval Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "mode is %d\n", mode);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "time is %d\n", time);
        
        Csf_sendIntervalConfigRequest(&pDstAddr,
                                    framecontrol,
                                    mode,
                                    time);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_motionConfigReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->motionconfigreqmsg->framecontrol;
        uint8_t count = (uint8_t)msgTxDataReq->motionconfigreqmsg->count;
        uint16_t time = (uint16_t)msgTxDataReq->motionconfigreqmsg->time;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Motion Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "count is %d\n", count);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "time is %d\n", time);
        
        Csf_sendMotionConfigRequest(&pDstAddr,
                                    framecontrol,
                                    count,
                                    time);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_resistanceConfigReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->resistanceconfigreqmsg->framecontrol;
        uint8_t value = (uint8_t)msgTxDataReq->resistanceconfigreqmsg->value;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Resistance Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "value is %d\n", value);
        
        Csf_sendResistanceConfigRequest(&pDstAddr,
                                    framecontrol,
                                    value);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_microwaveConfigReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->microwaveconfigreqmsg->framecontrol;
        uint8_t enable = (uint8_t)msgTxDataReq->microwaveconfigreqmsg->enable;
        uint8_t sensitivity = (uint8_t)msgTxDataReq->microwaveconfigreqmsg->sensitivity;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Microwave Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "enable is %d\n", enable);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "sensitivity is %d\n", sensitivity);
        
        Csf_sendMicrowaveConfigRequest(&pDstAddr,
                                    framecontrol,
                                    enable,
                                    sensitivity);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_pirConfigReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->pirconfigreqmsg->framecontrol;
        uint8_t enable = (uint8_t)msgTxDataReq->pirconfigreqmsg->enable;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "PIR Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "enable is %d\n", enable);
        
        Csf_sendPirConfigRequest(&pDstAddr,
                                    framecontrol,
                                    enable);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_setUnsetConfigReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->setunsetconfigreqmsg->framecontrol;
        uint8_t state = (uint8_t)msgTxDataReq->setunsetconfigreqmsg->state;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "SetUnset Config Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "state is %d\n", state);
        
        Csf_sendSetUnsetConfigRequest(&pDstAddr,
                                    framecontrol,
                                    state);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_removeDevice)
    {
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Remove Device\n");
        
        Csf_removeDevice(&pDstAddr);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_clean)
    {
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Clean\n");
        
        Csf_removeAllDevice();
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_disconnectReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->disconnectreqmsg->framecontrol;
        uint16_t time = (uint16_t)msgTxDataReq->disconnectreqmsg->time;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Disconnect Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "time is %d\n", time);
        
        Csf_sendDisconnectRequest(&pDstAddr,
                                    framecontrol,
                                    time);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_electricLockActionReq)
    {
        uint16_t framecontrol = (uint16_t)msgTxDataReq->electriclockactionreqmsg->framecontrol;
        uint8_t relay = (uint8_t)msgTxDataReq->electriclockactionreqmsg->relay;
        
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "Electric Lock Action Request\n");
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "framecontrol is %d\n", framecontrol);
        LOG_printf(LOG_APPSRV_MSG_CONTENT, "relay is %d\n", relay);
        
        Csf_sendElectricLockActionRequest(&pDstAddr,
                                    framecontrol,
                                    relay);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_antennaReq)
    {
        uint8_t state = (uint8_t)msgTxDataReq->antennareqmsg->state;
        LOG_printf(LOG_ALWAYS, "appsrv_processTxDataReq antennaReq state: %d\n", state);
        Csf_sendAntennaRequest(state);
    }

    if(msgTxDataReq->msgid == SMSGS_CMD_IDS__Smsgs_cmdIds_setIntervalReq)
    {
        uint32_t reporting = (uint32_t)msgTxDataReq->setintervalreqmsg->reporting;
        uint32_t polling = (uint32_t)msgTxDataReq->setintervalreqmsg->polling;
        LOG_printf(LOG_ALWAYS, "appsrv_processTxDataReq setIntervalReq reporting: %d, polling: %d\n", reporting, polling);
        Csf_setInterval(&pDstAddr, reporting, polling);
    }

fail:
    send_AppsrvTxDataCnf(status);

    if( msgTxDataReq != NULL )
    {
        appsrv_tx_data_req__free_unpacked(msgTxDataReq, &protobuf_allocator);
    }
}

/*!
 * @brief handle a join permit request from the gateway
 * @param pCONN - where the request came from
 * @param pIncommingMsg - the msg from the gateway
 */
static void appsrv_processSetJoinPermitReq(struct appsrv_connection *pCONN,
                                           struct mt_msg *pIncomingMsg)
{

    int status;
    AppsrvSetJoinPermitReq *msgJoinReq;
    (void)pCONN;
    /* assume total failure */
    status = ApiMac_status_invalidParameter;

    /* de-serialize the incoming message */
    msgJoinReq =
        appsrv_set_join_permit_req__unpack(&protobuf_allocator,
                                           pIncomingMsg->expected_len,
                                           &pIncomingMsg->iobuf[
                                               pIncomingMsg->iobuf_idx]);
    if(msgJoinReq == NULL)
    {
        LOG_printf(LOG_ERROR, "Error parsing appsrv_processSetJoinPermitReq\n");
        goto fail;
    }

    MT_MSG_rdBuf(pIncomingMsg, NULL, pIncomingMsg->expected_len);
    MT_MSG_parseComplete(pIncomingMsg);
    if(pIncomingMsg->is_error)
    {
        goto fail;
    }
    /* Set the join permit */
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "sending duration is ____ 0x%x\n",
               msgJoinReq->duration);
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    status = Cllc_setJoinPermit(msgJoinReq->duration);
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "sending Permit confirm message\n");
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");

fail:
    send_AppSrvJoinPermitCnf(status);

    if( msgJoinReq != NULL )
    {
        appsrv_set_join_permit_req__free_unpacked(msgJoinReq, 
                                                  &protobuf_allocator);
    }
}

/*!
 * @brief handle a getnetwork info request from the gateway
 * @param pCONN - where the request came from
 */
static void appsrv_processGetNwkInfoReq(struct appsrv_connection *pCONN)
{
    struct mt_msg *pMsg;
    AppsrvGetNwkInfoCnf *pAnswer;
    size_t len;

    pAnswer = copy_AppsrvGetNwkInfoCnf();
    if( pAnswer == NULL ){
        LOG_printf(LOG_ERROR, "out of memory\n");
        return;
    }
        

    len = appsrv_get_nwk_info_cnf__get_packed_size(pAnswer);
    pMsg =
        MT_MSG_alloc((int)len,
                     MT_MSG_cmd0_areq(
                         TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                     APPSRV__CMD_ID__APPSRV_GET_NWK_INFO_CNF);

    if(pMsg)
    {
        pMsg->pLogPrefix = "get-nwk-info-req";
        /* this is a broadcast, so we do not have a */
        /* specific interface... so use the template for now */
        MT_MSG_setDestIface(pMsg, &(pCONN->socket_interface));
        appsrv_get_nwk_info_cnf__pack(pAnswer, &pMsg->iobuf[ pMsg->iobuf_idx ]);
        MT_MSG_wrBuf(pMsg, NULL, len);
        MT_MSG_txrx(pMsg);
        MT_MSG_free(pMsg);
    }

    free_AppsrvGetNwkInfoCnf(pAnswer);
    pAnswer = NULL;
}

/*!
 * @brief  Process incoming getDeviceArrayReq message
 *
 * @param pConn - the connection
 */
static void appsrv_processGetDeviceArrayReq(struct appsrv_connection *pCONN)
{
    AppsrvGetDeviceArrayCnf *pAnswer;
    struct mt_msg *pMsg;
    size_t len;

    pMsg = NULL;

    pAnswer = copy_AppsrvGetDeviceArrayCnf();

    if(pAnswer)
    {
        len = appsrv_get_device_array_cnf__get_packed_size(pAnswer);
        pMsg =
            MT_MSG_alloc(
                (int)len,
                MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                APPSRV__CMD_ID__APPSRV_GET_DEVICE_ARRAY_CNF);
        if(pMsg)
        {
            pMsg->pLogPrefix = "device-array-req";
            MT_MSG_setDestIface(pMsg, &(pCONN->socket_interface));
            appsrv_get_device_array_cnf__pack(pAnswer,
                                              &pMsg->iobuf[ pMsg->iobuf_idx ]);
            MT_MSG_wrBuf(pMsg, NULL, len);
            MT_MSG_txrx(pMsg);
        }
    }

    if(pMsg)
    {
        MT_MSG_free(pMsg);
    }
    free_AppsrvGetDeviceArrayCnf(pAnswer);
}

/******************************************************************************
 Function Implementation
*****************************************************************************/

/*
  Broadcast a message to all connections.
  Public function in appsrv.h
*/
void appsrv_broadcast(struct mt_msg *pMsg)
{
    struct appsrv_connection *pCONN;
    struct mt_msg *pClone;

    /* mark all connections as "ready to broadcast" */
    lock_connection_list();

    for(pCONN = all_connections ; pCONN ; pCONN = pCONN->pNext)
    {
        pCONN->is_busy = false;
    }

    unlock_connection_list();

next_connection:
    /* find first connection in "ready-state"
     * NOTE: this loop is funny we goto the top
     * and we restart scanning from the head
     * because ... while broadcasting a new
     * connection may appear, or one may go away
     */
    lock_connection_list();
    for(pCONN = all_connections ; pCONN ; pCONN = pCONN->pNext)
    {
        /* this one is dead */
        if(pCONN->is_dead)
        {
            continue;
        }
        /* is this one ready? */

        if(pCONN->is_busy == false)
        {
            /* Yes we have found one */
            pCONN->is_busy = true;
            break;
        }
    }
    unlock_connection_list();

    /* Did we find a connection? */
    if(pCONN)
    {
        /* we have a connection we can send */
        pClone = MT_MSG_clone(pMsg);
        if(pClone)
        {
            MT_MSG_setDestIface(pClone, &(pCONN->socket_interface));
            MT_MSG_txrx(pClone);
            MT_MSG_free(pClone);
        }
        /* leave this connection as 'busy'
         * busy really means: "done"
         * so that we do not repeat this connection
         * we clean up the list later
         *---
         * Go back to the *FIRST* connection
         * And search again.... from the top..
         * Why? Because connections may have died...
         */
        goto next_connection;
    }

    /* if we get here - we have no more connections to broadcast to */
    /* mark all items as idle */
    lock_connection_list();
    for(pCONN = all_connections ; pCONN ; pCONN = pCONN->pNext)
    {
        pCONN->is_busy = false;
    }
    unlock_connection_list();
}

/*!
  Csf module calls this function to inform the user/appClient
  that the application has either started/restored the network

  Public function defined in appsrv_Collector.h
*/
void appsrv_networkUpdate(bool restored, Llc_netInfo_t *networkInfo)
{
    AppsrvNwkInfoUpdateInd *pData;

    struct mt_msg *pMsg;
    size_t len;

    (void)(restored);
    pData = copy_AppsrvNwkInfoUpdateInd(networkInfo);

    if(pData)
    {
        len = appsrv_nwk_info_update_ind__get_packed_size(pData);

        pMsg =
            MT_MSG_alloc(
                (int)len,
                MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                APPSRV__CMD_ID__APPSRV_NWK_INFO_IND);

        if(pMsg)
        {
            /* this is a broadcast, so we do not have a */
            /* specific interface... so use the template for now */
            pMsg->pLogPrefix = "network-info-update-ind";

            MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
            appsrv_nwk_info_update_ind__pack(pData, &(pMsg->iobuf[pMsg->iobuf_idx]));
            MT_MSG_wrBuf(pMsg, NULL, len);
            appsrv_broadcast(pMsg);
            MT_MSG_free(pMsg);
            pMsg = NULL;
        }
    }

    free_AppsrvNwkInfoUpdateInd(pData);
    pData = NULL;
}

/*!

  Csf module calls this function to inform the user/appClient
  that a device has joined the network

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceUpdate(Llc_deviceListItem_t *pDevListItem)
{

    AppsrvDeviceUpdateInd *pData;

    struct mt_msg *pMsg;
    size_t len;

    pData = copy_AppsrvDeviceUpdateInd(pDevListItem);

    if(pData)
    {
        len = appsrv_device_update_ind__get_packed_size(pData);

        pMsg = MT_MSG_alloc((int)len,
                            MT_MSG_cmd0_areq(
                                TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                            APPSRV__CMD_ID__APPSRV_DEVICE_JOINED_IND);

        if(pMsg)
        {
            /* this is a broadcast, so we do not have a */
            /* specific interface... so use the template for now */
            pMsg->pLogPrefix = "device-update-ind";

            MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
            appsrv_device_update_ind__pack(pData, &(pMsg->iobuf[pMsg->iobuf_idx]));
            MT_MSG_wrBuf(pMsg, NULL, len);
            appsrv_broadcast(pMsg);
            MT_MSG_free(pMsg);
            pMsg = NULL;
        }

    }

    free_AppsrvDeviceUpdateInd(pData);
    pData = NULL;
}

/*
 * @brief common code to handle sensor data messages
 * @param pSrcAddr - address related to this message
 * @param rssi - signal strength from device
 * @param pDataMsg - the sensor message
 * @param pRspMsg - the response
 * @param pSerCfgPirRspMsg - the response
 *
 * In the end, the message is sent to the gateway
 */
static void appsrv_deviceSensorData_common(ApiMac_sAddr_t *pSrcAddr,
                                           int8_t rssi,
                                           Smsgs_sensorMsg_t *pDataMsg,
                                           Smsgs_configRspMsg_t *pRspMsg)
{

    AppsrvDeviceDataRxInd *pData;

    struct mt_msg *pMsg;
    size_t len;

    pData = copy_AppsrvDeviceDataRxInd(pSrcAddr, rssi, pDataMsg, pRspMsg);
    if(!pData)
    {
        LOG_printf(LOG_ERROR, "no memory for AppsrvDeviceConfigRspInd\n");
        return;
    }

    len = appsrv_device_data_rx_ind__get_packed_size(pData);

    pMsg = MT_MSG_alloc((int)(len),
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_DEVICE_DATA_RX_IND);
    if(pMsg)
    {
        pMsg->pLogPrefix = "device-data-ind";

        MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
        appsrv_device_data_rx_ind__pack(pData, &(pMsg->iobuf[pMsg->iobuf_idx]));
        MT_MSG_wrBuf(pMsg, NULL, len);
        LOG_printf( LOG_ALWAYS, "LOG_hexdump pMsg---------------------------\n");
        LOG_hexdump(LOG_DBG_MT_MSG_raw,
                        0,
                        pMsg->iobuf,
                        pMsg->iobuf_nvalid);
        appsrv_broadcast(pMsg);

		
        MT_MSG_free(pMsg);
        pMsg = NULL;
    }
    free_AppsrvDeviceDataRxInd(pData);

    pData = NULL;
}

/*! ======================================================================= */
static void appsrv_deviceResponseData_common(ApiMac_sAddr_t *pSrcAddr,
                                           	int8_t rssi,
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

    AppsrvDeviceDataRxInd *pData;

    struct mt_msg *pMsg;
    size_t len;

    pData = copy_AppsrvDeviceResponseDataRxInd(pSrcAddr, rssi, pAlarmCfgRspMsg, 
    															pGSensorCfgRspMsg, 
    															pElectricCfgRspMsg,
    															pSignalCfgRspMsg,
    															pTempCfgRspMsg,
    															pLowBatteryCfgRspMsg,
    															pDistanceCfgRspMsg,
                                                                pMusicCfgRspMsg,
                                                                pIntervalCfgRspMsg,
                                                                pMotionCfgRspMsg,
                                                                pResistanceCfgRspMsg,
                                                                pMicrowaveCfgRspMsg,
                                                                pPirCfgRspMsg,
                                                                pSetUnsetCfgRspMsg,
                                                                pDisconnectRspMsg,
                                                                pElectricActRspMsg);
    if(!pData)
    {
        LOG_printf(LOG_ERROR, "no memory for AppsrvDeviceConfigRspInd\n");
        return;
    }

    len = appsrv_device_data_rx_ind__get_packed_size(pData);

    pMsg = MT_MSG_alloc((int)(len),
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_DEVICE_DATA_RX_IND);
    if(pMsg)
    {
        pMsg->pLogPrefix = "device-data-ind";

        MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
        appsrv_device_data_rx_ind__pack(pData, &(pMsg->iobuf[pMsg->iobuf_idx]));
        MT_MSG_wrBuf(pMsg, NULL, len);
        LOG_printf( LOG_ALWAYS, "LOG_hexdump pMsg---------------------------\n");
        LOG_hexdump(LOG_DBG_MT_MSG_raw,
                        0,
                        pMsg->iobuf,
                        pMsg->iobuf_nvalid);
        appsrv_broadcast(pMsg);

		
        MT_MSG_free(pMsg);
        pMsg = NULL;
    }
    free_AppsrvDeviceResponseDataRxInd(pData);

    pData = NULL;
}

void appsrv_deviceAlarmConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_alarmConfigRspMsg_t *pCfgRspMsg)
{
	appsrv_deviceResponseData_common(pSrcAddr, rssi, pCfgRspMsg, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceGSensorConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_gSensorConfigRspMsg_t *pCfgRspMsg)
{
	appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, pCfgRspMsg, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceElectricLockConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_electricLockConfigRspMsg_t *pCfgRspMsg)
{
	appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, pCfgRspMsg, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceSignalConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_signalConfigRspMsg_t *pCfgRspMsg)
{
	appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, pCfgRspMsg, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceTempConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_tempConfigRspMsg_t *pCfgRspMsg)
{
	appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, pCfgRspMsg, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceLowBatteryConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_lowBatteryConfigRspMsg_t *pCfgRspMsg)
{
	appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, pCfgRspMsg, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceDistanceConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_distanceConfigRspMsg_t *pCfgRspMsg)
{
	appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, pCfgRspMsg, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceMusicConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_musicConfigRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, pCfgRspMsg,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceIntervalConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_intervalConfigRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        pCfgRspMsg, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceMotionConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_motionConfigRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, pCfgRspMsg, NULL, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceResistanceConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_resistanceConfigRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, pCfgRspMsg, NULL, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceMicrowaveConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_microwaveConfigRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, pCfgRspMsg, NULL, NULL,
                                                        NULL, NULL);
}

void appsrv_devicePirConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_pirConfigRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, pCfgRspMsg, NULL,
                                                        NULL, NULL);
}

void appsrv_deviceSetUnsetConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_setUnsetConfigRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, pCfgRspMsg,
                                                        NULL, NULL);
}

void appsrv_deviceDisconnectUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_disconnectRspMsg_t *pCfgRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        pCfgRspMsg, NULL);
}

void appsrv_deviceElectricLockActionUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_electricLockActionRspMsg_t *pActRspMsg)
{
    appsrv_deviceResponseData_common(pSrcAddr, rssi, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, NULL, NULL, NULL, NULL, NULL,
                                                        NULL, pActRspMsg);
}

/*!
  Csf module calls this function to inform the user/appClient
  that the device has responded to the configuration request

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceConfigUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                               Smsgs_configRspMsg_t *pRspMsg)
{
    appsrv_deviceSensorData_common(pSrcAddr, rssi, NULL, pRspMsg);
}

void appsrv_pairingUpdate(int8_t state)
{
    size_t len;
    AppsrvPairingUpdateInd *pDATA;
    struct mt_msg *pMsg;
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    LOG_printf(LOG_APPSRV_MSG_CONTENT,
               "sending pairing message (newstate: %d)\n",
               (int)(state));
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    pDATA = copy_AppsrvPairingUpdateInd(state);
    if(pDATA == NULL)
    {
        return;
    }

    len = appsrv_pairing_update_ind__get_packed_size(pDATA);

    pMsg = MT_MSG_alloc((int)len,
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_PAIRING_IND);

    if(pMsg)
    {
        pMsg->pLogPrefix = "appsrv_pairing";
        MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
        appsrv_pairing_update_ind__pack(pDATA,
                                    &(pMsg->iobuf[
                                    pMsg->iobuf_idx]));
        MT_MSG_wrBuf(pMsg, NULL, len);
        appsrv_broadcast(pMsg);

        MT_MSG_free(pMsg);
        pMsg = NULL;
    }

    free_AppsrvPairingUpdateInd(pDATA);
}

void appsrv_antennaUpdate(int8_t state)
{
    size_t len;
    AppsrvAntennaUpdateInd *pDATA;
    struct mt_msg *pMsg;
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    LOG_printf(LOG_APPSRV_MSG_CONTENT,
               "sending antenna message (newstate: %d)\n",
               (int)(state));
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    pDATA = copy_AppsrvAntennaUpdateInd(state);
    if(pDATA == NULL)
    {
        return;
    }

    len = appsrv_antenna_update_ind__get_packed_size(pDATA);

    pMsg = MT_MSG_alloc((int)len,
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_ANTENNA_IND);

    if(pMsg)
    {
        pMsg->pLogPrefix = "appsrv_antenna";
        MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
        appsrv_antenna_update_ind__pack(pDATA,
                                    &(pMsg->iobuf[
                                    pMsg->iobuf_idx]));
        MT_MSG_wrBuf(pMsg, NULL, len);
        appsrv_broadcast(pMsg);

        MT_MSG_free(pMsg);
        pMsg = NULL;
    }

    free_AppsrvAntennaUpdateInd(pDATA);
}

void appsrv_setIntervalUpdate(ApiMac_sAddr_t *pSrcAddr, uint32_t reporting, uint32_t polling)
{
    size_t len;
    AppsrvSetIntervalUpdateInd *pDATA;
    struct mt_msg *pMsg;
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    LOG_printf(LOG_APPSRV_MSG_CONTENT,
               "sending set interval message (reporting: %lu, polling: %lu)\n",
               (unsigned long)(reporting), (unsigned long)(polling));
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    pDATA = copy_AppsrvSetIntervalUpdateInd(pSrcAddr, reporting, polling);
    if(pDATA == NULL)
    {
        return;
    }

    len = appsrv_set_interval_update_ind__get_packed_size(pDATA);

    pMsg = MT_MSG_alloc((int)len,
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_SET_INTERVAL_IND);

    if(pMsg)
    {
        pMsg->pLogPrefix = "appsrv_setInverval";
        MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
        appsrv_set_interval_update_ind__pack(pDATA,
                                    &(pMsg->iobuf[
                                    pMsg->iobuf_idx]));
        MT_MSG_wrBuf(pMsg, NULL, len);
        appsrv_broadcast(pMsg);

        MT_MSG_free(pMsg);
        pMsg = NULL;
    }

    free_AppsrvSetIntervalUpdateInd(pDATA);
}

/*!
  Csf module calls this function to inform the user/appClient
  that a device is no longer active in the network

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceNotActiveUpdate(ApiMac_deviceDescriptor_t *pDevInfo,
                                  bool timeout)
{
    AppsrvDeviceNotActiveUpdateInd *pData;
    struct mt_msg *pMsg;
    size_t len;

    pData = copy_AppsrvDeviceNotActiveUpdateInd(pDevInfo, timeout);
    if(pData)
    {
        len = appsrv_device_not_active_update_ind__get_packed_size(pData);

        pMsg =
            MT_MSG_alloc((int)len,
                         MT_MSG_cmd0_areq(
                             TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                         APPSRV__CMD_ID__APPSRV_DEVICE_NOTACTIVE_UPDATE_IND);
        if(pMsg)
        {
            pMsg->pLogPrefix = "device-not-active-ind";

            /* this is a broadcast, so we do not have a
               specific interface... so use the template for now */
            MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);

            appsrv_device_not_active_update_ind__pack(pData,
                                                      &(pMsg->iobuf[
                                                            pMsg->iobuf_idx]));
            MT_MSG_wrBuf(pMsg, NULL, len);

            appsrv_broadcast(pMsg);
            MT_MSG_free(pMsg);
            pMsg = NULL;
        }
    }
    free_AppsrvDeviceNotActiveUpdateInd(pData);
    pData = NULL;
}

/*!
  Csf module calls this function to inform the user/appClient
  of the reported sensor data from a network device

  Public function defined in appsrv_Collector.h
*/
void appsrv_deviceSensorDataUpdate(ApiMac_sAddr_t *pSrcAddr, int8_t rssi,
                                   Smsgs_sensorMsg_t *pSensorMsg)
{
    appsrv_deviceSensorData_common(pSrcAddr, rssi, pSensorMsg, NULL);
}

/*!
  TBD

  Public function defined in appsrv_Collector.h
*/
void appsrv_stateChangeUpdate(Cllc_states_t state)
{
    size_t len;
    AppsrvCollectorStateCngUpdateInd *pDATA;
    struct mt_msg *pMsg;
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    LOG_printf(LOG_APPSRV_MSG_CONTENT,
               "sending state chng message (newstate: %d)\n",
               (int)(state));
    LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
    pDATA = copy_AppsrvCollectorStateCngUpdateInd(state);
    if(pDATA == NULL)
    {
        return;
    }

    len = appsrv_collector_state_cng_update_ind__get_packed_size(pDATA);

    pMsg = MT_MSG_alloc((int)len,
                        MT_MSG_cmd0_areq(TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV),
                        APPSRV__CMD_ID__APPSRV_COLLECTOR_STATE_CNG_IND);

    if(pMsg)
    {
        pMsg->pLogPrefix = "appsrv_collector_state_cng";
        MT_MSG_setDestIface(pMsg, &appClient_mt_interface_template);
        appsrv_collector_state_cng_update_ind__pack(pDATA,
                                                    &(pMsg->iobuf[
                                                          pMsg->iobuf_idx]));
        MT_MSG_wrBuf(pMsg, NULL, len);
        appsrv_broadcast(pMsg);

        MT_MSG_free(pMsg);
        pMsg = NULL;
    }

    free_AppsrvCollectorStateCngUpdateInd(pDATA);
}

/*********************************************************************
 * Local Functions
 *********************************************************************/

/*!
 * @brief handle a request fromm a client.
 * @param pCONN - the client connection details
 * @param pMsg  - the message we received.
 */
static void appsrv_handle_appClient_request( struct appsrv_connection *pCONN,
                                             struct mt_msg *pMsg )
{
    int subsys = _bitsXYof(pMsg->cmd0 , 4, 0);
    int handled = true;
    if(subsys != TIMAC_APP_SRV_SYS_ID__RPC_SYS_PB_TIMAC_APPSRV)
    {
        handled = false;
    }
    else
    {
        switch(pMsg->cmd1)
        {
        default:
            handled = false;
            break;
            /*
             * NOTE: ADD MORE ITEMS HERE TO EXTEND THE EXAMPLE
             */
        case APPSRV__CMD_ID__APPSRV_GET_DEVICE_ARRAY_REQ:
            /* Rcvd data from Client */
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "rcvd get device array msg\n");
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            appsrv_processGetDeviceArrayReq(pCONN);
            break;

        case APPSRV__CMD_ID__APPSRV_GET_NWK_INFO_REQ:
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            LOG_printf(LOG_APPSRV_MSG_CONTENT,"getnwkinfo req message\n");
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            appsrv_processGetNwkInfoReq(pCONN);
            break;

        case APPSRV__CMD_ID__APPSRV_SET_JOIN_PERMIT_REQ:
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            LOG_printf(LOG_APPSRV_MSG_CONTENT,"rcvd join premit message\n ");
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            appsrv_processSetJoinPermitReq(pCONN, pMsg);
            break;

        case APPSRV__CMD_ID__APPSRV_TX_DATA_REQ:
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            LOG_printf(LOG_APPSRV_MSG_CONTENT,"rcvd req to send message to a device\n ");
            LOG_printf(LOG_APPSRV_MSG_CONTENT, "______________________________\n");
            appsrv_processTxDataReq(pCONN, pMsg);
            break;
        }
    }
    if(!handled)
    {
        MT_MSG_log(LOG_ERROR, pMsg, "unknown msg\n");
    }
}

/*
 * @brief specific connection thread
 * @param cookie - opaque parameter that is the connection details.
 *
 * The server thread creates these as needed
 *
 * This thread lives until the connection dies.
 */
static intptr_t s2appsrv_thread(intptr_t cookie)
{
    struct appsrv_connection *pCONN;
    struct mt_msg *pMsg;
    int r;
    char iface_name[30];
    char star_line[30];
    int star_line_char;

    pCONN = (struct appsrv_connection *)(cookie);
    if( pCONN == NULL )
    {
        BUG_HERE("pCONN is null?\n");
    }

    /* create our upstream interface */
    (void)snprintf(iface_name,
                   sizeof(iface_name),
                   "s2u-%d-iface",
                   pCONN->connection_id);
    pCONN->socket_interface.dbg_name = iface_name;

    /* Create our interface */
    r = MT_MSG_interfaceCreate(&(pCONN->socket_interface));
    if(r != 0)
    {
        BUG_HERE("Cannot create socket interface?\n");
    }

    /* Add this connection to the list. */
    lock_connection_list();
    pCONN->pNext = all_connections;
    pCONN->is_busy = false;
    all_connections = pCONN;
    unlock_connection_list();

    star_line_char = 0;
    /* Wait for messages to come in from the socket.. */
    for(;;)
    {
        /* Did the other guy die? */
        if(pCONN->is_dead)
        {
            break;
        }

        /* did the socket die? */
        if(pCONN->socket_interface.is_dead)
        {
            pCONN->is_dead = true;
            continue;
        }

        /* get our message */
        pMsg = MT_MSG_LIST_remove(&(pCONN->socket_interface),
                                  &(pCONN->socket_interface.rx_list), 1000);
        if(pMsg == NULL)
        {
            /* must have timed out. */
            continue;
        }

        pMsg->pLogPrefix = "web-request";

        /* Print a *MARKER* line in the log to help trace this message */
        star_line_char++;
        /* Cycle through the letters AAAA, BBBB, CCCC .... */
        star_line_char = star_line_char % 26;
        memset((void *)(star_line),
               star_line_char + 'A',
               sizeof(star_line) - 1);
        star_line[sizeof(star_line) - 1] = 0;
        LOG_printf(LOG_DBG_MT_MSG_traffic, "START MSG: %s\n", star_line);

        /* Actually process the request */
        appsrv_handle_appClient_request(pCONN, pMsg);

        /* Same *MARKER* line at the end of the message */
        LOG_printf(LOG_DBG_MT_MSG_traffic, "END MSG: %s\n", star_line);
        MT_MSG_free(pMsg);
        pMsg = NULL;
    }

    /* There is an interock here.
     * FIRST: We set "is_dead"
     *   The broadcast code will skip this item.
     *   if and only if it is dead.
     * HOWEVER
     *   Q: What happens if we die in the middle of broadcasting?
     *   A: We must wait until the broad cast is complete
     *      We know this by checking the bcast_state.
     */
    while(pCONN->is_busy)
    {
        /* we must wait ... a broadcast is active. */
        /* so sleep alittle and try again */
        TIMER_sleep(10);
    }

    /* we can now remove this DEAD connection from the list. */
    lock_connection_list();
    {
        struct appsrv_connection **ppTHIS;

        /* find our self in the list of connections. */
        for(ppTHIS = &all_connections ;
            (*ppTHIS) != NULL ;
            ppTHIS = &((*ppTHIS)->pNext))
        {
            /* found? */
            if((*ppTHIS) == pCONN)
            {
                /* yes we are done */
                break;
            }
        }

        /* did we find this one? */
        if(*ppTHIS)
        {
            /* remove this one from the list */
            (*ppTHIS) = pCONN->pNext;
            pCONN->pNext = NULL;
        }
    }
    unlock_connection_list();
    /* socket is dead */
    /* we need to destroy the interface */
    MT_MSG_interfaceDestroy(&(pCONN->socket_interface));

    /* destroy the socket. */
    SOCKET_ACCEPT_destroy(pCONN->socket_interface.hndl);

    /* we can free this now */
    free((void *)pCONN);

    /* we do not destroy free the connection */
    /* this is done in the uart thread */
    return 0;
}

/*
 * @brief This thread handles all connections from the nodeJS/gateway client.
 *
 * This server thread spawns off threads that handle
 * each connection from each gateway app.
 */

static intptr_t appsrv_server_thread(intptr_t _notused)
{
    (void)(_notused);

    int r;
    intptr_t server_handle;
    struct appsrv_connection *pCONN;
    int connection_id;
    char buf[30];

    pCONN = NULL;
    connection_id = 0;

    server_handle = SOCKET_SERVER_create(&appClient_socket_cfg);
    if(server_handle == 0)
    {
        BUG_HERE("cannot create socket to listen\n");
    }

    r = SOCKET_SERVER_listen(server_handle);
    if(r != 0)
    {
        BUG_HERE("cannot set listen mode\n");
    }

    /* Wait for connections :-) */
    for(;;)
    {
        if(STREAM_isError(server_handle))
        {
            LOG_printf(LOG_ERROR, "server (accept) socket is dead\n");
            break;
        }
        if(pCONN == NULL)
        {
            pCONN = calloc(1, sizeof(*pCONN));
            if(pCONN == NULL)
            {
                BUG_HERE("No memory\n");
            }
            pCONN->connection_id = connection_id++;

            /* clone the connection details */
            pCONN->socket_interface = appClient_mt_interface_template;

            (void)snprintf(buf,sizeof(buf),
                           "connection-%d",
                           pCONN->connection_id);
            pCONN->dbg_name = strdup(buf);
            if(pCONN->dbg_name == NULL)
            {
                BUG_HERE("no memory\n");
            }
        }

        /* wait for a connection.. */
        r = SOCKET_SERVER_accept(&(pCONN->socket_interface.hndl),
                                 server_handle,
                                 appClient_socket_cfg.connect_timeout_mSecs);
        if(r < 0)
        {
            BUG_HERE("cannot accept!\n");
        }
        if(r == 0)
        {
            LOG_printf(LOG_APPSRV_CONNECTIONS, "no connection yet\n");
            continue;
        }
        /* we have a connection */
        pCONN->is_dead = false;

        /* create our connection threads */

        /* set name final use */
        (void)snprintf(buf, sizeof(buf),
                       "connection-%d",
                       pCONN->connection_id);
        pCONN->dbg_name = strdup(buf);
        if(pCONN->dbg_name == NULL)
        {
            BUG_HERE("no memory\n");
        }

        (void)snprintf(buf, sizeof(buf),
                       "thread-u2s-%d",
                       pCONN->connection_id);
        pCONN->thread_id_s2appsrv = THREAD_create(buf,
                                                  s2appsrv_thread,
                                                  (intptr_t)(pCONN),
                                                  THREAD_FLAGS_DEFAULT);

        pCONN = NULL;

    }
    return 0;
}


/*
 * @brief the primary "collector thread"
 *
 * This thread never exists and performs all of
 * the 'collector' application tasks.
 */

static intptr_t collector_thread(intptr_t dummy)
{
    (void)(dummy);
    for(;;)
    {
        /* this will "pend" on a semaphore */
        /* waiting for messages to come */
        Collector_process();
    }
#if defined(__linux__)
    /* gcc complains, unreachable.. */
    /* other analisys tools do not .. Grrr. */
    return 0;
#endif
}

/*
  This is the main application, "linux_main.c" calls this.
*/
void APP_main(void)
{
    int r;
    intptr_t server_thread_id;
    intptr_t collector_thread_id;
    struct appsrv_connection *pCONN;

    all_connections_mutex = MUTEX_create("all-connections");

    if(all_connections_mutex == 0)
    {
        BUG_HERE("cannot create connection list mutex\n");
    }

    Collector_init();
    r = MT_DEVICE_version_info.transport |
        MT_DEVICE_version_info.product |
        MT_DEVICE_version_info.major |
        MT_DEVICE_version_info.minor |
        MT_DEVICE_version_info.maint;
    if( r == 0 )
    {
        FATAL_printf( "Did not get device version info at startup - Bailing out\n");
    } 

    LOG_printf( LOG_ALWAYS, "Found Mac Co-Processor Version info is:\n");
    LOG_printf( LOG_ALWAYS, "Transport: %d\n", MT_DEVICE_version_info.transport );
    LOG_printf( LOG_ALWAYS, "  Product: %d\n", MT_DEVICE_version_info.product   );
    LOG_printf( LOG_ALWAYS, "    Major: %d\n", MT_DEVICE_version_info.major     );
    LOG_printf( LOG_ALWAYS, "    Minor: %d\n", MT_DEVICE_version_info.minor     );
    LOG_printf( LOG_ALWAYS, "    Maint: %d\n", MT_DEVICE_version_info.maint     );

    fprintf( stderr, "Found Mac Co-Processor Version info is:\n");
    fprintf( stderr, "Transport: %d\n", MT_DEVICE_version_info.transport );
    fprintf( stderr, "  Product: %d\n", MT_DEVICE_version_info.product   );
    fprintf( stderr, "    Major: %d\n", MT_DEVICE_version_info.major     );
    fprintf( stderr, "    Minor: %d\n", MT_DEVICE_version_info.minor     );
    fprintf( stderr, "    Maint: %d\n", MT_DEVICE_version_info.maint     );
    fprintf( stderr, "----------------------------------------\n");
    fprintf( stderr, "Start the gateway application\n");

    server_thread_id = THREAD_create("server-thread",
                                     appsrv_server_thread, 0,
                                     THREAD_FLAGS_DEFAULT);

    collector_thread_id = THREAD_create("collector-thread",
                                        collector_thread, 0, THREAD_FLAGS_DEFAULT);
  

    for(;;)
    {
        /* every 10 seconds.. */
        /* 10 seconds is an arbitrary value */
        TIMER_sleep(10 * 1000);
        r = 0;

        if(THREAD_isAlive(collector_thread_id))
        {
            r += 1;
        }

        if(THREAD_isAlive(server_thread_id))
        {
            r += 1;
        }
        /* we stay here while both *2* threads are alive */
        if(r != 2)
        {
            break;
        }
    }

    /* wait at most (N) seconds then we just 'die' */
    r = 0;
    while(r < 10)
    {

        lock_connection_list();
        pCONN = all_connections;
        /* mark them all as dead */
        while(pCONN)
        {
            pCONN->is_dead = true;
            LOG_printf(LOG_APPSRV_CONNECTIONS,
                       "Connection: %s is still alive\n",
                       pCONN->dbg_name);
            pCONN = pCONN->pNext;
        }
        unlock_connection_list();

        /* still have connections? */
        if(all_connections)
        {
            /* wait a second.. */
            TIMER_sleep(10000);
            /* and try again */
            r++;
            continue;
        }
        else
        {
            break;
        }
    }
    /* thread exit */
}

/*!
 * Set default items for the application.
 * These are set before the ini file is read.
 */
void APP_defaults(void)
{
    memset( (void *)(&appClient_socket_cfg), 0, sizeof(appClient_socket_cfg) );
    /*
      NOTE: All of these settings can be modified by ini file.
    */
    appClient_socket_cfg.inet_4or6 = 4;
    /*! this is a 's'=server, not a 'c'client */
    appClient_socket_cfg.ascp = 's';
    appClient_socket_cfg.host = NULL;
    /*! we listen on port 5000 */
    appClient_socket_cfg.service = strdup( "5000" );
    /*! and only allow one connection */
    appClient_socket_cfg.server_backlog = 1;
    appClient_socket_cfg.device_binding = NULL;
    /*! print a 'non-connect' every minute */
    appClient_socket_cfg.connect_timeout_mSecs = 60 * 1000;
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

