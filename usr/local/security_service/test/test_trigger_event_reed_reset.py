#coding=UTF-8

import requests
import datetime
import time
import json

LOCALHOST_URL='http://127.0.0.1:5001'

API_HEADER = {'Content-Type': 'application/json'}



############################
# d02 = 無線磁簧感測器, 9=磁簧復歸
data_obj={
 "d02":
  { 
    "device_id":"5149013103542656",  
    "trigger_type":"9"
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
