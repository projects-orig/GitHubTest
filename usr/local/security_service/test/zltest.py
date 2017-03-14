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
# test only:
'''
data_obj={"testonly":""}
json_str=json.dumps(data_obj)

#print("data str=" + json_str)
hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
# http://stackoverflow.com/questions/3561381/custom-http-headers-naming-conventions
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/test/1/', headers=API_HEADER,  data=json_str)
'''
################################################################################
'''
# network config test:
data_obj={"networkconfigtest":""}
json_str=json.dumps(data_obj)

#print("data str=" + json_str)
hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
# http://stackoverflow.com/questions/3561381/custom-http-headers-naming-conventions
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/test/2/', headers=API_HEADER,  data=json_str)
'''
################################################################################

# antenna config test:
data_obj={"antennaconfigtest":""}
json_str=json.dumps(data_obj)

#print("data str=" + json_str)
hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
# http://stackoverflow.com/questions/3561381/custom-http-headers-naming-conventions
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/test/3/', headers=API_HEADER,  data=json_str)


################################################################################
# Engineering API test:


#json_str='{"host_name": "host", "static_ip": "192.168.159.111", "pppoe_pass": "123456", "pppoe_id": "amigo", "static_mask": "255.255.255.0", "alarm_contact_output_set": "0", "pin_code": "hjhhj", "connection_mac": "00aabb112233", "alarm_contact_output_action_time": "18", "alarm_action_time_set": "38", "customer_number": "11345", "phone_number": "06", "phone_password": "vbbbbbbbb", "battery_low_power_gap": "30", "phone_account": "amigo", "static_gate": "192.168.159.1", "urgent_bt_alarm_sound_set": "0", "v_contact_output_set": "0", "network_set": "6", "action_temperature_sensing": "80", "sim_password": "7978", "switch_without_network": "0", "battery_low_power_set": "70", "partial_transmission_time_when_disabled": "10", "v_output_contact_action_status": "0", "alarm_contact_output_action_status": "1", "action_temperature_sensing_gap": "10", "v_output_contact_action_time": "180", "partial_transmission_time_when_enabled": "60"}'

#json_str='{"HostSetting":[{"alarm_action_time_set": "38", "host_name": "host", "network_set": "6", "partial_transmission_time_when_disabled": "10", "static_gate": "192.168.159.1", "urgent_bt_alarm_sound_set": "0", "battery_low_power_gap": "30", "alarm_contact_output_action_status": "1", "static_ip": "192.168.159.111", "partial_transmission_time_when_enabled": "60", "time": "2016-12-06 18:07:38.712355", "pppoe_pass": "123456", "v_contact_output_set": "0", "v_output_contact_action_time": "180", "switch_without_network": "0", "action_temperature_sensing_gap": "10", "connection_mac": "00aabb112233", "phone_number": "06", "static_mask": "255.255.255.0", "battery_low_power_set": "70", "wifi_ssid": "test_5g", "customer_number": "11345", "alarm_contact_output_set": "0", "phone_account": "amigo", "pin_code": "hjhhj", "pppoe_id": "amigo", "action_temperature_sensing": "80", "wifi_pass": "66666", "sim_password": "7978", "alarm_contact_output_action_time": "18", "phone_password": "vbbbbbbbb", "v_output_contact_action_status": "0"}]}'

#data_obj=...

#data_obj={"CouplingLoop":[{"optical_coupling":"1","loop_id":["1","loop_4","5"],"status":"0"}]}
#data_obj={"Partition":[ {"partition_id":"1","loop_id":["1","loop_4"], "status":"0"} ]}

#data_obj={"RemoteControl_Partition":[{"device_id":"zl_05_01","partition_id":["1"]} ] }

#data_obj={"RemoteControl_LockReader":[{"device_id":"zl_05_01","reader_id":["zl_04_01","zl_04_02"]} ] }

'''
data_obj={"RemoteControl_PartitionReader":[
    {
      "device_id":"444", 
      "reader_array" :[
        {"reader_id":"123456", "partition_id":["1"]},
      ]                                        
    }
]}
'''

#data_obj={"Partition":[{"partition_id": "2", "loop_id":["0", "1"] }]}
#json_str=json.dumps(data_obj)

#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
# http://stackoverflow.com/questions/3561381/custom-http-headers-naming-conventions
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/new/', headers=API_HEADER,  data=json_str)


