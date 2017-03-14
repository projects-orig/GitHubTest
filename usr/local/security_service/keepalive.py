#coding=UTF-8

import os
import sys, time
import signal
import threading, datetime
import requests
import json
import hashlib
import paho.mqtt.client as mqtt
import random

import app.views
import app.server_lib
import app.host_lib
import app.device_lib
import app.ipcam_lib
import app.audio

try:
  import app.modem
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[import modem] unknown error.",  flush=True)

try: 
  import app.gpio_io_api
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[import gpio_io_api] unknown error.",  flush=True)

try: 
  import app.led_api
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[import led_api] unknown error.",  flush=True)
  
try: 
  import app.lcm_api
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[import lcm_api] unknown error.",  flush=True)

try:
  import Adafruit_BBIO.ADC as ADC
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[import Adafruit_BBIO.ADC] unknown error.",  flush=True)
  
  

LED8_ALM_G = 8
LED7_ALM_B = 7
LED6_ALM_R = 6
LED5_SET_G = 5
LED4_SET_B = 4
LED3_SET_R = 3
LED2_SYS_B = 2
LED1_SYS_R = 1


interrupted=False

#KEEPALIVE_INTERVAL=3
KEEPALIVE_INTERVAL=10

DELAY_SET_TRIGGER_FLAG=False
DELAY_SET_TRIGGER_FLAG2=False
DELAY_SET_TRIGGER_FLAG3=False

DELAY_SET_NONSETUP_FLAG=False

TRIGGER_BASE_TIME=datetime.datetime.now()

# 0=正常, 1=異常
now_connection_status="0"

timeButtonCount=0
timeButtonHour=0
timeButtonMinute=0

dateButtonCount=0
dateButtonYear=1
dateButtonMonth=1
dateButtonDay=1

################################################################################
# redirect output of print to a TXT file 
# http://stackoverflow.com/questions/4110891/python-how-to-simply-redirect-output-of-print-to-a-txt-file-with-a-new-line-crea
# https://docs.python.org/3/library/functions.html
#DEBUG_FILE_PATH = '/tmp/'
#debug_file=DEBUG_FILE_PATH + "keepalive_debug.txt"
#sys.stdout = open(debug_file, "a+")
#print(str(datetime.datetime.now()) + " : debug output starts ...",  flush=True)


################################################################################
# MQTT subscriber for 保全主機逆控

MQTT_USERNAME = "amigo"
MQTT_PASSWORD = "swetop"
MQTT_QOS = 2
MQTT_CA_PATH = app.host_lib.get_service_dir() + "/ca/"
MQTT_CA_CERTS = MQTT_CA_PATH + "rootCA.pem"
MQTT_CERTFILE = MQTT_CA_PATH + "client.crt"
MQTT_KEYFILE = MQTT_CA_PATH + "client.key"

def mqtt_client_thread():
    print("[%s][mqtt_client_thread] starts." %(str(datetime.datetime.now())),  flush=True)
    # check if the customer_number exist
    # note: if customer_number is changed, 
    # communication_service must restart keepalive_service to restart 
    # in order to subscribe new topic.  
    while not interrupted:
      if app.views.get_customer_number() == "" :
        print("[mqtt_client_thread]: customer_number does NOT exist. wait ...",  flush=True)
        time.sleep(10)
      else:
        break
        
          
    client_id = app.host_lib.get_host_mac()
    print("[%s][mqtt_client_thread] client_id=%s" %(str(datetime.datetime.now()), client_id),  flush=True)
    
    # https://pypi.python.org/pypi/paho-mqtt/1.1
    # http://stackoverflow.com/questions/31980126/mqtt-broker-bridge-data-persistence
    #client = mqtt.Client(client_id=client_id, clean_session=False)
    client = mqtt.Client(client_id=client_id)

    # If broker asks user/password.
    # client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
    
    client.on_connect = on_connect
    client.on_message = on_message
    client.on_subscribe = on_subscribe
    
    try:
        server_addr = app.server_lib.get_current_server_address()
        mqtt_port = app.server_lib.get_current_mqtt_port()
        print("MQTT service=%s:%s" %(server_addr, mqtt_port),  flush=True)
        client.connect(server_addr, port=int(mqtt_port))
    except:
        print("MQTT Broker is not online. Connect later.",  flush=True)

    cnt = 0
    while not interrupted:
        client.loop()
        cnt += 1
        if cnt > 20:
            try:
                print("[%s][mqtt_client_thread] loop quits ???" %(str(datetime.datetime.now())),  flush=True)
                client.reconnect()  # to avoid 'Broken pipe' error.
            except:
                time.sleep(1)
            cnt = 0

    print("quit mqtt thread",  flush=True)
    client.disconnect()


#def on_connect(mq, userdata, rc, _):
def on_connect(mq, userdata, flags, rc):
    print("[%s] [on_connect]:" %(str(datetime.datetime.now())), flush=True)
    print("Connected with result code "+str(rc))
    topic = 'H' + app.views.get_customer_number()
    print("MQTT topic=" + topic,  flush=True)
    try:
      mq.subscribe(topic, qos=MQTT_QOS)
    except:
      print("[on_connect] subscribe error" , flush=True)
      pass  

def on_subscribe(mosq, obj, mid, granted_qos):
    print("[%s] [on_subscribe]:" %(str(datetime.datetime.now())), flush=True)
    print("%s, %s, %s" %(str(mid), str(obj), str(granted_qos)),  flush=True)
        
def on_message(mq, userdata, msg):
    print("[%s][on_message] topic: %s" %(str(datetime.datetime.now()), msg.topic),  flush=True)
    #print("qos: %d" % msg.qos,  flush=True)
    #print("[on_message] payload: %s" % msg.payload,  flush=True)
    
    # got backcontrol message => check if the payload is json format 
    try:
        backcontrol_obj = json.loads(msg.payload.decode('utf-8'))
    except ValueError:
        print("NOT json payload: %s" % msg.payload,  flush=True)
        return
    except:
        print("[on_message] unknown error...",  flush=True)
        return
                
    print("[on_message] backcontrol_obj: %s" % backcontrol_obj,  flush=True)    
    try:
      backcontrol_process(backcontrol_obj)
      print("[%s][on_message] done." %(str(datetime.datetime.now())),  flush=True)
    except:
      print("[on_message] backcontrol_process() error \n",  flush=True)


    
