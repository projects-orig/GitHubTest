#coding=UTF-8

import sys
import time
import datetime
import requests
import json
import hashlib
import ftplib

import signal
import os,subprocess
from collections import OrderedDict

from flask import Flask, jsonify, abort, request
from flask import render_template, flash, redirect
from werkzeug import secure_filename

from app import app
from app import server_lib
from app import device_lib
from app import host_lib
from app import ipcam_lib
from app import db, models
from app.models import MainHost, CustomerIP, Preservation_host, KeepAlive,\
                       KeepAliveDevice, Partition, PartitionLoop, Loop, LoopDevice,\
                       Device, DeviceType, CouplingLoop, DeviceSetting, WLReadSensor,\
                       WLRemoteControl, WRS_WRC, WLDoubleBondBTemperatureSensor,\
                       WLReedSensor, WLCamera, Signal, Signal_Event

 
JSON_FILE_PATH = '/usr/local/share/security_service/'
FTP_FILE_PATH = '/usr/local/share/security_service/'

CUSTOMER_ID_CACHE=""

################################################################################
def get_customer_number():
    global CUSTOMER_ID_CACHE
    if CUSTOMER_ID_CACHE !="":
      return CUSTOMER_ID_CACHE
      
    customer_id=""
    try:
      with open(JSON_FILE_PATH + "customer_id.json", "r") as f:
        file_content=json.load(f)
        customer_id=str(file_content['customer_id'])
        CUSTOMER_ID_CACHE=customer_id
    except IOError:
      print ("[customer_id.json] file doesn't exist...")
      pass
    except:
      print ("[customer_id.json] unknown error...")
      pass
      
    return customer_id
    
    
def set_customer_number(customer_id):
    global CUSTOMER_ID_CACHE
    CUSTOMER_ID_CACHE=customer_id
    
    # save to json file
    file_content=dict()
    file_content["customer_id"]=customer_id
    try:
      with open(JSON_FILE_PATH + "customer_id.json", "w") as f:
        json.dump(file_content, f)
        
        # update IPCam to server
        server_lib.update_all_ipcam_to_server()
        
        # register all devices to server when customer_id is set
        server_lib.register_device_to_server("")
                
        # notify keepalive_service to restart
        command="/etc/init.d/keepalive_service restart"
        print("[set_customer_number] command=" + command,  flush=True)
        os.system(command)
        
        
    except:
      print ("[customer_id.json] update fails...")
      pass

def get_json_file_dir():
    return JSON_FILE_PATH


################################################################################
# syn the nick_name to Device.json: 
# if record is for device config (WLReadSensor.json, etc.) save its nick_name to Device.json 
def update_to_Device_json_file(table_name, record):
    print("[update_to_Device_json_file] table_name=" + table_name,  flush=True)
    if table_name=="WLReedSensor" or table_name=="WLDoubleBondBTemperatureSensor" or table_name=="WLReadSensor" or table_name=="WLIPCam" or table_name=="HostCamPir":
      try:
        device_id=str(record["device_id"])
        if "nick_name" in record and record["nick_name"] != "" :
          nick_name=record["nick_name"]
        else:
          print("[update_to_Device_json_file] no nick_name to update!",  flush=True)
          return 
          
        #print("device_id=" + device_id + ", nick_name=" + nick_name,  flush=True)
        item_found=False
        with open(JSON_FILE_PATH + "Device.json", "r") as f:
          device_file_content=json.load(f)
          device_list=device_file_content["Device"]
          for dev_obj in device_list:
            if str(dev_obj["device_id"])==str(device_id):
              dev_obj["nick_name"]=nick_name
              item_found=True
              break
              
          # sync nick_name to this device's extend device(s)    
          for dev_obj in device_list:
            if str(dev_obj["type_id"])=="d07" and str(device_id) + "-" in str(dev_obj["device_id"]):
              dev_obj["nick_name"]=nick_name + str(dev_obj["device_id"])[-2:]
              
      except IOError:
        print ("[Device.json] doesn't exist...")
        return
      except:
        print ("[Device.json] unknown error")
        return

      if item_found:
        with open(JSON_FILE_PATH + "Device.json", "w") as f:
          json.dump(device_file_content, f)
      else:
        print ("device(" + device_id + ") is NOT found")
        pass
    
################################################################################
# save json file content: 
def save_file_content(table_name, file_content):
    with open(JSON_FILE_PATH + table_name + ".json", "w") as f:
      json.dump(file_content, f)
        
