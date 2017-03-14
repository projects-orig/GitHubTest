#coding=UTF-8

import requests
import datetime

import json

LOCALHOST_URL='http://127.0.0.1:5001'

API_HEADER = {'Content-Type': 'application/json'}

data_obj={
  "d02":{
    "device_id": "avod",
    "signal_power_status": "22", 
    #"connection_status": "1"
  }
}

json_str=json.dumps(data_obj)
##########################################################################################

print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/update/device_status/', headers=API_HEADER,  data=json_str)

print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(json.dumps(r.json()))

print("----------------------------------------------------------")