/******************************************************************************
 @file appClient.js
 
 @brief TIMAC-2.0.0 Example Application - appClient Implementation
 
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

/* *********************************************************************
 * Require Modules for connection with TIMAC Application Server
 * ********************************************************************/
var net = require("net");
var protobuf = require("protocol-buffers");
var fs = require("fs");
var events = require("events");
var request = require('request');

var Device = require('./devices/device.js');
var NwkInfo = require('./nwkinfo/nwkinfo.js');

/* *********************************************************************
 * Defines
 * ********************************************************************/
/* APP Server Port, this should match the port number defined in the
 file: appsrv.c */
const APP_SERVER_PORT = 5000;
/* Timeout in ms to attempt to reconnect to app server */
const APP_CLIENT_RECONNECT_TIMEOUT = 5000;
const PKT_HEADER_SIZE = 4;
const PKT_HEADER_LEN_FIELD = 0;
const PKT_HEADER_SUBSYS_FIELD = 2;
const PKT_HEADER_CMDID_FIELD = 3;

const PYTHON_IP = '127.0.0.1';
const PYTHON_PORT = '5001';

const TYPE_EVE003 = 'eve003Sensor';
const TYPE_EVE004 = 'eve004Sensor';
const TYPE_EVE005 = 'eve005Sensor';
const TYPE_EVE006 = 'eve006Sensor';

const STATUS_NONE = 0x00;
const STATUS_SENT = 0x01;
const RUN_MAX = 2;
const RETRY_MAX = 3;

/* *********************************************************************
 * Variables
 * ********************************************************************/
/* AppClient Instance */
var appClientInstance;

/* ********************************************************************
 * Initialization Function
 **********************************************************************/
/*!
 * @brief      Constructor for appClient
 *
 * @param      none
 *
 * @retun      none
 */