################################################################################
# read json file content: 
# if file does not exist, create the empty content by case 
def get_file_content(table_name):
    file_content=dict()
    # load corresponding json file content
    # https://pythonadventures.wordpress.com/2012/03/30/catch-a-specific-ioerror/
    try:
        with open(JSON_FILE_PATH + table_name + ".json", "r") as f:
          file_content=json.load(f)
    except IOError:
        print ("[" + table_name + ".json] doesn't exist. create the default content!")
        file_content[table_name]=list()
        # special cases handling:        
        if table_name=="HostSetting": 
          file_content[table_name]=[{"time":"","battery_low_power_set":"30","action_temperature_sensing":"50"}]
        elif table_name=="ServerAddress":
          file_content[table_name]=[{"server1":{}, "server2":{},"server3":{},"server4":{}}]
        elif table_name=="DeviceSetting":
          file_content[table_name]=[{"sendMsg_time_when_enabled":"", "sendMsg_time_when_disabled":""}]  
        
        elif table_name=="WLReedSensor":
          # d02 = 無線磁簧感測器
          try:
            with open(JSON_FILE_PATH + "Device.json", "r") as f:
              device_file_content=json.load(f)
              device_list=device_file_content["Device"]
              for dev_obj in device_list:
                if dev_obj["type_id"]=="d02":
                  file_content[table_name].append({"device_id":dev_obj["device_id"]})
          except IOError:
            print ("[Device.json] doesn't exist...")
            
        elif table_name=="WLDoubleBondBTemperatureSensor":
          # d03 = 無線雙鍵式感測器
          try:
            with open(JSON_FILE_PATH + "Device.json", "r") as f:
              device_file_content=json.load(f)
              device_list=device_file_content["Device"]
              for dev_obj in device_list:
                if dev_obj["type_id"]=="d03":
                  file_content[table_name].append({"device_id":dev_obj["device_id"]})
          except IOError:
            print ("[Device.json] doesn't exist...")

        elif table_name=="WLReadSensor":
          # d04 = 無線雙向式讀訊器
          try:                                                        
            with open(JSON_FILE_PATH + "Device.json", "r") as f:
              device_file_content=json.load(f)
              device_list=device_file_content["Device"]
              for dev_obj in device_list:
                if dev_obj["type_id"]=="d04":
                  file_content[table_name].append({"device_id":dev_obj["device_id"]})
          except IOError:
            print ("[Device.json] doesn't exist...")

        elif table_name=="WLIPCam" or table_name=="HostCamPir":
          # d06 = 無線攝影機
          try:
            with open(JSON_FILE_PATH + "Device.json", "r") as f:
              device_file_content=json.load(f)
              device_list=device_file_content["Device"]
              for dev_obj in device_list:
                print ("device_id : " + dev_obj["device_id"] + " type_id : "+dev_obj["type_id"])
                if dev_obj["type_id"]=="d06":
                  file_content[table_name].append({"device_id":dev_obj["device_id"]})
          except IOError:
            print ("[Device.json] doesn't exist...")
        
            
    return file_content            

            

################################################################################
# send os signal in-python to other process 

# http://stackoverflow.com/questions/26688936/python-how-to-get-pid-by-process-name
from subprocess import check_output

# return the list of int. e.g. [27698, 27678, 27665, 27649 ...]
def get_pid(name):
    return map(int,check_output(["pidof",name]).split())


# http://stackoverflow.com/questions/27356837/send-sigint-in-python-to-os-system
def notify_setting_change(process_name):
    pid_list=get_pid(process_name)
    for pid in pid_list: 
      print("pid=" + str(pid))
      #os.kill(pid, signal.SIGUSR1)



################################################################################
# FTP test
# http://stackoverflow.com/questions/17438096/ftp-upload-files-python
# https://docs.python.org/3/library/ftplib.html
def ftp_test():
    filename="1.mp4"
    local_filepath = FTP_FILE_PATH + filename
    ftp_server="211.22.242.18"
    ftp_user="ftpuser"
    ftp_pass="12345678"
    remote_dir="video/"
    
    ftp = ftplib.FTP(ftp_server)
    ftp.login(ftp_user, ftp_pass)
    ftp.cwd(remote_dir)
    ftp.storbinary("STOR " + filename, open(local_filepath, 'rb'))

    
################################################################################
# get http client MAC addr
# http://stackoverflow.com/questions/22188020/how-can-i-find-the-mac-address-of-a-client-on-the-same-network-using-python-fla
# if ImportError: No module named 'netifaces'
# as root, pip3 install netifaces
import netifaces as nif


HOST_MAC_SETTING_OBJ=dict()

def mac_for_ip(ip):
    for i in nif.interfaces():
        addrs = nif.ifaddresses(i)
        try:
            if_mac = addrs[nif.AF_LINK][0]['addr']
            if_ip = addrs[nif.AF_INET][0]['addr']
        except (IndexError, KeyError): #ignore ifaces that dont have MAC or IP
            if_mac = if_ip = None
        
        if if_ip == ip:
            return if_mac
    return None

def open_hostmacsetting_json_file():
    global HOST_MAC_SETTING_OBJ
    try:
      with open(JSON_FILE_PATH + "HostMacSetting.json", "r") as f:
        file_content=json.load(f)
    except IOError:
      print ("[HostMacSetting.json] doesn't exist. The account is NOT opened!")
      return 
    
    HOST_MAC_SETTING_OBJ=file_content["HostMacSetting"][0]


def check_mac_white_list(client_mac):
    
    if HOST_MAC_SETTING_OBJ["enable_list"]=="1":
      found=False
      mac_list=HOST_MAC_SETTING_OBJ["mac_list"]
      for allowd_mac in mac_list:
        if client_mac==allowd_mac:
          found=True
          break     
      
      if found:
        print("client MAC is allowed!")
        return True
      else:
        print("client MAC is NOT allowed!")
        return False  
        
    else:
      print("MAC white-list is NOT enabled!")
      return True  

################################################################################
# SHA-256 Digest Generation 
# 128-byte random string by http://www.unit-conversion.info/texttools/random-string-generator/
HASH_SEED='rEwIdbB9aHFHvdAheX4h7uNuh2fkNBr3WpDfFnyUqXipzOv2AVeKejc2ixffZARNAgNwCX678n0ZzrN6kRwGgd2xvxdJ17rzbRGO3KTAQRtWcWho47KAemZ8VBIwVKri'

# HASH_SECRET: resident variable to reduce its computing time  
# it may contain unreadable bytes
HASH_SECRET=b''