def backcontrol_process(backcontrol_obj):         
    try:
        with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
          file_content=json.load(f)
          #print(file_content,  flush=True)
    except IOError:
        #print ("[KeepAlive.json] file doesn't exist !!!", flush=True)
        #return
        print ("[KeepAlive.json] file doesn't exist. Create empty content.", flush=True)
        file_content=dict()
        file_content["KeepAlive"]=[{}]
    except:
        print ("[backcontrol_process] unknown error. Create empty content.", flush=True)
        file_content=dict()
        file_content["KeepAlive"]=[{}]
        
    keepalive_obj=file_content["KeepAlive"][0]

    # check the current version of keep-alive file
    #if keepalive_obj['json_version'] != backcontrol_obj['ver']:
    if True:
        #print("got new backcontrol message: json_version=" + backcontrol_obj['ver'],  flush=True)
        keepalive_obj['json_version'] = backcontrol_obj['ver']
        
        ########################################################
        # {
        # "cusno": "0005585",
        # "ver": "20161215093822076",
        # "data": {
        #    "HostState": "N",
        #    "API": "http://211.22.242.13:8164/api/Security/Actionconfirm"
        #  }
        # }
        # merge the payload(backcontrol) to KeepAlive.json
        transform_backcontrol_to_keepalive(keepalive_obj, backcontrol_obj["data"])  
        
        # save to json file
        with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
          json.dump(file_content, f)

        ########################################################
        # update ServerAddress.json
        try:
          with open(app.views.get_json_file_dir() + "ServerAddress.json", "r") as f:
            data_obj=json.load(f)
        except IOError:
          print ("[ServerAddress.json] file doesn't exist! Create basic structure")
          json_str='{"ServerAddress":[{"server1":{},"server2":{},"server3":{},"server4":{}}]}'
          data_obj=json.loads(json_str)
        except:
          print ("[backcontrol_process] unknown error. Create basic structure.", flush=True)
          json_str='{"ServerAddress":[{"server1":{},"server2":{},"server3":{},"server4":{}}]}'
          data_obj=json.loads(json_str)
            
        server_address_dict=data_obj["ServerAddress"][0]            
        if "send_message_major_url" in keepalive_obj:
          server_address_dict["server1"]["addr"] =keepalive_obj["send_message_major_url"]
          
        if "send_message_minor_url" in keepalive_obj:
          server_address_dict["server2"]["addr"] =keepalive_obj["send_message_minor_url"]  
        
        if "send_message_third_url" in keepalive_obj:
          server_address_dict["server3"]["addr"] =keepalive_obj["send_message_third_url"]
          
        if "send_message_fourth_url" in keepalive_obj:
          server_address_dict["server4"]["addr"] =keepalive_obj["send_message_fourth_url"]
        
        # save to json file
        with open(app.views.get_json_file_dir() + "ServerAddress.json", "w") as f:
          json.dump(data_obj, f)
          
        
        global DELAY_SET_TRIGGER_FLAG
        global DELAY_SET_TRIGGER_FLAG2
        global DELAY_SET_TRIGGER_FLAG3
        global TRIGGER_BASE_TIME
        global DELAY_SET_NONSETUP_FLAG

        if "DelayTime" in backcontrol_obj["data"] :
          print("DelayTime changed !!! \n\n\n") 
          DELAY_SET_TRIGGER_FLAG2=False
          DELAY_SET_TRIGGER_FLAG3=False
          DELAY_SET_NONSETUP_FLAG=False
          
        if "NonSetup" in backcontrol_obj["data"]:
          print("NonSetup changed !!! \n\n\n")
          DELAY_SET_TRIGGER_FLAG2=False
          DELAY_SET_TRIGGER_FLAG3=False
          DELAY_SET_NONSETUP_FLAG=True
          TRIGGER_BASE_TIME=datetime.datetime.now()


          
        if "TouchTimeDelay" in backcontrol_obj["data"] and backcontrol_obj["data"]["TouchTimeDelay"]=='Y':
          print('[backcontrol_process] trigger delay set record !',  flush=True)
          # check "appid" in backcontrol_obj data
          try: 
            appid=backcontrol_obj["data"]["appid"]
          except:
            appid=""
          
          # check "SecurityId" in backcontrol_obj data
          try: 
            securityid=backcontrol_obj["data"]["SecurityId"]
          except:
            securityid=""
          
          signal_obj=dict()
          signal_obj["type_id"]="d01"
          signal_obj["alert_catalog"]="U"
          signal_obj["abnormal_catalog"]="GU05"
          current_time=datetime.datetime.now().strftime("%H:%M")
          if appid != "": 
              signal_obj["setting_value"]="APP"
              signal_obj["abnormal_value"]=appid
          elif securityid != "": 
              signal_obj["setting_value"]="Security"
              signal_obj["abnormal_value"]=securityid
          else:
              signal_obj["setting_value"]=keepalive_obj["preservation_set_time"]
              signal_obj["abnormal_value"]=current_time
          
          app.host_lib.set_event_id()
          
          obj = app.server_lib.trigger_to_server(signal_obj)
          if obj['status']==200:
            print("send event of host security delay set ok",  flush=True)
            pass
    
        if "TouchEarlyRelease" in backcontrol_obj["data"] and backcontrol_obj["data"]["TouchEarlyRelease"]=='Y':
          print('[backcontrol_process] trigger early unset record !',  flush=True)
          # check "appid" in backcontrol_obj data
          try: 
            appid=backcontrol_obj["data"]["appid"]
          except:
            appid=""
          
          # check "SecurityId" in backcontrol_obj data
          try: 
            securityid=backcontrol_obj["data"]["SecurityId"]
          except:
            securityid=""

          current_time=datetime.datetime.now().strftime("%H:%M")

          signal_obj=dict()
          signal_obj["type_id"]="d01"
          signal_obj["alert_catalog"]='U'
          signal_obj["abnormal_catalog"]='GU01'
          signal_obj["device_id"]=app.host_lib.get_host_mac()
          if appid != "": 
              signal_obj["setting_value"]="APP"
              signal_obj["abnormal_value"]=appid
          elif securityid != "": 
              signal_obj["setting_value"]="Security"
              signal_obj["abnormal_value"]=securityid
          else:
              signal_obj["setting_value"]=keepalive_obj["preservation_lift_time"]
              signal_obj["abnormal_value"]=current_time
          
          app.host_lib.set_event_id()
          # trigger to server                 
          obj=app.server_lib.trigger_to_server(signal_obj)
          if obj['status']==200:
            print("send event of host security early unset ok",  flush=True)
            pass
        
        if "StdSettingTime" in backcontrol_obj["data"] or "StdReleaseTime" in backcontrol_obj["data"]:
          print("StdSettingTime or StdReleaseTime changed !!! \n\n\n")
          DELAY_SET_TRIGGER_FLAG=False
        
        ########################################################
        # check if backcontrol has "battery" setting change    
        # {
        #   "cusno": "0005815",
        #   "ver": "2017-01-12 11:40:37.585",
        #   "data": {
        #     "Kind": "Device",
        #     "DeviceType": "d02",
        #     "battery": "70",
        #     "API": "http://211.22.242.13:8164/api/Security/BasicTU"
        #   }
        # }
        if "battery" in backcontrol_obj["data"] :
          type_id=str(backcontrol_obj["data"]["DeviceType"])
          value=str(backcontrol_obj["data"]["battery"])
          app.host_lib.save_user_token("sapido_admin")
          if type_id=="d01":
            app.host_lib.battery_config(value)
          else:
            app.device_lib.sensor_battery_config(type_id, value)          

          
                         
        ########################################################
        # check if backcontrol has "HostState" setting change
        # {  
        #  "cusno": "003301",  
        #  "ver": "20161206111307103",
        #  "data": {"HostState": "0", "API": "http://211.22.242.13:8164/api/Security/Actionconfirm"} 
        # }
        if "HostState" in backcontrol_obj["data"] :
          print("HostState changed !!! \n\n\n")
          DELAY_SET_TRIGGER_FLAG=False

          # check "appid" in backcontrol_obj data
          try: 
            appid=backcontrol_obj["data"]["appid"]
          except:
            appid=""
          
          # check "SecurityId" in backcontrol_obj data
          try: 
            securityid=backcontrol_obj["data"]["SecurityId"]
          except:
            securityid=""
                          
          if backcontrol_obj["data"]["HostState"]=="Y" or backcontrol_obj["data"]["HostState"]=="1":
            print("new host state=set",  flush=True)
            abnormal_catalog='GS01'

            # all sensor resetting status must be "1" before security set
            if app.device_lib.check_all_sensor_resetting_status():
              audio_obj=app.audio.Audio() 
              audio_obj.play(app.host_lib.get_service_dir() + "system_set_ok.mp3")
              app.device_lib.ask_readers_make_sound(6, 1)
            else:
              audio_obj=app.audio.Audio()
              # "system_set_fail.mp3" takes 2.8sec, set 8.4sec to play it three times 
              audio_obj.play(app.host_lib.get_service_dir() + "system_set_fail.mp3", 8.4)
              app.device_lib.ask_readers_make_sound(5, 3)
              print("[backcontrol_process] security set FAILED !!!",  flush=True)
              # restore KeepAlive.json...
              keepalive_obj["start_host_set"]="0"  
              # save to json file
              with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
                json.dump(file_content, f)
              return
              
            app.device_lib.sensor_security_set(1)
            
            # update partition status
            app.host_lib.update_partition_status("1")

            app.host_lib.host_setunset_gpio("1")
            
            # trigger to server
            # 20161220
            signal_obj={
            "type_id": "d01",
            "alert_catalog": "S",
            "abnormal_catalog": abnormal_catalog,
            }
            if appid != "": 
              signal_obj["setting_value"]="APP"
              signal_obj["abnormal_value"]=appid
            elif securityid != "": 
              signal_obj["setting_value"]="Security"
              signal_obj["abnormal_value"]=securityid
            
            app.host_lib.set_event_id()
            
            obj=app.server_lib.trigger_to_server(signal_obj)
            if obj['status']==200:
              print("trigger to server ok",  flush=True)
              pass
            
            
          else:
            print("new host state=unset",  flush=True)
            
            audio_obj=app.audio.Audio()
            audio_obj.stop() 
            audio_obj.play(app.host_lib.get_service_dir() + "system_unset.mp3")
            app.device_lib.ask_readers_make_sound(4, 1)
            # clear alarm_voice_flag
            app.host_lib.clear_alarm_voice_flag()
            
            app.host_lib.lcm_clear_and_update()

            app.device_lib.sensor_security_set(0)
            
            # update partition status
            app.host_lib.update_partition_status("0")

            app.host_lib.host_setunset_gpio("0")
            
            print("[backcontrol_process] stop LED.")
            app.led_api.OffLed(LED6_ALM_R)
            
            app.host_lib.stop_coupling_operation()
            
            signal_obj={
              "type_id": "d01",
            }
            
            signal_obj["alert_catalog"]='S'
            signal_obj["abnormal_catalog"]='GS02'
            signal_obj["setting_value"]="APP"
            signal_obj["device_id"]=str(appid)
            signal_obj["abnormal_value"]=str(appid)
            
            app.host_lib.set_event_id()
            
            # trigger to server
            obj=app.server_lib.trigger_to_server(signal_obj)
            if obj['status']==200:
              print("trigger to server ok",  flush=True)
              pass
              
            # check if it is early unset ...
            if app.host_lib.check_early_lift(keepalive_obj):
              print("host security is early unset !!!",  flush=True)
              #print("preservation_lift_time=" + keepalive_obj["preservation_lift_time"],  flush=True)
              current_time=datetime.datetime.now().strftime("%H:%M")
              print("current time=" + current_time,  flush=True) 
              signal_obj["alert_catalog"]='U'
              signal_obj["abnormal_catalog"]='GU01'
              signal_obj["device_id"]=str(appid)
              signal_obj["setting_value"]=keepalive_obj["preservation_lift_time"]
              signal_obj["abnormal_value"]=current_time
              app.host_lib.set_event_id()
              # trigger to server
              obj=app.server_lib.trigger_to_server(signal_obj)
              if obj['status']==200:
                print("trigger to server ok",  flush=True)
                pass
            # check if security_always_set_enable
            elif app.host_lib.check_security_always_set_enabled(keepalive_obj):
              print("host security always-set is enabled !!!",  flush=True)
              signal_obj["alert_catalog"]='U'
              signal_obj["abnormal_catalog"]='GU01'
              signal_obj["device_id"]=str(appid)
              #signal_obj["setting_value"]=keepalive_obj["preservation_lift_time"]
              #signal_obj["abnormal_value"]=current_time
              
              app.host_lib.set_event_id()
              
              # trigger to server
              obj=app.server_lib.trigger_to_server(signal_obj)
              if obj['status']==200:
                print("trigger to server ok",  flush=True)
                pass
            # check if it is holiday unset
            elif app.host_lib.check_holiday_lift(keepalive_obj):
              print("host security holiday unset !!!",  flush=True)
              signal_obj["alert_catalog"]='U'
              signal_obj["abnormal_catalog"]='GU09'
              signal_obj["device_id"]=str(appid)
              
              app.host_lib.set_event_id()
              
              # trigger to server
              obj=app.server_lib.trigger_to_server(signal_obj)
              if obj['status']==200:
                print("trigger to server ok",  flush=True)
                pass
            
  
        ########################################################                                                                  
        # obsolete: send Keep-alive packet right away 
        app.server_lib.send_keepalive_to_server()
        
        # send MQTT Actionconfirm API
        send_mqtt_action_confirm(backcontrol_obj)
        
        # check-security right away  
        security_check_routine()    
    
    

