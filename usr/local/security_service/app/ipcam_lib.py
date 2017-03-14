#coding=UTF-8

import sys
import time
import datetime
import requests
import json
import hashlib
import ftplib

import signal
import os
from collections import OrderedDict

from flask import Flask, jsonify, abort, request
from flask import render_template, flash, redirect

import app.views, app.server_lib, app.device_lib, app.host_lib

'''
# abnormal_catalog:
GI01 改為 PG01:IPCAM與主機回覆傳訊
GI02 改為 PG02:IPCAM解析度設定
GI03 改為 PG03:IPCAM影像儲存
IA01 改為 PA01:IPCAM與主機失去訊號
IA02 改為 PA02:IPCAM紅外線異常
IA03 改為 PA03:IPCAM電池低壓
IA04 改為 PA04:IPCAM重新啟動
IA05 改為 PA05:IPCAM斷電
IA06 改為 PA06:IPCAM WiFi異常
'''

def ipcam_open_port(record):
    print("record type" + str(type(record)))
    print("record: " + str(record))

    #update Device.json file
    device_id=record["device_id"]
    type_id="d06"
    app.device_lib.update_device_to_Device_json(device_id,type_id)

    #register device to server
    app.server_lib.register_device_to_server(device_id)

    #update WLIPCam.json
    table_name="WLIPCam"
    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
            file_content=json.load(f)
            file_content_list=file_content[table_name]
    except IOError:
        print ("the given file doesn't exist. create EMPTY list")
        file_content=dict()
        file_content[table_name]=list()
        file_content_list=file_content[table_name]

    item_found=False
    for element in file_content_list:
        if device_id==element["device_id"]:
            item_found=True
            element_found=element
            print("device_id found: " + str(device_id))
            break

    if item_found==False:
        # add default setting for new IPCam
        record["frame"]="30"
        record["pixel"]="720P"
        record["e_before_seconds"]="30"
        record["e_after_seconds"]="5"
        record["real_time_monitor"]="1"
        record["media_upload_set"]="1"
        record["PIR_set"]="1"
        record["battery_low_power_set"]="20"
        record["battery_low_power_gap"]="5"
        
        file_content[table_name].append(record)
        
    else:
        #print("element_found: " + str(element_found))
        element_found["private_port"]=record["private_port"]
        element_found["public_port"]=record["public_port"]
        element_found["private_ip"]=record["private_ip"]
        element_found["public_ip"]=record["public_ip"]
        element_found["user_name"]=record["user_name"]
        element_found["password"]=record["password"]

    app.views.save_file_content(table_name, file_content)
    
    app.host_lib.init_host_cam_pir(device_id)
    
    # update to server
    app.server_lib.update_to_server(table_name, record)
   
    return True

def start_login(device_id):
    
    print("Start IPCam login !!!")
    
    table_name='WLIPCam'
    #get ip address
    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
            file_content=json.load(f)
            file_content_list=file_content[table_name]
    except IOError:
        print ("the given file doesn't exist. Return error")

    item_found=False
    try:
      for element in file_content_list:
        if device_id==element["device_id"]:
            item_found=True
            private_ip_addr=element["private_ip"]
            username=element["user_name"]
            password=element["password"]
            break
    except:
      print("[start_login] unknown error.",  flush=True)
      item_found=False
      
    if not item_found:
        print("Device is not register!!!")
        print("item_found= " + str(item_found))
        token=""
        private_ip_addr=""
        return token, private_ip_addr

    url = 'http://' + private_ip_addr + '/api/1.0/login.json'
    payload = '{"username":"' + username + '","password":"' + (hashlib.sha1(password.encode("utf-8")).hexdigest()) + '"}'
    print(url,  flush=True)
    print("payload=" + str(payload),  flush=True)
    API_HEADER = dict()
    API_HEADER['Content-Type']='application/json;charset=utf-8'
    API_HEADER['Set-Cookie']='token=null'
    
    requests_type = 'post'
    response=requests_select(requests_type,url,API_HEADER,payload)
    if "token" not in response:
        token=""
        print("[start_login] IPCam lost connection from Host, device_id= " + str(device_id))
        signal_obj=dict()
        signal_obj["device_id"]=device_id
        signal_obj["type_id"]="d06"
        signal_obj["alert_catalog"]="A"
        signal_obj["abnormal_catalog"]="PA01"
        obj=app.server_lib.trigger_to_server(signal_obj)
    else:
        token=response["token"]

    return token,private_ip_addr