def blending():
    
    global HASH_SECRET
    
    #print("[blending] CUSTOMER_ID=" + get_customer_number(),  flush=True)
    if len(HASH_SECRET) > 0:
        #print('use resident HASH_SECRET...',  flush=True)
        #print(HASH_SECRET,  flush=True)
        return HASH_SECRET
        
    customer_id=get_customer_number()
    customer_id_len=len(customer_id)
    if customer_id_len > 0 :
        tmp_buffer=''
        index=0
        for ch in HASH_SEED:
           #print(ch)
           # http://stackoverflow.com/questions/227459/ascii-value-of-a-character-in-python
           tmp_buffer += chr(ord(ch) ^ ord(customer_id[index % customer_id_len])) 
           index += 1
           
        #print(tmp_buffer,  flush=True)
        HASH_SECRET = tmp_buffer.encode('utf-8')  
    else:
        HASH_SECRET = HASH_SEED.encode('utf-8')                
     
    return HASH_SECRET    

def check_hash(data, data_hash):
    
    customer_id=get_customer_number()
    if len(customer_id) > 0 :
      return check_sha256(data, data_hash)
    else:
      print("[customer_id] does NOT exists on host ==> skip hash-check",  flush=True)
      return True
        
def check_sha256(data, data_hash):
    hash_object = hashlib.sha256(data + blending())
    #print("Digest_on_host=" + hash_object.hexdigest(),  flush=True)
    return hash_object.hexdigest()==data_hash
    
def check_md5(data, data_hash):
    m = hashlib.md5()
    m.update(data + blending())
    #print(m.hexdigest(),  flush=True)
    return m.hexdigest()==data_hash 

def gen_hash(data_Str):
    return gen_sha256(data_Str)

# http://pythoncentral.io/hashing-strings-with-python/
def gen_sha256(data_str):
    hash_object = hashlib.sha256(data_str.encode('utf-8') + blending())
    return hash_object.hexdigest()

    
################################################################################
# code by jason.lin 
UPLOAD_FOLDER = '/'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

#curl -F uploadfile=@/tmp/ttt.tgz http://127.0.0.1:5000/updatefmw
@app.route('/updatefmw',methods=['GET','POST'])
def updatefmw():
  if request.method == 'POST':
    file = request.files['uploadfile']
    filename = secure_filename(file.filename)
    fullpath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
    file.save(fullpath)
    subprocess.Popen(["tar","zxf",fullpath,"-C","/"],shell=False)
    return 'upload file done!\n'

  if request.method == 'GET':
    return "it's uploading page\n"

#curl http://127.0.0.1:5000/setmac?mac=00:11:22:33:44:88
@app.route('/setmac')
def setmac():
  if 'mac' in request.args:
    set_macaddr(request.args['mac'])
    return 'mac set done!'
  else:
    set_macaddr("00:11:22:33:44:55")
    return 'set default mac'
    
################################################################################    
    
# upload all device to IOT service, test only
@app.route('/test/1/', methods=['POST'])
def test_1():
    server_lib.register_device_to_server("")
    return jsonify({"test":"done"}), 200    
    
################################################################################
@app.route('/test/2/', methods=['POST'])
def test_2():
    host_lib.start_network_config()
    return jsonify({"test":"done"}), 200    

################################################################################
@app.route('/test/3/', methods=['POST'])
def test_3():
    host_lib.start_antenna_wifi_config()
    host_lib.start_antenna_sub1g_config()
    return jsonify({"test":"done"}), 200    

################################################################################
@app.route('/test/4/', methods=['POST'])
def test_4():
    device_lib.pairing_enable()
    return jsonify({"test":"done"}), 200    

##########################################################################
# API for engineering APP

@app.route('/api/send/', methods=['POST'])
def api_send():
    #print("headers=" + str(request.headers),  flush=True)
    
    # MAC white-list test ok, 20161128
    #client_ip=request.remote_addr
    #print("client_ip=" + client_ip)
    #open_hostmacsetting_json_file()
    #client_mac=mac_for_ip(client_ip)
    #print("client_mac=" + client_mac)
    #if not check_mac_white_list(client_mac):
    #    return jsonify({'error_code': 1, 'error_msg': "Refused by MAC while-list"}), 501
        
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
        print(test_obj)
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
    
    if "command" not in request.json:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect API format."}), 501
    
    command=request.json["command"]
    
    # exclude comamnd(s) for hash-check 
    if command!="get_customer_id":
        #print("[api_send] request.headers:")      
        #print(request.headers)
        if "sapido-hash" in request.headers:
          hash=request.headers['sapido-hash']
        else:
          return jsonify({'error_code': 1, 'error_msg': "'sapido-hash' does NOT exist in header."}), 501
          
        if not check_hash(request.data, hash):
          print("hash-check: Incorrect Digest",  flush=True)
          return jsonify({'error_code': 1, 'error_msg': "Incorrect Digest."}), 501

    # set default response in advance...
    api_response=dict()
    api_response["command"]=command
    api_response["result"]="fail"
    api_response_status=501
    
    global HASH_SECRET
    data_obj=dict()
    if command =="test":
        param=request.json["param"]
        notify_setting_change(param)
        
        api_response["result"]="pass"
        api_response_status=200
        
    elif command =="wifi_scan":        
        
        host_lib.get_wifi_scan_result()
        
        file_content=list()
        try:
          with open(JSON_FILE_PATH + "wifi_scan_result.json", "r") as f:
            file_content = json.load(f)
        except IOError:
          print ("[wifi_scan_result.json] file doesn't exist. create default list")
          file_content=[{"ssid":"NotAvailable", "signal":0,"security":""}]

          
        api_response["result"]="pass"
        api_response["ssid_list"]=file_content
        api_response_status=200
            
    elif command =="login":
        data_obj=dict()
        try:
          validate_code=request.json["auth_code"]
        except:
          validate_code=""
          
        try:
          username=request.json["username"]
        except:
          username=""
        try:
          password=request.json["password"]
        except:
          password=""
        
        # in case the current setting is cleared by login input of 9876,'',''
        global CUSTOMER_ID_CACHE
        CUSTOMER_ID_CACHE=""    
        
        result=host_lib.login_process(validate_code, username, password)
        if result:
          api_response["result"]="pass"
          api_response["customer_id"]=get_customer_number()
          api_response_status=200
        else:
          api_response["result"]="fail"
          api_response_status=501              
    
    elif command =="set_customer_id":
        if "customer_id" in request.json:
          print("[set_customer_id] new customer_id=" + request.json["customer_id"],  flush=True)

          # CUSTOMER_ID should be updated
          set_customer_number(request.json["customer_id"])
          
          # HASH_SECRET should be reset 
          HASH_SECRET=b''

          
          api_response["result"]="pass"
          api_response_status=200

    elif command =="get_customer_id":
        api_response["customer_id"] = get_customer_number()
        api_response["result"]="pass"
        api_response_status=200
    
    elif command =="delete_devices":
        # {"command”:"delete_devices" ,"device_list":["device_id1","device_id2","device_id3",...]}
        device_list=request.json["device_list"]
        host_lib.delete_device_list(device_list)
        api_response["result"]="pass"
        api_response_status=200
        
    else:                    
        api_response["result"]="Unknown command."
        
    return jsonify(api_response), api_response_status 

