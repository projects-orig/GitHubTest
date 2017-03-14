#coding=UTF-8
import datetime
import json
import requests
import sys  
import os

import app.views, app.host_lib 
import copy

DEFAULT_SERVER_ADDRESS='211.22.242.13'
CURRENT_SERVER_ADDRESS=DEFAULT_SERVER_ADDRESS
IOT_PORT='8081'
MQTT_PORT='1883'
SERVER_INDEX=0

LED8_ALM_G = 8
LED7_ALM_B = 7
LED6_ALM_R = 6
LED5_SET_G = 5
LED4_SET_B = 4
LED3_SET_R = 3
LED2_SYS_B = 2
LED1_SYS_R = 1

################################################################################
def get_iot_service_url():
  return 'http://' + CURRENT_SERVER_ADDRESS + ':' + IOT_PORT

################################################################################
# server communication function

server_common_struct={
"server_register_host":{"control":"mainhost","action":"register_mainhost"},
"server_add_host_setting":{"control":"mainhost","action":"add_mainhostset"},
"server_add_server_address":{"control":"mainhost","action":"add_customerip"},

"server_add_loop":{"control":"partition","action":"add_loop"},
"server_add_couplingloop":{"control":"partition","action":"add_couplingloop"},
"server_add_partition":{"control":"partition","action":"add_partition"},
"server_add_partitionloop":{"control":"partition","action":"add_partitionloop"},
"server_add_loopdevice":{"control":"partition","action":"add_loopdevice"},
"server_add_device":{"control":"device","action":"add_device"},
"server_add_devicetypeset":{"control":"device","action":"add_devicetypeset"},

"server_del_partition":{"control":"parititon","action":"del_partition"},
"server_del_couplingloop":{"control":"partition","action":"del_couplingloop"},
"server_del_loopdevice":{"control":"partition","action":"del_loopdevice"},
"server_del_device":{"control":"device","action":"del_device"},
#"server_del_partitionloop":{"control":"partition","action":"del_partitionloop"},

"server_add_readsensor":{"control":"device","action":"add_wlreadsensorset"},
"server_add_remotecontrol":{"control":"device","action":"add_wlremotecontrolset"},
"server_add_reedsensor":{"control":"device","action":"add_wlreedsensorset"},
"server_add_doublebondbtemperaturesensor":{"control":"device","action":"add_wldoublebondbtemperaturesensorset"},
"server_add_wlcamera":{"control":"device","action":"add_wlcameraset"},

"server_login": {"control":"complex","action":"log"},
"server_get_device_type":{"control":"device","action":"get_devicetype"},

#12_Add_WLS_Partiton	新增_無線讀訊器-分區對應
#Request (json)	value={"Common":{"control":"device","action":"add_wlremotecontrolset"},"RequestAttr":{"time":"時間","customer_number":"管制編號","device_id":"裝置id","patterns":"種類","partition_id":"分區id"}}
"server_add_wrc_partition":{"control":"device","action":"add_wlremotecontrolset"},

#7_Add_WRCWRS	新增讀訊機與遙控器的配對 for lock
#Request(json)	value={"Common":{"control":"device","action":"add_wrcwrs"},"RequestAttr":{"time":"時間","customer_number":"管制編號","WRC_ID":"遙控器ID","WRS_ID":"讀訊機ID"}}
"server_add_wrc_lock_reader":{"control":"device","action":"add_wrcwrs"},

#http://211.22.242.13:8081/sapido
#Request(json)	value={"Common":{"control":"mainhost","action":"add_maclist"},"RequestAttr":{"time":"日期","customer_number":"管制編號","mac_list":"mac名單","enable_list":"狀態"}}
"server_add_maclist":{"control":"mainhost","action":"add_maclist"},

#10_Add_Update_CamPir	新增更新cam及pir狀態
#Request(json)	value={"Common":{"control":"mainhost","action":"add_campir"},"RequestAttr":{"time":"日期","customer_number":"管制編號","device_id":"無線攝影機裝置id","enable_pir":"pir狀態"}}
"server_add_cam_pir":{"control":"mainhost","action":"add_campir"},

#20_Add_WRSPartition	新增遙控器-讀訊器 及 讀訊器-分區對應
#Request	value={"Common":{"control":"device","action":"add_wrc_wrs_partition"},"RequestAttr":{"time":"時間","customer_number":"管制編號","device_id":"遙控器device_id","data":"[{'reader_id':'讀訊器device_id','partition_id':['分區id']}]"}}
"server_add__wrc_wrs_partition":{"control":"device","action":"add_wrc_wrs_partition"},

}
 

def register_device_to_server(dev_id):
    try:
      with open(app.views.get_json_file_dir() + "Device.json", "r") as f:
        file_content=json.load(f)
        device_list=file_content["Device"]
    except IOError:
      print ("Device.json] doesn't exist. ignore it!")
      return 
    
    
    if dev_id=="":
      # upload all devices to IOT service, test only
      for device_obj in device_list:
        send_request_to_server("server_add_device", device_obj)
    else:  
      device_found=False
      for device_obj in device_list:
        if device_obj["device_id"]==dev_id:
          send_request_to_server("server_add_device", device_obj)
          device_found=True
          break

      if not device_found:
        print("device(" + dev_id + ") is NOT found. ignore it!",  flush=True)
        pass
      