################################################################################
# 逆控JSON
#MQTT通知 ---JSON格式
# {  
#  "cusno": "8888",  
#  "ver": "20161206111307103",
#  "data": {"HostState": "0", "API": "http://211.22.242.13:8164/api/Security/Actionconfirm"} 
# }
# 欄位名稱              FieldName   欄位資料        備註      SAM的KeepAlive
#-----------------------------------------------------------------------------------------
# JSON版本碼            ver                                 json_version
# 保全設定時間_延後設定 DelayTime   2016/11/4 18:40         preservation_set_time_delay_set 
# 保全設定時間_不設定   NonSetup    2016/8/20               preservation_set_time_no_set 
# 假日                  Holiday     2016/8/20               holiday 
# 保全設定時間_暫改     Temp_Setup  2016/11/4 18:00         preservation_set_time_change
# 保全解除時間_暫改 Temp_ReleaseTime 2016/11/4 09:00        preservation_lift_time_change 
# 觸發逾期設定異常紀錄  TouchTimeDelay Y                    trigger_behindtime_set_unusual_record
# 觸發提早解除異常紀錄  TouchEarlyRelease Y                 trigger_early_lift_unusual_record 
# 啟動保全設定          HostState     1   1 啟動  / 0 解除  start_host_set 
# 觸發拋送KeepAlive     TouchKeepAlive Y                    start_send_keepalive 
# 保全標準設定時間      StdSettingTime 18:00                preservation_set_time 
# 保全標準解除時間      StdReleaseTime 09:00                preservation_lift_time 
# 保全設解延遲時間(分鐘) CheckDelayTime 30                  preservation_delay_time 
# 標準延後設定時間(分鐘) StdDelayTime   60                  standard_delay_set_time 
# 超級密碼              SuperPWD   898989                   admin_password
# 勤務APP推撥           JobMQTT       Y     Y 主動派勤  / N 管制中心派勤 service_push_notification_status
# 客戶APP推撥           CusMQTT       F     Y 即時傳送 / N 不 傳送 / F 管制中心結案後傳送 customer_push_notification_status
# 客戶異常影像傳送      VideoMQTT     Y     Y 傳送  / N 不傳送 customer_unusual_image_send
# 客戶狀態              CusState 停機 01:開通, 02:停機, 03:待拆 customer_status
# 傳訊主要網址          MajorUrl   www.evergreen1.com.tw    send_message_major_url 
# 傳訊次要網址          MinorUrl   www.erergree2.com.tw     send_message_minor_url
# 第三網址              ThirdUrl                            send_message_third_url
# 第四網址              FourthUrl                           send_message_fourth_url