@app.route('/api/delete/', methods=['POST'])
def api_delete():
    #print("headers=" + str(request.headers),  flush=True)
    hash=request.headers['sapido-hash']
    if not check_hash(request.data, hash):
        print("hash-check: Incorrect Digest",  flush=True)
        return jsonify({'error_code': 1, 'error_msg': "Incorrect Digest."}), 501
    
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
    
    '''
    {
      <table_name#1>:{
        "filter:"{<field>:<value>, ...},
      },
      <table_name#2>
    }
    '''
    # get table-name from request
    # http://stackoverflow.com/questions/15789059/python-json-only-get-keys-in-first-level
    for table_name in request.json.keys():
      print("table_name=" + table_name,  flush=True)
      filter=request.json[table_name]["filter"]
      print("filter=" + str(filter),  flush=True)
      # load corresponding json file content
      # https://pythonadventures.wordpress.com/2012/03/30/catch-a-specific-ioerror/
      try:
        with open(JSON_FILE_PATH + table_name + ".json", "r") as f:
          file_content=json.load(f)
          #print(file_content,  flush=True)
      except IOError:
        print ("the given file doesn't exist. return error!")
        return jsonify({'error_code': 1, 'error_msg': "Delete data error"}), 501
        #continue
           
      # filter records of current file content                  
      #print(file_content[table_name],  flush=True)
      record_index=0
      delete_list=list()
      for record in file_content[table_name]:
        
        #"filter":{<field>:<value>, ...},
        # http://stackoverflow.com/questions/3294889/iterating-over-dictionaries-using-for-loops-in-python
        filter_number=0
        filter_match=0
        for field, value in filter.items():
          #print("field =" + field,  flush=True)
          #print("value =" + value,  flush=True)
          filter_number += 1
          if field in record and record[field] == value:
            filter_match += 1
        
        if filter_number==filter_match:
          print("got it: " + str(record_index),  flush=True)
          # save record index for later deletion
          delete_list.append(record_index)
          
        record_index+=1
      # end of record for-loop
      
      print(delete_list,  flush=True)
      dev_id_list=list() 
      # delete record(s)  
      #for delete_index in delete_list.reverse():
      for delete_index in sorted(delete_list, reverse=True):
          # delete corresponding item on IOT SERVICE...
          if table_name == 'CouplingLoop':
            server_lib.send_request_to_server("server_del_couplingloop", file_content[table_name][delete_index])
          elif table_name == 'Partition':
            server_lib.send_request_to_server("server_del_partition", file_content[table_name][delete_index])
          elif table_name == 'LoopDevice':
            server_lib.send_request_to_server("server_del_loopdevice", file_content[table_name][delete_index])
          elif table_name == 'Device':
            # record delete-target device_id to process later
            dev_id_list.append(file_content[table_name][delete_index]["device_id"])
            
          else:
            print('skip IOT processing for table(%s)' %(table_name),  flush=True)
          
          print("delete index: " + str(delete_index),  flush=True)
          del file_content[table_name][delete_index]
          
                
      # save to json file
      with open(JSON_FILE_PATH + table_name + ".json", "w") as f:
        json.dump(file_content, f)
      
      if table_name == 'Device':
        # special process for device deletetion because the other tables are also affected.
        host_lib.delete_device_list(dev_id_list)
      
    # end of table for-loop    

    return jsonify('{"message":"Delete data ok"}'), 200
    
    
