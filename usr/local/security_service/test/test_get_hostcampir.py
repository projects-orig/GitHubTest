#coding=UTF-8

import requests
import datetime

import json
import hashlib

LOCALHOST_URL='http://127.0.0.1:5001'

JSON_FILE_PATH = '/usr/local/share/security_service/'
CUSTOMER_ID=''
API_HEADER = {'Content-Type': 'application/json'}

################################################################################

API_HEADER['api-version']='0.01'
r=requests.get(LOCALHOST_URL + '/ipcam/get_host_cam_pir/', headers=API_HEADER)

################################################################################                                                                               

print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(r.json())

print("----------------------------------------------------------")