def trigger_to_server(signal_obj):
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[trigger_to_server][KeepAlive.json] doesn't exist. The account is NOT opened => skip this operation",  flush=True)
      return {"status": 501, "message":"[KeepAlive.json] doesn't exist. The account is NOT opened!"} 
    except:
      print ("[trigger_to_server][KeepAlive.json] unknown error => skip this operation",  flush=True)
      return {"status": 501, "message":"[KeepAlive.json] unknown error. skip this operation."}
      
    if not app.host_lib.check_account_open(keepalive_obj):
      print ("[trigger_to_server] The account is NOT opened!",  flush=True)
      # test, zl
      audio_obj=app.audio.Audio() 
      audio_obj.play(app.host_lib.get_service_dir() + "account_not_open.mp3")
      #ending, zl
      return {"status": 501, "message":"The account is NOT opened!"}
    
    # don't check the host-alarm-set because the event may be just the set/unset...
    #if not app.host_lib.check_host_alarm_set(keepalive_obj):
    #  print("host security is unset ==> sensor event is ignored by host !",  flush=True)
    #  return {"status": 501, "message":"host security is unset ==> sensor event is ignored by host !"}
      
    signal_obj["customer_number"]=app.views.get_customer_number()
    # debug only !!!!!
    #signal_obj["customer_number"]="0005585"
    
    if "customer_push_notification_status" in keepalive_obj:
      signal_obj["send_customer_status"]=keepalive_obj["customer_push_notification_status"]
    else:
      signal_obj["send_customer_status"]="N"
      
    if "service_push_notification_status" in keepalive_obj:         
      signal_obj["send_service_status"]=keepalive_obj["service_push_notification_status"]
    else:
      signal_obj["send_service_status"]="N"
      
    datetime_obj=datetime.datetime.now()
    signal_obj["event_id"]=app.host_lib.get_event_id()
    signal_obj["time"]=str(datetime_obj)
    
    app.host_lib.save_to_event_log(signal_obj)

        
    # 1 ReceiveSignal to IOT:
    # URL http://211.22.242.13:8081/receive/signal
    # Request(POST) value={"RequestAttr":{"time":"時間","customer_number":"管制編號","event_id":"事件訊號id","loop_id":"迴路id","type_id":"裝置種類id","device_id":"裝置id","setting_value":"設定值","abnormal_value":"異常值","alert_catalog":"警報大類","abnormal_catalog":"異常類別","media_file_path":"影像上傳路徑","send_customer_status":"客戶推播","send_service_status":"勤務推播"}}
    url =  get_iot_service_url() + '/receive/signal' 
    payload = 'value={\'RequestAttr\':' + json.dumps(signal_obj) + '}'
    hash_str = app.views.gen_hash(payload)
        
    API_HEADER = dict()
    API_HEADER['Content-Type']='application/x-www-form-urlencoded;charset=utf-8'
    API_HEADER['sapido-hash']=hash_str
    API_HEADER['api-version']='0.01'
    API_HEADER['Connection']='close'
    print(url,  flush=True)
    print("payload=" + payload,  flush=True)
    try:
        app.host_lib.set_led(LED2_SYS_B, 0)
        
        if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
          # modem dialup
          app.host_lib.modem_dial_func()
    
        r = requests.post(url, data=payload, headers=API_HEADER, timeout=5)
        print("server message:",  flush=True) 
        print(r.content.decode("utf-8"),  flush=True)
        app.host_lib.off_led(LED2_SYS_B)
        obj = r.json()
        if "status" not in obj:
          obj["status"]=r.status_code  
        if "message" not in obj:
          obj["message"]=r.content.decode('utf-8')
          
    except ValueError as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"server error"}
    except requests.exceptions.ConnectionError as e:
        print("Connection error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)    
        obj = {"status": 501, "message":"network connection error"} 
        save_event_log_when_network_offline(signal_obj)

    except requests.exceptions.RequestException as e:
        print("Request exception error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"request exception"}
        save_event_log_when_network_offline(signal_obj)

    except UnicodeEncodeError as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"Unicode display error"}
    except:         
        print("Unknown error",  flush=True)  
        app.host_lib.set_led(LED2_SYS_B, 0.5)  
        obj = {"status": 501, "message":"Unknown error"}
    
    return obj
    
    
def send_request_to_server(common_key, src_obj):
    # avoid changing the original data    
    data_obj=copy.deepcopy(src_obj) # deep (recursive) copy
    
    data_obj['customer_number']=app.views.get_customer_number()
    
    #if 'time' in data_obj and data_obj['time']=='':
    print("add host real-time!!",  flush=True)
    data_obj['time']=str(datetime.datetime.now())            

    #if user_token exists...
    current_user_token=app.host_lib.get_user_token()
    if  current_user_token != "":
      data_obj['user_token']=current_user_token
    
    url = get_iot_service_url() + '/sapido'
    payload = 'value={"Common":' + json.dumps(server_common_struct[common_key]) + ',"RequestAttr":' + json.dumps(data_obj) + ', "validate":"abc"}'
    hash_str = app.views.gen_hash(payload)
    print(url,  flush=True)
    print("payload=" + payload,  flush=True)
    print("hash_str=" + hash_str,  flush=True)
    API_HEADER = dict()
    API_HEADER['Content-Type']='application/x-www-form-urlencoded;charset=utf-8'
    API_HEADER['sapido-hash']=hash_str
    API_HEADER['api-version']='0.01'
    API_HEADER['Connection']='close'
    try:
        app.host_lib.set_led(LED2_SYS_B, 0)
        
        if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
          # modem dialup
          app.host_lib.modem_dial_func()

        r = requests.post(url, data=payload, headers=API_HEADER, timeout=5)
        print("server message:",  flush=True) 
        print(r.content.decode("utf-8"),  flush=True)
        app.host_lib.off_led(LED2_SYS_B)
        obj = r.json()
        if "status" not in obj:
          obj["status"]=r.status_code  
        if "message" not in obj:
          obj["message"]=r.content.decode('utf-8')

    except ValueError as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"server error"}
    except requests.exceptions.ConnectionError as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)      
        obj = {"status": 501, "message":"network connection error"} 
    except requests.exceptions.RequestException as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"request exception"}
    except UnicodeEncodeError as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"Unicode display error"}
    except:         
        print("Unknown error",  flush=True)  
        app.host_lib.set_led(LED2_SYS_B, 0.5)  
        obj = {"status": 501, "message":"Unknown error"}
            
            
    return obj