@app.route('/api/update/', methods=['POST'])
def api_update():
    #print("headers=" + str(request.headers),  flush=True)
    hash=request.headers['sapido-hash']
    if not check_hash(request.data, hash):
        print("hash-check: Incorrect Digest",  flush=True)
        return jsonify({'error_code': 1, 'error_msg': "Incorrect Digest."}), 501
    
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
    
    '''
    JSON:
    {
      <table_name#1>:{
        ??謢ter??{<field>:<value>, ...},
        ???date??{<field>:<value>, ...}
      },
      <table_name#2> ??
    }
    '''
    ipcam_update_flag = True
    # get table-name from request
    # http://stackoverflow.com/questions/15789059/python-json-only-get-keys-in-first-level
    for table_name in request.json.keys():
      print("table_name=" + table_name,  flush=True)
      
      # for sensor and IPCam config change, check if security is set
      try:
        if table_name=="WLReedSensor" or table_name=="WLDoubleBondBTemperatureSensor" or table_name=="WLReadSensor" or table_name=="WLIPCam":
          with open(get_json_file_dir() + "KeepAlive.json", "r") as f:
            file_content=json.load(f)
            keepalive_obj=file_content["KeepAlive"][0]
            if host_lib.check_host_alarm_set(keepalive_obj):
              print("host security is set ==> sensor config is disabled by host !",  flush=True)
              return jsonify('{"message":"Update data fail because host security is set"}'), 501
      except:
        print ("[KeepAlive.json] unknown error => ignore this check.",  flush=True)
        pass
      
      try:
        filter=request.json[table_name]["filter"]  
      except KeyError:
        print("no filter!",  flush=True)
        filter=dict()
        
      try:
        update_field=request.json[table_name]["update"]
        #print("update_field=%s" %(str(update_field)),  flush=True)
      except KeyError:
        print("no data to update!",  flush=True)
        continue
        
      print("filter=" + str(filter),  flush=True)
      file_content=get_file_content(table_name)
      print(str(file_content),  flush=True)
      
      hostsetting_modem_set_change=False
      hostsetting_network_set_change=False
      hostsetting_antenna_wifi_type_change=False
      hostsetting_antenna_sub1g_type_change=False
             
      # filter records of current file content                  
      for record in file_content[table_name]:
        #print("record=" + json.dumps(record),  flush=True)
        #"filter":{<field>:<value>, ...},
        # http://stackoverflow.com/questions/3294889/iterating-over-dictionaries-using-for-loops-in-python
        filter_number=0
        filter_match=0
        
        # to store update fields...
        record_subset=dict()
        
        record_found=False
        for field, value in filter.items():
          #print("field=" + field,  flush=True)
          #print("value=" + value,  flush=True)
          filter_number += 1
          if field in record and str(record[field]) == str(value):
            filter_match += 1
        
        if filter_number==filter_match:
          print("got it!",  flush=True)
          record_found=True
          
          if "device_id" in record:
            record_subset["device_id"]=record["device_id"]
          
          # update record
          #"update":{<field>:<value>, ...}
          for field, value in update_field.items():
            #print("field=" + field,  flush=True)
            #print("value=" + str(value),  flush=True)
            record[field]=value
            record_subset[field]=value
        
        if record_found:
          if table_name=="WLIPCam":
            print("IPcam setting start")

            #put necessary setting value
            ipcam_update_flag=ipcam_lib.update_ipcam_setting_value(record)

          elif table_name=="HostSetting":
            if "phone_number" in record_subset:
              hostsetting_modem_set_change=True
              
            if "network_set" in record_subset:
              hostsetting_network_set_change=True

            if "antenna_wifi_type" in record_subset:
              hostsetting_antenna_wifi_type_change=True

            if "antenna_sub1g_type" in record_subset:
              hostsetting_antenna_sub1g_type_change=True
          
          else:
            # device communication
            # only send commands for update fields...
            print("record_subset=" + str(record_subset),  flush=True)
            device_lib.send_command_to_device(record_subset)
          
          # update device_name to Device.json
          update_to_Device_json_file(table_name, record)
          # update to server
          server_lib.update_to_server(table_name, record)
        else:
          print("record is NOT matched",  flush=True)
          pass
          
      # end of record for-loop 
        
      # save to json file
      save_file_content(table_name, file_content)
      
      # if the HostSetting is updated       
      if table_name=="HostSetting":
        host_lib.sync_host_setting_to_keepalive()
        
        if hostsetting_modem_set_change:
          host_lib.modem_config()
        
        if hostsetting_network_set_change:
          host_lib.start_network_config()
          #host_lib.send_network_init_event()
          
        if hostsetting_antenna_wifi_type_change:
          host_lib.start_antenna_wifi_config()
          
        if hostsetting_antenna_sub1g_type_change:
          host_lib.start_antenna_sub1g_config()
          
      elif table_name=="CouplingLoop":
        host_lib.stop_coupling_operation()

      elif table_name=="HostCamPir":
        ipcam_lib.PIR_setting()
      
      elif table_name=="ServerAddress":
        # sync to KeepAlive.json
        host_lib.sync_server_address_to_keepalive()
        
    # end of table for-loop    

    if ipcam_update_flag:
      return jsonify('{"message":"Update data ok"}'), 200
    else:
      return jsonify('{"message":"IPCam offline"}'), 500
    
    
@app.route('/api/get/', methods=['POST'])
def api_get():
    #print("headers=\n" + str(request.headers),  flush=True)
    #print("body=\n" + str(request.data),  flush=True)
    hash=request.headers['sapido-hash']
    if not check_hash(request.data, hash):
        print("hash-check: Incorrect Digest",  flush=True)    
        return jsonify({'error_code': 1, 'error_msg': "Incorrect Digest."}), 501

    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
    
        
    '''    
    JSON:
    {
      <table_name#1>:{
        ??謢ter??{<field>:<value>, ...},
      },
      <table_name#2> ??
    }
    '''
    api_response=dict()
    # get table-name from request
    # http://stackoverflow.com/questions/15789059/python-json-only-get-keys-in-first-level
    for table_name in request.json.keys():
      #print("table_name=" + table_name,  flush=True)
      file_content=get_file_content(table_name)
      
      print("%s.json: content=%s" %(table_name, str(file_content)),  flush=True)
      if "filter" in request.json[table_name]: 
        filter=request.json[table_name]["filter"]
        print("filter=" + str(filter),  flush=True)
      else:
        # no filter, return whole table content
        print("no filter !",  flush=True)
        api_response[table_name]=file_content[table_name]
        continue
      
      api_response[table_name]=list()             
      # filter records of current file content                  
      #print(file_content[table_name],  flush=True)
      for record in file_content[table_name]:
        #"filter":{<field>:<value>, ...},
        # http://stackoverflow.com/questions/3294889/iterating-over-dictionaries-using-for-loops-in-python
        filter_number=0
        filter_match=0
        for field, value in filter.items():
          #print("field =" + field,  flush=True)
          #print("value =" + value,  flush=True)
          filter_number += 1
          if field in record and record[field] == value:
            filter_match += 1
        
        if filter_number==filter_match:
          #print("got it!",  flush=True)
          # append the record to api_response
          # http://stackoverflow.com/questions/252703/append-vs-extend
          api_response[table_name].append(record)
      # end of record for-loop     
    
    # end of table for-loop    
    
    return jsonify(api_response), 200     
    
    