BACKCONTROL_KEEPALIVE_MAP={
#      "ver":"json_version",
      "DelayTime":"preservation_set_time_delay_set",    
      "NonSetup":"preservation_set_time_no_set",
      "Holiday":"holiday",
      "Temp_Setup":"preservation_set_time_change",
      "Temp_ReleaseTime":"preservation_lift_time_change",
      "TouchTimeDelay":"trigger_behindtime_set_unusual_record",
      "TouchEarlyRelease":"trigger_early_lift_unusual_record",
      "HostState":"start_host_set",
      "TouchKeepAlive":"start_send_keepalive",
      "StdSettingTime":"preservation_set_time",
      "StdReleaseTime":"preservation_lift_time",
      "CheckDelayTime":"preservation_delay_time",
      "StdDelayTime":"standard_delay_set_time",
      "SuperPWD":"admin_password",
      "JobMQTT":"service_push_notification_status",
      "CusMQTT":"customer_push_notification_status",
      "VideoMQTT":"customer_unusual_image_send",
      "CusState":"customer_status",
      "MajorUrl":"send_message_major_url",
      "MinorUrl":"send_message_minor_url",
      "ThirdUrl":"send_message_third_url",
      "FourthUrl":"send_message_fourth_url",
      
}

def send_mqtt_action_confirm(mqtt_obj):
    print("[%s][send_mqtt_action_confirm]:" %(str(datetime.datetime.now())),  flush=True)
    
    # MQTT backcontrol
    # {  
    #  "cusno": "003301",  
    #  "ver": "20161206111307103",
    #  "data": {"HostState": "0", "API": "http://211.22.242.13:8164/api/Security/Actionconfirm"} 
    # }
    url=mqtt_obj["data"]["API"]
    
    # MQTT Actionconfirm
    # {
    #  "Ver": "..."
    #  "CusID": "003301"
    # }
    data_obj=dict()
    data_obj["Ver"]=str(mqtt_obj["ver"])
    data_obj["CusID"]=str(mqtt_obj["cusno"])
    #data_obj["ver"]=mqtt_obj["ver"]
    payload = json.dumps(data_obj)     
    print(url,  flush=True)
    print("payload=" + payload,  flush=True)
    API_HEADER = dict()
    API_HEADER['Content-Type']='application/json'
    try:
      if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
        # modem dialup
        app.host_lib.modem_dial_func()

      r = requests.post(url, data=payload, headers=API_HEADER, timeout=5)
      print("server message:",  flush=True) 
      print(r.content.decode("utf-8"),  flush=True)
    except requests.exceptions.ConnectionError as e:
      print("error: %s" %str(e),  flush=True)
    except UnicodeEncodeError as e:
      print("error: %s" %str(e),  flush=True)            
    except:         
      print("Unknown error",  flush=True)    
        
def transform_backcontrol_to_keepalive(dest_obj, src_obj):
    # "data": 
    # {"HostState": "0", "API": "http://211.22.242.13:8164/api/Security/Actionconfirm"} 
    
    for item_name in src_obj:
      try:
        dest_obj[BACKCONTROL_KEEPALIVE_MAP[item_name]]=str(src_obj[item_name])
      except KeyError:
        print(item_name + " NOT found in the map. skip it.",  flush=True)
        continue
      except:
        print(item_name + ": unknown error. skip it.",  flush=True)
        continue                     
    
def keepalive_thread(interval):
    print('[%s][keepalive_thread] starts.' %str(datetime.datetime.now()),  flush=True)

    while not interrupted:
      update_rtc_datetime()
      
      # check current server connection
      global now_connection_status
      server_addr=app.server_lib.get_current_server_address()
      server_port=app.server_lib.get_current_iot_port()
      if app.server_lib.check_server(server_addr, server_port):
        # connection status is ok now
        if now_connection_status=="1":
          # connection status changed: abnormal -> ok
          now_connection_status="0"
          # update KeepAlive.json
          app.host_lib.update_keepalive_json_file("now_connection_status", now_connection_status)
          
          #Send offline event when network back online
          app.server_lib.send_event_when_network_reconnect()
          
          # send network recovery event
          #app.host_lib.send_network_recovery_event()
          
      else:
        # connection status is abnormal now
        if now_connection_status=="0":
          # connection status changed: ok -> abnormal
          now_connection_status="1"
          # update KeepAlive.json
          app.host_lib.update_keepalive_json_file("now_connection_status", now_connection_status)
          
          # send network lost event
          # note: this event is actulally preserved, and then sent out after network works again ...
          #app.host_lib.send_network_lost_event()
          
      keepalive_obj=open_keepalive_json_file()      
      if check_keepalive_enable(keepalive_obj) and app.views.get_customer_number()!="" :
        try:
          app.server_lib.send_keepalive_to_server()
        except:
          print("[keepalive_thread] app.server_lib.send_keepalive_to_server() error \n",  flush=True)
        
        try:
          app.server_lib.send_keepalivedevice_to_server()
        except:
          print("[keepalive_thread] app.server_lib.send_keepalivedevice_to_server() error \n",  flush=True)
          
      '''
      # too much CPU usage ...
      # memory usage debug
      # package: sudo pip3 install pympler
      from pympler import muppy
      from pympler import summary
      all_objects = muppy.get_objects()
      sum1 = summary.summarize(all_objects)
      summary.print_(sum1)  
      '''
      
      debug_log_rotation("/var/log/keepalive_service.log")
      debug_log_rotation("/var/log/communication_service.log")
      
      # sleep interval between keepalive messages is determined by network connection type(modem or not) 
      if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
        # modem connection
        modem_interval=60
        try:
          hostsetting_obj=app.host_lib.open_hostsetting_json_file()
          if app.host_lib.check_host_alarm_set(keepalive_obj):
            modem_interval=int(hostsetting_obj["partial_transmission_time_when_enabled"])  
          else:
            modem_interval=int(hostsetting_obj["partial_transmission_time_when_disabled"])
        except:
          print("[keepalive_thread] get modem_interval error ",  flush=True)
          pass
        print("[keepalive_thread] modem_interval=%d" %(modem_interval),  flush=True)
        time.sleep(modem_interval)
        
      else:
        print("[keepalive_thread] interval=%d" %(interval),  flush=True)
        time.sleep(interval)
    
    print("quit keep-alive thread",  flush=True)

################################################################################
def host_security_thread():
    print('[%s][host_security_thread] starts.' %str(datetime.datetime.now()),  flush=True)
    while not interrupted:
      if app.views.get_customer_number()!="" :
        # check host environment status: power, temperature
        try:
          app.host_lib.check_env_status()
        except:
          print("[host_security_thread] check_env_status() error \n",  flush=True)
      
        try:
          security_check_routine()
        except:
          print("[host_security_thread] security_check_routine() error \n",  flush=True)
          
      time.sleep(10)
    
    print("quit host_security_thread",  flush=True)