################################################################################
def send_keepalive_to_server():
    timestamp=str(datetime.datetime.now())
    print("\n[%s][send_keepalive_to_server]:" %(timestamp),  flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[send_keepalive_to_server][KeepAlive.json] doesn't exist. The account is NOT opened => skip this operation")
      return
    except:
      print ("[send_keepalive_to_server][KeepAlive.json] unknown error => skip this operation",  flush=True)
      return
      
    #print("keepalive_obj=" + json.dumps(keepalive_obj),  flush=True)
    data=keepalive_obj
    
    # (1)_Add_KeepAlive	新增保全主機當下設定值
    # URL	http://211.22.242.13:8081/sapido
    # Request (json)	value={"Common":{"control":"keepalive","action":"add_keepalive"},"RequestAttr":{"time":"時間","customer_number":"管制編號","now_connection_way":"目前連線方式","now_connection_status":"目前連線狀態","support_connection_way":"備緩通信方式","foolproof_status":"防拆狀態","power_source":"電力來源","low_power_control":"低電量管控","power_status":"電量狀態","temperature_control":"溫度管控","temperature_status":"溫度目前狀態","control_way":"人為操控","json_version":"json版本碼","preservation_set_time":"保全設定時間","preservation_lift_time":"保全解除時間","preservation_delay_time":"保全延遲時間(分鐘)","standard_delay_set_time":"標準延後設定時間(分鐘)","preservation_set_time_delay_set":"保全設定時間_延後設定","preservation_set_time_change":"保全設定時間_暫改","preservation_lift_time_change":"保全解除時間_暫改","holiday":"假日","trigger_behindtime_set_unusual_record":"觸發逾期設定異常紀錄","trigger_early_lift_unusual_record":"觸發提早解除異常紀錄","start_host_set":"啟動保全設定","start_send_keepalive":"啟動拋送keepalive","admin_password":"超級密碼","service_push_notification_status":"勤務APP推播","customer_push_notification_status":"客戶APP推播","customer_unusual_image_send":"客戶異常影像傳送","customer_status":"客戶狀態","send_message_major_url":"傳訊主要網址","send_message_minor_url":"傳訊次要網址"},"validate":"abc"}
    url = get_iot_service_url() + '/sapido'
    data["time"]=timestamp
    data["customer_number"]=app.views.get_customer_number()
    data["device_id"]=app.host_lib.get_host_mac()
    
    #temp, zl .... app.host_lib.save_to_keepalive_log(data) 
   
    payload = 'value={"Common":{"control":"keepalive","action":"add_keepalive"}, "RequestAttr":' + json.dumps(data, sort_keys=False) + ',"validate":"abc"}'
    hash_str = app.views.gen_hash(payload)
    print("[send_keepalive_to_server] %s" %(url),  flush=True)
    #print("payload=" + payload,  flush=True)
    #print("hash_str=" + hash_str,  flush=True)
    API_HEADER = dict()
    API_HEADER['Content-Type']='application/x-www-form-urlencoded;charset=utf-8'
    API_HEADER['sapido-hash']=hash_str
    API_HEADER['api-version']='0.01'
    API_HEADER['Connection']='close'
    try:
      app.host_lib.set_led(LED2_SYS_B, 0)
      
      if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
        # modem dialup
        app.host_lib.modem_dial_func()

      r = requests.post(url, data=payload, headers=API_HEADER, timeout=5)
      print("[send_keepalive_to_server] server message:",  flush=True) 
      print(r.content.decode("utf-8"),  flush=True)
      app.host_lib.off_led(LED2_SYS_B)
    except requests.exceptions.ConnectionError as e:
      print("error: %s" %str(e),  flush=True)
      app.host_lib.set_led(LED2_SYS_B, 0.5)
      
      if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
        print("modem connection => do not operate eth0",  flush=True)
      else:
        print("do some things to help network-connection work...",  flush=True)
        # kill previous dhclient process(es) ...
        app.host_lib.kill_dhclient_process()
      
        command="route del -net default" 
        print("command=" + command,  flush=True)
        os.system(command)
        command="dhclient eth0"
        print("command=" + command,  flush=True)
        os.system(command)
    
    # test, 20170307, zl        
    except OSError as e:
      print("[send_keepalive_to_server] ZL TEST",  flush=True)
      print("error: %s" %str(e),  flush=True)
      app.host_lib.set_led(LED2_SYS_B, 0.5)
      
      if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
        print("modem connection => do not operate eth0",  flush=True)
      else:
        print("do some things to help network-connection work...",  flush=True)
        # kill previous dhclient process(es) ...
        app.host_lib.kill_dhclient_process()
      
        command="route del -net default" 
        print("command=" + command,  flush=True)
        os.system(command)
        command="dhclient eth0"
        print("command=" + command,  flush=True)
        os.system(command)
      
    except UnicodeEncodeError as e:
      print("error: %s" %str(e),  flush=True)
      app.host_lib.set_led(LED2_SYS_B, 0.5)            
    except:         
        print("[send_keepalive_to_server] Unknown error")
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        
    return
      
def send_keepalivedevice_to_server():
    timestamp=str(datetime.datetime.now())
    print("\n[%s][send_keepalivedevice_to_server]:" %(timestamp),  flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[send_keepalivedevice_to_server][KeepAlive.json] doesn't exist. The account is NOT opened => skip this operation", flush=True)
      #keepalive_obj={"json_version":""}
      return
    except:
      print ("[send_keepalivedevice_to_server][KeepAlive.json] unknown error => skip this operation", flush=True)
      return
      
    # Add_KeepAliveDevice	
    # URL	http://211.22.242.13:8081/sapido
    # Request	value={"Common":{"control":"keepalive","action":"add_keepalivedevice"},"RequestAttr":{"time":"時間","customer_number":"管制編號","loop_id":"迴路id","type_id":"裝置種類id","device_id":"裝置id","temperature_set":"溫度設定值","power_set":"電量設定值","connection_status":"目前連線狀態","signal_power_status":"裝置訊號狀態","temperature_status":"溫度狀態","power_status":"電量狀態","action":"動作","json_version":"json版本碼"},"validate":"abc"}
    url = get_iot_service_url() + '/sapido'
    
    # open json files: LoopDevice.json, Device.json, WL*.json
    try:  
      with open(app.views.get_json_file_dir() + "LoopDevice.json", "r") as f:
        loop_device_table=json.load(f)
        #print(loop_device_table,  flush=True)
    except IOError:
      print ("(LoopDevice.json) doesn't exist! ")
      return
    except:
      print ("[send_keepalivedevice_to_server][LoopDevice.json] unknown error ")
      return

    try:  
      with open(app.views.get_json_file_dir() + "Device.json", "r") as f:
        device_table=json.load(f)
        #print(device_table,  flush=True)
    except IOError:
      print ("(Device.json) doesn't exist! ")
      return
    except:
      print ("[send_keepalivedevice_to_server][Device.json] unknown error ")
      return
    
    # iterate each device on each loop
    for loop_record in loop_device_table["LoopDevice"]:        
      print("\nloop:" + loop_record["loop_id"],  flush=True)
      for device_record in device_table["Device"]:
        if loop_record["device_id"]== device_record["device_id"]:  
          print("device:" + device_record["device_id"],  flush=True)                        
          print("type:" + device_record["type_id"],  flush=True)
          
          data=dict()
          data["time"]=str(datetime.datetime.now())
          data["customer_number"]=app.views.get_customer_number()
          data["loop_id"]=loop_record["loop_id"]
          data["device_id"]=device_record["device_id"]
          data["type_id"]=device_record["type_id"]
          #data["action"]="1"  # device_record[""]????
          
          data["json_version"]=keepalive_obj["json_version"]
          
          # set reset_status for all types ...
          if app.device_lib.get_sensor_resetting_status(device_record["device_id"]) == "1" :
            # note: 0=triggered state for server definition
            data["reset_status"]="0"
          else:
            # note: 1=resetting ok for server definition
            data["reset_status"]="1"
            
          if "connection_status" in device_record:
            data["connection_status"]=device_record["connection_status"]
            
          if "signal_power_status" in device_record:
            data["signal_power_status"]=device_record["signal_power_status"]
            
          if "temperature_status" in device_record:
            data["temperature_status"]=device_record["temperature_status"]
            
          if "power_status" in device_record:
            data["power_status"]=device_record["power_status"]
          
          
          # d02 = 無線磁簧感測器, d03 = 無線雙鍵式感測器, d04 = 無線雙向式讀訊器
          if device_record["type_id"] == "d02":
            d02_table=dict()
            try:  
              with open(app.views.get_json_file_dir() + "WLReedSensor.json", "r") as f:
                d02_table=json.load(f)
                #print(d02_table,  flush=True)
            except IOError:
              print ("(WLReedSensor.json) doesn't exist! ")
              d02_table["WLReedSensor"]=list()  

            for d02_record in d02_table["WLReedSensor"]:
              if device_record["device_id"]== d02_record["device_id"]:
                if "temperature_sensing" in d02_record:
                  data["temperature_set"]=d02_record["temperature_sensing"]
                if "battery_low_power_set" in d02_record:            
                  data["power_set"]=d02_record["battery_low_power_set"]

                break
          
          elif device_record["type_id"] == "d03":
            d03_table=dict()
            try:  
              with open(app.views.get_json_file_dir() + "WLDoubleBondBTemperatureSensor.json", "r") as f:
                d03_table=json.load(f)
                #print(d03_table,  flush=True)
            except IOError:
              print ("(WLDoubleBondBTemperatureSensor.json) doesn't exist! ")
              d03_table["WLDoubleBondBTemperatureSensor"]=list()  

            for d03_record in d03_table["WLDoubleBondBTemperatureSensor"]:
              if device_record["device_id"]== d03_record["device_id"]:
                if "temperature_sensing" in d03_record:
                  data["temperature_set"]=d03_record["temperature_sensing"]
                if "battery_low_power_set" in d03_record:            
                  data["power_set"]=d03_record["battery_low_power_set"]

                break
                
          elif device_record["type_id"] == "d04":
            d04_table=dict()
            try:  
              with open(app.views.get_json_file_dir() + "WLReadSensor.json", "r") as f:
                d04_table=json.load(f)
                #print(d04_table,  flush=True)
            except IOError:
              print ("(WLReadSensor.json) doesn't exist! ")
              d04_table["WLReadSensor"]=list()    

            for d04_record in d04_table["WLReadSensor"]:
              if device_record["device_id"]== d04_record["device_id"]:
                if "temperature_sensing" in d04_record:
                  data["temperature_set"]=d04_record["temperature_sensing"]
                if "battery_low_power_set" in d04_record:            
                  data["power_set"]=d04_record["battery_low_power_set"]
                
                break
                     
          else:
            print("type_id(%s) is NOT matched!" %(type_id),  flush=True)
            break
          
          payload = 'value={"Common":{"control":"keepalive","action":"add_keepalivedevice"},"RequestAttr":' + json.dumps(data) + ',"validate":"abc"}'
          hash_str = app.views.gen_hash(payload)
          print(url,  flush=True)
          print("payload=" + payload,  flush=True)
          #print("hash_str=" + hash_str,  flush=True)
          API_HEADER = dict()
          API_HEADER['Content-Type']='application/x-www-form-urlencoded;charset=utf-8'
          API_HEADER['sapido-hash']=hash_str
          API_HEADER['api-version']='0.01'
          API_HEADER['Connection']='close'
          try:
            app.host_lib.set_led(LED2_SYS_B, 0)
            
            if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
              # modem dialup
              app.host_lib.modem_dial_func()

            r = requests.post(url, data=payload, headers=API_HEADER, timeout=5)
            print("server message:"+ r.content.decode('utf-8'),  flush=True)
            app.host_lib.off_led(LED2_SYS_B)
          except requests.exceptions.ConnectionError as e:
            print("error: %s" %str(e),  flush=True)
            app.host_lib.set_led(LED2_SYS_B, 0.5)
            
            if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
              print("modem connection => do not operate eth0",  flush=True)
            else:
              print("do some things to help network-connection work...",  flush=True)
              # kill previous dhclient process(es) ...
              app.host_lib.kill_dhclient_process()
            
              command="route del -net default" 
              print("command=" + command,  flush=True)
              os.system(command)
              command="dhclient eth0"
              print("command=" + command,  flush=True)
              os.system(command)
          except UnicodeEncodeError as e:
            print("error: %s" %str(e),  flush=True)
            app.host_lib.set_led(LED2_SYS_B, 0.5)            
          except:         
            print("[send_keepalivedevice_to_server] Unknown error")
            app.host_lib.set_led(LED2_SYS_B, 0.5)

          
          break
        # end of device_id match
      # end of device-table iteration  
    # end of loop-device-table iteration
    
################################################################################
# check network status test:

# check [IOT SERVICE] echo
def check_server(server_addr, server_port):
    print("[%s][check_server] server_addr=%s" %(str(datetime.datetime.now()), server_addr),  flush=True)
    # 測試server連線狀態
    # URL   http://<server_addr>:<port>/sapido/status
    # Request(GET)  
    # Response(json)    連線正常:{ status:200, message:"on line" }   
    url = 'http://' + server_addr + ':' + server_port + '/sapido/status'
    #print(url,  flush=True)
    
    # http://stackoverflow.com/questions/16511337/correct-way-to-try-except-using-python-requests-module
    try:
      if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
        # modem dialup
        app.host_lib.modem_dial_func()
        
      r = requests.get(url, timeout=3)
      #print("server message:",  flush=True) 
      #print(r.content.decode("utf-8"),  flush=True)
      print("[%s][check_server] ok" %(str(datetime.datetime.now())),  flush=True)
      return True
    except requests.exceptions.RequestException as e:
      print("check_server() error ...",  flush=True) 
      #print(e)
      return False
    except:         
      print("Unknown error",  flush=True) 
      return False
        
def get_current_server_address():
    return CURRENT_SERVER_ADDRESS

def get_current_iot_port():
    return IOT_PORT
    
def get_current_mqtt_port():
    return MQTT_PORT
            
# 偵測網路狀態: 
def check_network_status():
    global CURRENT_SERVER_ADDRESS
    global IOT_PORT
    global MQTT_PORT
    global SERVER_INDEX
    
    try:
      with open(app.views.get_json_file_dir() + "ServerAddress.json", "r") as f:
        data_obj=json.load(f)
        server_address_dict=data_obj["ServerAddress"][0]
    except IOError:
        #print ("[ServerAddress.json] file doesn't exist.")
        server_address_dict=dict()
        #return False
    
    server_addr=""
    server_port=""
    try:
        server1=server_address_dict["server1"]
        if "addr" in server1:  
          server_addr=server1["addr"]
        if "iot_port" in server1:  
          server_port=str(server1["iot_port"])
        else:
          server_port='8081'
          
        if "mqtt_port" in server1:  
          mqtt_port=str(server1["mqtt_port"])
        else:
          mqtt_port='1883'
                        
        print('server1=' + server_addr +':' + server_port,  flush=True)
        if check_server(server_addr, server_port):
          CURRENT_SERVER_ADDRESS=server_addr
          IOT_PORT=server_port
          MQTT_PORT=mqtt_port
          SERVER_INDEX=1
          print("server1 is ok!",  flush=True)
          return True   
    except KeyError:
        print("server1 is NOT found. Skip it.",  flush=True)
     
    try:
        server2=server_address_dict["server2"]
        if "addr" in server2:  
          server_addr=server2["addr"]
        if "iot_port" in server2:  
          server_port=str(server2["iot_port"])
        else:
          server_port='8081'              

        if "mqtt_port" in server2:  
          mqtt_port=str(server2["mqtt_port"])
        else:
          mqtt_port='1883'

        print('server2=' + server_addr +':' + server_port,  flush=True)
        if check_server(server_addr, server_port):
          CURRENT_SERVER_ADDRESS=server_addr
          IOT_PORT=server_port
          MQTT_PORT=mqtt_port
          SERVER_INDEX=2
          print("server2 is ok!",  flush=True)
          return True   
    except KeyError:
        print("server2 is NOT found. Skip it.",  flush=True)
     
    try:
        server3=server_address_dict["server3"]
        if "addr" in server3:  
          server_addr=server3["addr"]
        if "iot_port" in server3:  
          server_port=str(server3["iot_port"])
        else:
          server_port='8081' 
        
        if "mqtt_port" in server3:  
          mqtt_port=str(server3["mqtt_port"])
        else:
          mqtt_port='1883'
               
        print('server3=' + server_addr +':' + server_port,  flush=True)
        if check_server(server_addr, server_port):
          CURRENT_SERVER_ADDRESS=server_addr
          IOT_PORT=server_port
          MQTT_PORT=mqtt_port
          SERVER_INDEX=3
          print("server3 is ok!",  flush=True)
          return True   
    except KeyError:
        print("server3 is NOT found. Skip it.",  flush=True)
        
    try:
        server4=server_address_dict["server4"]
        if "addr" in server4:  
          server_addr=server4["addr"]
        if "iot_port" in server4:  
          server_port=str(server4["iot_port"])
        else:
          server_port='8081' 
        
        if "mqtt_port" in server4:  
          mqtt_port=str(server4["mqtt_port"])
        else:
          mqtt_port='1883'
               
        print('server4=' + server_addr +':' + server_port,  flush=True)
        if check_server(server_addr, server_port):
          CURRENT_SERVER_ADDRESS=server_addr
          IOT_PORT=server_port
          MQTT_PORT=mqtt_port
          SERVER_INDEX=4
          print("server4 is ok!",  flush=True)
          return True   
    except KeyError:
        print("server4 is NOT found. Skip it.",  flush=True)            
    
    # check default server ...
    server_addr=DEFAULT_SERVER_ADDRESS
    server_port='8081'              
    if check_server(server_addr, server_port):
      CURRENT_SERVER_ADDRESS=server_addr
      IOT_PORT=server_port
      MQTT_PORT='1883'
      SERVER_INDEX=0
      print("DEFAULT server is ok!",  flush=True)
      return True   
    else:
      print("DEFAULT server is disconnected",  flush=True)            

    print('NO server is available!',  flush=True)
    return False


################################################################################
# update to server
def update_to_server(table_name, record):
        if table_name == 'CouplingLoop':
          #convert "loop_id":["aa","bb"] to "loop_id":"aa,bb" for server compatibility ...
          # in case the key doesn't exist...
          try:
            org_list=record["loop_id"]
          except KeyError:
            record["loop_id"]=list()
            org_list=list()
          
          tmp_str=""
          for loop_id in record["loop_id"]:
            if tmp_str == '':
              tmp_str=str(loop_id)
            else:
              tmp_str=tmp_str + "," + str(loop_id)
          record["loop_id"]=tmp_str
          send_request_to_server("server_add_couplingloop", record)
          #restore "loop_id" ...
          record["loop_id"]=org_list

        elif table_name == 'RemoteControl_Partition':
          #convert "partition_id":["aa","bb"] to "partition_id":"aa,bb" for server compatibility ...
          # in case the key doesn't exist...
          try:
            org_list=record["partition_id"]
          except KeyError:
            record["partition_id"]=list()
            org_list=list()
            
          tmp_str=""
          for partition_id in record["partition_id"]:
            if tmp_str == '':
              tmp_str=str(partition_id)
            else:
              tmp_str=tmp_str + "," + str(partition_id)
          
          record["partition_id"]=tmp_str
          send_request_to_server("server_add_wrc_partition", record)
          
          #restore "partition_id" list ...
          record["partition_id"]=org_list
            
        elif table_name == 'RemoteControl_LockReader':
          #convert "reader_id":["aa","bb"] to "reader_id":"aa,bb" for server compatibility ...
          # in case the key doesn't exist...
          try:
            org_list=record["reader_id"]
          except KeyError:
            record["reader_id"]=list()
            org_list=list()

          tmp_str=""
          for reader_id in record["reader_id"]:
            if tmp_str == '':
              tmp_str=str(reader_id)
            else:
              tmp_str=tmp_str + "," + str(reader_id)
          
          record["reader_id"]=tmp_str
          
          send_request_to_server("server_add_wrc_lock_reader", record)
          
          #restore "reader_id" list  ...
          record["reader_id"]=org_list

        elif table_name == 'RemoteControl_PartitionReader':
          # convert  ”reader_array” :[ {"reader_id":”aa”, "partition_id”:["p1”, "p2”,...]}, .. ]                                       
          # to "data":"[{'reader_id':'','partition_id':['分區id']} ...]"
          # for server compatibility ...
          # in case the key doesn't exist...
          try:
            org_list=record["reader_array"]
          except KeyError:
            record["reader_array"]=list()
            org_list=list()

          record["data"]=str(org_list)
          record.pop("reader_array", None)
          
          send_request_to_server("server_add__wrc_wrs_partition", record)
          
          #restore "reader_array" list  ...
          record["reader_array"]=org_list
          
          #remove "data" 
          record.pop("data", None)

        
        elif table_name == 'Loop':
          #register_device_to_server("") # just in case
          send_request_to_server("server_add_loop", record)

        elif table_name == 'Partition':
          # add partition to server first
          tmp={ "partition_id":record["partition_id"] } 
          send_request_to_server("server_add_partition", tmp)
          
          #convert "loop_id":["aa","bb"] to "loop_id":"aa,bb" for server compatibility ...
          # in case the key doesn't exist...
          try:
            org_list=record["loop_id"]
          except KeyError:
            record["loop_id"]=list()
            org_list=list()
          
          tmp_str=""
          for loop_id in record["loop_id"]:
            if tmp_str == '':
              tmp_str=str(loop_id)
            else:
              tmp_str=tmp_str + "," + str(loop_id)
          record["loop_id"]=tmp_str
          
          send_request_to_server("server_add_partitionloop", record)
          #restore "loop_id" ...
          record["loop_id"]=org_list

        elif table_name == 'HostMacSetting':
          #convert "mac_list":["aa","bb"] to "mac_list":"aa,bb" for server compatibility ...
          # in case the key doesn't exist...
          try:
            org_list=record["mac_list"]
          except KeyError:
            record["mac_list"]=list()
            org_list=list()
            
          tmp_str=""
          for mac in record["mac_list"]:
            if tmp_str == '':
              tmp_str=str(mac)
            else:
              tmp_str=tmp_str + "," + str(mac)
          record["mac_list"]=tmp_str
          
          send_request_to_server("server_add_maclist", record)
          #restore "mac_list" ...
          record["mac_list"]=org_list
                    
        elif table_name == 'LoopDevice':
          register_device_to_server(record["device_id"]) #just in case
          send_request_to_server("server_add_loopdevice", record)
        elif table_name == 'Device':
          send_request_to_server("server_add_device", record)
        elif table_name == 'DeviceSetting':
          send_request_to_server("server_add_devicetypeset", record)
        elif table_name == 'ServerAddress':
          
          # {"server1":{"ftp_port": "23", "addr": "www.y4.com", "mqtt_port": "111", "iot_port": "222"}, ...}
          # to
          # "ip":"{'server1':{'addr':'78.78.78.78','mqtt_port':'801','ftp_port':'2,2,2,2','iot_port':'3,3,3,3'},'server2':{'addr':'39.39.39.39','mqtt_port':'801','ftp_port':'2,2,2,2','iot_port':'3,3,3,3'}}"
          # ip的值,必須是json字串.
          tmp=dict()
          tmp["ip"]=str(record)
          send_request_to_server("server_add_server_address", tmp)
          
        elif table_name == 'WLRemoteControl':
          #register_device_to_server(record["device_id"]) #just in case
          send_request_to_server("server_add_remotecontrol", record)
        elif table_name == 'WLReadSensor':
          #register_device_to_server(record["device_id"]) #just in case
          send_request_to_server("server_add_readsensor", record)
        elif table_name == 'WLReedSensor':
          #register_device_to_server(record["device_id"]) #just in case
          send_request_to_server("server_add_reedsensor", record)
        elif table_name == 'WLDoubleBondBTemperatureSensor':
          #register_device_to_server(record["device_id"]) #just in case
          send_request_to_server("server_add_doublebondbtemperaturesensor", record)
        elif table_name == 'WLIPCam':
          #register_device_to_server(record["device_id"]) #just in case
          send_request_to_server("server_add_wlcamera", record)
                            
        elif table_name == 'HostSetting':
          # http://stackoverflow.com/questions/6874906/convert-integer-to-hex-string-with-specific-format
          host_registration_obj={"mac_identify":app.host_lib.get_host_mac(), "customer_number":app.views.get_customer_number()}
          send_request_to_server("server_register_host", host_registration_obj)
          send_request_to_server("server_add_host_setting", record)

        elif table_name == 'HostCamPir':
          send_request_to_server("server_add_cam_pir", record)          
                  
        else:
          print('Unknown table, skip server processing',  flush=True)  


  
def update_media_file_path_to_server(data_obj):
    # set default response in advance...
    api_response=dict()
    # Update_media_file_path 更新事件影像檔路徑
    # URL http://211.22.242.13:8081/update/mediafilepath
    # Request (json)  value={"RequestAttr":{"customer_number":"管制編號", "event_id":"辨識訊號id", "media_file_path":"影像檔路徑"}}
    url =  get_iot_service_url() + '/update/mediafilepath' 
    payload = 'value={\'RequestAttr\':' + json.dumps(data_obj) + '}'
    headers = {'Content-Type': 'application/x-www-form-urlencoded;charset=utf-8'}
    print(url,  flush=True)
    print("payload=" + payload,  flush=True)
    try:
        app.host_lib.set_led(LED2_SYS_B, 0)
        
        if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
          # modem dialup
          app.host_lib.modem_dial_func()

        r = requests.post(url, data=payload, headers=headers, timeout=5)
        print("server message:",  flush=True) 
        print(r.content.decode("utf-8"),  flush=True)
        app.host_lib.off_led(LED2_SYS_B)
        obj = r.json()
        if "status" not in obj:
          obj["status"]=r.status_code  
        if "message" not in obj:
          obj["message"]=r.content.decode('utf-8')
  
    except ValueError as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"server error"}
    except requests.exceptions.ConnectionError as e:
        print("error: %s" %str(e),  flush=True)     
        app.host_lib.set_led(LED2_SYS_B, 0.5) 
        obj = {"status": 501, "message":"network connection error"} 
    except requests.exceptions.RequestException as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"request exception"}
    except UnicodeEncodeError as e:
        print("error: %s" %str(e),  flush=True)
        app.host_lib.set_led(LED2_SYS_B, 0.5)
        obj = {"status": 501, "message":"Unicode display error"}
    except:         
        print("Unknown error",  flush=True)  
        app.host_lib.set_led(LED2_SYS_B, 0.5)  
        obj = {"status": 501, "message":"Unknown error"}
        
    return obj

   