def update_ipcam_setting_value(data_obj):
    
    print("Start setting IPCam !!!")
    
    device_id=data_obj["device_id"]
    token,private_ip_addr=start_login(device_id)

    if token == "":
        print("[update_ipcam_setting_value] IPCam login fail, skip setting!!!")
        return False
    
    print("data_obj in update_ipcam_setting: " + str(data_obj))

    profile_name = 'baseline'
    stream_profile = 'main_profile'
    parameter_assign(token,private_ip_addr,profile_name,stream_profile,data_obj)

    stream_profile = 'sub_profile'
    parameter_assign(token,private_ip_addr,profile_name,stream_profile,data_obj)

    if "pixel" in data_obj:
        resolution_setting()

    return True 

def parameter_assign(token,private_ip_addr,profile_name,stream_profile,data_obj):
    
    parameter_L1=dict()
    parameter_L2=""
    new_token = token
    response = None
    #print("data_obj in parameter_assign:" + str(data_obj))

    for parameter_L1 in data_obj:
        parameter_value=""
        parameter_dict=dict()
        json_to_write =""

        #print("parameter_L1: " + str(parameter_L1))
        parameter_value = data_obj[parameter_L1]
        #print("parameter_value: " + str(parameter_value))

        #1.Check parameter in WLIPcam.json
        #2.Assign parameter to the target json file
        #----------------------------------------------------------event_video.json
        if parameter_L1 == 'e_before_seconds':
            parameter_L1 = 'before_sec'
            json_to_write = 'event_video.json'
        elif parameter_L1 == 'e_after_seconds':
            parameter_L1 = 'after_sec'
            json_to_write = 'event_video.json'
        elif parameter_L1 == 'media_upload_set':
            parameter_L1 = 'upload'
            json_to_write = 'event_video.json'

        #----------------------------------------------------------preview.json
        elif parameter_L1 == 'real_time_monitor':
            parameter_L1 = 'enable'
            json_to_write = 'preview.json'

        #----------------------------------------------------------pir_setting.json
        elif parameter_L1 == 'PIR_set':
            parameter_L1 = 'infrared'
            json_to_write = 'pir_setting.json'

        #----------------------------------------------------------electricity.json
        elif parameter_L1 == 'battery_low_power_set':
            parameter_L1 = 'low_power'
            json_to_write = 'electricity.json'
        elif parameter_L1 == 'battery_low_power_gap':
            parameter_L1 = 'low_power_gap'
            json_to_write = 'electricity.json'

        #----------------------------------------------------------video.json
        elif parameter_L1 == 'frame':
            parameter_L1 = 'frame_rate'
            parameter_L2 = stream_profile
            json_to_write = 'video.json'
        elif parameter_L1 == 'pixel':
            parameter_L1 = 'resolution'
            parameter_L2 = stream_profile
            json_to_write = 'video.json'
        else:
            print("Skip " + str(parameter_L1))
            continue       

        if parameter_L1 == 'frame_rate':
            parameter_dict[parameter_L1] = int(parameter_value)
        elif parameter_L1 == 'infrared' or parameter_L1 == 'enable' or parameter_L1 == 'upload':
            if parameter_value == "1":
                parameter_dict[parameter_L1] = True
            else:
                parameter_dict[parameter_L1] = False
        else:
            parameter_dict[parameter_L1] = parameter_value
        #print("parameter_dict: " + str(parameter_dict))        
    
        json_str=json.dumps(parameter_dict)
        print("json_str: " + str(json_str))
        
        
        url = 'http://'+ private_ip_addr +'/api/1.0/' + json_to_write

        #payload = '{"items":{"profile":"main","flip":"1","mirror":"1","main_profile":{"frame_rate":"30"}}}'
        if parameter_L2 != "" and json_to_write == 'video.json':
            payload = '{"items":' +'{"' + parameter_L2 + '":' + json_str + '}}'
        else:
            payload = '{"items":' + json_str + '}'

        print(url,  flush=True)
        print("payload=" + str(payload),  flush=True)
        print("new_token=" + str(new_token))
        API_HEADER = dict()
        API_HEADER['Content-Type']='application/json;charset=utf-8'
        API_HEADER['Set-Cookie']='token='+new_token

        requests_type = 'put'
        response=requests_select(requests_type,url,API_HEADER,payload)

    return response
    
