#!/usr/bin/python

import sys
import paho.mqtt.publish as publish
import paho.mqtt.client as mqtt
import json

JSON_FILE_PATH = '/usr/local/share/security_service/'

CUSTOMER_ID=''
try:
  with open(JSON_FILE_PATH + "customer_id.json", "r") as f:
    file_content=json.load(f)
    CUSTOMER_ID=file_content['customer_id']
except IOError:
  print ("[customer_id.json] file doesn't exist...")


# test only
#CUSTOMER_ID='0005571'



print("customer_id=" + CUSTOMER_ID)

TOPIC = 'H' + CUSTOMER_ID

#MQTT_HOST = "fs1p.etopnetwork.com.tw"
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

    
payload={
  "cusno": CUSTOMER_ID,
  "ver": "2017-01-12 11:40:37.585",
  "data": {
    "Kind": "Device",
    "DeviceType": "d04",
    "battery": "70",
    "API": "http://211.22.242.13:8164/api/Security/BasicTU"
  }
}
 
amigo_mqtt_pub(TOPIC, json.dumps(payload))

