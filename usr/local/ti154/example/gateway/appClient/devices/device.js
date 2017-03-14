/******************************************************************************

 @file device.js

 @brief device specific functions

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

/********************************************************************
 * Variables
 * *****************************************************************/
var protobuf = require("protocol-buffers");
var fs = require("fs");
/* set-up to decode/encode proto messages */
var dtimac_pb = protobuf(fs.readFileSync('/usr/local/ti154/example/gateway/appClient/protofiles/appsrv.proto'));

/*!
 * @brief      Constructor for device objects
 *
 * @param      shortAddress - 16 bit address of the device
 * 			   extAddress - 64 bit IEEE address of the device
 * 			   capabilityInfo - device capability information
 *
 * @retun      device object
 */
function Device(shortAddress, extAddress, capabilityInfo) {
    var devInfo = this;
    devInfo.shortAddress = shortAddress;
    devInfo.extAddress = extAddress;
    devInfo.capabilityInfo = capabilityInfo;
    devInfo.active = 'true';
    return devInfo;
}

/* Prototype Functions */
Device.prototype.rxSensorData = function (sensorData) {
  /* recieved message from the device, set as active */
  this.active = 'true';
	/* Check the support sensor Types and
	add information elements for those */
  if (sensorData.sDataMsg.frameControl &
      dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve003Sensor) {
      /* update the sensor values */
		  this.eve003sensor = {
          button: sensorData.sDataMsg.eve003Sensor.button,
          dip: sensorData.sDataMsg.eve003Sensor.dip,
          setting: sensorData.sDataMsg.eve003Sensor.setting,
          battery: sensorData.sDataMsg.eve003Sensor.battery
      };
  }
  if (sensorData.sDataMsg.frameControl &
        dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve004Sensor) {
      /* update the sensor values */
		  this.eve004sensor = {
          tempValue: sensorData.sDataMsg.eve004Sensor.tempValue,
          state: sensorData.sDataMsg.eve004Sensor.state,
          alarm: sensorData.sDataMsg.eve004Sensor.alarm,
          dip: sensorData.sDataMsg.eve004Sensor.dip,
          battery: sensorData.sDataMsg.eve004Sensor.battery,
          rssi: sensorData.sDataMsg.eve004Sensor.rssi
      };
  }
  if (sensorData.sDataMsg.frameControl &
        dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve005Sensor) {
      /* update the sensor values */
		  this.eve005sensor = {
		      tmpValue: sensorData.sDataMsg.eve005Sensor.tmpValue,
          batValue: sensorData.sDataMsg.eve005Sensor.batValue,
          rssi: sensorData.sDataMsg.eve005Sensor.rssi,
          button: sensorData.sDataMsg.eve005Sensor.button,
          dip_id: sensorData.sDataMsg.eve005Sensor.dip_id,
          dip_control: sensorData.sDataMsg.eve005Sensor.dip_control,
          sensor_s: sensorData.sDataMsg.eve005Sensor.sensor_s,
          alarm: sensorData.sDataMsg.eve005Sensor.alarm,
          resist1: sensorData.sDataMsg.eve005Sensor.resist1,
          resist2: sensorData.sDataMsg.eve005Sensor.resist2
    };
  }
  if (sensorData.sDataMsg.frameControl &
        dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve006Sensor) {
      /* update the sensor values */
		  this.eve006sensor = {
          button: sensorData.sDataMsg.eve006Sensor.button,
          state: sensorData.sDataMsg.eve006Sensor.state,
          abnormal: sensorData.sDataMsg.eve006Sensor.abnormal,
          dip: sensorData.sDataMsg.eve006Sensor.dip,
          dip_id: sensorData.sDataMsg.eve006Sensor.dip_id,
          temp: sensorData.sDataMsg.eve006Sensor.temp,
          humidity: sensorData.sDataMsg.eve006Sensor.humidity,
          battery: sensorData.sDataMsg.eve006Sensor.battery,
          resist1: sensorData.sDataMsg.eve006Sensor.resist1,
          resist2: sensorData.sDataMsg.eve006Sensor.resist2
      };
  }
  /* update rssi information */
  this.rssi = sensorData.rssi;
}

Device.prototype.rxConfigRspInd = function (devConfigData) {
    var device = this;
    if (devConfigData.sConfigMsg.status == 0) {
        device.active = 'true';
		    /* Check the support sensor Types and add
		      information elements for those */
        if (devConfigData.sConfigMsg.frameControl &
              dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve003Sensor) {
            /* initialize sensor information element */
	    	    this.eve003sensor = {
               	button: 0,
            	  dip: 0,
            	  setting: 0,
            	  battery: 0
            };
        }
        if (devConfigData.sConfigMsg.frameControl &
              dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve004Sensor) {
            /* initialize sensor information element */
	    	    this.eve004sensor = {
				        tempValue: 0,
            	  state: 0,
            	  alarm: 0,
            	  dip: 0,
            	  battery: 0,
            	  rssi: 0
            };
        }
        if (devConfigData.sConfigMsg.frameControl &
              dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve005Sensor) {
	    	    this.eve005sensor = {
                tmpValue: 0,
                batValue: 0,
                rssi: 0,
                button: 0,
                dip_id: 0,
                dip_control: 0,
                sensor_s: 0,
                alarm: 0,
                resist1: 0,
                resist2: 0
            };
        }
        if (devConfigData.sConfigMsg.frameControl &
              dtimac_pb.Smsgs_dataFields.Smsgs_dataFields_eve006Sensor) {
	    	    this.eve006sensor = {
                button: 0,
                state: 0,
                abnormal: 0,
                dip: 0,
                dip_id: 0,
                temp: 0,
                humidity: 0,
                battery: 0,
                resist1: 0,
                resist2: 0
            };
        }
        
        device.reportingInterval = devConfigData.sConfigMsg.reportingInterval;
        if (device.capabilityInfo.rxOnWhenIdle == 1) {
            device.pollingInterval = devConfigData.sConfigMsg.pollingInterval;
        } else {
            device.pollingInterval = "always on device";
        }
    }
}

Device.prototype.deviceNotActive = function (inactiveDevInfo) {
    this.active = 'false';
}

Device.prototype.devUpdateInfo = function (shortAddr, capInfo) {
    this.shortAddress = shortAddr;
    this.capabilityInfo = capInfo;
    this.active = 'true';
}

module.exports = Device;
