#coding=UTF-8

import requests
import datetime
import time
import json

LOCALHOST_URL='http://127.0.0.1:5001'

API_HEADER = {'Content-Type': 'application/json'}



############################
# d03 = WLDoubleBondBTemperatureSensor, 8=disconnect
data_obj={
 "d03":
  { 
    "device_id":"5149013103542777",  
    "trigger_type":"8"
  }
}

json_str=json.dumps(data_obj)

##########################################################################################

print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)

print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(json.dumps(r.json()))

print("----------------------------------------------------------")