#data_obj={"LoopDevice":{"filter":{"device_id":"111"}, "update":{"loop_id":"2"}}}
#data_obj={"HostMacSetting":""}
#json_str=json.dumps(data_obj)
#print("data str=" + json_str)
#hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
#API_HEADER['sapido-hash']=hash_str
#API_HEADER['api-version']='0.01'
#r=requests.post(LOCALHOST_URL + '/api/get/', headers=API_HEADER,  data=json_str)

#json_str='{"HostSetting":{"update":{"host_name":"host", "static_ip": "192.168.159.111", "pppoe_pass": "123456", "pppoe_id": "amigo", "static_mask": "255.255.255.0", "alarm_contact_output_set": "0", "pin_code": "hjhhj", "connection_mac": "00aabb112233", "alarm_contact_output_action_time": "18", "alarm_action_time_set": "38", "customer_number": "11345", "phone_number": "06", "phone_password": "vbbbbbbbb", "battery_low_power_gap": "30", "phone_account": "amigo", "static_gate": "192.168.159.1", "urgent_bt_alarm_sound_set": "0", "v_contact_output_set": "0", "network_set": "6", "action_temperature_sensing": "80", "sim_password": "7978", "switch_without_network": "0", "battery_low_power_set": "70", "partial_transmission_time_when_disabled": "10", "v_output_contact_action_status": "0", "alarm_contact_output_action_status": "1", "action_temperature_sensing_gap": "10", "v_output_contact_action_time": "180", "partial_transmission_time_when_enabled": "60"}}}'
#data_obj={"HostSetting":{"filter":{},"update":{"host_name":"name_6"} }}
#data_obj={"HostMacSetting":{"update":{"enable_list":"0", "mac_list":["123456","112233"]}}}
#data_obj={"WLReadSensor":{"filter":{"device_id": "dev_444"}, "update":{ "nick_name": "reader ZL2"}}}
#data_obj={"WLReadSensor":{"filter":{"device_id": "dev_xx"}, "update":{ "nick_name": "reader ZL"}}}

#data_obj={"ServerAddress":{"update":{"server4": {"serialVersionUID": 0, "ftp_port": "23", "addr": "www.y4.com", "mqtt_port": "111", "iot_port": "222"}, "server2": {"serialVersionUID": 0, "ftp_port": "23", "addr": "www.y2.com", "mqtt_port": "111", "iot_port": "222"}, "customer_number": "0005581", "server1": {"serialVersionUID": 0, "ftp_port": "23", "addr": "www.y1.com", "mqtt_port": "111", "iot_port": "222"}, "server3": {"serialVersionUID": 0, "ftp_port": "23", "addr": "www.y3.com", "mqtt_port": "111", "iot_port": "222"}}}}
#data_obj={"Partition":{"filter":{"partition_id": "1"}, "update":{"loop_id":["0", "1"] }}}

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
#data_obj={"command":"update_validatecode","parameter":{"customer_number":CUSTOMER_ID,"group_id":"id123","validate_code":"abcde"}}
#data_obj={"command":"receive_signal","parameter":{"time":"", "customer_number":CUSTOMER_ID,"loop_id":"loop_4","type_id":"d04","device_id":"dev_444","setting_value":"","abnormal_value":"","alert_catalog":"A","abnormal_catalog":"A01","media_file_path":"","send_customer_status":"N","send_service_status":"Y"}}
#data_obj={"command":"login","auth_code":"9876", "username":"", "password":""}
#data_obj={"command":"set_customer_id","customer_id":"" }
#data_obj={"command":"get_customer_id" }
#data_obj={"command":"wifi_scan"}
#data_obj={"command":"test", "param":"sshd"}

'''
json_str=json.dumps(data_obj)
#print("data str=" + json_str)
hash_str = gen_hash(json_str)
#print("hash_str=" + hash_str)
API_HEADER['sapido-hash']=hash_str
API_HEADER['api-version']='0.01'
r=requests.post(LOCALHOST_URL + '/api/send/', headers=API_HEADER,  data=json_str)
'''

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

# 5_Get_Event	取得事件
# r=requests.post(LOCALHOST_URL + '/api/get/event/',json={"customer_number":CUSTOMER_ID,"group_id":"g111"})
#6_Get_EventSignal	取得訊號
# r=requests.post(LOCALHOST_URL + '/api/get/eventsignal/',json={"customer_number":CUSTOMER_ID,"group_id":"g111"})


################################################################################                                                                               
print("Status Code:"+str(r.status_code) )
print("----------------------------------------------------------")
print(r.json())

print("----------------------------------------------------------")