################################################################################
# send security signal to [IOT service]
def send_host_emergency_button_signal():
    signal_obj=dict()
    signal_obj["type_id"]="d01"
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="GA04"
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
      print("trigger to server ok",  flush=True)
      pass


def send_device_lost_signal():
    signal_obj=dict()
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="GA19"

    app.host_lib.set_event_id()
    
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
      print("trigger to server ok",  flush=True)
      pass




################################################################################
# security related stuff

def open_keepalive_json_file():
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      keepalive_obj={"json_version":""}
    except:
      print("[open_keepalive_json_file] unknown error",  flush=True)
      keepalive_obj={"json_version":""}
      
    #print("keepalive_obj=" + json.dumps(keepalive_obj),  flush=True)
    return keepalive_obj 

 
# 是否要拋送keepalive
def check_keepalive_enable(data):
    start_send_keepalive=False
    if "start_send_keepalive" in data and ( data["start_send_keepalive"]=="Y" or data["start_send_keepalive"]=="1"):  
      start_send_keepalive=True
      
    return start_send_keepalive


# 是否觸發逾期設定異常紀錄  
def check_trigger_behindtime_set_unusual_record_enable(data):
    if "trigger_behindtime_set_unusual_record" in data and data["trigger_behindtime_set_unusual_record"]=='Y':
      return True
    else:
      return False


# 是否觸發提早解除異常紀錄    
def check_trigger_early_lift_unusual_record_enable(data):
    if "trigger_early_lift_unusual_record" in data and data["trigger_early_lift_unusual_record"]=='Y':
      return True
    else:
      return False


# 是否要做勤務APP推播
def check_service_push_notification_enable(data):
    if "service_push_notification_status" in data and data["service_push_notification_status"]=='Y':
      return True
    else:
      return False

# 是否要做客戶APP推播
def check_customer_push_notification_enable(data):
    if "customer_push_notification_status" in data and data["customer_push_notification_status"]=='Y':
      return True
    else:
      return False


def security_check_routine():
    global DELAY_SET_TRIGGER_FLAG
    global DELAY_SET_TRIGGER_FLAG2
    global DELAY_SET_TRIGGER_FLAG3
    timestamp=str(datetime.datetime.now())
    print("\n[%s][security_check_routine]:" %(timestamp),  flush=True)
    
    # important note: don't print Chinese debug message, or the service at boot will stop right here  
    #print('[security_check_routine] 帳號開通???',  flush=True)
    
    keepalive_obj=open_keepalive_json_file()
    if not app.host_lib.check_account_open(keepalive_obj):
      print('[security_check_routine] Account is NOT opened. skip all checking ...',  flush=True)
      return    
    else:
      print('[security_check_routine] Account is opened.',  flush=True)
      if app.host_lib.check_host_alarm_set(keepalive_obj):
        print('[security_check_routine] security is set',  flush=True)
        
      else:
        print('[security_check_routine] security is unset',  flush=True)
        print("DELAY_SET_TRIGGER_FLAG=" + str(DELAY_SET_TRIGGER_FLAG))
        if app.host_lib.check_delay_set(keepalive_obj):
          print('[security_check_routine] host security is NOT set yet!',  flush=True)
          print("DELAY_SET_TRIGGER_FLAG=" + str(DELAY_SET_TRIGGER_FLAG))
          if DELAY_SET_TRIGGER_FLAG==False:
            DELAY_SET_TRIGGER_FLAG=True
          
            DELAY_SET_TRIGGER_FLAG2=False
            DELAY_SET_TRIGGER_FLAG3=False
        
            # trigger to server
            signal_obj=dict()
            signal_obj["type_id"]="d01"
            signal_obj["alert_catalog"]="U"
            signal_obj["abnormal_catalog"]="GU05"
            current_time=datetime.datetime.now().strftime("%H:%M")
            signal_obj["setting_value"]=keepalive_obj["preservation_set_time"]
            signal_obj["abnormal_value"]=current_time
          
            app.host_lib.set_event_id()
            
            obj = app.server_lib.trigger_to_server(signal_obj)
            if obj['status']==200:
              print("send event of host security delay setting to server: ok",  flush=True)
              pass
          else:
            print("trigger_to_server just once")
            
          #if app.host_lib.allow_delay_set(keepalive_obj):
          if not DELAY_SET_NONSETUP_FLAG:
            print('[security_check_routine] host security is delayed...',  flush=True)
            if app.host_lib.check_delay_set2(keepalive_obj):
              print('[security_check_routine] delay setting is over the user config time!',  flush=True)
              if DELAY_SET_TRIGGER_FLAG2==False:
                DELAY_SET_TRIGGER_FLAG2=True
                '''
                # trigger to server 
                signal_obj=dict()
                signal_obj["type_id"]="d01"
                signal_obj["alert_catalog"]="U"
                signal_obj["abnormal_catalog"]="GU06"
                current_time=datetime.datetime.now().strftime("%H:%M")
                signal_obj["setting_value"]=keepalive_obj["preservation_set_time"]
                signal_obj["abnormal_value"]=current_time
                app.host_lib.set_event_id()
                app.server_lib.trigger_to_server(signal_obj)
                '''
              else:
                print("trigger_to_server just once")  
          
          else:
            print("DELAY_SET_NONSETUP_FLAG=True.\n\n",  flush=True)
            '''
            # trigger to server 
            signal_obj=dict()
            signal_obj["type_id"]="d01"
            signal_obj["alert_catalog"]="U"
            signal_obj["abnormal_catalog"]="GU07"
            current_time=datetime.datetime.now().strftime("%H:%M")
            signal_obj["abnormal_value"]=current_time
            app.host_lib.set_event_id()
            app.server_lib.trigger_to_server(signal_obj)
            '''
        #elif app.host_lib.check_early_lift(keepalive_obj):
        #  print('alert is lifted too early!',  flush=True)
        #elif app.host_lib.check_holiday_lift(keepalive_obj):
        #  print('alert is lifted on holiday!',  flush=True)
    
    '''
    if check_service_push_notification_enable(keepalive_obj):
      print('[security_check_routine][service APP push] is enabled',  flush=True)
    if check_customer_push_notification_enable(keepalive_obj):
      print('[security_check_routine][client APP push] is enabled',  flush=True)
    '''
          
################################################################################
# debug log rotation to avoid enormous log file 
def debug_log_rotation(debug_log_file):
    try:
      statinfo = os.stat(debug_log_file)
      if statinfo.st_size > 512*1024:
        command='cp {current_file_name} {old_file_name}'.format(current_file_name=debug_log_file, old_file_name=debug_log_file + ".1")    
        os.system(command)
        command='echo "debug start:" > {current_file_name}'.format(current_file_name=debug_log_file)    
        os.system(command)
    except FileNotFoundError:
      pass
    except:
      pass  
             
################################################################################
# push button callback
def alarmCallback(state):
    timer = threading.Timer(0.1, alarmCallback_thread, [state])
    timer.start()

