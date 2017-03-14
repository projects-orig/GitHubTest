#coding=UTF-8

import requests
import datetime
import time
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


# 128-byte random string by http://www.unit-conversion.info/texttools/random-string-generator/
HASH_SECRET='rEwIdbB9aHFHvdAheX4h7uNuh2fkNBr3WpDfFnyUqXipzOv2AVeKejc2ixffZARNAgNwCX678n0ZzrN6kRwGgd2xvxdJ17rzbRGO3KTAQRtWcWho47KAemZ8VBIwVKri'

def blending():
    customer_id=CUSTOMER_ID
    customer_id_len=len(customer_id)
    if customer_id_len > 0 :
        tmp_buffer=''
        index=0
        for bbb in HASH_SECRET:
           #print(bbb)
           # http://stackoverflow.com/questions/227459/ascii-value-of-a-character-in-python
           tmp_buffer += chr(ord(bbb) ^ ord(customer_id[index % customer_id_len])) 
           index += 1
           
        #print(tmp_buffer)            
        return tmp_buffer.encode('utf-8')  
    else:
        return HASH_SECRET.encode('utf-8')                


def gen_hash(data_Str):
    return gen_sha256(data_Str)


# http://pythoncentral.io/hashing-strings-with-python/
def gen_sha256(data_str):
    hash_object = hashlib.sha256(data_str.encode('utf-8') + blending())
    return hash_object.hexdigest()
    

def gen_md5(data_str):
    m = hashlib.md5()
    m.update((data_str+HASH_SECRET).encode('utf-8'))
    return m.hexdigest() 

def print_status():
    print("----------------------------------------------------------")
    print("Status Code:"+str(r.status_code) )
    print(r.json())
    print("----------------------------------------------------------")

d06_device_id = "000023344588"

data_obj={

    "device_id":d06_device_id,
    "abnormal_value":"10"
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/ipcam/low_battery/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/ipcam/reboot/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/ipcam/power_fail/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/ipcam/wifi_error/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/ipcam/reconnect_to_host/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/ipcam/disconnect_to_host/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

data_obj={

    "device_id":d06_device_id,
    "setting_value":"720P"
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/ipcam/resolution_setting/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)