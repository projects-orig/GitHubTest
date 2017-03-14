#coding=UTF-8

import requests
import datetime
import time
import json

LOCALHOST_URL='http://127.0.0.1:5001'

API_HEADER = {'Content-Type': 'application/json'}



############################
# d02 = 無線磁簧感測器, 8=磁簧觸動
data_obj={
 "d02":
  { 
    "device_id":"5149013115785703",  
    "trigger_type":"8"
  }
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
time.sleep(2)

# d04 = 無線雙向式讀訊器, 8=壓扣鈕
data_obj={
 "d04":
  { 
    "device_id":"5149013103542531",  
    "trigger_type":"8"
  }
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
time.sleep(2)

# d05 = 無線雙向遙控器, 7=巡查
data_obj={
 "d05":
  { 
    "device_id":"5149013115785625",   
                 
    "trigger_type":"7"
  }
}
############################

'''
# d05 = 無線雙向遙控器, 6=緊急按鍵
data_obj={
 "d05":
  { 
    "device_id":"555",  
    "trigger_type":"6"
  }
}
'''

'''
# device_connect
data_obj={
 "d04":
  { 
    "device_id":"123456",  
    "trigger_type":"2"
  }
}
'''

'''
# d05 = 無線雙向遙控器, 3=設解按鍵
data_obj={
 "d05":
  { 
    "device_id":"555",  
    "trigger_type":"3"
  }
}
'''

json_str=json.dumps(data_obj)
##########################################################################################

print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)

print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(json.dumps(r.json()))

print("----------------------------------------------------------")