def alarmCallback_thread(*args):
    state=int(args[0])
    print("[%s][alarmCallback] state=%d" %(str(datetime.datetime.now()), state),  flush=True)
    
    #print("[alarmCallback] ZL TEST\n\n\n", flush=True)
    #return
    
    '''
    # TODO: not sure ...
    # check host-security first
    keepalive_obj=open_keepalive_json_file()    
    if not app.host_lib.check_host_alarm_set(keepalive_obj):
      print("[alarmCallback] host security is NOT set ==> ignore this event !",  flush=True)    
      return
    '''
    
    if state==1:
      app.led_api.SetLed(app.led_api.LED6_ALM_R,0.5)
      
      # check if host urgent sound is enabled ...
      if app.host_lib.get_host_urgent_sound_set():
        audio_obj=app.audio.Audio()
        audio_obj.play(app.host_lib.get_service_dir() + "urgent.mp3", 0)
        #app.device_lib.ask_readers_make_sound(xx, 180)
      else:
        print("[alarmCallback] host urgent sound is disabled !" %state, flush=True)
        pass
      
         
      # set alarm voice flag
      # disaster = "[fire][gas][sos]"
      app.host_lib.set_alarm_voice_flag("0", 1)
      # display alarm info on LCM
      app.host_lib.lcm_display_alarm_info()
      
      
      print("[alarmCallback] launch alarm-output action !",  flush=True)
      app.host_lib.launch_alarm_output_action()
      print("[alarmCallback] launch 12v-output action !",  flush=True)
      app.host_lib.launch_12v_output_action()
      
      app.host_lib.set_event_id()
      send_host_emergency_button_signal()
      # notify IPCam to upload video
      app.ipcam_lib.ask_ipcam_to_upload_video(app.host_lib.get_event_id())
    
   

    else:
      app.led_api.OffLed(app.led_api.LED6_ALM_R)
      
      audio_obj=app.audio.Audio()
      audio_obj.stop()
      # clear alarm_voice_flag
      app.host_lib.clear_alarm_voice_flag()
      # clear alarm info on LCM
      app.host_lib.lcm_clear_and_update()      

      
def setUnsetCallback():
    timer = threading.Timer(0.1, setUnsetCallback_thread)
    timer.start()

#def setUnsetCallback():
def setUnsetCallback_thread():
    print("[%s][setUnsetCallback] starts." %(str(datetime.datetime.now())),  flush=True)
    app.led_api.SetLed(app.led_api.LED3_SET_R, 0)
    
    keepalive_obj=open_keepalive_json_file()    
    if app.host_lib.check_host_alarm_set(keepalive_obj):
      print("New security status will be unset!", flush=True)    
      
      result=app.host_lib.host_security_unset()
      print("[setUnsetCallback][security unset] make sound !!!\n\n", flush=True)
      if result:
        '''
        audio_obj=app.audio.Audio()
        audio_obj.play(app.host_lib.get_service_dir() + "system_unset.mp3")
        app.device_lib.ask_readers_make_sound(4, 1)
        '''  
        pass
      else:
        print("[setUnsetCallback][security unset] failed ...", flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "system_unset_fail.mp3")
        return
      
      # clear alarm_voice_flag
      app.host_lib.clear_alarm_voice_flag()

      app.host_lib.lcm_clear_and_update()
                        
      #app.host_lib.host_security_unset()
      
      if app.host_lib.check_security_always_set_enabled(keepalive_obj):
        signal_obj={
          "type_id":"d01",
          "device_id":"",
          "alert_catalog": "U",
          "abnormal_catalog": "GU01", 
          "abnormal_value": ""
        } 
        app.host_lib.set_event_id()
        
        app.server_lib.trigger_to_server(signal_obj)

    else:
      print("New security status will be set!", flush=True)
      result=app.host_lib.host_security_set()
      if result:
        '''
        print("[security set] make sound !!!\n\n", flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.stop()
        audio_obj.play(app.host_lib.get_service_dir() + "system_set_ok.mp3")
        '''
        pass
      else:
        print("[setUnsetCallback][security set] make sound !!!\n\n", flush=True)
        audio_obj=app.audio.Audio() 
        # "system_set_fail.mp3" takes 2.8sec, set 8.4sec to play it three times
        audio_obj.play(app.host_lib.get_service_dir() + "system_set_fail.mp3", 8.4)
      
        app.device_lib.ask_readers_make_sound(5, 3)
        
    app.led_api.OffLed(app.led_api.LED3_SET_R)

################################################################################
def year_flash_thread():
    while dateButtonCount == 1 and not interrupted:
      app.lcm_api.lcm_to_seg(152,  [0x00,0x00])
      time.sleep(0.6)
      
      val=str(dateButtonYear)
      app.lcm_api.lcm_to_seg(152, app.lcm_api.year_list[val[2:]])
      time.sleep(0.6)

def month_flash_thread():
    while dateButtonCount == 2 and not interrupted:
      app.lcm_api.lcm_to_seg(141, [0x00])
      time.sleep(0.6)
      
      val=str(dateButtonMonth)
      app.lcm_api.lcm_to_seg(141, app.lcm_api.month_list[val])
      time.sleep(0.6)

def day_flash_thread():
    while dateButtonCount == 3 and not interrupted:
      app.lcm_api.lcm_to_seg(145, [0x00,0x03])
      time.sleep(0.6)
      
      val=str(dateButtonDay)
      app.lcm_api.lcm_to_seg(145, app.lcm_api.day_list[val])
      time.sleep(0.6)

    
def dateButtonCallback():
    global timeButtonCount
    if timeButtonCount > 0:
      # clear time setting status when dateButton is clicked
      timeButtonCount=0
      time.sleep(0.6)
      app.host_lib.lcm_display_normal_info()
      #app.host_lib.lcm_clear_and_update()
      print("[dateButtonCallback] clear time setting status and return",  flush=True)
      return
    
    global dateButtonCount, dateButtonYear, dateButtonMonth, dateButtonDay
    dateButtonCount = (dateButtonCount+1) % 4
    print("[%s][dateButtonCallback] starts(%d)." %(str(datetime.datetime.now()), dateButtonCount),  flush=True)
    current_datetime_obj=datetime.datetime.now()

    if dateButtonCount == 1:
      app.lcm_api.lcm_init()
      # init year value
      dateButtonYear=current_datetime_obj.year
      tt=threading.Thread(target=year_flash_thread)
      tt.start()
    elif dateButtonCount == 2:   
      # init month value
      dateButtonMonth=current_datetime_obj.month
      tt=threading.Thread(target=month_flash_thread)
      tt.start()
    elif dateButtonCount == 3:
      # init day value
      dateButtonDay=current_datetime_obj.day
      tt=threading.Thread(target=day_flash_thread)
      tt.start()
            
    else:   # dateButtonCount == 0
      # e.g. date 122109082016
      new_datetime_obj = datetime.datetime(dateButtonYear, dateButtonMonth, dateButtonDay, current_datetime_obj.hour, current_datetime_obj.minute, 0)
      command='date ' + new_datetime_obj.strftime("%m%d%H%M%Y")
      print("[timeButtonCallback] command=%s" %(command),  flush=True)    
      os.system(command)
      
      command='hwclock -w'    
      os.system(command)
      
      app.host_lib.lcm_display_normal_info()
      
################################################################################
def hour_flash_thread():
    while timeButtonCount == 1 and not interrupted:
      app.lcm_api.lcm_send_data_list(0x00, [0x00,0x10])
      time.sleep(0.6)
      
      val=str(timeButtonHour)
      app.lcm_api.lcm_send_data_list(0x00,app.lcm_api.hour_list[val])
      time.sleep(0.6)

