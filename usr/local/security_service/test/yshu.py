#coding=UTF-8

import requests
import datetime

import json
import hashlib

LOCALHOST_URL='http://127.0.0.1:5001'
JSON_FILE_PATH = '/public/'
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

################################################################################
# Engineering API test:

#json_str='{"Loop":[{"time":"","version":"xx","customer_number":'+ CUSTOMER_ID + ',"loop_id":"loop_999","status":"2"}],"Device":[ {"time":"","version":"xx","customer_number":' + CUSTOMER_ID + ',"type_id":"d02","device_id":"dev_xxx","identity":"001122334455"}]}'
#json_str='{"CouplingLoop":[ {"version": "xx", "optical_coupling": "optical_111", "customer_number": 87654321, "loop_id": "1", "status": "0", "time": "2016-11-17 14:56:24.407987"}, {"time":"","version":"xx","customer_number":' + CUSTOMER_ID + ',"optical_coupling":"optical_222","loop_id":"6","status":"0"}]}'
#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
# http://stackoverflow.com/questions/3561381/custom-http-headers-naming-conventions
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/new/', headers=API_HEADER,  data=json_str)
#json_str='{"HostCamPir":[{"device_id":"111","enable_pir":"1"},{"device_id":"222","enable_pir":"0"},{"device_id":"333","enable_pir":"1"},]}'

#json_str='{"Loop":{"filter":{"loop_id":"loop_2"}}, "Device":{"filter":{"device_id":"dev_12x"}}}'
#json_str='{"Loop":"", "Device":{"filter":{"device_id":"dev_123"}}}'
#json_str='{"HostSetting":""}'
#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/get/', headers=API_HEADER,  data=json_str)

#
#data_obj={"HostSetting":{"filter":{},"update":{"host_name":"name_6"} }}
#json_str=json.dumps(data_obj)
#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/update/', headers=API_HEADER,  data=json_str)


#json_str='{"Loop":{"filter":{"loop_id":"loop_2"}}}'
#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/delete/', headers=API_HEADER,  data=json_str)



#json_str='{"command":"login","username":"admin","password":"admin"}'
#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/send/', headers=API_HEADER,  data=json_str)


#data_obj={"command":"get_event", "parameter":{"customer_number":CUSTOMER_ID,"group_id":"g123"}}
#data_obj={"command":"get_validatecode","parameter":{"customer_number":"1234", "group_id":"11111"}}
#data_obj={"command":"get_validatecode","parameter":{"customer_number":"1234", "group_id":"123420161116002"}}
#data_obj={"command":"update_validatecode","parameter":{"customer_number":CUSTOMER_ID,"group_id":"id123","validate_code":"abcde"}}
#data_obj={"command":"receive_signal","parameter":{"time":"", "customer_number":CUSTOMER_ID,"loop_id":"loop_4","type_id":"d04","device_id":"dev_444","setting_value":"","abnormal_value":"","alert_catalog":"A","abnormal_catalog":"A01","media_file_path":"","send_customer_status":"N","send_service_status":"Y"}}
#data_obj={"command":"login","auth_code":"abcde", "username":"admin", "password":"admin"}
#data_obj={"command":"set_customer_id","customer_id":"" }
#data_obj={"HostCamPir":[{"device_id":"111","enable_pir":"1"},{"device_id":"222","enable_pir":"0"},{"device_id":"333","enable_pir":"1"},]}

#data_obj={"command":"get_customer_id" }

#data_obj={"CustomerIP":[{"time":"","version":"","ip1":"","ip2":"","ip3":"","ip4":""}]}
#data_obj={"RemoteControl_Partition":{"filter":{"device_id":"444"}}}

#data_obj={"HostSetting":{"filter":{},"update":{"host_name":"name_6"} }}
#json_str=json.dumps(data_obj)
#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/update/', headers=API_HEADER,  data=json_str)

#data_obj={"ServerAddress":{"filter":{},"update":{
#"server1": {"addr": "www.evergreenX.com.tw","iot_port":"1231","mqtt_port":"1456","ftp_port":"1789"}, 
#"server2": {"addr": "www.customer3.com.tw","iot_port":"1232","mqtt_port":"2456","ftp_port":"2789"}, 
#"server3": {"addr": "www.customer1.com.tw","iot_port":"1233","mqtt_port":"3456","ftp_port":"3789"}, 
#"server4": {"addr": "www.customer2.com.tw","iot_port":"1234","mqtt_port":"4456","ftp_port":"4789"}
#}}}
data_obj={"command":"get_customer_id"}
json_str=json.dumps(data_obj)
#print("data str=" + json_str)
hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/api/send/', headers=API_HEADER,  data=json_str)


################################################################################
# Signal API test:
# 20161109 note: not necessary to test for now

# 1 ReceiveSignal:	接收異常事件
# r=requests.post(LOCALHOST_URL + '/api/receive/signal/', json={"customer_number":CUSTOMER_ID,"loop_id":"loop_4","type_id":"d04","device_id":"dev_444","setting_value":"","abnormal_value":"","alert_catalog":"A","abnormal_catalog":"A01","media_file_path":"","send_customer_status":"N","send_service_status":"Y"})
# ==> server message:{'status': 501, 'message': '2016/11/9 下午 02:32:59:ReceiveSignal:並未將物件參考設定為物件的執行個體。InnerException::'}
# 2_Update_ValidateCode	更新驗證碼
# group_id=??
# r=requests.post(LOCALHOST_URL + '/api/update/validatecode/', json={"customer_number":CUSTOMER_ID,"group_id":"g111","validate_code":"12345"})
# 3_Update_EventStatus	更新事件狀態(結案)
# group_id=??
# r=requests.post(LOCALHOST_URL + '/api/update/eventstatus/', json={"customer_number":CUSTOMER_ID,"group_id":"g111"})
# 4_Get_ValidateCode	取得事件驗證碼
# r=requests.post(LOCALHOST_URL + '/api/get/validatecode/',json={"customer_number":CUSTOMER_ID,"group_id":"g111"})
# 5_Get_Event	取得事件
# r=requests.post(LOCALHOST_URL + '/api/get/event/',json={"customer_number":CUSTOMER_ID,"group_id":"g111"})
#6_Get_EventSignal	取得訊號
# r=requests.post(LOCALHOST_URL + '/api/get/eventsignal/',json={"customer_number":CUSTOMER_ID,"group_id":"g111"})


################################################################################                                                                               
print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(r.json())

print("----------------------------------------------------------")