def get_ipcam_info(device_id):

    if not request.json:
        return jsonify({'error_code': 3, 'error_msg': "Incorrect json format."}), 501

    print("Getting IPCam info~~~")

    token,private_ip_addr=start_login(device_id)

    url = 'http://'+ private_ip_addr +'/api/1.0/video.json'
    
    payload = "?items[]=profile&items[]=flip&items[]=mirror&items[]=main_profile&items[]=sub_profile"

    new_url=url+payload
    new_token = token
    print("new_url= " + str(new_url))
    
    print(url,  flush=True)
    #print("payload= " + str(payload),  flush=True)
    print("new_token=" + str(new_token))
    API_HEADER = dict()
    API_HEADER['Content-Type']='application/json;charset=utf-8'
    API_HEADER['Set-Cookie']='token='+new_token
    requests_type = 'get'
    response=requests_select(requests_type,new_url,API_HEADER,payload)

    return response

def requests_select(requests_type,url,API_HEADER,payload):

    print("requests_type= " + str(requests_type))
    try:
        if requests_type == 'put':
            r = requests.put(url, data=payload, headers=API_HEADER, timeout=2)
        elif requests_type == 'post':
            r = requests.post(url, data=payload, headers=API_HEADER, timeout=2)
        elif requests_type == 'get':
            r = requests.get(url, headers=API_HEADER, timeout=2)

        print("Host message:",  flush=True) 
        print(r.content.decode("utf-8"),  flush=True)
        print("Host message end---------------")
        obj = r.json()

        if "status" not in obj:
          obj["status"]=r.status_code  
        if "message" not in obj:
          obj["message"]=r.content.decode('utf-8')

    except ValueError as e:
        print("error: %s" %str(e),  flush=True)
        obj = {"status": 501, "message":"server error"}
    except requests.exceptions.ConnectionError as e:
        print("error: %s" %str(e),  flush=True)      
        obj = {"status": 501, "message":"network connection error"} 
    except requests.exceptions.RequestException as e:
        print("error: %s" %str(e),  flush=True)
        obj = {"status": 501, "message":"request exception"}
    except UnicodeEncodeError as e:
        print("error: %s" %str(e),  flush=True)
        obj = {"status": 501, "message":"Unicode display error"}
    except:         
        print("Unknown error",  flush=True)    
        obj = {"status": 501, "message":"Unknown error"}

    return obj

def update_video_url(data_obj):

    # 更新事件影像檔路徑
    # URL	http://211.22.242.13:8081/update/mediafilepath
    # Request (json)value={"RequestAttr":{"time":"時間","customer_number":"管制編號","event_id":"辨識事件id","device_id":"IPCAM_id","url":"ftp路徑"}}
    
    print("[update_video_url] Update video URL~~~")

    update_content=dict()
    datetime_obj=datetime.datetime.now()
    update_content["time"]=str(datetime_obj)
    update_content["customer_number"]=app.views.get_customer_number()

    if "event_id" in data_obj:
        update_content["event_id"]=data_obj["event_id"]
    else:
        # trigger IPCam pir event to server (event_id is created in trigger_to_server())
        print("Get event_id~~~")
        pir_abnormal()
        update_content["event_id"]= app.host_lib.get_event_id()
        
    update_content["device_id"]=data_obj["device_id"]
    update_content["url"]=data_obj["media_file_path"]

    app.server_lib.update_media_file_path_to_server(update_content)

    # trigger IPCam-video-upload event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="PG03"
    #signal_obj["abnormal_value"]=""
    #signal_obj["setting_value"]=""
    obj=app.server_lib.trigger_to_server(signal_obj)
    
    return 

#--------------------------------------
#trigger event PA01~PA06 / PG01~PG03

def disconnect_to_host(data_obj): 

    table_name="Device"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
      
      item_found=False
      connection_status = "1"
      for dev_item in device_list:
        if dev_item["device_id"] == data_obj["device_id"]:
            dev_item["connection_status"] = connection_status
            print("[ipcam_disconnect_to_host] set connection status to 1")
            item_found=True
            break
      
      if item_found:  
        with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
          json.dump(file_content, f)
      else:
        print("[ipcam_disconnect_to_host] device is NOT found ???")
    except IOError:
      print("[ipcam_disconnect_to_host] [" + table_name + ".json] unknown error => skip this operation")
      return

    print("[ipcam_disconnect_to_host] IPCam disconnect to host(PA01)!!!")

    # trigger IPCam low battery event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="PA01"
    #signal_obj["abnormal_value"]=""
    #signal_obj["setting_value"]=""
    obj=app.server_lib.trigger_to_server(signal_obj)