def minute_flash_thread():
    while timeButtonCount == 2 and not interrupted:
      app.lcm_api.lcm_to_seg(6, [0xc0,0x00])
      time.sleep(0.6)
      
      val=str(timeButtonMinute)
      app.lcm_api.lcm_to_seg(6, app.lcm_api.minute_list[val])
      time.sleep(0.6)
      
def timeButtonCallback():
    global dateButtonCount
    if dateButtonCount > 0:
      # clear date setting status when timeButton is clicked
      dateButtonCount=0
      time.sleep(0.6)
      app.host_lib.lcm_display_normal_info()
      #app.host_lib.lcm_clear_and_update()
      print("[timeButtonCallback] clear date setting status and return",  flush=True)
      return
    
    global timeButtonCount, timeButtonHour, timeButtonMinute
    timeButtonCount = (timeButtonCount+1) % 3
    print("[%s][timeButtonCallback] starts(%d)." %(str(datetime.datetime.now()), timeButtonCount),  flush=True)
    current_datetime_obj=datetime.datetime.now()
    
    if timeButtonCount == 1:
      app.lcm_api.lcm_init()
      # init hour value
      timeButtonHour=current_datetime_obj.hour
      
      tt=threading.Thread(target=hour_flash_thread)
      tt.start()
    elif timeButtonCount == 2:   
      # init minute value
      timeButtonMinute=current_datetime_obj.minute
      
      tt=threading.Thread(target=minute_flash_thread)
      tt.start()
    else:   # timeButtonCount == 0
      # e.g. date 122109082016
      new_datetime_obj = datetime.datetime(current_datetime_obj.year, current_datetime_obj.month, current_datetime_obj.day, timeButtonHour, timeButtonMinute, 0)
      command='date ' + new_datetime_obj.strftime("%m%d%H%M%Y")
      print("[timeButtonCallback] command=%s" %(command),  flush=True)    
      os.system(command)
      
      command='hwclock -w'    
      os.system(command)
            
      app.host_lib.lcm_display_normal_info()
      
      
################################################################################    
def funcButtonCallback():
    print("[%s][funcButtonCallback] starts." %(str(datetime.datetime.now())),  flush=True)
    global timeButtonCount, timeButtonHour, timeButtonMinute
    global dateButtonCount, dateButtonYear, dateButtonMonth, dateButtonDay
    
    # for hour value increment
    if timeButtonCount == 1:
      # increase hour value, the value is updated on LCM by hour_flash_thread()
      timeButtonHour = (timeButtonHour+1) % 24
    
    # for minute value increment
    elif timeButtonCount == 2:   
      # increase minute value, the value is updated on LCM by minute_flash_thread()
      timeButtonMinute = (timeButtonMinute+1) % 60
    
    # for year value increment   
    elif dateButtonCount == 1:   
      # increase year value, the value is updated on LCM by year_flash_thread()
      # https://en.wikipedia.org/wiki/Year_2038_problem
      # time value ,stored or calculated as signed 32-bit integer, is interpreted 
      # as the number of seconds since 00:00:00 UTC on 1 January 1970 (the epoch)
      # Such implementations cannot encode times after 03:14:07 UTC on 19 January 2038
      # 2000 ~ 2037
      dateButtonYear = ((dateButtonYear+1) % 100) % 38 + 2000
    
    # for month value increment       
    elif dateButtonCount == 2:   
      # increase month value, the value is updated on LCM by month_flash_thread()
      # 1 ~ 12
      dateButtonMonth = (dateButtonMonth % 12) + 1
    
    # for day value increment 
    elif dateButtonCount == 3:   
      # increase day value, the value is updated on LCM by day_flash_thread()
      if dateButtonMonth==1 or dateButtonMonth==3 or dateButtonMonth==5 or dateButtonMonth==7 or dateButtonMonth==8 or dateButtonMonth==10 or dateButtonMonth==12:
        # 1 ~ 31
        dateButtonDay = (dateButtonDay % 31) + 1
      elif dateButtonMonth==4 or dateButtonMonth==6 or dateButtonMonth==9 or dateButtonMonth==11:
        # 1 ~ 30
        dateButtonDay = (dateButtonDay % 30) + 1
      elif ((dateButtonYear%4)==0 and (dateButtonYear%100)!=0) or (dateButtonYear%400)==0:
        # February of leap years  
        dateButtonDay = (dateButtonDay % 29) + 1
      else:        
        # February of non-leap years
        dateButtonDay = (dateButtonDay % 28) + 1
        
    else:
      print("[funcButtonCallback] no valid setting status, just return.",  flush=True)
      pass

################################################################################
def lcm_display_thread():
    print("[%s][lcm_display_thread] starts." %(str(datetime.datetime.now())),  flush=True)
    global dateButtonCount, timeButtonCount
    
    try:
      app.host_lib.lcm_clear_and_update()
    except:
      print("[lcm_display_thread] lcm_clear operation error...",  flush=True)

    while not interrupted:
      try:
        # do not update the lcm when user is setting the date/time ...
        if dateButtonCount == 0 and timeButtonCount == 0:
          #print("[lcm_display_thread] LCM display ...",  flush=True)
          app.host_lib.lcm_display_normal_info()
        else:
          print("[lcm_display_thread] do not update the LCM because user is setting the date/time ...",  flush=True)
          pass  
      except:
        print("[lcm_display_thread] LCM fail...",  flush=True)
        pass
        
      time.sleep(3)
                
################################################################################
def resetButtonCallback(press_period):
    print("[%s][resetButtonCallback] starts." %(str(datetime.datetime.now())),  flush=True)
    print("[resetButtonCallback] press_period=%f" %(press_period),  flush=True)
    
    if press_period < 2 : 
      app.host_lib.host_reboot()
    else:
      app.host_lib.host_reset_and_reboot()
      
    
    
def coverOpenCallback(state):
    print("[%s][coverOpenCallback] starts." %(str(datetime.datetime.now())),  flush=True)
    
    if state:
      print("[coverOpenCallback] cover is opened !",  flush=True)
      # TODO: 
      # voice prompt ?
      
      # update 'foolproof_status' in KeepAlive.json
      app.host_lib.update_keepalive_json_file("foolproof_status", "1")
      
      signal_obj=dict()
      signal_obj["type_id"]="d01"
      signal_obj["alert_catalog"]='A'
      signal_obj["abnormal_catalog"]='GA05'
      signal_obj["device_id"]=app.host_lib.get_host_mac()
      
      app.host_lib.set_event_id()
      
      obj=app.server_lib.trigger_to_server(signal_obj)
      if obj['status']==200:
        print("trigger to server ok",  flush=True)
        pass
        
    else:
      print("[coverOpenCallback] cover is closed.",  flush=True)
      # update 'foolproof_status' in KeepAlive.json
      app.host_lib.update_keepalive_json_file("foolproof_status", "0")                
    