function Appclient() {
    
    /* There should be only one app client */
    if (typeof appClientInstance !== "undefined") {
        return appClientInstance;
    }
    
    /* set-up the instance */
    appClientInstance = this;
    /* set-up to emit events */
    events.EventEmitter.call(this);
    /* set-up to connect to app server */
    var appClient = net.Socket();
    /* set-up to decode/encode proto messages */
    var timac_pb = protobuf(fs.readFileSync('/usr/local/ti154/example/gateway/appClient/protofiles/appsrv.proto'));
    /* Start the connection with app Server */
    appClient.connect(APP_SERVER_PORT, '127.0.0.1', function () {
        console.log("Connected to App Server");
        /* Request Network Information */
        appC_getNwkInfoFromAppServer();
    });
    /* set-up callback for incoming data from the app server */
    appClient.on('data', function (data) {
        /* Call the incoming data processing function */
        appC_processIncoming(data);
    });
    
    /* set-up to handle error event */
    appClient.on('error', function (data) {
        /* connection lost or unable to make connection with the
           appServer, need to get network and device info back again
           as those may have changed */
        console.log("ERROR: Rcvd Error on the socket connection with AppServer");
        appClientReconnect();
    });
    /* Device list array */
    this.connectedDeviceList = [];
    self = this;
    /* Netowrk Information var */
    this.nwkInfo;
    this.sensorMap = {};
    /* engineer remote control list */
    this.engineerRCList = [];

    this.queue = [];
    this.firstRunning = true;
    this.joinDuration = 0;

    this.noTypeDeviceList = [];

    this.rspDictionary = {'setAlarmConfig':'alarmConfigRsp',
                            'setGSensorConfig':'gSensorConfigRsp',
                            'setElectricLockConfig':'electricLockConfigRsp',
                            'setSignalConfig':'signalConfigRsp',
                            'setTempConfig':'tempConfigRsp',
                            'setLowBatteryConfig':'lowBatteryConfigRsp',
                            'setDistanceConfig':'distanceConfigRsp',
                            'setMusicConfig':'musicConfigRsp',
                            'setIntervalConfig':'intervalConfigRsp',
                            'setMotionConfig':'motionConfigRsp',
                            'setResistanceConfig':'resistanceConfigRsp',
                            'setMicrowaveConfig':'microwaveConfigRsp',
                            'setPirConfig':'pirConfigRsp',
                            'setSetUnsetConfig':'setUnsetConfigRsp',
                            'sendElectricLockAction':'electricLockActionRsp',
                            'sendDisconnect':'disconnectRsp'};

    /*
     * @brief      This function is called to  attempt to reconnect with
     * 			   the application server
     *
     * @param      none
     *
     * @retun      none
     */
    function appClientReconnect() {
        if (typeof appClientInstance.clientReconnectTimer === 'undefined') {
            /*start a connection timer that tries to reconnect 5s */
            appClientInstance.clientReconnectTimer = setTimeout(function () {
                appClient.destroy();
                appClient.connect(APP_SERVER_PORT, '127.0.0.1', function () {
                    console.log("Connected to App Server");
                    /* Request Network Information, we just
                        reconnected get info again in case something may
                        have changed */
                    appC_getNwkInfoFromAppServer();
                });
                clearTimeout(appClientInstance.clientReconnectTimer);
                delete appClientInstance.clientReconnectTimer;
            }, APP_CLIENT_RECONNECT_TIMEOUT);
        }
    }
    
    /* *****************************************************************
     * Process Incoming Messages from the App Server
     *******************************************************************/
    /*!
     * @brief        This function is called to handle incoming messages
     *				from the app server
     *
     * @param
     *
     * @return
     */
    function appC_processIncoming(data) {
        var dataIdx = 0;
        while (dataIdx < data.length) {
            var rx_pkt_len = data[dataIdx + PKT_HEADER_LEN_FIELD] + (data[dataIdx + PKT_HEADER_LEN_FIELD + 1] << 8) + PKT_HEADER_SIZE;
            var ByteBuffer = require("bytebuffer");
            var rx_pkt_buf = new ByteBuffer(rx_pkt_len, ByteBuffer.LITTLE_ENDIAN);
            rx_pkt_buf.append(data.slice(dataIdx, dataIdx + rx_pkt_len), "hex", 0);
            dataIdx = dataIdx + rx_pkt_len;
            var rx_cmd_id = rx_pkt_buf.readUint8(PKT_HEADER_CMDID_FIELD);
            switch (rx_cmd_id) {
                case timac_pb.appsrv_CmdId.APPSRV_GET_NWK_INFO_CNF:
                    var networkInfoCnf = timac_pb.appsrv_getNwkInfoCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processGetNwkInfoCnf(JSON.stringify(networkInfoCnf));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_GET_DEVICE_ARRAY_CNF:
                    var devArray = timac_pb.appsrv_getDeviceArrayCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processGetDevArrayCnf(JSON.stringify(devArray));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_NWK_INFO_IND:
                    var networkInfoInd = timac_pb.appsrv_nwkInfoUpdateInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processNetworkUpdateIndMsg(JSON.stringify(networkInfoInd));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_DEVICE_JOINED_IND:
                    var newDeviceInfo = timac_pb.appsrv_deviceUpdateInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processDeviceJoinedIndMsg(JSON.stringify(newDeviceInfo));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_DEVICE_NOTACTIVE_UPDATE_IND:
                    var inactiveDevInfo = timac_pb.appsrv_deviceNotActiveUpdateInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processDeviceNotActiveIndMsg(JSON.stringify(inactiveDevInfo));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_DEVICE_DATA_RX_IND:
                    var devData = timac_pb.appsrv_deviceDataRxInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processDeviceDataRxIndMsg(JSON.stringify(devData));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_COLLECTOR_STATE_CNG_IND:
                    var coordState = timac_pb.appsrv_collectorStateCngUpdateInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processStateChangeUpdate(JSON.stringify(coordState));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_SET_JOIN_PERMIT_CNF:
                    var permitJoinCnf = timac_pb.appsrv_setJoinPermitCnf.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processSetJoinPermitCnf(JSON.stringify(permitJoinCnf));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_TX_DATA_CNF:
                    /* Add Applciation specific handling here*/
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_PAIRING_IND:
                    var pairingState = timac_pb.appsrv_pairingUpdateInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processPairingUpdate(JSON.stringify(pairingState));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_ANTENNA_IND:
                    var antennaState = timac_pb.appsrv_antennaUpdateInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processAntennaUpdate(JSON.stringify(antennaState));
                    break;
                case timac_pb.appsrv_CmdId.APPSRV_SET_INTERVAL_IND:
                    var setIntervalData = timac_pb.appsrv_setIntervalUpdateInd.decode(rx_pkt_buf.copy(PKT_HEADER_SIZE, rx_pkt_len).buffer);
                    appC_processSetIntervalUpdate(JSON.stringify(setIntervalData));
                    break;
                default:
                    console.log("ERROR: appClient: CmdId not processed: ", rx_cmd_id);
            }
        }
    }
    
    /*!
     * @brief        This function is called to handle incoming network update
     * 				message from the application
     *
     * @param
     *
     * @return
     */
    function appC_processGetNwkInfoCnf(networkInfo) {
        var nInfo = JSON.parse(networkInfo);
        if (nInfo.status != 1) {
            /* Network not yet started, no nwk info returned
             by app server keep waiting until the server
             informs of the network info via network update indication
             */
            console.log("network not started yet, now waiting for updates");
            return;
        }
        if (typeof self.nwkInfo === "undefined") {
            /* create a new network info element */
            self.nwkInfo = new NwkInfo(nInfo);
        } else {
            /* update the information */
            self.nwkInfo.updateNwkInfo(nInfo);
        }

        if (self.nwkInfo.infoLog) console.log("appC_processGetNwkInfoCnf: ", JSON.stringify(nInfo, null, 2));
        /* send the network information */
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
        /* Get Device array from appServer */
        appC_getDevArrayFromAppServer();
    }
    
    /*!
     * @brief        This function is called to handle incoming device array
     * 				cnf message from the application
     *
     * @param
     *
     * @return
     */
    function appC_processGetDevArrayCnf(deviceArray) {
        /* erase the exsisting infomration we will update
         information with the incoming information */
        self.connectedDeviceList = [];
        var devArray = JSON.parse(deviceArray);
        var i;
        for (i = 0; i < devArray.devInfo.length; i++) {
            var devInfo = devArray.devInfo[i];
            var deviceInfo = devInfo.devInfo;
            var extAddress = deviceInfo.extAddress;
            var newDev = new Device(deviceInfo.shortAddress, deviceInfo.extAddress, devInfo.capInfo);
            /* Add device to the list */
            self.connectedDeviceList.push(newDev);
            appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);
        }
        if (self.nwkInfo.infoLog) console.log("appC_processGetDevArrayCnf: ", JSON.stringify(devArray, null, 2));

        if (self.firstRunning) {
            self.firstRunning = false;

            var data = {"d01": {"trigger_type":"1"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("appClient startup response error:\n" + error);
                    return;
                }
                
                if (self.nwkInfo.infoLog) console.log("appClient startup response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (self.nwkInfo.infoLog) console.log("appClient startup response body:\n" + JSON.stringify(body));
                }
            });
        }
    }
    /*!
     * @brief        This function is called to handle incoming network update
     * 				ind message from the application
     *
     * @param
     *
     * @return
     */
    function appC_processNetworkUpdateIndMsg(networkInfo) {
        var nInfo = JSON.parse(networkInfo);
        if (typeof self.nwkInfo === "undefined") {
            /* create a new network info element */
            self.nwkInfo = new NwkInfo(nInfo);
        } else {
            /* update the information */
            self.nwkInfo.updateNwkInfo(nInfo);
        }

        if (self.nwkInfo.infoLog) console.log("appC_processNetworkUpdateIndMsg: ", JSON.stringify(nInfo, null, 2));
        /* send the netwiork information */
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    }
    
    /*!
     * @brief        This function is called to handle incoming device joined
     * 				ind message informing of new device join from the
     * 				application
     *
     * @param 		deviceInfo - new device information
     *
     * @return       none
     */
    function appC_processDeviceJoinedIndMsg(deviceInfo) {
        var devInfo = JSON.parse(deviceInfo);

        var extAddr = devInfo.devDescriptor.extAddress;
        self.noTypeDeviceList.push(extAddr);
        if (self.nwkInfo.infoLog) console.log("\nnoTypeDeviceList push(" + extAddr +")!");

        /* Check if the device already exists */
        for (var i = 0; i < self.connectedDeviceList.length; i++) {
            /* check if extended address match */
            if (self.connectedDeviceList[i].extAddress == extAddr) {
                self.connectedDeviceList[i].devUpdateInfo(devInfo.devDescriptor.shortAddress, devInfo.devCapInfo);
                /* send update to web client */
                appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);
                return;
            }
        }
        var newDev = new Device(devInfo.devDescriptor.shortAddress, extAddr, devInfo.devCapInfo);
        /* Add device to the list */
        self.connectedDeviceList.push(newDev);

        if (self.nwkInfo.infoLog) console.log("appC_processDeviceJoinedIndMsg: ", JSON.stringify(devInfo, null, 2));
        /* send update to web client */
        appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);
    }
    
    /*!
     * @brief        This function is called to handle incoming message informing that
     * 				 a device is now inactive(?)
     *
     * @param 		inactiveDevInfo - inactive device information
     *
     * @return
     */
    function appC_processDeviceNotActiveIndMsg(inactiveDevInfo) {
        var inactivedeviceInfo = JSON.parse(inactiveDevInfo);
        /* Find the index of the device in the list */
        var deviceIdx = findDeviceIndexShortAddr(inactivedeviceInfo.devDescriptor.shortAddress);
        if (deviceIdx !== -1) {
            var deviceData = self.connectedDeviceList[deviceIdx];
            deviceData.deviceNotActive(inactivedeviceInfo);

            var showLog = false;
            var extAddress = deviceData.extAddress;
            if (extAddress in self.sensorMap) {
                var type = self.sensorMap[extAddress];
                if (type === TYPE_EVE003) {
                    // delete self.sensorMap[extAddress];

                    // if (self.nwkInfo.infoLog) console.log("\neve003(" + extAddress +") disconnected!");

                    // var data = {"d05": {"device_id": extAddress, "trigger_type":"9"}};
                    // request.post(generalPythonUrl("/trigger/event/"), {
                    //     json:data
                    // }, function (error, response, body) {
                    //     if (error) {
                    //         console.log("eve003 disconnected response error:\n" + error);
                    //         return;
                    //     }
              
                    //     if (self.nwkInfo.eve003Log) console.log("eve003 disconnected response statusCode:" + response.statusCode);
            
                    //     if (response.statusCode == 200) {
                    //         if (self.nwkInfo.eve003Log) console.log("eve003 disconnected response body:\n" + JSON.stringify(body));
                    //     }
                    // });
                }else if (type === TYPE_EVE004) {
                    delete self.sensorMap[extAddress];

                    if (self.nwkInfo.infoLog) console.log("\neve004(" + extAddress +") disconnected!");

                    var data = {"d04": {"device_id": extAddress, "trigger_type":"9"}};
                    request.post(generalPythonUrl("/trigger/event/"), {
                        json:data
                    }, function (error, response, body) {
                        if (error) {
                            console.log("eve004 disconnected response error:\n" + error);
                            return;
                        }
              
                        if (self.nwkInfo.eve004Log) console.log("eve004 disconnected response statusCode:" + response.statusCode);
            
                        if (response.statusCode == 200) {
                            if (self.nwkInfo.eve004Log) console.log("eve004 disconnected response body:\n" + JSON.stringify(body));
                        }
                    });
                }else if (type === TYPE_EVE005) {
                    delete self.sensorMap[extAddress];

                    if (self.nwkInfo.infoLog) console.log("\neve005(" + extAddress +") disconnected!");

                    var data = {"d02": {"device_id": extAddress, "trigger_type":"10"}};
                    request.post(generalPythonUrl("/trigger/event/"), {
                        json:data
                    }, function (error, response, body) {
                        if (error) {
                            console.log("eve005 disconnected response error:\n" + error);
                            return;
                        }
              
                        if (self.nwkInfo.eve005Log) console.log("eve005 disconnected response statusCode:" + response.statusCode);
            
                        if (response.statusCode == 200) {
                            if (self.nwkInfo.eve005Log) console.log("eve005 disconnected response body:\n" + JSON.stringify(body));
                        }
                    });
                }else if (type === TYPE_EVE006) {
                    delete self.sensorMap[extAddress];

                    if (self.nwkInfo.infoLog) console.log("\neve006(" + extAddress +") disconnected!");

                    var data = {"d03": {"device_id": extAddress, "trigger_type":"8"}};
                    request.post(generalPythonUrl("/trigger/event/"), {
                        json:data
                    }, function (error, response, body) {
                        if (error) {
                            console.log("eve006 disconnected response error:\n" + error);
                            return;
                        }
              
                        if (self.nwkInfo.eve006Log) console.log("eve006 disconnected response statusCode:" + response.statusCode);
            
                        if (response.statusCode == 200) {
                            if (self.nwkInfo.eve006Log) console.log("eve006 disconnected response body:\n" + JSON.stringify(body));
                        }
                    });
                }else{
                    showLog = true;
                }
            }else{
                showLog = true;
            }

            if (showLog && self.nwkInfo.infoLog) console.log("appC_processDeviceNotActiveIndMsg: ", JSON.stringify(inactivedeviceInfo, null, 2));
            /* send update to web client */
            appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);
        } else {
            console.log("ERROR: rcvd inactive status info for non-existing device");
        }
    }

    function generalPythonUrl(path) {
        return "http://" + PYTHON_IP + ":" + PYTHON_PORT + path;
    }

    function getTempValue(value) {
        var temp = value & 0x7F;
        var symbol = value & 0x80;
        if (symbol > 0) {
            symbol = -1;
        }else{
            symbol = 1;
        }
        var tempValue = temp * symbol;
        return tempValue;
    }

    function getEve003RssiPercentage(value) {
        if (value >= -40) {
            return 90;
        }else if (value >= -72) {
            return 80;
        }else if (value >= -78) {
            return 70;
        }else if (value >= -80) {
            return 60;
        }else if (value >= -86) {
            return 50;
        }else if (value >= -88) {
            return 40;
        }else if (value >= -90) {
            return 30;
        }else if (value >= -93) {
            return 20;
        }else if (value >= -96) {
            return 10;
        }
        return 0;
    }

    function getEve004RssiPercentage(value) {
        if (value >= -40) {
            return 90;
        }else if (value >= -60) {
            return 80;
        }else if (value >= -70) {
            return 70;
        }else if (value >= -81) {
            return 60;
        }else if (value >= -83) {
            return 50;
        }else if (value >= -85) {
            return 40;
        }else if (value >= -87) {
            return 30;
        }else if (value >= -90) {
            return 20;
        }else if (value >= -97) {
            return 10;
        }
        return 0;
    }

    function getEve005RssiPercentage(value) {
        if (value >= -47) {
            return 92;
        }else if (value >= -52) {
            return 84;
        }else if (value >= -65) {
            return 76;
        }else if (value >= -70) {
            return 68;
        }else if (value >= -75) {
            return 60;
        }else if (value >= -82) {
            return 52;
        }else if (value >= -85) {
            return 44;
        }else if (value >= -88) {
            return 36;
        }else if (value >= -92) {
            return 28;
        }else if (value >= -95) {
            return 20;
        }else if (value >= -97) {
            return 12;
        }
        return 0;
    }

    function getEve006RssiPercentage(value) {
        if (value >= -40) {
            return 92;
        }else if (value >= -56) {
            return 84;
        }else if (value >= -73) {
            return 76;
        }else if (value >= -76) {
            return 68;
        }else if (value >= -80) {
            return 60;
        }else if (value >= -83) {
            return 52;
        }else if (value >= -85) {
            return 44;
        }else if (value >= -87) {
            return 36;
        }else if (value >= -90) {
            return 28;
        }else if (value >= -93) {
            return 20;
        }else if (value >= -95) {
            return 12;
        }
        return 0;
    }
    
    function eve003SensorTrigger(frameControl, extAddress, sDataMsg, rssi) {
        var CONSOLE_ENABLE = self.nwkInfo.eve003Log;
        var eve003Sensor = sDataMsg.eve003Sensor;
        
        var eve003Type = eve003Sensor.dip & 0x01;
        var isCustomerMode = !(eve003Type > 0);

        var hasSensor =  extAddress in self.sensorMap;
        if (!hasSensor) {
            self.sensorMap[extAddress] = TYPE_EVE003;

            if (isCustomerMode) {
                if (self.nwkInfo.infoLog) console.log("\ncustomerRC(" + extAddress + ") connected!");

                var data = {"d05": {"device_id": extAddress, "trigger_type": "2", "power_status": eve003Sensor.battery}};
                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve003(" + extAddress +") connected response error:\n" + error);
                        return;
                    }
                    
                    if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") connected response statusCode:" + response.statusCode);
                
                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") connected response body:\n" + JSON.stringify(body));
                    }
                });
            }else{
                var rcIndex = self.engineerRCList.indexOf(extAddress);
                if (rcIndex < 0) {
                    self.engineerRCList.push(extAddress);
                    if (self.nwkInfo.infoLog) console.log("\nengineerRC(" + extAddress +") connected!");
                }
            }
        }
        
        var alarmEnableValue = eve003Sensor.dip & 0x02;
        var alarmEnable = alarmEnableValue > 0;
        
        var button3Type = eve003Sensor.dip & 0x04;
        var isElectricLockMode = button3Type > 0;
        
        if (CONSOLE_ENABLE) {
            console.log("\n");
            console.log("+--------------+--------------+--------------+--------------+--------------+");
            console.log("|                            EVE003(" + extAddress + ")                      |");
            console.log("+--------------+--------------+--------------+--------------+--------------+");
            console.log("|     Mode     |    Alarm     |     Type     |    Battery   |     RSSI     |");
            console.log("+--------------+--------------+--------------+--------------+");
            console.log("|  " + (isCustomerMode ? "   User   " : "Engineering") + "  |    " + (alarmEnable ? "enable " : "disable") + "    |    " + (isElectricLockMode ? " Lock " : "Region") + "       |       " + eve003Sensor.battery + "     |       " + rssi + "      |");
            console.log("+--------------+--------------+--------------+--------------+--------------+");
        }
        
        var buttonState = eve003Sensor.button & 0x08;
        var isButtonLongPress = buttonState > 0;
        if (isButtonLongPress) {
            if (CONSOLE_ENABLE) console.log("Button long press");
        }else{
            if (CONSOLE_ENABLE) console.log("Button short press");
        }
        
        var button1State = eve003Sensor.button & 0x01;
        if (button1State > 0 && isCustomerMode) {
            if (isButtonLongPress) {
                if (CONSOLE_ENABLE) console.log("Unset trigger");

                var data = {"d05": {"device_id": extAddress, "trigger_type": "8", "power_status": eve003Sensor.battery}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve003(" + extAddress +") Unset trigger response error:\n" + error);
                        return;
                    }

                    if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Unset trigger response statusCode:" + response.statusCode);
                    
                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Unset trigger response body:\n" + JSON.stringify(body));
                    }
                });
            }else{
                if (CONSOLE_ENABLE) console.log("Set trigger");

                var data = {"d05": {"device_id": extAddress, "trigger_type": "3", "power_status": eve003Sensor.battery}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve003(" + extAddress +") Set trigger response error:\n" + error);
                        return;
                    }
                         
                    if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Set trigger response statusCode:" + response.statusCode);
                         
                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Set trigger response body:\n" + JSON.stringify(body));
                    }
                });
            }
        }
        
        var button2State = eve003Sensor.button & 0x02;
        if (button2State > 0) {
            if (isCustomerMode) {
                if (isElectricLockMode) {
                    if (CONSOLE_ENABLE) console.log("Electric lock trigger");

                    var data = {"d05": {"device_id": extAddress, "trigger_type": "4", "power_status": eve003Sensor.battery}};

                    request.post(generalPythonUrl("/trigger/event/"), {
                        json:data
                    }, function (error, response, body) {
                        if (error) {
                            console.log("eve003(" + extAddress +") Electric lock trigger response error:\n" + error);
                            return;
                        }
                        
                        if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Electric lock trigger response statusCode:" + response.statusCode);
                    
                        if (response.statusCode == 200) {
                            if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Electric lock trigger response body:\n" + JSON.stringify(body));
                        }
                    });
                }else{
                    if (isButtonLongPress) {
                        if (CONSOLE_ENABLE) console.log("Region unset trigger");

                        var data = {"d05": {"device_id": extAddress, "trigger_type": "10", "power_status": eve003Sensor.battery}};

                        request.post(generalPythonUrl("/trigger/event/"), {
                            json:data
                        }, function (error, response, body) {
                            if (error) {
                                console.log("eve003(" + extAddress +") Region unset trigger response error:\n" + error);
                                return;
                            }
                        
                            if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Region unset trigger response statusCode:" + response.statusCode);
                    
                            if (response.statusCode == 200) {
                                if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Region unset trigger response body:\n" + JSON.stringify(body));
                            }
                        });
                    }else{
                        if (CONSOLE_ENABLE) console.log("Region set trigger");

                        var data = {"d05": {"device_id": extAddress, "trigger_type": "5", "power_status": eve003Sensor.battery}};

                        request.post(generalPythonUrl("/trigger/event/"), {
                            json:data
                        }, function (error, response, body) {
                            if (error) {
                                console.log("eve003(" + extAddress +") Region set trigger response error:\n" + error);
                                return;
                            }
                        
                            if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Region set trigger response statusCode:" + response.statusCode);
                    
                            if (response.statusCode == 200) {
                                if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Region set trigger response body:\n" + JSON.stringify(body));
                            }
                        });
                    }
                }
            }else{
                if (CONSOLE_ENABLE) console.log("Inspection trigger");

                var data = {"d05": {"device_id": extAddress, "trigger_type": "7", "power_status": eve003Sensor.battery}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve003(" + extAddress +") Inspection trigger response error:\n" + error);
                        return;
                    }
                    
                    if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Inspection trigger response statusCode:" + response.statusCode);
                    
                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Inspection trigger response body:\n" + JSON.stringify(body));
                    }
                });
            }
        }
        
        var button3State = eve003Sensor.button & 0x04;
        if (button3State > 0) {
            if (isCustomerMode) {
                // if (isButtonLongPress) {

                // }else{

                // }
                if (CONSOLE_ENABLE) console.log("Emergency trigger");

                var data = {"d05": {"device_id": extAddress, "trigger_type": "6", "power_status": eve003Sensor.battery}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve003(" + extAddress +") Emergency trigger response error:\n" + error);
                        return;
                    }
                    
                    if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Emergency trigger response statusCode:" + response.statusCode);
                    
                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Emergency trigger response body:\n" + JSON.stringify(body));
                    }
                });
            }
        }
        
        var resetButton = eve003Sensor.setting & 0x01;
        if (resetButton > 0) {
            if (CONSOLE_ENABLE) console.log("Reset trigger");

            var data = {"d05": {"device_id": extAddress, "trigger_type": "1", "power_status": eve003Sensor.battery}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve003(" + extAddress +") Reset trigger response error:\n" + error);
                    return;
                }
                
                if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Reset trigger response statusCode:" + response.statusCode);
                
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") Reset trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var pairingButton = eve003Sensor.setting & 0x02;
        if (pairingButton > 0) {
            if (CONSOLE_ENABLE) console.log("pairing trigger");
        }

        var cleanButton = eve003Sensor.setting & 0x04;
        if (cleanButton > 0) {
            if (CONSOLE_ENABLE) console.log("Clean trigger");
            appC_sendRemoveDeviceMsgToAppServer({device_id: extAddress});
            
            if (self.nwkInfo.infoLog) console.log("\neve003(" + extAddress +") disconnected!");

            var data = {"d05": {"device_id": extAddress, "trigger_type": "9", "power_status": eve003Sensor.battery}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve003(" + extAddress +") disconnected response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") disconnected response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve003(" + extAddress +") disconnected response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        if (CONSOLE_ENABLE) console.log("+--------------+--------------+--------------+--------------+");
    }
    
    function eve004SensorTrigger(frameControl, extAddress, sDataMsg, rssi) {
        var CONSOLE_ENABLE = self.nwkInfo.eve004Log;
        var eve004Sensor = sDataMsg.eve004Sensor;

        var dipId = (eve004Sensor.dip & 0x70) >> 4;

        var hasSensor =  extAddress in self.sensorMap;
        if (!hasSensor) {
            self.sensorMap[extAddress] = TYPE_EVE004;

            if (self.nwkInfo.infoLog) console.log("\neve004(" + extAddress +") connected!");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"2", "identity": dipId, "extend_out": "0"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") connected response error:\n" + error);
                    return;
                }
                
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") connected response statusCode:" + response.statusCode);
                
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") connected response body:\n" + JSON.stringify(body));
                }
            });
        }

        var tempValue = getTempValue(eve004Sensor.tempValue);

        var dip12VOutputEnableValue = eve004Sensor.dip & 0x01;
        var dip12VOutputEnable = dip12VOutputEnableValue > 0;
        
        var dipElectricLockEnableValue = eve004Sensor.dip & 0x02;
        var dipElectricLockEnable = dipElectricLockEnableValue > 0;
        
        if (CONSOLE_ENABLE) {
            console.log("\n");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|                                    EVE004(" + extAddress + ")                             |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|     Temp     |   DIP 12V    |   DIP Lock   |    DIP ID    |    Battery   |     RSSI     |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|      " + tempValue + "      |     " + dip12VOutputEnable + " " + (dip12VOutputEnable ? " " : "") + "   |     " + dipElectricLockEnable + "  " + (dipElectricLockEnable ? " " : "") + "  |       " + dipId +"      |     " + eve004Sensor.battery + "      |     " + rssi + "     |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+");
        }
        
        var gSensorState = eve004Sensor.state & 0x01;
        if (gSensorState > 0) {
            if (CONSOLE_ENABLE) console.log("G-Sensor trigger");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"7"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") G-Sensor trigger response error:\n" + error);
                    return;
                }
                
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") G-Sensor trigger response statusCode:" + response.statusCode);
                
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") G-Sensor trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var checkButton = eve004Sensor.state & 0x02;
        if (checkButton > 0) {
            if (CONSOLE_ENABLE) console.log("Check trigger");

            var durationSec = 1 * 60;
            appC_setJoinPermitAtAppServer({ open: true, duration: durationSec});

            var data = {"d04": {"device_id": extAddress, "trigger_type":"8"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") Check trigger response error:\n" + error);
                    return;
                }
                
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Check trigger response statusCode:" + response.statusCode);
                         
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Check trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var resetButton = eve004Sensor.state & 0x04;
        if (resetButton > 0) {
            if (CONSOLE_ENABLE) console.log("Reset trigger");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"1"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") Reset trigger response error:\n" + error);
                    return;
                }
                
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Reset trigger response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Reset trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var pairingButton = eve004Sensor.state & 0x08;
        if (pairingButton > 0) {
            if (CONSOLE_ENABLE) console.log("Pairing trigger");
        }
        
        var relay1Button = eve004Sensor.state & 0x10;
        if (relay1Button > 0) {
            if (CONSOLE_ENABLE) console.log("Relay1 trigger");
        }
        
        var relay2Button = eve004Sensor.state & 0x20;
        if (relay2Button > 0) {
            if (CONSOLE_ENABLE) console.log("Relay2 trigger");
        }

        var cleanButton = eve004Sensor.state & 0x40;
        if (cleanButton > 0) {
            if (CONSOLE_ENABLE) console.log("Clean trigger");
            appC_sendRemoveDeviceMsgToAppServer({device_id: extAddress});
            
            if (self.nwkInfo.infoLog) console.log("\neve004(" + extAddress +") disconnected!");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"9"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") disconnected response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") disconnected response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") disconnected response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var tempAlarmState = eve004Sensor.alarm & 0x01;
        if (tempAlarmState > 0) {
            if (CONSOLE_ENABLE) console.log("Temp alarm");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"3", "temperature_status": tempValue}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") Temp alarm response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Temp alarm response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Temp alarm response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var tempAbnormalState = eve004Sensor.alarm & 0x02;
        if (tempAbnormalState > 0) {
            if (CONSOLE_ENABLE) console.log("Temp abnormal");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"4", "temperature_status": tempValue}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") Temp abnormal response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Temp abnormal response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Temp abnormal response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var batteryAlarmState = eve004Sensor.alarm & 0x04;
        if (batteryAlarmState > 0) {
            if (CONSOLE_ENABLE) console.log("Battery alarm");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"5", "power_status": eve004Sensor.battery}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") Battery alarm response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Battery alarm response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Battery alarm response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var batteryAbnormalState = eve004Sensor.alarm & 0x08;
        if (batteryAbnormalState > 0) {
            if (CONSOLE_ENABLE) console.log("Battery abnormal");

            var data = {"d04": {"device_id": extAddress, "trigger_type":"6", "power_status": eve004Sensor.battery}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve004(" + extAddress +") Battery abnormal response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Battery abnormal response statusCode:\n" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Battery abnormal response body:" + JSON.stringify(body));
                }
            });
        }

        var data = {"d04": {"device_id": extAddress, "trigger_type":"10", "identity": dipId, "temperature_status": tempValue, "power_status": eve004Sensor.battery, "signal_power_status": getEve004RssiPercentage(rssi), "extend_out": "0"}};
        request.post(generalPythonUrl("/trigger/event/"), {
            json:data
        }, function (error, response, body) {
            if (error) {
                console.log("eve004(" + extAddress +") Status update response error:\n" + error);
                return;
            }
            
            if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Status update response statusCode:" + response.statusCode);
            
            if (response.statusCode == 200) {
                if (CONSOLE_ENABLE) console.log("eve004(" + extAddress +") Status update response body:\n" + JSON.stringify(body));
            }
        });
        
        if (CONSOLE_ENABLE) console.log("+--------------+--------------+--------------+--------------+--------------+--------------+");
    }
    
    function eve005SensorTrigger(frameControl, extAddress, sDataMsg, rssi) {
        var CONSOLE_ENABLE = self.nwkInfo.eve005Log;
        var eve005Sensor = sDataMsg.eve005Sensor;
        
        var v12EnableValue = eve005Sensor.dip_control & 0x04;
        var v12Enable = v12EnableValue > 0;

        var hasSensor =  extAddress in self.sensorMap;
        if (!hasSensor) {
            self.sensorMap[extAddress] = TYPE_EVE005;

            if (self.nwkInfo.infoLog) console.log("\neve005(" + extAddress +") connected!");

            var data = {"d02": {"device_id": extAddress, "trigger_type": "2", "identity": eve005Sensor.dip_id, "extend_out": (v12Enable ? "1" : "0"), "extend_flag": eve005Sensor.dip_control & 0x03}};
            
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") connected response error:\n" + error);
                    return;
                }
                
                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") connected response statusCode:" + response.statusCode);
                
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") connected response body:\n" + JSON.stringify(body));
                }
            });
        }

        var tempValue = getTempValue(eve005Sensor.tmpValue);
        
        var z1EnableValue = eve005Sensor.dip_control & 0x01;
        var z1Enable = z1EnableValue > 0;
        var z2EnableValue = eve005Sensor.dip_control & 0x02;
        var z2Enable = z2EnableValue > 0;
        //var reedEnableValue = eve005Sensor.dip_control & 0x08;
        var reedEnable = true;

        if (CONSOLE_ENABLE) {
            console.log("\n");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|                                                              EVE005(" + extAddress + ")                                                               |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|     Temp     |    Dip id    |      Z1      |      Z2      |      12V     |      Reed    |    Battery   |     RSSI     |    Resist1   |    Resist2   |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|     " + tempValue + "    |    " + eve005Sensor.dip_id + "    |    " + (z1Enable ? "enable " : "disable") + "    |    " + (z2Enable ? "enable " : "disable") + "    |    " + (v12Enable ? "enable " : "disable") + "    |    " + (reedEnable ? "enable " : "disable") + "    |       " + eve005Sensor.batValue + "      |        " + rssi + "        |       " + (eve005Sensor.resist1 / 10) + "       |      " + (eve005Sensor.resist2 / 10) + "    |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
        }
        
        var resetButton = eve005Sensor.button & 0x01;
        if (resetButton > 0) {
            if (CONSOLE_ENABLE) console.log("Reset trigger");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"1"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") Reset trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Reset trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Reset trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var pairingButton = eve005Sensor.button & 0x02;
        if (pairingButton > 0) {
            if (CONSOLE_ENABLE) console.log("Pairing trigger");
        }

        var cleanButton = eve005Sensor.button & 0x04;
        if (cleanButton > 0) {
            if (CONSOLE_ENABLE) console.log("Clean trigger");

            appC_sendRemoveDeviceMsgToAppServer({device_id: extAddress});
            
            if (self.nwkInfo.infoLog) console.log("\neve005(" + extAddress +") disconnected!");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"10"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") disconnected response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") disconnected response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") disconnected response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        if (reedEnable) {
            var reedState = eve005Sensor.sensor_s & 0x01;
            if (reedState > 0) {
                if (CONSOLE_ENABLE) console.log("Reed open");

                var data = {"d02": {"device_id": extAddress, "trigger_type":"8"}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve005(" + extAddress +") Reed open response error:\n" + error);
                        return;
                    }
                
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Reed open response statusCode:" + response.statusCode);
                
                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Reed open response body:\n" + JSON.stringify(body));
                    }
                });
            }else{
                if (CONSOLE_ENABLE) console.log("Reed close");

                var data = {"d02": {"device_id": extAddress, "trigger_type":"9"}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve005(" + extAddress +") Reed close response error:\n" + error);
                        return;
                    }

                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Reed close response statusCode:" + response.statusCode);

                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Reed close response body:\n" + JSON.stringify(body));
                    }
                });
            }
        }
        
        var z1k2State = eve005Sensor.sensor_s & 0x02;
        if (z1k2State > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k State1 trigger");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"12", "impedance_status": eve005Sensor.resist1 / 10}};
            if (CONSOLE_ENABLE) console.log("\n2.2k State1 trigger:" + JSON.stringify(data));
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") 2.2k State1 trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k State1 trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k State1 trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var z2k2State = eve005Sensor.sensor_s & 0x04;
        if (z2k2State > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k State2 trigger");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"13", "impedance_status": eve005Sensor.resist2 / 10}};
            if (CONSOLE_ENABLE) console.log("\n2.2k State2 trigger:" + JSON.stringify(data));
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") 2.2k State2 trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k State2 trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k State2 trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var z1State = eve005Sensor.sensor_s & 0x08;
        if (z1State > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k Loop1 trigger");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"14", "extend_status": "1"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") 2.2k Loop1 trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop1 trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop1 trigger response body:\n" + JSON.stringify(body));
                }
            });
        }else{
            if (CONSOLE_ENABLE) console.log("2.2k Loop1 reset");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"14", "extend_status": "0"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") 2.2k Loop1 reset response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop1 reset response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop1 reset response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var z2State = eve005Sensor.sensor_s & 0x10;
        if (z2State > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k Loop2 trigger");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"15", "extend_status": "1"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") 2.2k Loop2 trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop2 trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop2 trigger response body:\n" + JSON.stringify(body));
                }
            });
        }else {
            if (CONSOLE_ENABLE) console.log("2.2k Loop2 reset");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"15", "extend_status": "0"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") 2.2k Loop2 reset response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop2 reset response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") 2.2k Loop2 reset response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var gSensorState = eve005Sensor.sensor_s & 0x20;
        if (gSensorState > 0) {
            if (CONSOLE_ENABLE) console.log("G-Sensor trigger");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"7"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") G-Sensor trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") G-Sensor trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") G-Sensor trigger response body:\n" + JSON.stringify(body));
                }
            });
        }

        var tempAlarmState = eve005Sensor.alarm & 0x01;
        if (tempAlarmState > 0) {
            if (CONSOLE_ENABLE) console.log("Temp alarm");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"3", "temperature_status": tempValue}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") Temp alarm response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Temp alarm response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Temp alarm response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var tempAbnormalState = eve005Sensor.alarm & 0x02;
        if (tempAbnormalState > 0) {
            if (CONSOLE_ENABLE) console.log("Temp abnormal");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"4", "temperature_status": tempValue}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") Temp abnormal response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Temp abnormal response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Temp abnormal response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var batteryAlarmState = eve005Sensor.alarm & 0x04;
        if (batteryAlarmState > 0) {
            if (CONSOLE_ENABLE) console.log("Battery alarm");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"5", "power_status": eve005Sensor.batValue}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") Battery alarm response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Battery alarm response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Battery alarm response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var batteryAbnormalState = eve005Sensor.alarm & 0x08;
        if (batteryAbnormalState > 0) {
            if (CONSOLE_ENABLE) console.log("Battery abnormal");

            var data = {"d02": {"device_id": extAddress, "trigger_type":"6", "power_status": eve005Sensor.batValue}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve005(" + extAddress +") Battery abnormal response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Battery abnormal response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Battery abnormal response body:\n" + JSON.stringify(body));
                }
            });
        }

        var data = {"d02": {"device_id": extAddress, "trigger_type":"11", "identity": eve005Sensor.dip_id, "temperature_status": tempValue, "power_status": eve005Sensor.batValue, "signal_power_status": getEve005RssiPercentage(rssi), "extend_out": (v12Enable ? "1" : "0"), "extend_flag": eve005Sensor.dip_control & 0x03}};

        request.post(generalPythonUrl("/trigger/event/"), {
            json:data
        }, function (error, response, body) {
            if (error) {
                console.log("eve005(" + extAddress +") Status update response error:\n" + error);
                return;
            }

            if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Status update response statusCode:" + response.statusCode);

            if (response.statusCode == 200) {
                if (CONSOLE_ENABLE) console.log("eve005(" + extAddress +") Status update response body:\n" + JSON.stringify(body));
            }
        });
        
        if (CONSOLE_ENABLE) console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
    }

    function eve006SensorTrigger(frameControl, extAddress, sDataMsg, rssi) {
        var CONSOLE_ENABLE = self.nwkInfo.eve006Log;
        var eve006Sensor = sDataMsg.eve006Sensor;

        var v12EnableValue = eve006Sensor.dip & 0x04;
        var v12Enable = v12EnableValue > 0;

        var hasSensor =  extAddress in self.sensorMap;
        if (!hasSensor) {
            self.sensorMap[extAddress] = TYPE_EVE006;

            if (self.nwkInfo.infoLog) console.log("\neve006(" + extAddress +") connected!");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"2", "identity": eve006Sensor.dip_id, "extend_out": (v12Enable ? "1" : "0"), "extend_flag": eve006Sensor.dip & 0x03}};
            
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") connected response error:\n" + error);
                    return;
                }
                  
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") connected response statusCode:" + response.statusCode);
                
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") connected response body:\n" + JSON.stringify(body));
                }
            });
        }

        var tempValue = getTempValue(eve006Sensor.temp);
        var ioEnableValue1 = eve006Sensor.dip & 0x01;
        var ioEnable1 = ioEnableValue1 > 0;

        var ioEnableValue2 = eve006Sensor.dip & 0x02;
        var ioEnable2 = ioEnableValue2 > 0;

        if (CONSOLE_ENABLE) {
            console.log("\n");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|                                                       EVE006(" + extAddress + ")                                                       |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|     Temp     |    Dip id    |   humidity   |    battery   |      IO1     |      IO2     |      RSSI    |    Resist1   |    Resist2   |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
            console.log("|      " + tempValue + "      |      " + eve006Sensor.dip_id + "      |     " + eve006Sensor.humidity + "     |    " + eve006Sensor.battery + "    |    " + (ioEnable1 ? "enable " : "disable") + "    |    " + (ioEnable2 ? "enable " : "disable") + "    |        " + rssi + "       |       " + (eve006Sensor.resist1 / 10) + "       |       " + (eve006Sensor.resist2 / 10) + "      |");
            console.log("+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+--------------+");
        }

        var k22IoState1 = eve006Sensor.state & 0x04;
        if (k22IoState1 > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k State1 trigger");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"12", "impedance_status": eve006Sensor.resist1 / 10}};
            if (CONSOLE_ENABLE) console.log("\n2.2k State1 trigger:" + JSON.stringify(data));
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") 2.2k State1 trigger response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k State1 trigger response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k State1 trigger response body\n" + JSON.stringify(body));
                }
            });
        }

        var k22IoLoop1 = eve006Sensor.state & 0x08;
        if (k22IoLoop1 > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k Loop1 trigger");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"14", "extend_status": "1"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") 2.2k Loop1 trigger response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop1 trigger response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop1 trigger response body\n" + JSON.stringify(body));
                }
            });
        }else{
            if (CONSOLE_ENABLE) console.log("2.2k Loop1 reset");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"14", "extend_status": "0"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") 2.2k Loop1 reset response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop1 reset response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop1 reset response body\n" + JSON.stringify(body));
                }
            });
        }

        var k22IoState2 = eve006Sensor.state & 0x10;
        if (k22IoState2 > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k State2 trigger");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"13", "impedance_status": eve006Sensor.resist2 / 10}};
            if (CONSOLE_ENABLE) console.log("\n2.2k State2 trigger:" + JSON.stringify(data));
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") 2.2k State2 trigger response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k State2 trigger response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k State2 trigger response body\n" + JSON.stringify(body));
                }
            });
        }

        var k22IoLoop2 = eve006Sensor.state & 0x20;
        if (k22IoLoop2 > 0) {
            if (CONSOLE_ENABLE) console.log("2.2k Loop2 trigger");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"15", "extend_status": "1"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") 2.2k Loop2 trigger response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop2 trigger response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop2 trigger response body\n" + JSON.stringify(body));
                }
            });
        }else{
            if (CONSOLE_ENABLE) console.log("2.2k Loop2 reset");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"15", "extend_status": "0"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") 2.2k Loop2 reset response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop2 reset response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") 2.2k Loop2 reset response body\n" + JSON.stringify(body));
                }
            });
        }

        var resetButton = eve006Sensor.button & 0x01;
        if (resetButton > 0) {
            if (CONSOLE_ENABLE) console.log("Reset trigger");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"1"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") Reset trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Reset trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Reset trigger response body:\n" + JSON.stringify(body));
                }
            });
        }
        
        var pairingButton = eve006Sensor.button & 0x02;
        if (pairingButton > 0) {
            if (CONSOLE_ENABLE) console.log("Pairing trigger");
        }

        var cleanButton = eve006Sensor.button & 0x04;
        if (cleanButton > 0) {
            if (CONSOLE_ENABLE) console.log("Clean trigger");
            appC_sendRemoveDeviceMsgToAppServer({device_id: extAddress});

            if (self.nwkInfo.infoLog) console.log("\neve006(" + extAddress +") disconnected!");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"8"}};
            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") Clean trigger response error:\n" + error);
                    return;
                }
              
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Clean trigger response statusCode:" + response.statusCode);
            
                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Clean trigger response body\n" + JSON.stringify(body));
                }
            });
        }

        var gSensorState = eve006Sensor.abnormal & 0x01;
        if (gSensorState > 0) {
            if (CONSOLE_ENABLE) console.log("G-Sensor trigger");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"7"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") G-Sensor trigger response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") G-Sensor trigger response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") G-Sensor trigger response body:\n" + JSON.stringify(body));
                }
            });
        }

        var tempAlarmState = eve006Sensor.abnormal & 0x02;
        if (tempAlarmState > 0) {
            if (CONSOLE_ENABLE) console.log("Temp Alarm");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"3", "temperature_status": tempValue}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") Temp Alarm response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Temp Alarm response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Temp Alarm response body:\n" + JSON.stringify(body));
                }
            });
        }

        var tempAbnormalState = eve006Sensor.abnormal & 0x04;
        if (tempAbnormalState > 0) {
            if (CONSOLE_ENABLE) console.log("Temp abnormal");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"4", "temperature_status": tempValue}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") Temp abnormal response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Temp abnormal response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Temp abnormal response body:\n" + JSON.stringify(body));
                }
            });
        }

        var batteryAlarmState = eve006Sensor.abnormal & 0x08;
        if (batteryAlarmState > 0) {
            if (CONSOLE_ENABLE) console.log("Battery Alarm");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"5", "power_status": eve006Sensor.battery}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") Battery Alarm response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Battery Alarm response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Battery Alarm response body:\n" + JSON.stringify(body));
                }
            });
        }

        var batteryAbnormalState = eve006Sensor.abnormal & 0x10;
        if (batteryAbnormalState > 0) {
            if (CONSOLE_ENABLE) console.log("Battery abnormal");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"6", "power_status": eve006Sensor.battery}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") Battery abnormal response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Battery abnormal response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Battery abnormal response body:\n" + JSON.stringify(body));
                }
            });
        }

        var pirMwAbnormalStateValue = eve006Sensor.abnormal & 0x20;
        var pirMwAbnormalState = pirMwAbnormalStateValue > 0;
        var pirState = eve006Sensor.state & 0x01;
        var mircrowaveState = eve006Sensor.state & 0x02;

        if (mircrowaveState > 0) {
            if (CONSOLE_ENABLE) console.log("Mircrowave trigger");
        }

        if (pirState > 0) {
            if (CONSOLE_ENABLE) console.log("Pir trigger");
        }

        if (pirMwAbnormalState) {
            if (mircrowaveState > 0) {
                if (CONSOLE_ENABLE) console.log("Mircrowave abnormal");

                var data = {"d03": {"device_id": extAddress, "trigger_type":"9"}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("Mircrowave abnormal response error:" + error);
                        return;
                    }

                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Mircrowave abnormal response statusCode:" + response.statusCode);

                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Mircrowave abnormal response body:\n" + JSON.stringify(body));
                    }
                });
            }else if (pirState > 0) {
                if (CONSOLE_ENABLE) console.log("Pir abnormal");

                var data = {"d03": {"device_id": extAddress, "trigger_type":"10"}};

                request.post(generalPythonUrl("/trigger/event/"), {
                    json:data
                }, function (error, response, body) {
                    if (error) {
                        console.log("eve006(" + extAddress +") Pir abnormal response error:\n" + error);
                        return;
                    }

                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Pir abnormal response statusCode:" + response.statusCode);

                    if (response.statusCode == 200) {
                        if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Pir abnormal response body:\n" + JSON.stringify(body));
                    }
                });
            }
        }else{
            if (CONSOLE_ENABLE) console.log("Sensor reset");

            var data = {"d03": {"device_id": extAddress, "trigger_type":"16"}};

            request.post(generalPythonUrl("/trigger/event/"), {
                json:data
            }, function (error, response, body) {
                if (error) {
                    console.log("eve006(" + extAddress +") Sensor reset response error:\n" + error);
                    return;
                }

                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Sensor reset response statusCode:" + response.statusCode);

                if (response.statusCode == 200) {
                    if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Sensor reset response body:\n" + JSON.stringify(body));
                }
            });
        }

        var data = {"d03": {"device_id": extAddress, "trigger_type":"11", "identity": eve006Sensor.dip_id, "power_status": eve006Sensor.battery, "temperature_status": tempValue, "signal_power_status": getEve006RssiPercentage(rssi), "extend_out": (v12Enable ? "1" : "0"), "extend_flag": eve006Sensor.dip & 0x03}};

        request.post(generalPythonUrl("/trigger/event/"), {
            json:data
        }, function (error, response, body) {
            if (error) {
                console.log("eve006(" + extAddress +") Pir response error:\n" + error);
                return;
            }

            if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Pir response statusCode:" + response.statusCode);

            if (response.statusCode == 200) {
                if (CONSOLE_ENABLE) console.log("eve006(" + extAddress +") Pir response body:\n" + JSON.stringify(body));
            }
        });

        if (CONSOLE_ENABLE) console.log("+--------------+--------------+--------------+--------------+--------------+--------------+");
    }
    
    /*!
     * @brief        This function is called to handle incoming message informing of
     * 				reception of sensor data message/device config resp
     *				from a network device
     *
     * @param 		devData - incoming message
     *
     * @return       none
     */
    function appC_processDeviceDataRxIndMsg(devData) {
        var deviceIdx = -1;
        var deviceData = JSON.parse(devData);
        // console.log("=====appC_processDeviceDataRxIndMsg:" + JSON.stringify(deviceData, null, 2));
        /* Find the index of the device in the list */
        if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_short) {
            deviceIdx = findDeviceIndexShortAddr(deviceData.srcAddr.shortAddr);
        } else if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_extended) {
            deviceIdx = findDeviceIndexExtAddr(deviceData.srcAddr.extAddress.data);
        } else {
            console.log("ERROR: illegal addr mode value rcvd in sensor data msg", deviceData.srcAddr.addrMode);
            return;
        }
        
        if (deviceIdx !== -1) {
            var device = self.connectedDeviceList[deviceIdx];
            var extAddress = device.extAddress;

            if (deviceData.sDataMsg) {
                device.rxSensorData(deviceData);
                /* send the update to web client */
                var sDataMsg = deviceData.sDataMsg;
                var frameControl = sDataMsg.frameControl;
                var rssi = deviceData.rssi;
                
                if (frameControl & timac_pb.Smsgs_dataFields.Smsgs_dataFields_eve003Sensor) {
                    eve003SensorTrigger(frameControl, extAddress, sDataMsg, rssi);
                }else if (frameControl & timac_pb.Smsgs_dataFields.Smsgs_dataFields_eve004Sensor) {
                    eve004SensorTrigger(frameControl, extAddress, sDataMsg, rssi);
                }else if (frameControl & timac_pb.Smsgs_dataFields.Smsgs_dataFields_eve005Sensor) {
                    eve005SensorTrigger(frameControl, extAddress, sDataMsg, rssi);
                }else if (frameControl & timac_pb.Smsgs_dataFields.Smsgs_dataFields_eve006Sensor) {
                    eve006SensorTrigger(frameControl, extAddress, sDataMsg, rssi);
                }else {
                    return;
                }

                var index = self.noTypeDeviceList.indexOf(extAddress);
                if (index >= 0) {
                    self.noTypeDeviceList.splice(index, 1);
                    if (self.nwkInfo.infoLog) console.log("\nnoTypeDeviceList remove(" + extAddress +")!");
                }

                appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);
            }
            else if (deviceData.sConfigMsg)
            {
                self.connectedDeviceList[deviceIdx].rxConfigRspInd(deviceData);

                //if (self.nwkInfo.infoLog) console.log("appC_processDeviceDataRxIndMsg: ", JSON.stringify(deviceData, null, 2));
                /* send the update to web client */
                appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);
            }
            else if (deviceData.sAlarmConfigMsg)
            {
                var msg = deviceData.sAlarmConfigMsg;
                var data = {device_id: extAddress, time: msg.time};
                processResponse('alarmConfigRsp', data);
            }
            else if (deviceData.sGSensorConfigMsg)
            {
                var msg = deviceData.sGSensorConfigMsg;
                var data = {device_id: extAddress, enable: msg.enable, sensitivity: msg.sensitivity};
                processResponse('gSensorConfigRsp', data);
            }
            else if (deviceData.sElectricLockConfigMsg)
            {
                var msg = deviceData.sElectricLockConfigMsg;
                var data = {device_id: extAddress, enable: msg.enable, time: msg.time};
                processResponse('electricLockConfigRsp', data);
            }
            else if (deviceData.sSignalConfigMsg)
            {
                var msg = deviceData.sSignalConfigMsg;
                var data = {device_id: extAddress, mode: msg.mode, value: msg.value, offset: msg.offset};
                processResponse('signalConfigRsp', data);
            }
            else if (deviceData.sTempConfigMsg)
            {
                var msg = deviceData.sTempConfigMsg;
                var data = {device_id: extAddress, value: msg.value, offset: msg.offset};
                processResponse('tempConfigRsp', data);
            }
            else if (deviceData.sLowBatteryConfigMsg)
            {
                var msg = deviceData.sLowBatteryConfigMsg;
                var data = {device_id: extAddress, value: msg.value, offset: msg.offset};
                processResponse('lowBatteryConfigRsp', data);
            }
            else if (deviceData.sDistanceConfigMsg)
            {
                var msg = deviceData.sDistanceConfigMsg;
                var data = {device_id: extAddress, mode: msg.mode, distance: msg.distance};
                processResponse('distanceConfigRsp', data);
            }
            else if (deviceData.sMusicConfigMsg)
            {
                var msg = deviceData.sMusicConfigMsg;
                var data;
                if ((msg.mode & 0x10) > 0) {
                    var enable = (msg.mode & 0x20) > 0 ? 1 : 0;
                    data = {device_id: extAddress, enable: enable};
                }else{
                    var mode = msg.mode & 0x0F;
                    data = {device_id: extAddress, mode: mode, time: msg.time};
                }
                processResponse('musicConfigRsp', data);
            }
            else if (deviceData.sIntervalConfigMsg)
            {
                var msg = deviceData.sIntervalConfigMsg;
                var data = {device_id: extAddress, mode: msg.mode, time: msg.time};
                processResponse('intervalConfigRsp', data);
            }
            else if (deviceData.sMotionConfigMsg)
            {
                var msg = deviceData.sMotionConfigMsg;
                var data = {device_id: extAddress, count: msg.count, time: msg.time};
                processResponse('motionConfigRsp', data);
            }
            else if (deviceData.sResistanceConfigMsg)
            {
                var msg = deviceData.sResistanceConfigMsg;
                var data = {device_id: extAddress, value: msg.value};
                processResponse('resistanceConfigRsp', data);
            }
            else if (deviceData.sMicrowaveConfigMsg)
            {
                var msg = deviceData.sMicrowaveConfigMsg;
                var data = {device_id: extAddress, enable: msg.enable, sensitivity: msg.sensitivity};
                processResponse('microwaveConfigRsp', data);
            }
            else if (deviceData.sPirConfigMsg)
            {
                var msg = deviceData.sPirConfigMsg;
                var data = {device_id: extAddress, enable: msg.enable};
                processResponse('pirConfigRsp', data);
            }
            else if (deviceData.sSetUnsetConfigMsg)
            {
                var msg = deviceData.sSetUnsetConfigMsg;
                var data = {device_id: extAddress, state: msg.state};
                processResponse('setUnsetConfigRsp', data);
            }
            else if (deviceData.sElectricLockActionMsg)
            {
                var msg = deviceData.sElectricLockActionMsg;
                var data = {device_id: extAddress, relay: msg.relay};
                processResponse('electricLockActionRsp', data);
            }
            else if (deviceData.sDisconnectMsg)
            {
                var msg = deviceData.sDisconnectMsg;
                var data = {device_id: extAddress, time: msg.time};
                processResponse('disconnectRsp', data);
            }
            else
            {
                // appClientInstance.emit('getdevArrayRsp', self.connectedDeviceList);
                if (self.nwkInfo.infoLog) console.log("Developers can write handlers for new message types ")
                if (self.nwkInfo.infoLog) console.log("appC_processDeviceDataRxIndMsg: ", JSON.stringify(deviceData, null, 2)); 
            }
            return;
        }
        else {
            if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_short) {
                console.log("ERROR: rcvd sensor data message for non-existing device(short:" + deviceData.srcAddr.shortAddr + ")");
            } else if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_extended) {
                console.log("ERROR: rcvd sensor data message for non-existing device(ext:" + deviceData.srcAddr.extAddress.data + ")");
            }else{
                console.log("ERROR: rcvd sensor data message for non-existing device!!!");
            }
        }
    }
    
    /*!
     * @brief        This function is called to handle incoming message informing change
     * 				in the state of the PAN-Coordiantor
     *
     * @param 		coordState - updated coordinator state
     *
     * @return       none
     */
    function appC_processStateChangeUpdate(coordState) {
        var state = JSON.parse(coordState);
        /* update state */
        self.nwkInfo.updateNwkState(state);

        if (self.nwkInfo.state == "close") {
            var arr = self.engineerRCList.concat(self.noTypeDeviceList);
            var length = arr.length;
            if (length > 0) {
                var extAddress = null;
                for (var i = 0; i < length; i++) {
                    extAddress = arr[i];
                    appC_sendDisconnectMsgToAppServer({device_id: extAddress, time:0});
                    appC_sendRemoveDeviceMsgToAppServer({device_id: extAddress});
                    if (self.nwkInfo.infoLog) console.log("\nRemove device(" + extAddress +")!");
                }
                self.engineerRCList = [];
                self.noTypeDeviceList = [];
            }
        }

        if (self.nwkInfo.infoLog) console.log("appC_processStateChangeUpdate: ", JSON.stringify(state, null, 2));
        /* send info to web client */
        if (self.nwkInfo.state == 'open') {
            appClientInstance.emit('joinPermitRsp', { open: true, duration: self.joinDuration});
        }else if (self.nwkInfo.state == 'started' || self.nwkInfo.state == 'close') {
            appClientInstance.emit('joinPermitRsp', { open: false, duration: self.joinDuration});
        }
        
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    }
    
    /*!
     * @brief        This function is called to handle incoming confirm for
     *				setjoinpermitreq
     *
     * @param 		permitJoinCnf - status of permit join req
     *
     * @return       none
     */
    function appC_processSetJoinPermitCnf(permitJoinCnf) {
        var cnf = JSON.parse(permitJoinCnf);
        // if (self.nwkInfo.infoLog) console.log("appC_processSetJoinPermitCnf: ", JSON.stringify(cnf, null, 2));
        appClientInstance.emit('permitJoinCnf', { status: cnf.status });
    }
    
    /************************************************************************
     * Device list utility functions
     * *********************************************************************/
    /*!
     * @brief        Find index of device in the list based on short address
     *
     * @param 		srcAddr - short address of the device
     *
     * @return      index of the device in the connected device list, if present
     *			   -1, if not present
     *
     */
    function findDeviceIndexShortAddr(srcAddr) {
        /* find the device in the connected device list and update info */
        for (var i = 0; i < self.connectedDeviceList.length; i++) {
            if (self.connectedDeviceList[i].shortAddress == srcAddr) {
                return i;
            }
        }
        return -1;
    }
    
    /*!
     * @brief        Find index of device in the list based on extended
     *				address
     *
     * @param 		extAddr - extended address of the device
     *
     * @return       index of the device in the connected device list, if present
     *			    -1, if not present
     */
    function findDeviceIndexExtAddr(extAddr) {
        /* Check if the device already exists */
        for (var i = 0; i < self.connectedDeviceList.length; i++) {
            /* check if extended address match */
            if (self.connectedDeviceList[i].extAddress == extAddr) {
                return i;
            }
        }
        return -1;
    }
    
    /*****************************************************************
     Functions to send messages to the app server
     *****************************************************************/
    /*!
     * @brief        Send Config req message to application server
     *
     * @param 		none
     *
     * @return       none
     */
    function appC_sendConfigReqToAppServer() {
        // This implementation only intends to provide an example
        // of how to send the message to an end node from the gateway
        // app. Hard coded values below were used to test
        // this implementation
        var devDesc = {
            panID: 0xACDC,
            shortAddress: 0x0001,
            extAddress: 0x00124B0008682c02
        };
        var configReq = {
            cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_configReq,
            frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
            reportingInterval: 1000,
            pollingInterval: 2000
        };
        
        var msg_buf = timac_pb.appsrv_txDataReq.encode({
                    cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                    msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_configReq,
                    devDescriptor: devDesc,
                    configReqMsg: configReq});
        
        if (self.nwkInfo.infoLog) {
            console.log("appC_sendConfigReqToAppServer: ", JSON.stringify(configReq, null, 2));
        }
        
        appC_sendMsgToAppServer(msg_buf);
    }
    
    /*!
     * @brief        Send Config req message to application server
     *
     * @param 		none
     *
     * @return       none
     */
    function appC_sendToggleLedMsgToAppServer(data) {
        // remove 0x from the address and then conver the hex value to decimal
        var dstAddr = data.dstAddr.substring(2).toString(10);
        // find the device index in the list
        var deviceIdx = findDeviceIndexShortAddr(dstAddr);
        if(deviceIdx != -1) {
            // found the device information
            var devDesc = {
                panID: self.nwkInfo.panCoord.panId,
                shortAddress: self.connectedDeviceList[deviceIdx].shortAddress,
                extAddress: self.connectedDeviceList[deviceIdx].extAddress
            };
            
            var toggleReq = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_toggleLedReq,
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_toggleLedReq,
                devDescriptor: devDesc,
                toggleLedReq: toggleReq});
            
            if (self.nwkInfo.infoLog) {
                console.log("appC_sendToggleLedMsgToAppServer: ", JSON.stringify(toggleReq, null, 2));
            }
            
            appC_sendMsgToAppServer(msg_buf);
        }
    }
    
    /*! ======================================================================= */
    function appC_sendCustomConfigMsgToAppServer(type, data) {
        self.queue.push({type: type, data: data, retry: 0, status: STATUS_NONE});
        execute();
    }

    function appC_sendBasicConfigMsgToAppServer(data, callback, failCallBack) {
        var deviceId = data.device_id;
        // find the device index in the list
        var deviceIdx = findDeviceIndexExtAddr(deviceId);
        if(deviceIdx != -1)
        {
            var dev = self.connectedDeviceList[deviceIdx];
            // found the device information
            var devDesc = {
                panID: self.nwkInfo.panCoord.panId,
                shortAddress: dev.shortAddress,
                extAddress: dev.extAddress
            };
            
            var msg_buf = (callback && typeof(callback) === "function") && callback(devDesc);
            appC_sendMsgToAppServer(msg_buf);
        } else {
            (failCallBack && typeof(failCallBack) === "function") && failCallBack(deviceId);
        }
    }

    function appC_sendAlarmConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceTime = data.time;
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_alarmConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                time: deviceTime
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_alarmConfigReq,
                devDescriptor: devDesc,
                alarmConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendAlarmConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendAlarmConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendGSensorConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceEnable = data.enable;
            var deviceSensitivity = data.sensitivity;
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_gSensorConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                enable: deviceEnable,
                sensitivity: deviceSensitivity
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_gSensorConfigReq,
                devDescriptor: devDesc,
                gSensorConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendGSensorConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendGSensorConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendElectricLockConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceEnable = data.enable;
            var deviceTime = data.time;
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_electricLockConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                enable: deviceEnable,
                time: deviceTime
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_electricLockConfigReq,
                devDescriptor: devDesc,
                electricLockConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendElectricLockConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendElectricLockConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendSignalConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceMode = data.mode;
            var deviceValue = data.value;
            var deviceOffset = data.offset;
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_signalConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                mode: deviceMode,
                value: deviceValue,
                offset: deviceOffset
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_signalConfigReq,
                devDescriptor: devDesc,
                signalConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendSignalConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendSignalConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendTempConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceValue = data.value;
            var deviceOffset = data.offset;
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_tempConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                value: deviceValue,
                offset: deviceOffset
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_tempConfigReq,
                devDescriptor: devDesc,
                tempConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendTempConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendTempConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendLowBatteryConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceValue = data.value;
            var deviceOffset = data.offset;
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_lowBatteryConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                value: deviceValue,
                offset: deviceOffset
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_lowBatteryConfigReq,
                devDescriptor: devDesc,
                lowBatteryConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendLowBatteryConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendLowBatteryConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendDistanceConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceMode = data.mode;
            var deviceDistance = data.distance;
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_distanceConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                mode: deviceMode,
                distance: deviceDistance
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_distanceConfigReq,
                devDescriptor: devDesc,
                distanceConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendDistanceConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendDistanceConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendMusicConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceMode = 0;
            var deviceTime = 0;
            if (data.hasOwnProperty('enable')) {
                deviceMode = 0x10 + (data.enable > 0 ? 0x20 : 0x00);
                deviceTime = 0;
            }else{
                deviceMode = data.mode;
                deviceTime = data.time;
            }
            
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_musicConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                mode: deviceMode,
                time: deviceTime
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_musicConfigReq,
                devDescriptor: devDesc,
                musicConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendMusicConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendMusicConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendIntervalConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceMode = data.mode;
            var deviceTime = data.time;

            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_intervalConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                mode: deviceMode,
                time: deviceTime
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_intervalConfigReq,
                devDescriptor: devDesc,
                intervalConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendIntervalConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendIntervalConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendMotionConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceCount = data.count;
            var deviceTime = data.time;

            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_motionConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                count: deviceCount,
                time: deviceTime
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_motionConfigReq,
                devDescriptor: devDesc,
                motionConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendMotionConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendMotionConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendResistanceConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceValue = data.value;
            
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_resistanceConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                value: deviceValue
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_resistanceConfigReq,
                devDescriptor: devDesc,
                resistanceConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendResistanceConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendResistanceConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendMicrowaveConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceEnable = data.enable;
            var deviceSensitivity = data.sensitivity;
            
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_microwaveConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                enable: deviceEnable,
                sensitivity: deviceSensitivity
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_microwaveConfigReq,
                devDescriptor: devDesc,
                microwaveConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendMicrowaveConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendMicrowaveConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendPirConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceEnable = data.enable;
            
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_pirConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                enable: deviceEnable
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_pirConfigReq,
                devDescriptor: devDesc,
                pirConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendPirConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendPirConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendSetUnsetConfigMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceState = data.state;
            
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_setUnsetConfigReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                state: deviceState
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_setUnsetConfigReq,
                devDescriptor: devDesc,
                setUnsetConfigReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendSetUnsetConfigMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendSetUnsetConfigMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendDisconnectMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceTime = data.time;
            
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_disconnectReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                time: deviceTime
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_disconnectReq,
                devDescriptor: devDesc,
                disconnectReqMsg: configReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendDisconnectMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendDisconnectMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendElectricLockActionMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceRelay = data.relay;
            
            var actionReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_electricLockActionReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                relay: deviceRelay
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_electricLockActionReq,
                devDescriptor: devDesc,
                electricLockActionReqMsg: actionReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendElectricLockActionMsgToAppServer: ", JSON.stringify(actionReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendElectricLockActionMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendRemoveDeviceMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var configReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_removeDevice,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_removeDevice,
                devDescriptor: devDesc,
                removeDeviceMsg: configReqMsg
            });

            var deviceId = data.device_id;
            removeConnectedDeviceByDeviceId(deviceId);
            
            var extAddress = deviceId;
            if (extAddress in self.sensorMap) {
                var type = self.sensorMap[extAddress];
                delete self.sensorMap[extAddress];

                if (type === TYPE_EVE003) {
                    var rcIndex = self.engineerRCList.indexOf(extAddress);
                    if (rcIndex < 0) {
                        self.engineerRCList.push(extAddress);
                    }
                }
            }

            if (self.nwkInfo.infoLog) console.log("appC_sendRemoveDeviceMsgToAppServer: ", JSON.stringify(configReqMsg, null, 2));

            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendRemoveDeviceMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function appC_sendCleanMsgToAppServer(data) {
        var devDesc = {
            panID: 0xACDC,
            shortAddress: 0x0001,
            extAddress: 0x00124B0008682c02
        };

        var configReqMsg = {
        };

        var msg_buf = timac_pb.appsrv_txDataReq.encode({
                    cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                    msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_clean,
                    devDescriptor: devDesc,
                    cleanMsg: configReqMsg});

        if (self.nwkInfo.infoLog) {
            console.log("appC_sendCleanMsgToAppServer: ", JSON.stringify({
                    cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                    msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_clean,
                    devDescriptor: devDesc,
                    cleanMsg: configReqMsg}, null, 2));
        }
        
        appC_sendMsgToAppServer(msg_buf);

        self.connectedDeviceList = [];
        appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);

        this.sensorMap = {};
        this.engineerRCList = [];
        this.queue = [];
        this.noTypeDeviceList = [];
    }
    /*!
     * @brief        Send Sensor Config req message to application server
     *
     * @param 		none
     *
     * @return       none
     */
    function appC_sendSensorConfigMsgToAppServer(data) {
        // remove 0x from the address and then conver the hex value to decimal
        var dstAddr = data.dstAddr.substring(2).toString(10);
        // find the device index in the list
        var deviceIdx = findDeviceIndexShortAddr(dstAddr);
        
        if(deviceIdx != -1)
        {
            // found the device information
            var devDesc = {
                panID: self.nwkInfo.panCoord.panId,
                shortAddress: self.connectedDeviceList[deviceIdx].shortAddress,
                extAddress: self.connectedDeviceList[deviceIdx].extAddress
            };
            
            var sensorConfigPirReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_setSensorConfigPirReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_pirSensor,
                gSensorEnable: 1,
                gSensorSensitivity: 50,
                tempValue: 20,
                tempOffset: 5
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_setSensorConfigPirReq,
                                devDescriptor: devDesc,
                                sensorConfigPirReqMsg: sensorConfigPirReqMsg});
            
            if (self.nwkInfo.infoLog) {
                console.log("appC_sendSensorConfigMsgToAppServer: ", JSON.stringify(sensorConfigPirReqMsg, null, 2));
            }
            appC_sendMsgToAppServer(msg_buf);
        }
    }

    function appC_sendAntennaConfigMsgToAppServer(data) {
        var state = data.state

        var devDesc = {
            panID: 0xACDC,
            shortAddress: 0x0001,
            extAddress: 0x00124B0008682c02
        };

        var antennaReqMsg = {
            state: state
        };

        var msg_buf = timac_pb.appsrv_txDataReq.encode({
                    cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                    msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_antennaReq,
                    devDescriptor: devDesc,
                    antennaReqMsg: antennaReqMsg});

        if (self.nwkInfo.infoLog) {
            console.log("appC_sendAntennaConfigMsgToAppServer: ", JSON.stringify({
                    cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                    msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_antennaReq,
                    devDescriptor: devDesc,
                    antennaReqMsg: antennaReqMsg}, null, 2));
        }
        
        appC_sendMsgToAppServer(msg_buf);
    }

    function appC_sendSetIntervalMsgToAppServer(data) {
        appC_sendBasicConfigMsgToAppServer(data, function(devDesc) {
            var deviceReporting = data.reporting * 1000;
            var devicePolling = data.polling * 1000;
            
            var setIntervalReqMsg = {
                cmdId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_setIntervalReq,
                frameControl: timac_pb.Smsgs_dataFields.Smsgs_dataFields_configSettings,
                reporting: deviceReporting,
                polling: devicePolling
            };
            
            var msg_buf = timac_pb.appsrv_txDataReq.encode({
                cmdId: timac_pb.appsrv_CmdId.APPSRV_TX_DATA_REQ,
                msgId: timac_pb.Smsgs_cmdIds.Smsgs_cmdIds_setIntervalReq,
                devDescriptor: devDesc,
                setIntervalReqMsg: setIntervalReqMsg
            });

            if (self.nwkInfo.infoLog) console.log("appC_sendSetIntervalMsgToAppServer: ", JSON.stringify(setIntervalReqMsg, null, 2));
            return msg_buf;
        }, function(deviceId) {
            if (self.nwkInfo.infoLog) console.log("appC_sendSetIntervalMsgToAppServer device is not found(" + deviceId + ")");
        });
    }

    function removeConnectedDeviceByDeviceId(deviceId) {
        var deviceIdx = findDeviceIndexExtAddr(deviceId);
        if (deviceIdx < 0) {
            console.log("ERROR: rcvd delete data for non-existing device");
            return;
        }

        self.connectedDeviceList.splice(deviceIdx, 1);
        appClientInstance.emit('connDevInfoUpdate', self.connectedDeviceList);
    }

    function appC_setInfoLogMsgToAppServer(data) {
        self.nwkInfo.infoLog = data.open;
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    }

    function appC_setEve003LogMsgToAppServer(data) {
        self.nwkInfo.eve003Log = data.open;
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    }

    function appC_setEve004LogMsgToAppServer(data) {
        self.nwkInfo.eve004Log = data.open;
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    }

    function appC_setEve005LogMsgToAppServer(data) {
        self.nwkInfo.eve005Log = data.open;
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    }

    function appC_setEve006LogMsgToAppServer(data) {
        self.nwkInfo.eve006Log = data.open;
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    }

    function appC_processPairingUpdate(data) {
        appC_setJoinPermitAtAppServer({ open: true, duration: 1 * 60});
        appClientInstance.emit('pairingUpdate', JSON.parse(data));
    }

    function appC_processAntennaUpdate(data) {
        appClientInstance.emit('antennaUpdate', JSON.parse(data));
    }

    function appC_processSetIntervalUpdate(data) {
        var deviceIdx = -1;
        var deviceData = JSON.parse(data);
        // console.log("=====appC_processSetIntervalUpdate:" + JSON.stringify(deviceData, null, 2));
        /* Find the index of the device in the list */
        if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_short) {
            deviceIdx = findDeviceIndexShortAddr(deviceData.srcAddr.shortAddr);
        } else if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_extended) {
            deviceIdx = findDeviceIndexExtAddr(deviceData.srcAddr.extAddress.data);
        } else {
            console.log("ERROR: illegal addr mode value rcvd in set interval msg", deviceData.srcAddr.addrMode);
            return;
        }
        
        if (deviceIdx !== -1) {
            var device = self.connectedDeviceList[deviceIdx];
            var extAddress = device.extAddress;

            var reporting = deviceData.reporting / 1000;
            var polling = deviceData.polling / 1000;
            appClientInstance.emit('setIntervalUpdate', {device_id:extAddress, reporting: reporting, polling: polling});
        }else{
            if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_short) {
                console.log("ERROR: rcvd set interval message for non-existing device(short:" + deviceData.srcAddr.shortAddr + ")");
            } else if (deviceData.srcAddr.addrMode == timac_pb.ApiMac_addrType.ApiMac_addrType_extended) {
                console.log("ERROR: rcvd set interval message for non-existing device(ext:" + deviceData.srcAddr.extAddress.data + ")");
            }else{
                console.log("ERROR: rcvd set interval message for non-existing device!!!");
            }
        }
    }
    
    
    /*!
     * @brief        Send get network Info Req to application server
     *
     * @param 		none
     *
     * @return       none
     */
    function appC_getNwkInfoFromAppServer() {
        /* create the message */
        var msg_buf = timac_pb.appsrv_getNwkInfoReq.encode({
                                cmdId: timac_pb.appsrv_CmdId.APPSRV_GET_NWK_INFO_REQ});
        
        // if (self.nwkInfo.infoLog) {
        //     console.log("appC_getNwkInfoFromAppServer: ", JSON.stringify({
        //                         cmdId: timac_pb.appsrv_CmdId.APPSRV_GET_NWK_INFO_REQ}, null, 2));
        // }
        appC_sendMsgToAppServer(msg_buf);
    }
    
    /*!
     * @brief        Send get device array Req to application server
     *
     * @param 		none
     *
     * @return       none
     */
    function appC_getDevArrayFromAppServer() {
        /* create the message */
        var msg_buf = timac_pb.appsrv_getNwkInfoReq.encode({
                                cmdId: timac_pb.appsrv_CmdId.APPSRV_GET_DEVICE_ARRAY_REQ});
        
        // if (self.nwkInfo.infoLog) {
        //     console.log("appC_getDevArrayFromAppServer: ", JSON.stringify({
        //                             cmdId: timac_pb.appsrv_CmdId.APPSRV_GET_DEVICE_ARRAY_REQ}, null, 2));
        // }
        appC_sendMsgToAppServer(msg_buf);
    }
    
    /*!
     * @brief        Send join permit Req to application server
     *
     * @param 		data - containinfo about action required
     *					"open" - open network for device joins
     *				    "close"- close netwwork for device joins
     *
     * @return
     */
    function appC_setJoinPermitAtAppServer(data) {
        var open = data.open;
        var duration = data.duration * 1000;
        if (open) {
            if (duration == 0) {
                duration = 0xFFFFFFFF;
            }
        } else {
            duration = 0x0;
        }

        self.joinDuration = duration;

        /* Set always open value */
        // duration = 0xFFFFFFFF;
        /* Set always close value */
        // duration = 0x0;

        /* create the message */
        var msg_buf = timac_pb.appsrv_setJoinPermitReq.encode({
                                    cmdId: timac_pb.appsrv_CmdId.APPSRV_SET_JOIN_PERMIT_REQ,
                                    duration: duration});
        
        if (self.nwkInfo.infoLog) {
            console.log("appC_setJoinPermitAtAppServer: ", JSON.stringify({
                                    cmdId: timac_pb.appsrv_CmdId.APPSRV_SET_JOIN_PERMIT_REQ,
                                    duration: duration}, null, 2));
        }
        appC_sendMsgToAppServer(msg_buf);
    }
    
    /*!
     * @brief        Send message to application server
     *
     * @param 		msg_buf - element containing data to be sent
     *					to the application server
     *
     * @return       none
     */
    function appC_sendMsgToAppServer(msg_buf) {
        var ByteBuffer = require("bytebuffer");
        var pkt_buf = new ByteBuffer(PKT_HEADER_SIZE + msg_buf.length, ByteBuffer.LITTLE_ENDIAN)
                            .writeShort(msg_buf.length, PKT_HEADER_LEN_FIELD)
                            .writeUint8(timac_pb.timacAppSrvSysId.RPC_SYS_PB_TIMAC_APPSRV, PKT_HEADER_SUBSYS_FIELD)
                            .writeUint8(msg_buf[1], PKT_HEADER_CMDID_FIELD)
                            .append(msg_buf, "hex", PKT_HEADER_SIZE);
        /* Send the message to server */
        appClient.write(pkt_buf.buffer);
    };
    
    /*!
     * @brief        Allows to request for network
     *				information
     *
     * @param 		none
     *
     * @return       network information
     */
    Appclient.prototype.appC_getNwkInfo = function () {
        /* send the netwiork information */
        appClientInstance.emit('nwkUpdate', self.nwkInfo);
    };
    
    /*!
     * @brief        Allows to request for device array
     *				information
     *
     * @param 		none
     *
     * @return       connected device list
     */
    Appclient.prototype.appC_getDeviceArray = function () {
        /* send the device information */
        appClientInstance.emit('getdevArrayRsp', self.connectedDeviceList);
    };
    
    /*!
     * @brief        Allows to modify permit join setting for the network
     *
     * @param 		none
     *
     * @return       data - containinfo about action required
     *					"open" - open network for device joins
     *				    "close"- close netwwork for device joins
     */
    Appclient.prototype.appC_setPermitJoin = function (data) {
        appC_setJoinPermitAtAppServer(data);
    }
    
    /*!
     * @brief        Allows send toggle command to a network device
     *
     * @param 		none
     *
     * @return       none
     */
    Appclient.prototype.appC_sendToggle = function (data) {
        appC_sendToggleLedMsgToAppServer(data);
    }
    
    /*!
     * @brief        Allows send config command to a network device
     *
     * @param 		none
     *
     * @return       none
     */
    Appclient.prototype.appC_sendConfig = function (data) {
        appC_sendConfigReqToAppServer(data);
    }
    
    /*! ======================================================================= */
    Appclient.prototype.appC_sendCustomConfig = function (type, data) {
        appC_sendCustomConfigMsgToAppServer(type, data);
    }

    Appclient.prototype.appC_sendAlarmConfig = function (data) {
        appC_sendAlarmConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendGSensorConfig = function (data) {
        appC_sendGSensorConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendElectricLockConfig = function (data) {
        appC_sendElectricLockConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendSignalConfig = function (data) {
        appC_sendSignalConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendTempConfig = function (data) {
        appC_sendTempConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendLowBatteryConfig = function (data) {
        appC_sendLowBatteryConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendDistanceConfig = function (data) {
        appC_sendDistanceConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendMusicConfig = function (data) {
        appC_sendMusicConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendIntervalConfig = function (data) {
        appC_sendIntervalConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendMotionConfig = function (data) {
        appC_sendMotionConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendResistanceConfig = function (data) {
        appC_sendResistanceConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendMicrowaveConfig = function (data) {
        appC_sendMicrowaveConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendPirConfig = function (data) {
        appC_sendPirConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendSetUnsetConfig = function (data) {
        appC_sendSetUnsetConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendDisconnect = function (data) {
        appC_sendDisconnectMsgToAppServer(data);
    }

    Appclient.prototype.appC_sendElectricLockAction = function (data) {
        appC_sendElectricLockActionMsgToAppServer(data);
    }

    Appclient.prototype.appC_removeDevice = function (data) {
        appC_sendRemoveDeviceMsgToAppServer(data);
    }

    Appclient.prototype.appC_clean = function (data) {
        appC_sendCleanMsgToAppServer(data);
    }
    
    Appclient.prototype.appC_setInfoLog = function (data) {
        appC_setInfoLogMsgToAppServer(data);
    }

    Appclient.prototype.appC_setEve003Log = function (data) {
        appC_setEve003LogMsgToAppServer(data);
    }

    Appclient.prototype.appC_setEve004Log = function (data) {
        appC_setEve004LogMsgToAppServer(data);
    }

    Appclient.prototype.appC_setEve005Log = function (data) {
        appC_setEve005LogMsgToAppServer(data);
    }

    Appclient.prototype.appC_setEve006Log = function (data) {
        appC_setEve006LogMsgToAppServer(data);
    }

    Appclient.prototype.appC_setAntennaConfig = function (data) {
        appC_sendAntennaConfigMsgToAppServer(data);
    }

    Appclient.prototype.appC_setInterval = function (data) {
        appC_sendSetIntervalMsgToAppServer(data);
    }

    function execute() {
        var item = getItemByNoneStatus();
        if (item == null) {
            return false;
        }

        //if (self.nwkInfo.infoLog) console.log("execute item:" + JSON.stringify(item));
        sendCustomConfigMsg(item);
        return true;
    }

    function sentTimeout(item) {
        item.retryTimeoutId = null;
        item.retry += 1;
        //if (self.nwkInfo.infoLog) console.log("sentTimeout retry:" + JSON.stringify(item));
        
        if (item.retry >= RETRY_MAX) {
            var index = self.queue.indexOf(item);
            if (index >= 0) {
                self.queue.splice(index, 1);
            }

            var type = self.rspDictionary[item.type]
            var data = item.data;

            if (type == 'disconnectRsp'){
                data.success = true;
                if (self.nwkInfo.infoLog) console.log(type + " success*:" + JSON.stringify(data));
                appC_sendRemoveDeviceMsgToAppServer(data);
            }else{
                data.success = false;
                if (self.nwkInfo.infoLog) console.log(type + " retry failed:" + JSON.stringify(item.data));
            }

            appClientInstance.emit(type, item.data);

            execute();
            return;
        }

        sendCustomConfigMsg(item);
    }

    function sendCustomConfigMsg(item) {
        switch (item.type) {
            case 'setAlarmConfig':
                appC_sendAlarmConfigMsgToAppServer(item.data);
                break;
            case 'setGSensorConfig':
                appC_sendGSensorConfigMsgToAppServer(item.data);
                break;
            case 'setElectricLockConfig':
                appC_sendElectricLockConfigMsgToAppServer(item.data);
                break;
            case 'setSignalConfig':
                appC_sendSignalConfigMsgToAppServer(item.data);
                break;
            case 'setTempConfig':
                appC_sendTempConfigMsgToAppServer(item.data);
                break;
            case 'setLowBatteryConfig':
                appC_sendLowBatteryConfigMsgToAppServer(item.data);
                break;
            case 'setDistanceConfig':
                appC_sendDistanceConfigMsgToAppServer(item.data);
                break;
            case 'setMusicConfig':
                appC_sendMusicConfigMsgToAppServer(item.data);
                break;
            case 'setIntervalConfig':
                appC_sendIntervalConfigMsgToAppServer(item.data);
                break;
            case 'setMotionConfig':
                appC_sendMotionConfigMsgToAppServer(item.data);
                break;
            case 'setResistanceConfig':
                appC_sendResistanceConfigMsgToAppServer(item.data);
                break;
            case 'setMicrowaveConfig':
                appC_sendMicrowaveConfigMsgToAppServer(item.data);
                break;
            case 'setPirConfig':
                appC_sendPirConfigMsgToAppServer(item.data);
                break;
            case 'setSetUnsetConfig':
                appC_sendSetUnsetConfigMsgToAppServer(item.data);
                break;
            case 'sendElectricLockAction':
                appC_sendElectricLockActionMsgToAppServer(item.data);
                break;
            case 'sendDisconnect':
                appC_sendDisconnectMsgToAppServer(item.data);
                break;
            default:
                return;
        }

        item.status = STATUS_SENT;
        item.retryTimeoutId = setTimeout(sentTimeout, 3000, item);
    }

    function processResponse(type, data) {
        var deviceId = data.device_id;
        var item = findItem(deviceId);
        if (item == null) {
            return;
        }

        if (item.retryTimeoutId != null) {
            clearTimeout(item.retryTimeoutId);
            item.retryTimeoutId = null;
        }

        var index = self.queue.indexOf(item);
        if (index >= 0) {
            self.queue.splice(index, 1);
        }
        
        data.success = true;
        if (self.nwkInfo.infoLog) console.log(type + " success:" + JSON.stringify(data));

        if (type == 'disconnectRsp'){
            appC_sendRemoveDeviceMsgToAppServer(data);
        }

        appClientInstance.emit(type, data);

        execute();
    }

    function findItem(device_id) {
        for (var i = 0; i < self.queue.length; i++) {
            item = self.queue[i];

            var data = item.data;
            if (data.device_id == device_id) {
                return item;
            }
        }
        return null;
    }

    function getItemByNoneStatus() {
        var count = getRuningCount();
        if (count >= RUN_MAX) {
            return null;
        }

        var item = null;
        for (var i = 0; i < self.queue.length; i++) {
            item = self.queue[i];

            if (item.status == STATUS_NONE) {
                return item;
            }
        }
        return null;
    }

    function getRuningCount() {
        var result = 0;
        var item = null;
        for (var i = 0; i < self.queue.length; i++) {
            item = self.queue[i];

            if (item.status == STATUS_SENT) {
                result += 1;
            }
        }
        return result;
    }
}

Appclient.prototype.__proto__ = events.EventEmitter.prototype;

module.exports = Appclient;