def pir_abnormal(data_obj):

    print("[ipcam_pir_abnormal] IPCam pir_abnormal(PA02)!!!")

    # trigger IPCam low battery event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="PA02"

    obj=app.server_lib.trigger_to_server(signal_obj)
    
    return

def low_battery(data_obj):

    print("[ipcam_low_battery] set abnormal_status 3rd bit to 1")
    device_id = data_obj["device_id"]
    app.device_lib.set_sensor_abnormal_status(device_id,2,1)

    print("[ipcam_low_battery] IPCam low battery alert(PA03)!!!")

    # trigger IPCam low battery event to server
    table_name='WLIPCam'
    file_content_list=list()

    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
            file_content=json.load(f)
            file_content_list=file_content[table_name]
    except IOError:
        print ("the given file doesn't exist. Return error")
        return 
        
    for element in file_content_list:
        if device_id == element["device_id"]:
            try:
                battery_low_power_set=element["battery_low_power_set"]
            except:
                print("[ipcam_low_battery] Missing battery_low_power_set value")
                battery_low_power_set = ""
            break
            
    signal_obj=dict()
    signal_obj["setting_value"]=battery_low_power_set
    signal_obj["device_id"]=device_id
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="PA03"
    try:
        signal_obj["abnormal_value"] = data_obj["abnormal_value"]
    except:
        print("[ipcam_low_battery] Missing abnormal_value!!!")

    obj=app.server_lib.trigger_to_server(signal_obj)
    
    return

def battery_back_to_normal(data_obj):

    print("[ipcam_battery_back_to_normal] Battery back to normal")
    print("[ipcam_battery_back_to_normal] set abnormal_status 3rd bit to 0")
    device_id = data_obj["device_id"]
    app.device_lib.set_sensor_abnormal_status(device_id,2,0)
    return

def reboot(data_obj):

    print("[ipcam_reboot] IPCam reboot(PA04)!!!")

    # trigger IPCam low battery event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="PA04"
    #signal_obj["abnormal_value"]=""
    #signal_obj["setting_value"]=""
    obj=app.server_lib.trigger_to_server(signal_obj)
    
    return

def power_fail(data_obj):

    print("[ipcam_power_fail] IPCam power fail(PA05)!!!")

    # trigger IPCam low battery event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="PA05"
    #signal_obj["abnormal_value"]=""
    #signal_obj["setting_value"]=""
    obj=app.server_lib.trigger_to_server(signal_obj)

def wifi_error(data_obj):

    print("[ipcam_wifi_error] IPCam WIFI error(PA06)!!!")

    # trigger IPCam low battery event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="PA06"
    #signal_obj["abnormal_value"]=""
    #signal_obj["setting_value"]=""
    obj=app.server_lib.trigger_to_server(signal_obj)

def reconnect_to_host(data_obj): 

    table_name="Device"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
      
      item_found=False
      connection_status = "0"
      for dev_item in device_list:
        if dev_item["device_id"] == data_obj["device_id"]:
            dev_item["connection_status"] = connection_status
            print("[ipcam_reconnect_to_host] set connection status to 0")
            item_found=True
            break
      
      if item_found:  
        with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
          json.dump(file_content, f)
      else:
        print("[ipcam_reconnect_to_host] device is NOT found ???")
    except IOError:
      print("[ipcam_reconnect_to_host] [" + table_name + ".json] unknown error => skip this operation")
      return

    print("[ipcam_reconnect_to_host] IPCam reconnect to host(PG01) ~~~")

    # trigger IPCam low battery event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="G"
    signal_obj["abnormal_catalog"]="PG01"
    #signal_obj["abnormal_value"]=""
    #signal_obj["setting_value"]=""
    obj=app.server_lib.trigger_to_server(signal_obj)