@app.route('/api/new/', methods=['POST'])
def api_new():
    #print("headers=" + str(request.headers),  flush=True)
    hash=request.headers['sapido-hash']
    if not check_hash(request.data, hash):
        print("hash-check: Incorrect Digest",  flush=True)
        return jsonify({'error_code': 1, 'error_msg': "Incorrect Digest."}), 501

    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
            
    '''
    JSON:
    { <table#1>:[{},{}, ...],
      <table#2>:[{},{}, ...],
      ...
    }
    ''' 
    ipcam_update_flag = True
    ## add new records to corresponding json file:
    # get table-name from request
    # http://stackoverflow.com/questions/15789059/python-json-only-get-keys-in-first-level
    for table_name in request.json.keys():
      print("table_name=" + table_name,  flush=True)
      
      # for sensor and IPCam config change, check if security is set
      try:
        if table_name=="WLReedSensor" or table_name=="WLDoubleBondBTemperatureSensor" or table_name=="WLReadSensor" or table_name=="WLIPCam" :
          with open(get_json_file_dir() + "KeepAlive.json", "r") as f:
            file_content=json.load(f)
            keepalive_obj=file_content["KeepAlive"][0]
            if host_lib.check_host_alarm_set(keepalive_obj):
              print("host security is set ==> sensor config is disabled by host !",  flush=True)
              return jsonify('{"message":"Update data fail because host security is set"}'), 501
      except:
        print ("[KeepAlive.json] unknown error => ignore this check.",  flush=True)
        pass

      
      # load corresponding json file content
      # https://pythonadventures.wordpress.com/2012/03/30/catch-a-specific-ioerror/
      file_content=dict()
      try:
        with open(JSON_FILE_PATH + table_name + ".json", "r") as f:
          file_content=json.load(f)
          #print(file_content,  flush=True)
          
      except IOError:
        print ("the given file doesn't exist. create EMPTY list")
        file_content[table_name]=list()
        
      #record=request.json[table_name][0]
      for record in request.json[table_name]:
        # process the record of various tables
        # add time value if needed
        if 'time' in record and record['time']=='':
          print("add host real-time!!",  flush=True)
          record['time']=str(datetime.datetime.now())            
        #print(record,  flush=True)
        
        # add new record to current file content                  
        #print(file_content[table_name],  flush=True)
        file_content[table_name].append(record)

        if table_name=="WLIPCam":
            print("IPcam setting start")

            #put necessary setting value
            ipcam_update_flag=ipcam_lib.update_ipcam_setting_value(record)
        else:
            # device communication
            device_lib.send_command_to_device(record)
        
        # update device_name to Device.json
        update_to_Device_json_file(table_name, record)
        
        # update to server
        server_lib.update_to_server(table_name, record)
        
      # end of record for-loop  
      
      # save to json file
      save_file_content(table_name, file_content)
      
    # end of table for-loop    
   
    if ipcam_update_flag:
      return jsonify('{"message":"Add new data ok"}'), 200
    else:
      return jsonify('{"message":"IPCam offline"}'), 500

# end of API for engineering APP


################################################################################
# API for Sensors

@app.route('/trigger/event/', methods=['POST'])
def trigger_event():
    trigger_result=True
    if not request.json:
      return jsonify({'error_code': 3, 'error_msg': "Incorrect json format."}), 501

    host_lib.set_event_id()
    
    if "d01" in request.json:
      trigger_result=host_lib.handle_host_event(request.json["d01"])        
    elif "d02" in request.json:
      trigger_result=device_lib.handle_reed_event(request.json["d02"])
    elif "d03" in request.json:
      trigger_result=device_lib.handle_doublebond_event(request.json["d03"])
    elif "d04" in request.json:
      trigger_result=device_lib.handle_reader_event(request.json["d04"])
    elif "d05" in request.json:
      trigger_result=device_lib.handle_remotecontrol_event(request.json["d05"])
    else:
      return jsonify({'error_code': 3, 'error_msg': "Incorrect API format."}), 501

    api_response=dict()
    if trigger_result:
      api_response["result"]="trigger ok"
      api_response_status=200
    else:
      api_response["result"]="trigger failed"
      api_response_status=501
    
    return jsonify(api_response), api_response_status

#########################################################