################################################################################
def modem_line_detect_thread():
    print("[%s][modem_line_detect_thread] starts." %(str(datetime.datetime.now())),  flush=True)
    previous_result=True
    while not interrupted:
       
      if app.host_lib.get_keepalive_json_property("now_connection_way") == "5":
        # current network is modem connection...
        try:
          with open("/tmp/modem_timestamp", "r") as f:
            modem_timestamp=int(f.read())
        except:
          modem_timestamp=0
        current_timestamp=int(time.mktime(datetime.datetime.now().timetuple()))
        time_diff=current_timestamp-modem_timestamp
        print("[modem_line_detect_thread] %d - %d = %d" %(current_timestamp, modem_timestamp, time_diff),  flush=True)
        if modem_timestamp > 0 and time_diff > 40:
          # kill modem connection if there's no modem traffic for over 40 seconds 
          print("[modem_line_detect_thread] kill modem connection (%d > 40)" %(time_diff),  flush=True)
          app.host_lib.kill_wvdial_process()
          with open("/tmp/modem_timestamp", 'w+') as f:
            f.write("0")
          
        # note: do not detect modem line connection now , or the dialup operation always fails.
        print("[%s][modem_line_detect_thread] skip detection during modem dialup/connection." %(str(datetime.datetime.now())),  flush=True)
        time.sleep(10)
        continue
          
      try:
        result=app.modem.line_detect()
        if previous_result != result:        
          if result:
            print("[%s][modem_line_detect_thread] connection ok." %(str(datetime.datetime.now())),  flush=True)
            # TODO: trigger to server ???
          else:
            print("[%s][modem_line_detect_thread] connection FAILED!" %(str(datetime.datetime.now())),  flush=True)
            signal_obj=dict()
            signal_obj["type_id"]="d01"
            signal_obj["alert_catalog"]='A'
            signal_obj["abnormal_catalog"]='GA18'
            signal_obj["device_id"]=app.host_lib.get_host_mac()
            app.host_lib.set_event_id()
            # trigger to server
            obj=app.server_lib.trigger_to_server(signal_obj)
            if obj['status']==200:
              print("trigger to server ok",  flush=True)
              pass
        else:
          print("[%s][modem_line_detect_thread] same status(result=%s) => no action." %(str(datetime.datetime.now()), str(result)),  flush=True)
          pass
        
        previous_result=result     

      except:
        print("[modem_line_detect_thread] unknown error.",  flush=True)
        pass
      
      time.sleep(10)
          
################################################################################
def socket_thread():
    print("[%s][socket_thread] starts." %(str(datetime.datetime.now())),  flush=True)

    # http://stackoverflow.com/questions/7749341/very-basic-python-client-socket-example
    import socket
    import select
    serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # add socket.SO_REUSEADDR flag to aviod TIME_WAIT effect
    serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)                           
    
    while not interrupted:
      try:
        serversocket.bind(('localhost', 5002))
        print("[%s][socket_thread] bind ok !" %(str(datetime.datetime.now())),  flush=True)
        break
      except:
        print("[%s][socket_thread] bind error ==>  retry again !" %(str(datetime.datetime.now())),  flush=True)
        time.sleep(3)
         
    serversocket.listen(5) # become a server socket, maximum 5 connections
    
    while not interrupted:
          connection, address = serversocket.accept()
          buf = connection.recv(512)
          if len(buf) > 0:
            command_str=buf.decode("utf-8")
            print("[%s][socket_thread] received data=%s" %(str(datetime.datetime.now()), command_str),  flush=True)
            command_obj=json.loads(command_str)
            if command_obj["command"] == "q" or command_obj["command"] == "Q":
              break  
            elif command_obj["command"] == "set_led":
              try:
                led_num=command_obj["arg1"]
                off_time=command_obj["arg2"]
                app.led_api.SetLed(led_num, off_time)
              except:
                print("[socket_thread] SetLed error",  flush=True)
                pass
            elif command_obj["command"] == "off_led":
              try:
                led_num=command_obj["arg1"]
                app.led_api.OffLed(led_num)
              except:
                print("[socket_thread] OffLed error",  flush=True)
                pass
                
          connection.close()
          print("[%s][socket_thread] command is done." %(str(datetime.datetime.now())),  flush=True)
    
    print("[socket_thread] quit !",  flush=True)
    serversocket.close()
    
    
################################################################################
def update_rtc_datetime():
    today = datetime.date.today()
    if today.year <= 2010 or today.year >= 2034:
      command='ntpdate -b -s -u pool.ntp.org'    
      os.system(command)
      print("[update_rtc_datetime] %s" %(datetime.datetime.now()),  flush=True)

                
################################################################################           
def stop_all(*args):
    global interrupted
    interrupted=True
    
    try:
      global led
      led.stop()
      global timer
      timer.stop()
    except:
      print("[stop_all] led stop error",  flush=True)
      pass
      
    # to stop socket_thread
    try:
      import socket
      import sys
      client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      client_socket.connect(('localhost', 5002))
      command_obj=dict() 
      command_obj["command"]="q"
      client_socket.send(json.dumps(command_obj).encode('utf-8'))
      client_socket.close()
    except:
      print("[stop_all] socket_thread stop error",  flush=True)
      pass 
      
      
# lauch threads: mqtt, keep-alive
def main_func():
    #send_device_lost_signal()
    
    global interrupted
    interrupted=False

    # the following is done in led_api.EventControl
    #try:
    #  ADC.setup()
    #  os.system('i2cset -f -y 0 0x24 0x9 0x01')
    #except:
    #  print("ADC.setup() error...",  flush=True)

    app.host_lib.stop_coupling_operation()
        
    app.server_lib.check_network_status()
    
    update_rtc_datetime()
                
    signal.signal(signal.SIGTERM, stop_all)
    signal.signal(signal.SIGQUIT, stop_all)
    signal.signal(signal.SIGINT,  stop_all)  # Ctrl-C
    
    t1=threading.Thread(target=keepalive_thread, args=(KEEPALIVE_INTERVAL,))
    t1.start()
    
    t2=threading.Thread(target=mqtt_client_thread)
    t2.start()

    t3=threading.Thread(target=socket_thread)
    t3.start()

    # modem line detection
    t4=threading.Thread(target=modem_line_detect_thread)
    t4.start()
        
    # LCM display
    t5=threading.Thread(target=lcm_display_thread)
    t5.start()
    
    t6=threading.Thread(target=host_security_thread)
    t6.start()
            
    try:    
      global led
      #led = app.led_api.EventControl(alarmCallback,setUnsetCallback)
      led = app.led_api.EventControl(alarmCallback,setUnsetCallback, dateButtonCallback, timeButtonCallback, funcButtonCallback, resetButtonCallback, coverOpenCallback)
      
      led.start()    
    except:
      print("EventControl operation error...",  flush=True)
    
    try:  
      global timer
      timer = app.led_api.LEDTimerControl()
      timer.start()
    except:
      print("LEDTimerControl operation error...",  flush=True)

    '''
    # LED test only, zl
    try:
      print("[main_func] trigger LED !",  flush=True)
      app.led_api.SetLed(LED1_SYS_R, 0.5)
      app.led_api.SetLed(LED2_SYS_B, 0.5)
      app.led_api.SetLed(LED3_SET_R, 0.5)
      app.led_api.SetLed(LED4_SET_B, 0.5)
      app.led_api.SetLed(LED5_SET_G, 0.5)
      app.led_api.SetLed(LED6_ALM_R, 0.5)
      app.led_api.SetLed(LED7_ALM_B, 0.5)
      app.led_api.SetLed(LED8_ALM_G, 0.5)
    except:
      print("[main_func][SetLed] failed",  flush=True)
      pass
    '''
      
if __name__ == "__main__":
    main_func()
    sys.exit(0)
    