def resolution_setting(data_obj): 

    print("[ipcam_resolution_setting] IPCam resolution_setting(PG02) ~~~")

    # trigger IPCam low battery event to server
    signal_obj=dict()
    signal_obj["device_id"]=data_obj["device_id"]
    signal_obj["type_id"]="d06"
    signal_obj["alert_catalog"]="G"
    signal_obj["abnormal_catalog"]="PG02"
    try:
        signal_obj["setting_value"]=data_obj["setting_value"]
    except:
        print("[ipcam_resolution_setting] Missing resolution setting value!!!")

    obj=app.server_lib.trigger_to_server(signal_obj)

#--------------------------------------

def ask_ipcam_to_upload_video(event_id):
    # iterate all IPCams in WLIPCam.json and ask them to upload video
    print("[ask_ipcam_to_upload_video] event_id=%s" %(event_id),  flush=True)

    table_name='WLIPCam'
    file_content_list=list()
    #get ip address
    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
            file_content=json.load(f)
            file_content_list=file_content[table_name]
    except IOError:
        print ("the given file doesn't exist. Return error")
        return 
        
    for element in file_content_list:
        update_content=dict()
        #update_content["before_sec"]=element["e_before_seconds"]
        #update_content["after_sec"]=element["e_after_seconds"]
        #update_content["upload"]=element["media_upload_set"]
        update_content["event_trigger"]=event_id
        device_id=element["device_id"]

        print("IPCam device_id= " + str(device_id))
        token,private_ip_addr=start_login(device_id)
        if token =="":
            print("IPCam not online!!! skipping~~~")
            continue

        json_str=json.dumps(update_content)
        url = 'http://'+ private_ip_addr +'/api/1.0/event_video.json'
        payload = '{"items":' + json_str + '}'

        print("url= " + str(url))
        print("payload= " + str(payload))

        API_HEADER = dict()
        API_HEADER['Content-Type']='application/json;charset=utf-8'
        API_HEADER['Set-Cookie']='token='+token

        requests_type = 'put'
        response=requests_select(requests_type,url,API_HEADER,payload)

    #return response
    
def ipcam_bind(bind_set):
    print("[ipcam_bind] bind ipcam setting start~~~")

    table_name='WLIPCam'
    file_content_list=list()
    #get ip address
    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
            file_content=json.load(f)
            file_content_list=file_content[table_name]
    except IOError:
        print ("the given file doesn't exist. Return error")
        return 
        
    for element in file_content_list:
        update_content=dict()

        #bind_set: 0: bind, 1: unbind
        update_content["unbind"]=str(bind_set)
        device_id=element["device_id"]

        print("[ipcam_bind] IPCam device_id= " + str(device_id))
        token,private_ip_addr=start_login(device_id)
        if token =="":
            print("[ipcam_bind] IPCam not online!!! skipping~~~")
            continue

        json_str=json.dumps(update_content)
        url = 'http://'+ private_ip_addr +'/api/1.0/event_video.json'
        payload = '{"items":' + json_str + '}'

        print("url= " + str(url))
        print("payload= " + str(payload))

        API_HEADER = dict()
        API_HEADER['Content-Type']='application/json;charset=utf-8'
        API_HEADER['Set-Cookie']='token='+token

        requests_type = 'put'
        response=requests_select(requests_type,url,API_HEADER,payload)

def PIR_setting():
    print("[ipcam_PIR_setting] PIR setting start~~~",  flush=True)

    table_name='HostCamPir'
    file_content_list=list()

    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
            file_content=json.load(f)
            file_content_list=file_content[table_name]
    except IOError:
        print ("the given file doesn't exist. Return error")
        return 

    for element in file_content_list:
        update_content=dict()
        enable_pir=element["enable_pir"]
        device_id=element["device_id"]
        if enable_pir == "1":
            update_content["infrared_A8"]=True
        else:
            update_content["infrared_A8"]=False

        print("[ipcam_PIR_setting] IPCam device_id= " + str(device_id))
        token,private_ip_addr=start_login(device_id)
        if token =="":
            print("[ipcam_PIR_setting] IPCam not online!!! skipping~~~")
            continue

        json_str=json.dumps(update_content)
        url = 'http://'+ private_ip_addr +'/api/1.0/pir_setting.json'
        payload = '{"items":' + json_str + '}'

        print("url= " + str(url))
        print("payload= " + str(payload))

        API_HEADER = dict()
        API_HEADER['Content-Type']='application/json;charset=utf-8'
        API_HEADER['Set-Cookie']='token='+token

        requests_type = 'put'
        response=requests_select(requests_type,url,API_HEADER,payload)
