#coding=UTF-8

import requests
import datetime
import time
import json

LOCALHOST_URL='http://127.0.0.1:5001'

API_HEADER = {'Content-Type': 'application/json'}


# d05 = 無線雙向遙控器, 11=urgent stop
data_obj={
 "d05":
  { 
    "device_id":"5149013115785703",  
    "trigger_type":"11"
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
