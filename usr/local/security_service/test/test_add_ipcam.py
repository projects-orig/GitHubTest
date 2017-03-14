#coding=UTF-8

import requests
import datetime

import json
import hashlib

LOCALHOST_URL='http://127.0.0.1:5001'
#LOCALHOST_URL='http://192.168.159.159:5001'
JSON_FILE_PATH = '/usr/local/share/security_service/'
CUSTOMER_ID=''
API_HEADER = {'Content-Type': 'application/json'}

try:
  with open(JSON_FILE_PATH + "customer_id.json", "r") as f:
    file_content=json.load(f)
    CUSTOMER_ID=file_content['customer_id']
except IOError:
  print ("file doesn't exist. ignore it...")

################################################################################

data_obj={
      "device_id": "113355",
      "private_port": "222",
      "public_port": "1111",
      "private_ip": "192.168.0.101",
      "public_ip": "192.168.1.112",
      "user_name": "admin",
      "password": "admin"
}

json_str=json.dumps(data_obj)
r=requests.post(LOCALHOST_URL + '/ipcam/open_port/', headers=API_HEADER,  data=json_str)

################################################################################                                                                               
print("data_obj: " + str(data_obj))
print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(r.json())

print("----------------------------------------------------------")
