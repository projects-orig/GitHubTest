#coding=UTF-8

import requests
import datetime
import time
import json
import os
import sys
import paho.mqtt.publish as publish
import paho.mqtt.client as mqtt

LOCALHOST_URL='http://127.0.0.1:5001'

API_HEADER = {'Content-Type': 'application/json'}

JSON_FILE_PATH = '/usr/local/share/security_service/'

############################
CUSTOMER_ID=''
try:
  with open(JSON_FILE_PATH + "customer_id.json", "r") as f:
    file_content=json.load(f)
    CUSTOMER_ID=file_content['customer_id']
except IOError:
  print ("[customer_id.json] file doesn't exist...")

print("customer_id=" + CUSTOMER_ID)

TOPIC = 'H' + CUSTOMER_ID

MQTT_HOST = "211.22.242.13"

MQTT_PORT = 1883
MQTT_USERNAME = "amigo"
MQTT_PASSWORD = "swetop"
MQTT_QOS = 2

def on_connect(mqttc, obj, flags, rc):
    print("connect rc: " + str(rc))

def on_disconnect(mqttc, obj, rc):
    print("on_disconnect" + str(rc))

    if obj == 0:
        mqttc.reconnect()

def on_log(mqttc, obj, level, string):
    print("log:" + string)


def amigo_mqtt_pub(topic, data):
    #auth = {'username': MQTT_USERNAME, 'password': MQTT_PASSWORD}
    
    publish.single(topic=topic, payload=data,
                   hostname=MQTT_HOST, port=MQTT_PORT, qos=2)

d02_device_id = "5149013103542656"
d03_device_id = "5149013103542553"
d04_device_id = "5149013103542545"
d05_device_id = "5149013115785703"

counter = 0
var = 1
while var==1:
    pass
    #command="echo 0 > /var/log/communication_service.log" 
    #print("--------------Clear communication_service.log----------------")
    #os.system(command)

    start_timestamp=int(time.time())

    
#---------------------------------d02 test start
    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"3"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security set-------------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"8"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------d02 test start-----------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"9"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"3"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"4",
        "temperature_status":"80"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"5"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"6",
        "power_status":"5"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"7"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"9"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"11",
        "identity":"1",
        "temperature_status":"30",
        "power_status":"70"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"8"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security unset!!!--------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

#---------------------------------d03 test start
    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"3"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security set-------------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"3"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------d03 test start-----------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"4",
        "temperature_status":"80"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"5"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"6",
        "power_status":"5"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"7"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"9"
      }
    }

    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"10"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d03":
      { 
        "device_id":d03_device_id,  
        "trigger_type":"11",
        "identity":"1",
        "temperature_status":"30",
        "power_status":"70"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"8"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security unset!!!--------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

#---------------------------------d04 test start
    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"3"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security set-------------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"3"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------d04 test start-----------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"4",
        "temperature_status":"80"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"5"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"6",
        "power_status":"5"
      }
    }

    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"7"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"8"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"10",
        "identity":"1",
        "temperature_status":"30",
        "power_status":"70"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"8"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security unset!!!--------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

#---------------------------------d02 d04 temperature & alarm d03 IR alarm
    
    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"3"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security set-------------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    print("\n-------d02/d04 temperature alarm & d03 IR alarm------------\n")
    data_obj={
    "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"4",
        "temperature_status":"80"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"4",
        "temperature_status":"80"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
    "d03":
    { 
      "device_id":d03_device_id,  
      "trigger_type":"10"
    }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d02":
      { 
        "device_id":d02_device_id,  
        "trigger_type":"11",
        "identity":"1",
        "temperature_status":"30",
        "power_status":"70"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d04":
      { 
        "device_id":d04_device_id,  
        "trigger_type":"10",
        "identity":"1",
        "temperature_status":"30",
        "power_status":"70"
      }
    }
    json_str=json.dumps(data_obj)
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

    data_obj={
     "d05":
      { 
        "device_id":d05_device_id,  
        "trigger_type":"8"
      }
    }
    json_str=json.dumps(data_obj)
    print("\n---------------------Security unset!!!--------------------\n")
    print("data str=" + json_str)
    r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
    time.sleep(2)

#---------------------------------------------------------------------
    
    print("Status Code:"+str(r.status_code) )
    print("----------------------------------------------------------")
    print(json.dumps(r.json()))
    print("----------------------------------------------------------")
    
#---------------------------------backcontrol test start

    datetime_obj=datetime.datetime.now()
    version =datetime_obj.strftime("%Y%m%d%H%M%S%f")
    print("version: " + version)

    #---------------------------------backcontrol security set
    payload1={
      "cusno": CUSTOMER_ID,
      "ver": version,
      "data": {
        "API": "http://211.22.242.13:8164/api/Security/Actionconfirm",
        "HostState": "1",
        "CheckDelayTime":"30",
        "StdDelayTime":"60",
        "CusState": "01" 
      }
    } 
    amigo_mqtt_pub(TOPIC, json.dumps(payload1))
    print("---------backcontrol security set-----------")
    time.sleep(2)

    #---------------------------------backcontrol battery set
    payload2={
      "cusno": CUSTOMER_ID,
      "ver": version,
      "data": {
        "Kind": "Device",
        "DeviceType": "d04",
        "battery": "70",
        "API": "http://211.22.242.13:8164/api/Security/BasicTU"
      }
    }
    amigo_mqtt_pub(TOPIC, json.dumps(payload2))
    print("----------backcontrol battery set-----------")
    time.sleep(2)

    #---------------------------------backcontrol security unset
    payload3={
      "cusno": CUSTOMER_ID,
      "ver": version,
      "data": {
        "API": "http://211.22.242.13:8164/api/Security/Actionconfirm",
        "HostState": "0",
        "CheckDelayTime":"30",
        "StdDelayTime":"60",
        "CusState": "01" 
      }
    }  
    amigo_mqtt_pub(TOPIC, json.dumps(payload3))
    print("---------backcontrol security unset---------")
    
#---------------------------------duration time
    end_timestamp=int(time.time())
    duration_time = end_timestamp - start_timestamp
    counter += 1
    print("test start time: " + str(start_timestamp))
    print("test end time:   " + str(end_timestamp))
    print("duration time: " + str(duration_time) + " seconds")
    print("counter: " + str(counter))
    time.sleep(5)