################################################################################
def update_all_ipcam_to_server():
    print ("[update_all_ipcam_to_server]",  flush=True)
    table_name="WLIPCam"
    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
            file_content=json.load(f)
            file_content_list=file_content[table_name]
    except IOError:
      print ("[update_all_ipcam_to_server] the given file doesn't exist.",  flush=True)
      return
      
    for record in file_content_list:
      device_id=str(record["device_id"])
      print ("[update_all_ipcam_to_server] device_id=%s" %(device_id),  flush=True)
      #register device to server
      app.server_lib.register_device_to_server(device_id)
      # update config to server
      app.server_lib.update_to_server(table_name, record)


def save_event_log_when_network_offline(log_obj):
    
    print("[save_event_log_when_network_offline] Save event log when network is down~~~")
    log_file_name="event_log_when_network_offline.json"
    
    # read the corresponding log file
    try:
      with open("/var/log/" + log_file_name, "r") as f:
        log_list=json.load(f)
        #print(log_list)
    except IOError:
      log_list=list()       
    
    log_list.append(log_obj)  
    # write back to the corresponding log file
    with open("/var/log/" + log_file_name, "w") as f:
      json.dump(log_list, f)                                                         
      
def send_event_when_network_reconnect():

    print("[send_event_when_network_reconnect] Sending trigger event when network reconnect~~~")
    try:
        with open(app.views.get_json_file_dir() + "event_log_when_network_offline.json", "r") as f:
          log_list=json.load(f)

    except IOError:
        print ("[event_log_when_network_offline.json] file doesn't exist !!!", flush=True)
        return
    
    list_num=len(log_list)
    print("list number: " + str(list_num))
    counter = 1
    #record index to delete
    record_index=0
    delete_list=list()

    for item in log_list:

        print("counter: " + str(counter))
        #print("list_current_num: " + str(list_current_num))
        print("event: "+ str(item))
        event_str = str(item)

        url = app.server_lib.get_iot_service_url() + '/receive/signal' 
        payload = 'value={\'RequestAttr\':' + event_str + '}'
        #payload_str = str(payload)
        hash_str = app.views.gen_hash(payload)
            
        API_HEADER = dict()
        API_HEADER['Content-Type']='application/x-www-form-urlencoded;charset=utf-8'
        API_HEADER['sapido-hash']=hash_str
        API_HEADER['api-version']='0.01'
        API_HEADER['Connection']='close'
        print(url,  flush=True)
        print("[send_event_when_network_reconnect]payload=" + payload,  flush=True)
        try:
            app.host_lib.set_led(LED2_SYS_B, 0)
            r = requests.post(url, data=payload, headers=API_HEADER, timeout=5)
            print("server message:",  flush=True) 
            print(r.content.decode("utf-8"),  flush=True)
            app.host_lib.off_led(LED2_SYS_B)
            obj = r.json()
            if "status" not in obj:
              obj["status"]=r.status_code  
            if "message" not in obj:
              obj["message"]=r.content.decode('utf-8')
            print("[send_event_when_network_reconnect] Send event success, remove record from json")

            #record index to delete list
            delete_list.append(record_index)   
            record_index+=1

        except ValueError as e:
            print("error: %s" %str(e),  flush=True)
            app.host_lib.set_led(LED2_SYS_B, 0.5)
            print("[send_event_when_network_reconnect] server error")
            break
        except requests.exceptions.ConnectionError as e:
            print("error: %s" %str(e),  flush=True)
            app.host_lib.set_led(LED2_SYS_B, 0.5)    
            print("[send_event_when_network_reconnect] network connection error")
            break 
        except requests.exceptions.RequestException as e:
            print("error: %s" %str(e),  flush=True)
            app.host_lib.set_led(LED2_SYS_B, 0.5)
            print("[send_event_when_network_reconnect] request exception!!!")
            break
        except UnicodeEncodeError as e:
            print("error: %s" %str(e),  flush=True)
            app.host_lib.set_led(LED2_SYS_B, 0.5)
            obj = {"status": 501, "message":"Unicode display error"}
        except:         
            print("Unknown error",  flush=True)  
            app.host_lib.set_led(LED2_SYS_B, 0.5)  
            obj = {"status": 501, "message":"Unknown error"}

        counter +=1

    #delete the event if sent success
    print(delete_list,  flush=True) 
    for delete_index in sorted(delete_list, reverse=True):
        print("delete index: " + str(delete_index),  flush=True)
        del log_list[delete_index]
    
    with open(app.views.get_json_file_dir() + "event_log_when_network_offline.json", "w") as f:
      json.dump(log_list, f) 