/******************************************************************************

 @file gateway.js

 @brief local gateway

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
var AppClient = require("./appClient/appclient.js");
var Webserver = require("./webserver/webserver.js");

/*!
 * @brief      Constructor for local gateway
 *
 * @param      none
 *
 * @retun      none
 */
function Gateway() {
	var applclient = new AppClient();
	var webserver = new Webserver();
	
	/* rcvd send config req */
	webserver.on('sendConfig', function (data) {
		/* send config request */
		applclient.appC_sendConfig(data);
	});

	/* rcvd send toggle req */
	webserver.on('sendToggle', function (data) {
		/* send toggle request */
		applclient.appC_sendToggle(data);
	});

	/* rcvd getDevArray Req */
	webserver.on('getDevArrayReq', function (data) {
		/* process the request */
		applclient.appC_getDeviceArray();
	});

	/* rcvd getNwkInfoReq */
	webserver.on('getNwkInfoReq', function (data) {
		/* process the request */
		applclient.appC_getNwkInfo();
	});

	/* send message to web-client */
	applclient.on('permitJoinCnf', function (data) {
		webserver.webserverSendToClient('permitJoinCnf', JSON.stringify(data));
	});

	/* send connected device info update to web-client */
	applclient.on('connDevInfoUpdate', function (data) {
		webserver.webserverSendToClient('connDevInfoUpdate', JSON.stringify(data));
	});

	/* send nwkUpdate to web-client */
	applclient.on('nwkUpdate', function (data) {
		webserver.webserverSendToClient('nwkUpdate', JSON.stringify(data));
	});

	/* send device array to web-client */
	applclient.on('getdevArrayRsp', function (data) {
		webserver.webserverSendToClient('getdevArrayRsp', JSON.stringify(data));
	});

	/*! ================================================================ */
	applclient.on('joinPermitRsp', function (data) {
        webserver.webserverSendToClient('joinPermitRsp', JSON.stringify(data));
	});
	applclient.on('alarmConfigRsp', function (data) {
        webserver.webserverSendToClient('alarmConfigRsp', JSON.stringify(data));
	});
	applclient.on('gSensorConfigRsp', function (data) {
		webserver.webserverSendToClient('gSensorConfigRsp', JSON.stringify(data));
	});
	applclient.on('electricLockConfigRsp', function (data) {
		webserver.webserverSendToClient('electricLockConfigRsp', JSON.stringify(data));
	});
	applclient.on('signalConfigRsp', function (data) {
		webserver.webserverSendToClient('signalConfigRsp', JSON.stringify(data));
	});
	applclient.on('tempConfigRsp', function (data) {
		webserver.webserverSendToClient('tempConfigRsp', JSON.stringify(data));
	});
	applclient.on('lowBatteryConfigRsp', function (data) {
		webserver.webserverSendToClient('lowBatteryConfigRsp', JSON.stringify(data));
	});
	applclient.on('distanceConfigRsp', function (data) {
		webserver.webserverSendToClient('distanceConfigRsp', JSON.stringify(data));
	});
	applclient.on('musicConfigRsp', function (data) {
		webserver.webserverSendToClient('musicConfigRsp', JSON.stringify(data));
	});
	applclient.on('intervalConfigRsp', function (data) {
		webserver.webserverSendToClient('intervalConfigRsp', JSON.stringify(data));
	});
	applclient.on('motionConfigRsp', function (data) {
		webserver.webserverSendToClient('motionConfigRsp', JSON.stringify(data));
	});
	applclient.on('resistanceConfigRsp', function (data) {
		webserver.webserverSendToClient('resistanceConfigRsp', JSON.stringify(data));
	});
	applclient.on('microwaveConfigRsp', function (data) {
		webserver.webserverSendToClient('microwaveConfigRsp', JSON.stringify(data));
	});
	applclient.on('pirConfigRsp', function (data) {
		webserver.webserverSendToClient('pirConfigRsp', JSON.stringify(data));
	});
	applclient.on('setUnsetConfigRsp', function (data) {
		webserver.webserverSendToClient('setUnsetConfigRsp', JSON.stringify(data));
	});
	applclient.on('disconnectRsp', function (data) {
		webserver.webserverSendToClient('disconnectRsp', JSON.stringify(data));
	});
	applclient.on('electricLockActionRsp', function (data) {
		webserver.webserverSendToClient('electricLockActionRsp', JSON.stringify(data));
	});
	applclient.on('pairingUpdate', function (data) {
		webserver.webserverSendToClient('pairingUpdate', JSON.stringify(data));
	});
	applclient.on('antennaUpdate', function (data) {
		webserver.webserverSendToClient('antennaUpdate', JSON.stringify(data));
	});
	applclient.on('setIntervalUpdate', function (data) {
		webserver.webserverSendToClient('setIntervalUpdate', JSON.stringify(data));
	});

	webserver.on('setJoinPermitReq', function (data) {
		applclient.appC_setPermitJoin(data);
	});
	webserver.on('setAlarmConfig', function (data) {
		applclient.appC_sendCustomConfig('setAlarmConfig', data);
	});
	webserver.on('setGSensorConfig', function (data) {
		applclient.appC_sendCustomConfig('setGSensorConfig', data);
	});
	webserver.on('setElectricLockConfig', function (data) {
		applclient.appC_sendCustomConfig('setElectricLockConfig', data);
	});
	webserver.on('setSignalConfig', function (data) {
		applclient.appC_sendCustomConfig('setSignalConfig', data);
	});
	webserver.on('setTempConfig', function (data) {
		applclient.appC_sendCustomConfig('setTempConfig', data);
	});
	webserver.on('setLowBatteryConfig', function (data) {
		applclient.appC_sendCustomConfig('setLowBatteryConfig', data);
	});
	webserver.on('setDistanceConfig', function (data) {
		applclient.appC_sendCustomConfig('setDistanceConfig', data);
	});
	webserver.on('setMusicConfig', function (data) {
		applclient.appC_sendCustomConfig('setMusicConfig', data);
	});
	webserver.on('setIntervalConfig', function (data) {
		applclient.appC_sendCustomConfig('setIntervalConfig', data);
	});
	webserver.on('setMotionConfig', function (data) {
		applclient.appC_sendCustomConfig('setMotionConfig', data);
	});
	webserver.on('setResistanceConfig', function (data) {
		applclient.appC_sendCustomConfig('setResistanceConfig', data);
	});
	webserver.on('setMicrowaveConfig', function (data) {
		applclient.appC_sendCustomConfig('setMicrowaveConfig', data);
	});
	webserver.on('setPirConfig', function (data) {
		applclient.appC_sendCustomConfig('setPirConfig', data);
	});
	webserver.on('setSetUnsetConfig', function (data) {
		applclient.appC_sendCustomConfig('setSetUnsetConfig', data);
	});
	webserver.on('sendDisconnect', function (data) {
		applclient.appC_sendCustomConfig('sendDisconnect', data);
	});
	webserver.on('sendElectricLockAction', function (data) {
		applclient.appC_sendCustomConfig('sendElectricLockAction', data);
	});
	webserver.on('removeDevice', function (data) {
		applclient.appC_removeDevice(data);
	});
	webserver.on('clean', function (data) {
		applclient.appC_clean(data);
	});
	webserver.on('setInfoLog', function (data) {
		applclient.appC_setInfoLog(data);
	});
	webserver.on('setEve003Log', function (data) {
		applclient.appC_setEve003Log(data);
	});
	webserver.on('setEve004Log', function (data) {
		applclient.appC_setEve004Log(data);
	});
	webserver.on('setEve005Log', function (data) {
		applclient.appC_setEve005Log(data);
	});
	webserver.on('setEve006Log', function (data) {
		applclient.appC_setEve006Log(data);
	});
	webserver.on('setAntennaConfig', function (data) {
		applclient.appC_setAntennaConfig(data);
	});
	webserver.on('setInterval', function (data) {
		applclient.appC_setInterval(data);
	});
}

/* create gateway */
var gateway = new Gateway();