@app.route('/receive/signal/', methods=['POST'])
def receive_signal():
    if not request.json:
        return jsonify({'error_code': 3, 'error_msg': "Incorrect json format."}), 501

    try:
      with open(JSON_FILE_PATH + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!")
      return jsonify({'error_code': 3, 'error_msg': "The account is NOT opened"}), 501 

    if "d01" in request.json:
        '''
        {"d01":
          {
          "alert_catalog":"警報大類", 
          "abnormal_catalog":"異常類別", 
          "setting_value":"設定值", 
          "abnormal_value":"異常值", 
          }
        }
        '''
        signal_obj=request.json["d01"]
        signal_obj["type_id"]="d01"

    elif "d02" in request.json:
        '''
        {"d02": #WLReedSensor
            {
            "time":"異常發生時間", 
            "device_id":"裝置id", 
            "alert_catalog":"警報大類", 
            "abnormal_catalog":"異常類別", 
            "setting_value":"設定值", 
            "abnormal_value":"異常值", 
            }
        }
        '''
        signal_obj=request.json["d02"]
        signal_obj["type_id"]="d02"
      
    elif "d03" in request.json:
        '''
        {"d03": #WLDoubleBondBTemperatureSensor
            {
            "time":"異常發生時間",
            "device_id":"裝置id", 
            "alert_catalog":"警報大類", 
            "abnormal_catalog":"異常類別", 
            "setting_value":"設定值", 
            "abnormal_value":"異常值"
            }
        }
        '''
        signal_obj=request.json["d03"]
        signal_obj["type_id"]="d03"
        
    elif "d04" in request.json:
        '''
        {"d04": #WLReadSensor
            {
            "time":"異常發生時間",
            "device_id":"裝置id", 
            "alert_catalog":"警報大類", 
            "abnormal_catalog":"異常類別", 
            "setting_value":"設定值", 
            "abnormal_value":"異常值"
            }
        }
        '''
        signal_obj=request.json["d04"]
        signal_obj["type_id"]="d04"

    elif "d05" in request.json:
        '''
        {"d05": #WLRemoteControl
            {
            "time":"異常發生時間",
            "device_id":"裝置id", 
            "alert_catalog":"警報大類", 
            "abnormal_catalog":"異常類別", 
            "abnormal_value":"異常值"
            }
        }
        '''
        signal_obj=request.json["d05"]
        signal_obj["type_id"]="d05"
        
    else:
      return jsonify({'error_code': 3, 'error_msg': "Incorrect API format."}), 501

	# if the type_id is not 'd01' and 'd05', search the loopdevice_list to find the matched loop by device_id
    if signal_obj["type_id"] != "d01" and signal_obj["type_id"] != "d05": 
      signal_obj["loop_id"]=device_lib.check_loop_id_from_device_id(signal_obj["device_id"])      
      if signal_obj["loop_id"]=="":
        print ("device is NOT on any loop!")
        return jsonify({'error_code': 3, 'error_msg': "device is NOT on any loop!"}), 501

    type_id=signal_obj["type_id"]
    abnormal_catalog=signal_obj["abnormal_catalog"]

    # mapping alarm_type with abnormal_catalog
    alarm_type_list=device_lib.Alert_type[type_id]
    #print("alarm_type_list: " + str(alarm_type_list))
    for item in alarm_type_list:
        if abnormal_catalog==item["abnormal_catalog"]:
            alarm_type_assign = item["alarm_type"]
    print("alarm_type_assign: " + str(alarm_type_assign),  flush=True)

    # check if setting value is needed
    setting_value_assign,check_result=device_lib.setting_value_assign_check(type_id,alarm_type_assign)
    if check_result==False:
        return jsonify({'error_code': 3, 'error_msg': "setting value assign error!"}), 50
    else:
        signal_obj["setting_value"]=setting_value_assign
    
    obj = server_lib.trigger_to_server(signal_obj)

    api_response=dict()
    try:
        api_response["result"]=obj['message']
        api_response_status=obj['status']
        api_response["event_id"]=signal_obj["event_id"]
    except KeyError:
        print("skip key error...",  flush=True)
        pass
    except ValueError:
        api_response["result"]="server error"
        api_response_status=501
    
    return jsonify(api_response), api_response_status


@app.route('/update/mediafilepath/', methods=['POST'])
def update_media_file_path():
    if not request.json:
        return jsonify({'error_code': 3, 'error_msg': "Incorrect json format."}), 501

    r = server_lib.update_media_file_path_to_server(request.json)
    print("server message:"+ str(r.json()),  flush=True)

    api_response["result"]=str(r.json())
    api_response_status=200
    
    return jsonify(api_response), api_response_status


@app.route('/update/device_status/', methods=['POST'])
def update_device_status():
    if not request.json:
        return jsonify({'error_code': 3, 'error_msg': "Incorrect json format."}), 501

    update_result=update_device_status_to_json_file(request.json)
    
    api_response=dict()
    if update_result:    
      api_response["result"]="OK"
      api_response_status=200
    else:
      api_response["result"]="Fail"
      api_response_status=501
          
    return jsonify(api_response), api_response_status 


def update_device_status_to_json_file(data_obj):
    if "d02" in data_obj:
        '''
        {"d02":{
              "device_id":"裝置ID",
              "connection_status":"連線狀態",
              "signal_power_status":"裝置無線訊號強度",
              "temperature_status":"裝置目前溫度", 
              "power_status":"裝置目前電量值",
              }
        }
        '''
        signal_obj=data_obj["d02"]
        table_name="WLReedSensor"

    elif "d03" in data_obj:
        '''
        {"d03":{ #WLDoubleBondBTemperatureSensor
              "device_id":"裝置ID",
              "connection_status":"連線狀態",
              "signal_power_status":"裝置無線訊號強度",
              "temperature_status":"裝置目前溫度", 
              "power_status":"裝置目前電量值",
              }
        }
        '''
        signal_obj=data_obj["d03"]
        table_name="WLDoubleBondBTemperatureSensor"

    elif "d04" in data_obj:
        '''
        {"d04":{ #WLReadSensor
              "device_id":"裝置ID",
              "connection_status":"連線狀態",
              "signal_power_status":"裝置無線訊號強度",
              "temperature_status":"裝置目前溫度", 
              "power_status":"裝置目前電量值",
              }
        }
        '''
        signal_obj=data_obj["d04"]
        table_name="WLReadSensor"

    elif "d05" in data_obj:
        '''
        {"d05": #WLRemoteControl
            {
              "device_id":"裝置ID", 
              "power_status":"裝置目前電量值",
            }
        }
        '''
        signal_obj=data_obj["d05"]
        table_name="WLRemoteControl"
    elif "d06" in data_obj:
        return False 
    else:
        print("Incorrect API format.",  flush=True)
        return False
      
    signal_obj["loop_id"]=device_lib.check_loop_id_from_device_id(signal_obj["device_id"])
    if signal_obj["loop_id"]=="":
        print ("device is NOT on any loop!")
        return jsonify({'error_code': 3, 'error_msg': "device is NOT on any loop!"}), 501
        
    try:
        with open(JSON_FILE_PATH + table_name + ".json", "r") as f:
            file_content=json.load(f)
            device_config_list=file_content[table_name]
    except IOError:
        print ("[" + table_name + ".json] doesn't exist. Return error")
        return False

    # search the list to find the matched item by device_id
    item_found=False
    for item in device_config_list:
        if item["device_id"]==signal_obj["device_id"]:
            for name, value in signal_obj.items():
                item[name]=value

            item_found=True
            break

    if not item_found:
        print("device is NOT a valid " + table_name,  flush=True)
        return False

    # save to json file
    with open(JSON_FILE_PATH + table_name + ".json", "w") as f:
      json.dump(file_content, f)

    return True

#IPCam API-----------------------------------------------------------------
@app.route('/ipcam/open_port/', methods=['POST'])
def ipcam_open_port():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.ipcam_open_port(request.json)
    return jsonify('{"message":"New IPCam setting ok"}'), 200


'''
@app.route('/ipcam/get_host_cam_pir/', methods=['GET'])
def get_host_cam_pir():

    hostcampir_list=list()
    try:
      with open(get_json_file_dir() + "HostCamPir.json", "r") as f:
        file_content=json.load(f)
        hostcampir_list=file_content["HostCamPir"]
    except IOError:
        print ("[HostCamPir.json] doesn't exist. Return empty list.")

    return jsonify(hostcampir_list), 200
'''
    

@app.route('/ipcam/update_video_url/', methods=['POST'])
def update_video_url():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.update_video_url(request.json)
    return jsonify('{"message":"update_video_url ok"}'), 200
    
@app.route('/ipcam/pir_abnormal/', methods=['POST'])
def ipcam_pir_abnormal():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.pir_abnormal(request.json)
    return jsonify('{"message":"IPCam pir_abnormal(PA02)!!!"}'), 200

@app.route('/ipcam/low_battery/', methods=['POST'])
def ipcam_low_battery():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.low_battery(request.json)
    return jsonify('{"message":"IPCam low battery alert(PA03)!!!"}'), 200

@app.route('/ipcam/battery_back_to_normal/', methods=['POST'])
def ipcam_battery_back_to_normal():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.battery_back_to_normal(request.json)
    return jsonify('{"message":"IPCam battery_back_to_normal"}'), 200

@app.route('/ipcam/reboot/', methods=['POST'])
def ipcam_reboot():
    try:
        print("/ipcam/reboot/: %s" %json.loads(request.data.decode('utf-8')),  flush=True)
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.reboot(request.json)
    return jsonify('{"message":"IPCam reboot(PA04)!!!"}'), 200

@app.route('/ipcam/power_fail/', methods=['POST'])
def ipcam_power_fail():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.power_fail(request.json)
    return jsonify('{"message":"IPCam power fail(PA05)!!!"}'), 200

@app.route('/ipcam/wifi_error/', methods=['POST'])
def ipcam_wifi_error():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.wifi_error(request.json)
    return jsonify('{"message":"IPCam WIFI error(PA06)!!!"}'), 200

@app.route('/ipcam/reconnect_to_host/', methods=['POST'])
def ipcam_reconnect_to_host():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.reconnect_to_host(request.json)
    return jsonify('{"message":"IPCam reconnect_to_host(PG01) ~~~"}'), 200

@app.route('/ipcam/disconnect_to_host/', methods=['POST'])
def ipcam_disconnect_to_host():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.disconnect_to_host(request.json)
    return jsonify('{"message":"IPCam disconnect_to_host(PA01)!!!"}'), 200

@app.route('/ipcam/resolution_setting/', methods=['POST'])
def ipcam_resolution_setting():
    try:
        test_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501

    ipcam_lib.resolution_setting(request.json)
    return jsonify('{"message":"IPCam resolution_setting(PG02)!!!"}'), 200

# DB operation
@app.route('/api/database_add/', methods=['POST'])
def database_add():
    try:
        data_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
    
    models.database_add(data_obj)
    return jsonify('Add to DB success!!!'), 200

@app.route('/api/database_update/', methods=['POST'])
def database_update():
    try:
        data_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
    
    models.database_update(data_obj)
    return jsonify('Update to DB success!!!'), 200

@app.route('/api/database_delete/', methods=['POST'])
def database_delete():
    try:
        data_obj = json.loads(request.data.decode('utf-8'))
    except ValueError:
        return jsonify({'error_code': 1, 'error_msg': "Incorrect json format."}), 501
    
    response=models.database_delete(data_obj)
    if response:
        return jsonify('Delete from DB success!!!'), 200
    else:
        return jsonify('No data in DB!!!'), 501