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

d02_device_id = "5149013103542656"
d03_device_id = "5149013103542553"
d04_device_id = "5149013103542545"
d05_device_id = "5149013115785703"

################################################################################
#login

data_obj={"command":"login","auth_code":"9876", "username":"", "password":""}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/api/send/', headers=API_HEADER,  data=json_str)
print("\n<<<<< Start login >>>>>")
print_status()

################################################################################
#set customer_id

data_obj={"command":"set_customer_id","customer_id":"0005829" }
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/api/send/', headers=API_HEADER,  data=json_str)
print("\n<<<<< set customer_id >>>>>")
print_status()

################################################################################
CUSTOMER_ID=data_obj["customer_id"]

################################################################################
# host registration/update
print("\n<<<<< host registration >>>>>")
data_obj={
  "HostSetting":{
    "update":{
      'alarm_action_time_set':'3'
    }
  }
}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/api/update/', headers=API_HEADER,  data=json_str)
print_status()


################################################################################
#device connect

print("\n<<<<< Devices connecting >>>>>")
data_obj={
  "d02":{
    "device_id":d02_device_id,
    "trigger_type":"2",
    "identity":"1"
  }
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

data_obj={
 "d03":
  { 
    "device_id":d03_device_id,  
    "trigger_type":"2",
    "identity":"1"
  }
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

data_obj={
 "d04":
  { 
    "device_id":d04_device_id,  
    "trigger_type":"2",
    "identity":"1"
  }
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

data_obj={
 "d05":
  { 
    "device_id":d05_device_id,  
    "trigger_type":"2",
    "identity":"1"
  }
}
json_str=json.dumps(data_obj)
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/trigger/event/', headers=API_HEADER,  data=json_str)
print_status()

################################################################################
#add loopdevice

data_obj={"register_all_devices":""}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/test/1/', headers=API_HEADER,  data=json_str)
print("\n<<<<< Register devices >>>>>")
print_status()

# add loops
data_obj={"Loop":[{"loop_id":"2", "status":"0"}, {"loop_id":"3", "status":"0"}, {"loop_id":"4", "status":"0"}]}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/api/new/', headers=API_HEADER,  data=json_str)
print("\n<<<<< Add loops >>>>> \n" + json_str)
print_status()

# add device to loop
data_obj={"LoopDevice":[
    {"loop_id":"2", "device_id":d02_device_id},
    {"loop_id":"3", "device_id":d03_device_id},
    {"loop_id":"4", "device_id":d04_device_id}
]}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/api/new/', headers=API_HEADER,  data=json_str)
print("\n<<<<< Add device to loop >>>>> \n" + json_str)                
print_status()

################################################################################
#device setting

print("\n<<<<< Devices setting >>>>>")
data_obj={
  "WLReedSensor":{
    "filter":{'device_id':d02_device_id},
    "update":{
    'temperature_sensing':'70',
    'temperature_sensing_gap':'5',
    'battery_low_power_set':'10',
    'battery_low_power_gap':'5'
    }
  }
}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/api/update/', headers=API_HEADER,  data=json_str)
print_status()

data_obj={
  "WLDoubleBondBTemperatureSensor":{
    "filter":{'device_id':d03_device_id},
    "update":{
    'temperature_sensing':'70',
    'temperature_sensing_gap':'5',
    'battery_low_power_set':'10',
    'battery_low_power_gap':'5',
    'microwave_sensitivity_set': '25', 
    'G_sensor_status': '1',
    'msg_send_strong_set_manual':'100%'
    }
  }
}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/api/update/', headers=API_HEADER,  data=json_str)
print_status()
time.sleep(2)

data_obj={
  "WLReadSensor":{
    "filter":{'device_id':d04_device_id},
    "update":{
    'temperature_sensing':'70',
    'temperature_sensing_gap':'5',
    'battery_low_power_set':'10',
    'battery_low_power_gap':'5',
    'speaker_enable': '1',
    }
  }
}
json_str=json.dumps(data_obj)
hash_str = gen_hash(json_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
print("data str=" + json_str)
r=requests.post(LOCALHOST_URL + '/api/update/', headers=API_HEADER,  data=json_str)
print_status()

################################################################################
