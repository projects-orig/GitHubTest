#coding=UTF-8

import requests
import datetime

import json

LOCALHOST_URL='http://127.0.0.1:5001'
JSON_FILE_PATH = '/usr/local/share/security_service/'

API_HEADER = {'Content-Type': 'application/json'}


################################################################################
# test example#1: read json from file
'''
with open(JSON_FILE_PATH + "receive_signal.json", "r") as f:
  file_content = json.load(f)
file_content["time"]=str(datetime.datetime.now())
json_str=json.dumps(file_content)
'''
##########################################################################################
# test example#2: build json by python data structure 

data_obj={
  #溫度異常
  "d01":{
    "alert_catalog": "A",
    "abnormal_catalog": "GA07",
    "setting_value": "30",
    "abnormal_value": "50",
  }
}

'''
data_obj={
  #機蓋開啟
  "d01":{
    "alert_catalog": "A",
    "abnormal_catalog": "GA05",
  }
}


data_obj={
  "d02":{
  	"time": str(datetime.datetime.now()),
  	"device_id":"avod",
    "alert_catalog":"A",
    "abnormal_catalog":"DA07",
    "abnormal_value":"88",
  }
}

data_obj={
  "d03":{
  	"time": str(datetime.datetime.now()),
  	"device_id":"949494",
    "alert_catalog":"A",
    "abnormal_catalog":"AA08",
    "abnormal_value":"88",
  }
}

#無線雙向式遙控器
data_obj={
  #緊急求救
  "d05":{
    "device_id":"444",
    "alert_catalog": "A",
    "abnormal_catalog": "KA01",
    "abnormal_value": "50"
  }
}
'''
json_str=json.dumps(data_obj)
##########################################################################################

print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/receive/signal/', headers=API_HEADER,  data=json_str)

print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(json.dumps(r.json()))

print("----------------------------------------------------------")