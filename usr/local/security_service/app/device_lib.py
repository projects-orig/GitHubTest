#coding=UTF-8

import os
import time, datetime


import json
import requests
import random 

from socketIO_client import SocketIO

import app.views, app.server_lib, app.host_lib, app.audio, app.ipcam_lib 


try: 
  import app.lcm_api
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[import lcm_api] unknown error.",  flush=True)

try: 
  import app.led_api
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[led_api] unknown error.",  flush=True)



LED8_ALM_G = 8
LED7_ALM_B = 7
LED6_ALM_R = 6
LED5_SET_G = 5
LED4_SET_B = 4
LED3_SET_R = 3
LED2_SYS_B = 2
LED1_SYS_R = 1

# for TemperatureAlarm, PowerAlarm, TerminatingImpedanceAlarm: trigger alarm just once
trigger_alarm_flag=dict()

################################################################################
#sensor_resetting_status=dict()

def open_sensor_resetting_status_file():
    table_name="sensor_resetting_status"
    file_content=dict()
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
    except IOError:
      print ("[" + table_name + ".json] doesn't exist.",  flush=True)
      
    return file_content

def save_sensor_resetting_status_file(data_obj):
    # save to json file
    table_name="sensor_resetting_status"
    with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
      json.dump(data_obj, f)
    
    
def check_all_sensor_resetting_status():
    print("[check_all_sensor_resetting_status] ",  flush=True)
    table_name="Device"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
      
      for dev_item in device_list:
        try:
          resetting_status=dev_item["abnormal_status"][0]
        except:
          resetting_status="0"
          
        print("dev=%s, resetting_status=%s" %(dev_item["device_id"],resetting_status),  flush=True)
        if resetting_status=="1":
          print(" => NOT in normal status", flush=True)
          return False
        
      return True     
    except:
      print ("[check_all_sensor_resetting_status] [" + table_name + ".json] unknown error => return True",  flush=True)
      return True  

    '''
    sensor_resetting_status=open_sensor_resetting_status_file()
    print(sensor_resetting_status)
    print("===============================")
    for dev,resetting_status in sensor_resetting_status.items():
      print("dev=%s, resetting_status=%s" %(dev,resetting_status),  flush=True)
      if resetting_status=="0":
        print("sensor(%s) has NOT been reset. security CANNOT be set ! \n\n\n" %(dev), flush=True)
        return False
        
    return True
    '''
    
    
# return value:
# 1: triggered state
# 0: resetting state         
def get_sensor_resetting_status(device_id):
    print("[get_sensor_resetting_status] ",  flush=True)
    
    table_name="Device"
    try:
      resetting_status="0"
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
      
      for dev_item in device_list:
        if dev_item["device_id"] == device_id:
          resetting_status=dev_item["abnormal_status"][0]
          break
      
      return resetting_status
           
    except:
      print ("[get_sensor_resetting_status] [" + table_name + ".json] unknown error => return 0",  flush=True)
      return "0"  
    
    '''    
    sensor_resetting_status=open_sensor_resetting_status_file()
    if device_id in sensor_resetting_status and sensor_resetting_status[device_id]=="0":
      return "0"
    return "1"  
    '''
    
def set_sensor_resetting_status(device_id, value):
    print("[set_sensor_resetting_status] device=%s, value=%s" %(device_id, value),  flush=True)
    set_sensor_abnormal_status(device_id, 0, value)
      
    '''
    sensor_resetting_status=open_sensor_resetting_status_file()
    sensor_resetting_status[device_id]=str(value)
    save_sensor_resetting_status_file(sensor_resetting_status)    
    '''                         
                             
# TODO: remove this function?
'''    
def remove_sensor_resetting_status(device_id):
    print("[remove_sensor_resetting_status] ",  flush=True)
    sensor_resetting_status=open_sensor_resetting_status_file()
    sensor_resetting_status.pop(device_id, None)
    save_sensor_resetting_status_file(sensor_resetting_status)        
'''    
    
################################################################################
def check_device_connection_status(device_id):
    #search the Device.json to find the matched device by device_id 
    try:
      with open(app.views.get_json_file_dir() + "Device.json", "r") as f:
        file_content=json.load(f)
        device_list=file_content["Device"]
    except:
      print("[check_device_connection_status][Device.json] unknown error. return false",  flush=True)
      return False
    
    
    for item in device_list:
      if str(item["device_id"]) == device_id:
        if "connection_status" in item and str(item["connection_status"]) == "0":    
          return True
    
    print("[check_device_connection_status][%s] does NOT exist or is DISCONNECTED." %(device_id),  flush=True)      
    return False
    
            
################################################################################

# send command to device
def send_command_to_device(record):
  if "device_id" not in record:
    print("[send_command_to_device]: device_id is absent. skip it.",  flush=True)
    return 
  
  #print("[send_command_to_device] ZL TEST\n\n\n")
  #return
  
  try:
    device_id=str(record["device_id"])
    print("[send_command_to_device][%s] (%s):" %(str(datetime.datetime.now()), device_id),  flush=True)
    
    # socket.emit('setGSensorConfig', {device_id: 'device_id', enable: 1, sensitivity: 70});
    if "G_sensor_status" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        data_obj["enable"]=int(record["G_sensor_status"])
        if "G_sensitivity" in record:
          try: 
            data_obj["sensitivity"]=int(record["G_sensitivity"][:-1]) # remove the ''%' at the end
          except:
            data_obj["sensitivity"]=100
          
        else:
          data_obj["sensitivity"]=100
          
        print(data_obj)
        device_command_api2('setGSensorConfig', data_obj)
    
    # socket.emit('setTempConfig', {device_id: 'device_id', value: 50, offset: 10});
    if "temperature_sensing" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        data_obj["value"]=int(record["temperature_sensing"])
        '''
        if "temperature_sensing_gap" in record:
          data_obj["offset"]=int(record["temperature_sensing_gap"])
        else:
          data_obj["offset"]=5    
        '''
        try:
          data_obj["offset"]=int(record["temperature_sensing_gap"])
        except:
          data_obj["offset"]=5
          
        device_command_api2("setTempConfig", data_obj)

    # socket.emit('setLowBatteryConfig', {device_id: 'device_id', value: 30, offset: 5});
    if "battery_low_power_set" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        data_obj["value"]=int(record["battery_low_power_set"])
        '''
        if "battery_low_power_gap" in record:
          data_obj["offset"]=int(record["battery_low_power_gap"])
        else:
          data_obj["offset"]=5
        '''
        try:
          data_obj["offset"]=int(record["battery_low_power_gap"])
        except:
          data_obj["offset"]=5
          
        device_command_api2("setLowBatteryConfig", data_obj)

    # socket.emit('setElectricLockConfig', {device_id: 'device_id', enable: 1, time: 60});
    # time: second
    if "power_lock_status" in record and "power_lock_action_time" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        
        try:
          data_obj["enable"]=int(record["power_lock_status"])
        except:
          data_obj["enable"]=0
        
        try:  
          data_obj["time"]=int(record["power_lock_action_time"])
        except:
          data_obj["time"]=10
            
        device_command_api2('setElectricLockConfig', data_obj)
    
    # socket.emit('setIntervalConfig', {device_id: 'device_id', mode:0, time:5});
    #   mode: 1 = auto  0 = manual
    #   time: second
    if "connection_time_set" in record and "connection_time_set_seconds_manual" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        try:
          # different definitions for app and sensor ...
          if record["connection_time_set"]=="1":
            data_obj["mode"]=0
          else:
            data_obj["mode"]=1
        except:
          data_obj["mode"]=1
            
        try:
          data_obj["time"]=int(record["connection_time_set_seconds_manual"])
        except:
          data_obj["time"]=0
          
        device_command_api2('setIntervalConfig', data_obj)

    # socket.emit('setDistanceConfig', {device_id: 'device_id', mode: 0, distance: 5000});
    #   mode : 1 = auto  0 = manual
    #   distance: meter
    if "transmission_distance" in record and "transmission_distance_set_manual" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        if int(record["transmission_distance"])==0:
          data_obj["mode"]=1
        else:          
          data_obj["mode"]=0
        
        try:
          data_obj["distance"]=int(record["transmission_distance_set_manual"])
        except:
          data_obj["distance"]=1000
          
        device_command_api2('setDistanceConfig', data_obj)
    
    # socket.emit('setResistanceConfig', {device_id: 'device_id', value:80});
    if "terminating_impedance" in record:
        print(record["terminating_impedance"],  flush=True)
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        try:
          data_obj["value"]=int(record["terminating_impedance"][:-1])
        except:
          data_obj["value"]=0
          
        print(data_obj["value"],  flush=True)
        device_command_api2('setResistanceConfig', data_obj)

    # socket.emit('setMotionConfig', {device_id: 'device_id', count:10, time:2});
    #   time: second
    if "action_count_frequency" in record and "action_count_time" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        try:
          data_obj["count"]=int(record["action_count_frequency"])
        except:
          data_obj["count"]=1

        try:    
          data_obj["time"]=int(record["action_count_time"])
        except:
          data_obj["time"]=0
          
        device_command_api2('setMotionConfig', data_obj)

    # socket.emit('setSignalConfig', {device_id: 'device_id', mode: 0, value:30 , offset: 10});
    #   mode: 1 = manual, 2: auto
    if "msg_send_strong_set" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])

        try:
          if int(record["msg_send_strong_set"])==0:
            data_obj["mode"]=2
          else:
            data_obj["mode"]=1
        except:
          data_obj["mode"]=2
          
        try:  
          data_obj["value"]=int(record["msg_send_strong_set_manual"][:-1]) # remove the '%' at the end
        except:
          data_obj["value"]=100
          
        try:
          data_obj["offset"]=int(record["msg_send_strong_set_manual_gap"])
        except:
          data_obj["offset"]=5
            
        device_command_api2('setSignalConfig', data_obj)
    
    # socket.emit('setMicrowaveConfig', {device_id: 'device_id', enable:1, sensitivity:50});
    # microwave_set 微波設定:  0=不啟用, 1=啟用
    # microwave_sensitivity_set 微波靈敏度(%)
    if "microwave_set" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        data_obj["enable"]=int(record["microwave_set"])
        try:
          data_obj["sensitivity"]=int(record["microwave_sensitivity_set"])
        except:
          data_obj["sensitivity"]=100
                      
        device_command_api2('setMicrowaveConfig', data_obj)

    # socket.emit('setPirConfig', {device_id: 'device_id', enable:1});
    # infrared_set 紅外線設定:  0=不啟用, 1=啟用
    if "infrared_set" in record:
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        
        try:
          data_obj["enable"]=int(record["infrared_set"])
        except:
          data_obj["enable"]=0
          
        device_command_api2('setPirConfig', data_obj)

    if "extend_out_output_time" in record:
        #TODO: wait for api definition
        '''
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        try:
          data_obj["xxx"]=int(record["extend_out_output_time"])
        except:
          data_obj["xxx"]=5
        device_command_api2('xxxxxx', data_obj)
        '''
        pass
        
    # for ReadSensor's speaker    
    if "speaker_enable" in record: 
        # music enable/disable
        # socket.emit('setMusicConfig', {device_id: extAddr, enable: 1});
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        try:
          data_obj["enable"]=int(record["speaker_enable"])
        except:
          data_obj["enable"]=0

        device_command_api2("setMusicConfig", data_obj)

    
    
    if "led_flash_time_period" in record: 
        # set ReadSensor's LED flash time period during disconnection
        # Eve004 慢閃時間設定 command
        # socket.emit('setAlarmConfig', {device_id: extAddr, time: 30});
        # time: second
        data_obj=dict() 
        data_obj["device_id"]=str(record["device_id"])
        try:
          data_obj["time"]=int(record["led_flash_time_period"])
        except:
          data_obj["time"]=30
        device_command_api2("setAlarmConfig", data_obj)
        
        
  except KeyError:
    print("[send_command_to_device] KeyError", flush=True)
    pass
  except:
    print("[send_command_to_device] Unknown Error", flush=True)
    pass

def on_socketio_response(*args):
    print("[on_socketio_response]",  flush=True)
    print(args)

def device_command_api2(type, data_obj):
    if "device_id" in data_obj : 
      if not check_device_connection_status(str(data_obj["device_id"])) :
        print("[device_command_api2][%s] is disconnected => skip this operation" %str(data_obj["device_id"]),  flush=True)
        return
      
      # skip extend device
      if "-" in str(data_obj["device_id"]):
        print("[device_command_api2][%s] is extend device => skip this operation" %str(data_obj["device_id"]),  flush=True)
        return
      
          
    print("[device_command_api2][%s] start !" %(type),  flush=True)
    
    #print("[device_command_api2] ZL TEST\n\n\n")
    #return

    try:
      # http://stackoverflow.com/questions/4762086/socket-io-client-library-in-python
      with SocketIO('127.0.0.1', 1310) as socketIO:
        socketIO.emit(type, data_obj, on_socketio_response)
        socketIO.wait_for_callbacks(seconds=1)
        print("[device_command_api2] done !",  flush=True)
    except:
        print("[device_command_api2] unknown error",  flush=True)
    
    

def pairing_enable():
    #ip:  127.0.0.1
    #port:1310
    #socket.emit('setJoinPermit', { open: true, duration: 1 * 60});
    #   open: true = open join,
    #      false = close join
    #   duration: second
    data_obj={"open": True, "duration": 60}
    
    print("[pairing_enable][%s] start !" %str(datetime.datetime.now()),  flush=True)
    try:
      # http://stackoverflow.com/questions/4762086/socket-io-client-library-in-python
      with SocketIO('127.0.0.1', 1310) as socketIO:
        socketIO.emit('setJoinPermit', data_obj, on_socketio_response)
        socketIO.wait_for_callbacks(seconds=2)
        print("[pairing_enable][%s] done !" %str(datetime.datetime.now()),  flush=True)
    except:
        print("[pairing_enable] unknown error",  flush=True)

################################################################################
def handle_reed_event(event_obj):
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
      
    
    '''
    { 
      "device_id":""  
      "trigger_type":""
    }
    '''
    result=False
    type_id="d02"
    device_id=str(event_obj["device_id"])
    trigger_type=str(event_obj["trigger_type"])
    if trigger_type=="1":
      # 1=reset
      trigger_status=""
      alarm_type="ResetAlarm"
      #result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
      result=trigger_silent(type_id,device_id,trigger_status,alarm_type)
    elif trigger_type=="2":
      # 2=device_connect
      result=device_connect(type_id, event_obj)
          
    elif trigger_type=="3":
      # 3=TemperatureWarning
      if "temperature_status" in event_obj: 
        trigger_status=str(event_obj["temperature_status"])
      else:  
        trigger_status="0"
      alarm_type="TemperatureWarning"
      result=trigger_warning(type_id,device_id,trigger_status,alarm_type)
          
    elif trigger_type=="4":
      # 4=TemperatureAlarm
      if "temperature_status" in event_obj: 
        trigger_status=str(event_obj["temperature_status"])
      else:  
        trigger_status="0"
      
      alarm_type="TemperatureAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
          
    elif trigger_type=="5":
      # 5=PowerWarning
      if "power_status" in event_obj: 
        trigger_status=str(event_obj["power_status"])
      else:  
        trigger_status="0"
      
      alarm_type="PowerWarning"
      result=trigger_warning(type_id,device_id,trigger_status,alarm_type)
      
    elif trigger_type=="6":
      # 6=PowerAlarm
      if "power_status" in event_obj: 
        trigger_status=str(event_obj["power_status"])
      else:  
        trigger_status="0"
      
      alarm_type="PowerAlarm"
      result=trigger_silent(type_id,device_id,trigger_status,alarm_type)
      
    elif trigger_type=="7":
      # 7=GSensorAlarm
      trigger_status=""
      alarm_type="GSensorAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
      
    elif trigger_type=="8":
      # 8=reed alarm
      result=reed_alarm(device_id)
    
    elif trigger_type=="9":
      # 9=reed reset (覆歸)
      result=reed_reset(device_id)
      
    elif trigger_type=="10":
      # 10=disconnect
      result=device_disconnect(type_id, event_obj)
      
      #TODO: not sure
      # clear sensor_resetting_status of this reed!
      #remove_sensor_resetting_status(device_id)
      set_sensor_resetting_status(device_id, 0)
    
    elif trigger_type=="11":
      #device_connect(type_id, event_obj)
      # 11=update status
      result=device_update_status(event_obj)
      
    elif trigger_type=="12":
      # 12=TerminatingImpedanceAlarm
      if "impedance_status" in event_obj: 
        trigger_status=str(event_obj["impedance_status"])
      else:  
        trigger_status="0"
      alarm_type="TerminatingImpedanceAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="13":
      # 13=TerminatingImpedanceAlarm
      # TODO: need definition: alert_catalog and abnormal_catalog ??
      if "impedance_status" in event_obj: 
        trigger_status=str(event_obj["impedance_status"])
      else:  
        trigger_status="0"
      alarm_type="TerminatingImpedanceAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="14":
      # extend device#1 event
      # check if extend in1 is enabled: extend_in1_function
      if get_extend_in_function("WLReedSensor", device_id, "1")=="0":
        print ("[handle_reed_event] extend_in1 event is ignored ..." ,  flush=True)
        return False
        
      # check "extend_status" 0:復置  1:異常
      extend_dev_id=device_id + "-1"
      current_extend_value=get_device_value(extend_dev_id, "extend_status")
      if current_extend_value=="":
        current_extend_value="0"
      try:  
        new_extend_value=str(event_obj["extend_status"])
        if current_extend_value != new_extend_value:
          print ("[handle_reed_event][ExtendDevice1Alarm] new_extend_value=%s" %(new_extend_value),  flush=True)
          set_device_value(extend_dev_id, "extend_status", new_extend_value)
          if new_extend_value == "1":
            trigger_status="0"
            alarm_type="ExtendDevice1Alarm"
            result=trigger_alarm("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 3, 1)  
            set_sensor_resetting_status(extend_dev_id, 1)
          else:
            trigger_status="0"
            alarm_type="ExtendDevice1Reset"
            result=trigger_silent("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 3, 0)
            set_sensor_resetting_status(extend_dev_id, 0)
        else:
          print ("[handle_reed_event][ExtendDevice1Alarm] extend_value is not changed => ignore this event",  flush=True)                
            
      except:
        print ("[handle_reed_event][ExtendDevice1Alarm] unknown error",  flush=True)
        pass
    
    elif trigger_type=="15":
      # extend device#2 event
      # check if extend in2 is enabled: extend_in2_function
      if get_extend_in_function("WLReedSensor", device_id, "2")=="0":
        print ("[handle_reed_event] extend_in2 event is ignored ..." ,  flush=True)
        return False
      
      # check "extend_status" 0:復置  1:異常
      extend_dev_id=device_id + "-2"
      current_extend_value=get_device_value(extend_dev_id, "extend_status")
      if current_extend_value=="":
        current_extend_value="0"
      try:  
        new_extend_value=str(event_obj["extend_status"])
        if current_extend_value != new_extend_value:
          print ("[handle_reed_event][ExtendDevice2Alarm] new_extend_value=%s" %(new_extend_value),  flush=True)
          set_device_value(extend_dev_id, "extend_status", new_extend_value)
          if new_extend_value == "1":
            trigger_status="0"
            alarm_type="ExtendDevice2Alarm"
            result=trigger_alarm("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 4, 1)
            set_sensor_resetting_status(extend_dev_id, 1)
          else:
            trigger_status="0"
            alarm_type="ExtendDevice2Reset"
            result=trigger_silent("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 4, 0)
            set_sensor_resetting_status(extend_dev_id, 0)
        else:
          print ("[handle_reed_event][ExtendDevice2Alarm] extend_value is not changed => ignore this event",  flush=True)                
        
      except:
        print ("[handle_reed_event][ExtendDevice2Alarm] unknown error",  flush=True)
        pass
    
                    
    return result

def reed_reset(device_id):
    print("reed(%s) is reset !!!"  %(device_id),  flush=True)
    if get_sensor_resetting_status(device_id) == "0":
      print("it is already in resetting state. Ignore this event!",  flush=True)
      return True

    set_sensor_resetting_status(device_id, "0")

    loop_id=str(check_loop_id_from_device_id(device_id))
    alarm_obj=dict()
    alarm_obj["type_id"]="d02"
    alarm_obj["device_id"]=str(device_id)
    alarm_obj["loop_id"]=loop_id
    alarm_obj["alert_catalog"]="G"
    alarm_obj["abnormal_catalog"]="DG01"
    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
        trigger_result=True
    else:  
        trigger_result=False
    
    return trigger_result
    
    
    
def reed_alarm(device_id):
    print("reed(%s) is triggered !!! \n\n"  %(device_id),  flush=True)
    if get_sensor_resetting_status(device_id) == "1":
      print("reed(%s) is already triggered. Ignore this event!"  %(device_id),  flush=True)
      return False
    set_sensor_resetting_status(device_id, "1")

    loop_id,loop_status=app.device_lib.check_loop_status_from_device_id(device_id)
    if loop_status=="2":    
      print("[reed_alarm][loop-type: customer] just trigger optical-coupling operation !",  flush=True)
      app.host_lib.trigger_coupling_operation(device_id)
      result=True
    else:
      trigger_status=""
      alarm_type="ReedAlarm"
      result=trigger_alarm("d02", device_id, trigger_status, alarm_type)
      
    return result    

################################################################################
def doublebond_reset(device_id):
    print("doublebond(%s) is reset !!!"  %(device_id),  flush=True)
    if get_sensor_resetting_status(device_id) == "0":
      print("it is already in resetting state. Ignore this event!",  flush=True)
      return True
    
    set_sensor_resetting_status(device_id, "0")

    loop_id=str(check_loop_id_from_device_id(device_id))
    alarm_obj=dict()
    alarm_obj["type_id"]="d03"
    alarm_obj["device_id"]=str(device_id)
    alarm_obj["loop_id"]=loop_id
    alarm_obj["alert_catalog"]="G"
    alarm_obj["abnormal_catalog"]="AG01"
    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
        trigger_result=True
    else:  
        trigger_result=False
    
    return trigger_result

################################################################################
def handle_doublebond_event(event_obj):
    result=False
    type_id="d03"
    device_id=str(event_obj["device_id"])
    trigger_type=str(event_obj["trigger_type"])
    if trigger_type=="1":
      # 1=reset
      trigger_status=""
      alarm_type="ResetAlarm"
      #result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
      result=trigger_silent(type_id,device_id,trigger_status,alarm_type)
    elif trigger_type=="2":
      # 2=device_connect
      result=device_connect(type_id, event_obj)

    elif trigger_type=="3":
      # 3=TemperatureWarning
      if "temperature_status" in event_obj: 
        trigger_status=str(event_obj["temperature_status"])
      else:  
        trigger_status="0"
      alarm_type="TemperatureWarning"
      result=trigger_warning(type_id,device_id,trigger_status,alarm_type)
      
    elif trigger_type=="4":
      # 4=TemperatureAlarm
      if "temperature_status" in event_obj: 
        trigger_status=str(event_obj["temperature_status"])
      else:  
        trigger_status="0"
      alarm_type="TemperatureAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
              
    elif trigger_type=="5":
      # 5=PowerWarning
      if "power_status" in event_obj: 
        trigger_status=str(event_obj["power_status"])
      else:  
        trigger_status="0"
      
      alarm_type="PowerWarning"
      result=trigger_warning(type_id,device_id,trigger_status,alarm_type)
      
    elif trigger_type=="6":
      # 6=PowerAlarm
      if "power_status" in event_obj: 
        trigger_status=str(event_obj["power_status"])
      else:  
        trigger_status="0"
      alarm_type="PowerAlarm"
      result=trigger_silent(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="7":
      # 7=GsensorAlarm
      trigger_status=""
      alarm_type="GSensorAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="8":
      # 8=disconnect
      result=device_disconnect(type_id, event_obj)
      #TODO: not sure
      # clear sensor_resetting_status!
      set_sensor_resetting_status(device_id, 0)
      
    elif trigger_type=="9":
      # 9=microwave alarm
      if get_sensor_resetting_status(device_id) == "1":
        print("(%s) is already triggered. Ignore this event!"  %(device_id),  flush=True)
        return False

      set_sensor_resetting_status(device_id, "1")
      
      trigger_status=""
      alarm_type="MicrowaveAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
      
    elif trigger_type=="10":
      # 10=IR alarm
      if get_sensor_resetting_status(device_id) == "1":
        print("(%s) is already triggered. Ignore this event!"  %(device_id),  flush=True)
        return False
      
      set_sensor_resetting_status(device_id, "1")
      
      trigger_status=""
      alarm_type="IrAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)

    elif trigger_type=="11":
      #device_connect(type_id, event_obj)
      # 11=update status
      result=device_update_status(event_obj)
      
    elif trigger_type=="12":
      # 12=TerminatingImpedanceAlarm
      if "impedance_status" in event_obj: 
        trigger_status=str(event_obj["impedance_status"])
      else:  
        trigger_status="0"
      alarm_type="TerminatingImpedanceAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="13":
      # 13=TerminatingImpedanceAlarm
      # TODO: need definition: alert_catalog and abnormal_catalog ??
      if "impedance_status" in event_obj: 
        trigger_status=str(event_obj["impedance_status"])
      else:  
        trigger_status="0"
      alarm_type="TerminatingImpedanceAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="14":
      # extend device#1 event
      # check if extend_in1 is enabled: extend_in1_function
      if get_extend_in_function("WLDoubleBondBTemperatureSensor", device_id, "1")=="0":
        print ("[handle_doublebond_event] extend_in1 event is ignored ..." ,  flush=True)
        return False
      
      # check "extend_status" 0:復置  1:異常
      extend_dev_id=device_id + "-1"
      current_extend_value=get_device_value(extend_dev_id, "extend_status")
      if current_extend_value=="":
        current_extend_value="0"
      try:
        new_extend_value=str(event_obj["extend_status"])
        if current_extend_value != new_extend_value:
          print ("[handle_doublebond_event][ExtendDevice1Alarm] new_extend_value=%s" %(new_extend_value),  flush=True)
          set_device_value(extend_dev_id, "extend_status", new_extend_value)
          if new_extend_value == "1":
            trigger_status="0"
            alarm_type="ExtendDevice1Alarm"
            result=trigger_alarm("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 3, 1)
            set_sensor_resetting_status(extend_dev_id, 1)
          else:
            trigger_status="0"
            alarm_type="ExtendDevice1Reset"
            result=trigger_silent("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 3, 0)
            set_sensor_resetting_status(extend_dev_id, 0)
        else:
          print ("[handle_doublebond_event][ExtendDevice1Alarm] extend_value is not changed => ignore this event",  flush=True)                
      except:
        print ("[handle_doublebond_event][ExtendDevice1Alarm] unknown error",  flush=True)
        pass
    
    elif trigger_type=="15":
      # extend device#2 event
      # check if extend_in2 is enabled: extend_in2_function
      if get_extend_in_function("WLDoubleBondBTemperatureSensor", device_id, "2")=="0":
        print ("[handle_doublebond_event] extend_in2 event is ignored ..." ,  flush=True)
        return False
      
      # check "extend_status" 0:復置  1:異常
      extend_dev_id=device_id + "-2"
      current_extend_value=get_device_value(extend_dev_id, "extend_status")
      if current_extend_value=="":
        current_extend_value="0"
      try:  
        new_extend_value=str(event_obj["extend_status"])
        if current_extend_value != new_extend_value:
          print ("[handle_doublebond_event][ExtendDevice2Alarm] new_extend_value=%s" %(new_extend_value),  flush=True)
          set_device_value(extend_dev_id, "extend_status", new_extend_value)
          if new_extend_value == "1":
            trigger_status="0"
            alarm_type="ExtendDevice2Alarm"
            result=trigger_alarm("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 4, 1)
            set_sensor_resetting_status(extend_dev_id, 1)
          else:
            trigger_status="0"
            alarm_type="ExtendDevice2Reset"
            result=trigger_silent("d07", extend_dev_id, trigger_status, alarm_type)
            set_sensor_abnormal_status(device_id, 4, 0)
            set_sensor_resetting_status(extend_dev_id, 0)
        else:
          print ("[handle_doublebond_event][ExtendDevice2Alarm] extend_value is not changed => ignore this event",  flush=True)                
    
      except:
        print ("[handle_doublebond_event][ExtendDevice2Alarm] unknown error",  flush=True)
        pass
    
    elif trigger_type=="16":
      result=doublebond_reset(device_id)
      
    
    
    return result

################################################################################
def handle_reader_event(event_obj):
    '''
     { 
            "device_id":"",  
            "trigger_type":"",
            "temperature_status":"",
            "power_status":""
     }
    '''
    result=False
    type_id="d04"
    device_id=str(event_obj["device_id"])
    trigger_type=str(event_obj["trigger_type"])
    if trigger_type=="1":
      # 1=鋆蔭reset
      trigger_status=""
      alarm_type="ResetAlarm"
      #result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
      result=trigger_silent(type_id,device_id,trigger_status,alarm_type)
    elif trigger_type=="2":
      # 2=device_connect
      result=device_connect(type_id, event_obj)
          
    elif trigger_type=="3":
      # 3=TemperatureWarning
      if "temperature_status" in event_obj:
        trigger_status=str(event_obj["temperature_status"])
      else:
        trigger_status="0"
      alarm_type="TemperatureWarning"
      result=trigger_warning(type_id,device_id,trigger_status,alarm_type)   
             
    elif trigger_type=="4":
      # 4=TemperatureAlarm
      if "temperature_status" in event_obj:
        trigger_status=str(event_obj["temperature_status"])
      else:
        trigger_status="0"        
      alarm_type="TemperatureAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
              
    elif trigger_type=="5":
      # 5=PowerWarning
      if "power_status" in event_obj: 
        trigger_status=str(event_obj["power_status"])
      else:  
        trigger_status="0"
      alarm_type="PowerWarning"
      result=trigger_warning(type_id,device_id,trigger_status,alarm_type)
      
    elif trigger_type=="6":
      # 6=PowerAlarm
      if "power_status" in event_obj: 
        trigger_status=str(event_obj["power_status"])
      else:  
        trigger_status="0"
      
      alarm_type="PowerAlarm"
      result=trigger_silent(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="7":
      # 7=GSensorAlarm
      trigger_status=""
      alarm_type="GSensorAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
    
    elif trigger_type=="8":
      # 8=push button
      result=set_check_button_for_reader(device_id)
      
    elif trigger_type=="9":
      # 9=disconnect
      result=device_disconnect(type_id, event_obj)
      
      
    elif trigger_type=="10":
      #device_connect(type_id, event_obj)
      # 10=update status
      result=device_update_status(event_obj)
    
    elif trigger_type=="11":
      # 11=TerminatingImpedanceAlarm
      if "impedance_status" in event_obj: 
        trigger_status=str(event_obj["impedance_status"])
      else:  
        trigger_status="0"
      alarm_type="TerminatingImpedanceAlarm"
      result=trigger_alarm(type_id,device_id,trigger_status,alarm_type)
      
            
    return result
    
# reader's check-button event handling
CHECK_BUTTON_TIMESTAMP=0
READER_ID_FOR_CHECK_BUTTON=''
# ??殷?謒?佗??Ｔ???
def set_check_button_for_reader(reader_id):
    global CHECK_BUTTON_TIMESTAMP, READER_ID_FOR_CHECK_BUTTON
    CHECK_BUTTON_TIMESTAMP=int(time.time())
    READER_ID_FOR_CHECK_BUTTON=reader_id
    
    # send reader event: [A, CA08] (Sam's setting) to IOT service
    loop_id=check_loop_id_from_device_id(reader_id)
    if loop_id=="":
      print("[set_check_button_for_reader] reader(%s) is NOT on any loop => return False" %(reader_id),  flush=True)
      return False
    
    signal_obj={
      "type_id":"d04",
      "device_id":reader_id,
      "loop_id":loop_id ,
      "alert_catalog": "A",
      "abnormal_catalog": "CA08",
    }
    obj=app.server_lib.trigger_to_server(signal_obj)
    try:           
        if obj['status']==200:
          result=True
        else: 
          result=False
    except ValueError:
        result=False
    
    return result 
    
    
    
def get_reader_for_check_button():
    global CHECK_BUTTON_TIMESTAMP,READER_ID_FOR_CHECK_BUTTON
    current_timestamp=int(time.time())
    
    #print("TEST:",  flush=True)
    #print(READER_ID_FOR_CHECK_BUTTON,  flush=True)
    #print(CHECK_BUTTON_TIMESTAMP,  flush=True)
    #print(current_timestamp,  flush=True)
    #print(current_timestamp-CHECK_BUTTON_TIMESTAMP,  flush=True)
    
    # remote-control button2 must be clicked in 60(?) seconds
    if current_timestamp-CHECK_BUTTON_TIMESTAMP <= 60:
      return READER_ID_FOR_CHECK_BUTTON
    else: 
      return ''
    
    
################################################################################
# socket.emit('setLowBatteryConfig', {device_id: 'device_id', value: 30, offset: 5});
def sensor_battery_config(type_id, value):
    # send battery-config command to all devices of the specified type
    if str(type_id) =="d02":
      table_name="WLReedSensor"
    elif str(type_id) =="d03":
      table_name="WLDoubleBondBTemperatureSensor"
    elif str(type_id) =="d04":
      table_name="WLReadSensor"
    elif str(type_id) =="d05":
      table_name="WLRemoteControl"
    else:
      print ("[sensor_battery_config] type_id(%s) is NOT supported. Return" %(str(type_id)),  flush=True)
      return False 

    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_config_list=file_content[table_name]
    except IOError:
      print ("[sensor_battery_config][%s.json] doesn't exist. Return error" %(table_name),  flush=True)
      return False

    # iterate the list 
    for record in device_config_list:
      print ("[sensor_battery_config][%s] battery_low_power_set=%s" %(str(record["device_id"]), str(value)),  flush=True)
      # update the battery config
      record["battery_low_power_set"]=str(value)
      
      # send command to the device
      data_obj=dict() 
      data_obj["device_id"]=str(record["device_id"])
      data_obj["value"]=int(value)
      if "battery_low_power_gap" in record :
        data_obj["offset"]=int(record["battery_low_power_gap"])
      else:
        data_obj["offset"]=10
      device_command_api2("setLowBatteryConfig", data_obj)
      
      # update config to server 
      app.server_lib.update_to_server(table_name, record)
        
    # save to json file
    with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
      json.dump(file_content, f)
      
        
################################################################################
# socket.emit('setSetUnsetConfig', {device_id: 'device_id', state:1});
# state: 1 = set, 0 = unset
def sensor_security_set(state):
    print("[sensor_security_set][%s] start. (state=%d)" %(str(datetime.datetime.now()), int(state)),  flush=True)
    
    period=30
    if state=="0":
      period=60  
    table_name="DeviceSetting"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        if state=="0":
          period=int(file_content[table_name][0]["sendMsg_time_when_disabled"])
        else:
          period=int(file_content[table_name][0]["sendMsg_time_when_enabled"])
    except:
      print("[sensor_security_set][%s] unknown error => use default setting" %(table_name),  flush=True)
      pass
    
    # send set/unset command to all sensor devices(d02,d03,d04)
    try:
      with open(app.views.get_json_file_dir() + "Device.json", "r") as f:
        file_content=json.load(f)
        device_list=file_content["Device"]
    except IOError:
      print ("[Device.json] doesn't exist. return",  flush=True)
      return
    except:
      print ("[sensor_security_set] unknown error. return",  flush=True)
      return        
    
    period=get_communication_time_by_security_state(str(state))
    for dev_obj in device_list:
      #if dev_obj["type_id"] != "d05" and dev_obj["type_id"] != "d06":
      if dev_obj["type_id"] == "d02" or dev_obj["type_id"] == "d03" or dev_obj["type_id"] == "d04" :
        # only for sensors
        device_id=str(dev_obj["device_id"])
        loop_id,loop_status=check_loop_status_from_device_id(device_id)
        print ("[sensor_security_set][%s,%s] loop_status=%s" %(loop_id,device_id,loop_status),  flush=True)
        if loop_status=="0":            
          # for general loop type: allow set/unset  
          data_obj=dict() 
          data_obj["device_id"]=device_id
          data_obj["state"]=int(state)
          device_command_api2('setSetUnsetConfig', data_obj)
          
          # 設定回報時間
          # socket.emit('setInterval', {device_id: extAddr, reporting: 10, polling: 2});
          # reporting: reporting second
          # polling: polling second
          data_obj=dict() 
          data_obj["device_id"]=device_id
          data_obj["reporting"]=period
          data_obj["polling"]=2
          device_command_api2('setInterval', data_obj)
          
        elif state==1:            
          # enable security for other loop types, just in case 
          data_obj=dict() 
          data_obj["device_id"]=device_id
          data_obj["state"]=1
          device_command_api2('setSetUnsetConfig', data_obj)
          
          # 設定回報時間
          # socket.emit('setInterval', {device_id: extAddr, reporting: 10, polling: 2});
          data_obj=dict() 
          data_obj["device_id"]=device_id
          data_obj["reporting"]=period
          data_obj["polling"]=2
          device_command_api2('setInterval', data_obj)
          
          
    print("[sensor_security_set][%s] end" %(str(datetime.datetime.now())),  flush=True)
      
################################################################################
def handle_remotecontrol_event(event_obj):
    '''
    { 
      "device_id":"",  
      "trigger_type":""
    }
    '''
    result=False
    type_id="d05"
    device_id=str(event_obj["device_id"])
    trigger_type=str(event_obj["trigger_type"])
    if trigger_type=="1":
      # 1=reset
      pass
    elif trigger_type=="2":
      # 2=device_connect
      result=device_connect(type_id, event_obj)
      
    elif trigger_type=="3":
      # 3=security set
      #result=remotecontrol_security_set(device_id)
      result=remotecontrol_partition_set(device_id)
      if not result:
        print("[security set] make sound !!!\n\n",  flush=True)
        audio_obj=app.audio.Audio() 
        # "system_set_fail.mp3" takes 2.8sec, set 8.4sec to play it three times
        audio_obj.play(app.host_lib.get_service_dir() + "system_set_fail.mp3", 8.4)
        app.device_lib.ask_readers_make_sound(5, 3)
        pass
        
    elif trigger_type=="4":
      # 4=electric lock button event
      result=remotecontrol_reader_lock(device_id)
      
    elif trigger_type=="5":
      # 5=partition security set by reader
      #result=remotecontrol_partition_set(device_id)
      result=remotecontrol_reader_partition_set(device_id)
      
    elif trigger_type=="6":
      # 6=urgent button
      result=remotecontrol_urgent_signal(device_id)               
    elif trigger_type=="7":
      # 7=patrol
      result=remotecontrol_patrol_signal(device_id)               
    
    elif trigger_type=="8":
      print("security unset!!!",  flush=True)
      # 8=security unset
      result=remotecontrol_partition_unset(device_id)
      if not result:
        print("[security unset] make sound !!!\n\n",  flush=True)
        audio_obj=app.audio.Audio() 
        # "system_unset_fail.mp3" takes 2.1sec, set 6.3sec to play it three times
        audio_obj.play(app.host_lib.get_service_dir() + "system_unset_fail.mp3", 6.3)
        pass
        
      # clear alarm_voice_flag
      #app.host_lib.clear_alarm_voice_flag()
      #app.host_lib.lcm_clear_and_update()
      #result=remotecontrol_security_unset(device_id)
      
      
      '''
      if result:
        print("[security unset] make sound !!!\n\n")
        audio_obj=app.audio.Audio()
        audio_obj.stop() 
        audio_obj.play(app.host_lib.get_service_dir() + "system_unset.mp3")
        pass
      else:
        print("[security unset] make sound !!!\n\n")
        audio_obj=app.audio.Audio() 
        audio_obj.stop()
        audio_obj.play(app.host_lib.get_service_dir() + "system_unset_fail.mp3")
        pass
      '''
      
      '''  
      try:
        with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
          file_content=json.load(f)
          keepalive_obj=file_content["KeepAlive"][0]
      except IOError:
        #print ("[KeepAlive.json] doesn't exist. The account is NOT opened!")
        keepalive_obj=dict()              
      
      if app.host_lib.check_security_always_set_enabled(keepalive_obj):
        signal_obj={
          "type_id":"d01",
          "device_id":device_id,
          "alert_catalog": "U",
          "abnormal_catalog": "GU01", 
          "abnormal_value": device_id
        } 
        app.server_lib.trigger_to_server(signal_obj)
      '''
    elif trigger_type=="9":
      # device disconnect
      print("unsupported operation ...",  flush=True)
      pass
      
    elif trigger_type=="10":
      # 10=partition security unset by reader
      result=remotecontrol_reader_partition_unset(device_id)
    
    elif trigger_type=="11":
      # 11=urgent stop
      result=remotecontrol_urgent_stop(device_id)               
      
    return result

def remotecontrol_patrol_signal(rc_id):
    print("[remotecontrol_patrol_signal][%s] start." %(str(datetime.datetime.now())),  flush=True)
    # six-digit random number
    code=""
    for i in range(0,6): 
      code= code + str(random.randrange(10))
    
    app.host_lib.save_validate_code(code)
    
    signal_obj={
      "type_id":"d05",
      "device_id":rc_id,
      "alert_catalog": "J",
      "abnormal_catalog": "CJ01",
      "validate_code":code,
      "internal_code":rc_id,
    }
    print("[remotecontrol_patrol_signal][%s] test 1." %(str(datetime.datetime.now())),  flush=True)
    # ask reader to make sound ...
    # use web socket
    target_reader_id=get_reader_for_check_button()
    data_obj={"device_id": target_reader_id, "mode":0, "time":3}
    device_command_api2("setMusicConfig", data_obj)
    print("[remotecontrol_patrol_signal][%s] test 2." %(str(datetime.datetime.now())),  flush=True)
    
    obj=app.server_lib.trigger_to_server(signal_obj)
    try:           
        if obj['status']==200:
          result=True
        else: 
          result=False
    except ValueError:
        result=False
    
    return result
    
################################################################################    
def remotecontrol_urgent_signal(rc_id):
    print("[remotecontrol_urgent_signal] make sound !!!\n\n",  flush=True)
    audio_obj=app.audio.Audio() 
    audio_obj.play(app.host_lib.get_service_dir() + "urgent.mp3", 0)
    
    #app.device_lib.ask_readers_make_sound(x, 180)
    
    # set alarm voice flag
    # disaster = "[fire][gas][sos]"
    disaster_index=1
    app.host_lib.set_alarm_voice_flag("", disaster_index)

    # display alarm info on LCM
    app.host_lib.lcm_display_alarm_info()
      
    print("[remotecontrol_urgent_signal] trigger LED !",  flush=True)
    app.host_lib.set_led(LED6_ALM_R, 0.5)
      
    # send remotecontrol urgent message: [A, KA01] to IOT service  
    signal_obj={
      "type_id":"d05",
      "device_id":rc_id,
      "alert_catalog": "A",
      "abnormal_catalog": "KA01",
      "abnormal_value": rc_id
    }
    obj=app.server_lib.trigger_to_server(signal_obj)
    try:           
        if obj['status']==200:
          result=True
        else: 
          result=False
    except ValueError:
        result=False
    
    return result
    
    
def remotecontrol_urgent_stop(rc_id):
    print("[remotecontrol_urgent_stop] stop sound.",  flush=True)
    audio_obj=app.audio.Audio()
    audio_obj.stop()
    
    '''
    # clear alarm_voice_flag ?
    app.host_lib.clear_alarm_voice_flag()
    # clear alarm info on LCM ?
    app.host_lib.lcm_clear_and_update()
    
    print("[remotecontrol_urgent_stop] stop LED ?",  flush=True)
    app.host_lib.off_led(LED6_ALM_R)
    '''
    
    # no event to IOT service for urgent stop ?   
    
    return True

################################################################################    
def remotecontrol_reader_lock(rc_id):
    try:
      with open(app.views.get_json_file_dir() + "RemoteControl_LockReader.json", "r") as f:
        file_content=json.load(f)
        rc_list=file_content["RemoteControl_LockReader"]
    except IOError:
      print ("RemoteControl_LockReader.json] file doesn't exist. skip it",  flush=True)
      return False
      
    print("[remotecontrol_reader_lock] rc_id=" + rc_id,  flush=True)
    item_found=False
    for item in rc_list:
      print("[remotecontrol_reader_lock] device_id=" + item["device_id"],  flush=True)
      if str(rc_id) == item["device_id"]:
        item_found=True  
        matched_reader_list=item["reader_id"]
        for reader_id in matched_reader_list:
          print("reader_id=" + reader_id,  flush=True)
          #socket.emit('sendElectricLockAction', {device_id: extAddr, relay:3});
          #relay: bit1 = relay1, bit2 = relay2
          data_obj=dict() 
          data_obj["device_id"]=str(reader_id)
          data_obj["relay"]=3
          device_command_api2('sendElectricLockAction', data_obj)
        
          # remarked, 20170222, zl
          '''
          # trigger electric-lock event to server for each reader
          signal_obj=dict()
          signal_obj["device_id"]=str(reader_id)
          signal_obj["type_id"]="d04"
          signal_obj["loop_id"]=check_loop_id_from_device_id(reader_id)
          signal_obj["alert_catalog"]="A"
          signal_obj["abnormal_catalog"]="KA02"
          #signal_obj["abnormal_value"]=""
          #signal_obj["setting_value"]=""
          obj=app.server_lib.trigger_to_server(signal_obj)
          '''
        break
        
    if item_found:
      # trigger electric-lock event to server for rc
      signal_obj=dict()
      signal_obj["device_id"]=str(rc_id)
      signal_obj["type_id"]="d05"
      #signal_obj["loop_id"]=""
      signal_obj["alert_catalog"]="A"
      signal_obj["abnormal_catalog"]="KA02"
      obj=app.server_lib.trigger_to_server(signal_obj)
                 
    return True    


################################################################################
def remotecontrol_security_set(rc_id):
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      print("[remotecontrol_security_set] FAILED.",  flush=True)
      return False 
    
    # check if security is already set
    if app.host_lib.check_host_alarm_set(keepalive_obj):
      print("[remotecontrol_security_set] security is already set.\n\n",  flush=True)
      
      # if alarm_voice_flag is not set, make voice prompt 
      flag,obj=app.host_lib.get_alarm_voice_flag()
      if flag==False:
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "already_set.mp3")
      else:
        print("[remotecontrol_security_set] alarm_voice_flag is set ==> do NOT interrupt alarm(event_happen) voice !\n\n",  flush=True)
        pass
        
      return True
    
    # check resetting status of reeds
    if check_all_sensor_resetting_status() == False:
      print("[remotecontrol_security_set] FAILED.",  flush=True)
      return False
    
        
    signal_obj={"type_id": "d01"}
    if True:
      print("new host state=set",  flush=True)
      security_set='1'
      alert_catalog='S'
      abnormal_catalog='GS01'
      signal_obj["device_id"]=str(rc_id)
      signal_obj["abnormal_value"]=str(rc_id)
      
    keepalive_obj["start_host_set"]=str(security_set)
    # save new security setting:
    with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
      json.dump(file_content, f)
    
    print("[security set] make sound !!!\n\n",  flush=True)
    audio_obj=app.audio.Audio() 
    audio_obj.play(app.host_lib.get_service_dir() + "system_set_ok.mp3")
         
    app.device_lib.ask_readers_make_sound(6, 1)
             
    sensor_security_set(1)
    
    # update partition status
    app.host_lib.update_partition_status("1")
           
    app.host_lib.host_setunset_gpio("1")
           
    # trigger to server
    signal_obj["alert_catalog"]=alert_catalog
    signal_obj["abnormal_catalog"]=abnormal_catalog
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
        trigger_result=True
        app.server_lib.send_keepalive_to_server()
    else:  
        trigger_result=False
    
    #return trigger_result
    return True
    

def remotecontrol_security_unset(rc_id):
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False 
    except:
      print("[remotecontrol_security_unset] unknown error...",  flush=True)
      return False
    
    # check if security is already unset
    if not app.host_lib.check_host_alarm_set(keepalive_obj):
      print("[remotecontrol_security_unset] security is already unset.",  flush=True)
      audio_obj=app.audio.Audio() 
      audio_obj.stop()
      audio_obj.play(app.host_lib.get_service_dir() + "already_unset.mp3")
      return True
      
    print("new host state=unset",  flush=True)
    security_set='0'  
    
    keepalive_obj["start_host_set"]=str(security_set)
    # save new security setting:
    with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
      json.dump(file_content, f)

    print("[security unset] make sound !!!\n\n",  flush=True)
    audio_obj=app.audio.Audio() 
    audio_obj.play(app.host_lib.get_service_dir() + "system_unset.mp3")
    
    app.device_lib.ask_readers_make_sound(4, 1)
    
    sensor_security_set(0)
    
    # update partition status
    app.host_lib.update_partition_status("0")
    
    app.host_lib.host_setunset_gpio("0")
    
    print("[remotecontrol_security_unset] stop LED.",  flush=True)
    app.host_lib.off_led(LED6_ALM_R)
    
      
    app.host_lib.stop_coupling_operation()
    
    signal_obj={"type_id": "d01"}
    alert_catalog='S'
    abnormal_catalog='GS02'
    signal_obj["device_id"]=str(rc_id)
    signal_obj["abnormal_value"]=str(rc_id)
    signal_obj["alert_catalog"]=alert_catalog
    signal_obj["abnormal_catalog"]=abnormal_catalog
    
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
        trigger_result=True
        app.server_lib.send_keepalive_to_server()
    else:
        trigger_result=False

    # check if it is early unset ...
    if app.host_lib.check_early_lift(keepalive_obj):
        print("host security is early unset !!!",  flush=True)
        #print("preservation_lift_time=" + keepalive_obj["preservation_lift_time"],  flush=True)
        current_time=datetime.datetime.now().strftime("%H:%M")
        print("current time=" + current_time,  flush=True)

        alert_catalog='U'
        abnormal_catalog='GU01'
        signal_obj["device_id"]=str(rc_id)
        signal_obj["setting_value"]=keepalive_obj["preservation_lift_time"]
        signal_obj["abnormal_value"]=current_time
        signal_obj["alert_catalog"]=alert_catalog
        signal_obj["abnormal_catalog"]=abnormal_catalog
        obj=app.server_lib.trigger_to_server(signal_obj)

    # check if it is holiday unset
    elif app.host_lib.check_holiday_lift(keepalive_obj):
        print("host security holiday unset !!!",  flush=True)
        alert_catalog='U'
        abnormal_catalog='GU09'
        signal_obj["device_id"]=str(rc_id)
        signal_obj["alert_catalog"]=alert_catalog
        signal_obj["abnormal_catalog"]=abnormal_catalog
        obj=app.server_lib.trigger_to_server(signal_obj)

    #return trigger_result
    return True

################################################################################    
def remotecontrol_partition_set(rc_id):
    print("[remotecontrol_partition_set] rc_id=%s" %rc_id,  flush=True)
    
    # TODO: not sure
    '''
    # if alarm_voice_flag is set(alarm is happening), skip this operation 
    flag,obj=app.host_lib.get_alarm_voice_flag()
    if flag:
      print("[remotecontrol_security_set] alarm_voice_flag is set => skip this operation !\n\n",  flush=True)
      return False
    '''
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
    except:
      print ("[remotecontrol_partition_set] [KeepAlive.json] unknown error",  flush=True)
      return False
      
    try:
      with open(app.views.get_json_file_dir() + "RemoteControl_Partition.json", "r") as f:
        file_content=json.load(f)
        rc_list=file_content["RemoteControl_Partition"]
    except IOError:
      print ("RemoteControl_Partition.json] file doesn't exist. skip it")
      return False
  
    try:
      with open(app.views.get_json_file_dir() + "Partition.json", "r") as f:
        partition_file_content=json.load(f)
        total_partition_list=partition_file_content["Partition"]
    except IOError:
      print ("[Partition.json] file doesn't exist. skip it",  flush=True)
      return False
    except:
      print ("[remotecontrol_partition_set][Partition.json] unknown error",  flush=True)
      return False
   
    
    # check if this remotecontrol is for host_security_set/unset
    ''' 
    {"RemoteControl_Partition": [
      {"device_id": "5149013115785703", "partition_id": ["0"]}
    ]} 
    '''
    for item in rc_list:
      #print("device_id=" + item["device_id"],  flush=True)
      if rc_id == item["device_id"] and "0" in item["partition_id"]:
        print ("[remotecontrol_partition_set] do host security set !",  flush=True)
        result=remotecontrol_security_set(rc_id)
        if not result:
          print("[remotecontrol_partition_set][host security set] failed !",  flush=True)
          audio_obj=app.audio.Audio() 
          # "system_set_fail.mp3" takes 2.8sec, set 8.4sec to play it three times
          audio_obj.play(app.host_lib.get_service_dir() + "system_set_fail.mp3", 8.4)
          app.device_lib.ask_readers_make_sound(5, 3)
          
        return result
         
    item_found=False
    already_set_flag=False
    set_ok_flag=dict()
    dev_unique_list=set()
    for item in rc_list:
      #print("device_id=" + item["device_id"],  flush=True)
      if rc_id == item["device_id"]:
        matched_partition_list=item["partition_id"]
        
        # just in case 
        if len(matched_partition_list)==0:
          break
        
        for part_id in matched_partition_list:
          print("part_id=" + part_id,  flush=True)
          set_ok_flag[part_id]=True
        
          for part_item in total_partition_list:
            if part_item["partition_id"]==part_id:
              # change the status of designated partition !!!
              if "status" in part_item:
                if part_item["status"]=="1":
                  print("[remotecontrol_partition_set] partition(%s) is already set" %part_id,  flush=True)
                  already_set_flag=True
                  break
                
              else:
                # refer to host security state
                if app.host_lib.check_host_alarm_set(keepalive_obj):
                  print("[remotecontrol_partition_set] partition(%s) is already set" %part_id,  flush=True)
                  part_item["status"]="1"
                  already_set_flag=True
                  break
                
              # check sensor resetting status for loops(devices) in the this partition
              for loop_id in part_item["loop_id"]:
                dev_id=check_device_id_from_loop_id(loop_id)
                if get_sensor_resetting_status(dev_id) == "1":
                  print("[remotecontrol_partition_set][%s,%s,%s] is NOT in resetting state !" %(part_id,loop_id,dev_id),  flush=True)
                  set_ok_flag[part_id]=False
                  break
              
              if set_ok_flag[part_id]:
                # set current partition status:0=unset, 1=set
                part_item["status"]="1"
                for loop_id in part_item["loop_id"]:
                  dev_id=check_device_id_from_loop_id(loop_id)
                  print("[remotecontrol_partition_set][%s,%s,%s] is ok to set" %(part_id,loop_id,dev_id),  flush=True)
                  dev_unique_list.add(dev_id)
                
              break

        item_found=True
        break                                             
        
    if item_found:
      # save current partition status
      with open(app.views.get_json_file_dir() + "Partition.json", "w") as f:
        json.dump(partition_file_content, f)

      # check if all partitions are set ok
      all_partition_ok_flag=True
      for partid,flag in set_ok_flag.items():
        if flag==False:
          all_partition_ok_flag=False
          break
          
      if all_partition_ok_flag:
        if already_set_flag:
          print("[remotecontrol_partition_set] some partitions are already set.",  flush=True)
          audio_obj=app.audio.Audio() 
          audio_obj.play(app.host_lib.get_service_dir() + "partition_already_set.mp3")
        else:
          print("[remotecontrol_partition_set] all partitions are set ok !",  flush=True)
          audio_obj=app.audio.Audio() 
          audio_obj.play(app.host_lib.get_service_dir() + "partition_set_ok.mp3")

        # all sensor security set according to host/partition setting ...
        try:
          with open(app.views.get_json_file_dir() + "Device.json", "r") as f:
            file_content=json.load(f)
            device_list=file_content["Device"]
          
          #for dev_item in device_list:
          print("[remotecontrol_partition_set] dev_unique_list=%s" %str(dev_unique_list),  flush=True)
          period=get_communication_time_by_security_state("1")
          for dev_id in dev_unique_list:
            if app.device_lib.get_sensor_security_set(dev_id):
              data_obj=dict() 
              data_obj["device_id"]=dev_id
              data_obj["state"]=1
              device_command_api2('setSetUnsetConfig', data_obj)
              
              # 設定回報時間
              # socket.emit('setInterval', {device_id: extAddr, reporting: 10, polling: 2});
              data_obj=dict() 
              data_obj["device_id"]=dev_id
              data_obj["reporting"]=period
              data_obj["polling"]=2
              device_command_api2('setInterval', data_obj)
          
        except:
          print ("[remotecontrol_partition_set][Device.json] unknow error => skip sensor security set",  flush=True)
          pass

                 
      else:
        print("[remotecontrol_partition_set] some partitions is NOT in resetting state !",  flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "partition_set_fail.mp3")
        
      # A:d05:KA04:無線遙控器分區設定
      # S:d05:GS05:無線遙控器分區解除
      # trigger to server
      signal_obj=dict()
      signal_obj["device_id"]=str(rc_id)
      signal_obj["type_id"]="d05"
      signal_obj["alert_catalog"]="A"
      signal_obj["abnormal_catalog"]="KA04"
      obj=app.server_lib.trigger_to_server(signal_obj)
        
    else:
      print("[remotecontrol_partition_set] rc(%s) is NOT found in setting file ==> ignore this event" %rc_id,  flush=True)
      audio_obj=app.audio.Audio() 
      audio_obj.play(app.host_lib.get_service_dir() + "rc_not_bind_to_part.mp3")
      return False
            
    return True

def remotecontrol_partition_unset(rc_id):
    print("[remotecontrol_partition_unset] rc_id=%s" %rc_id,  flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
    except:
      print ("[remotecontrol_partition_unset] [KeepAlive.json] unknown error",  flush=True)
      return False
      
    try:
      with open(app.views.get_json_file_dir() + "RemoteControl_Partition.json", "r") as f:
        file_content=json.load(f)
        rc_list=file_content["RemoteControl_Partition"]
    except IOError:
      print ("RemoteControl_Partition.json] file doesn't exist. skip it")
      return False
  
    try:
      with open(app.views.get_json_file_dir() + "Partition.json", "r") as f:
        partition_file_content=json.load(f)
        total_partition_list=partition_file_content["Partition"]
    except IOError:
      print ("[Partition.json] file doesn't exist. skip it",  flush=True)
      return False
    except:
      print ("[remotecontrol_partition_unset][Partition.json] unknown error",  flush=True)
      return False
    
    # check if this remotecontrol is for host_security_set/unset
    ''' 
    {"RemoteControl_Partition": [
      {"device_id": "5149013115785703", "partition_id": ["0"]}
    ]} 
    '''
    for item in rc_list:
      #print("device_id=" + item["device_id"],  flush=True)
      if rc_id == item["device_id"] and "0" in item["partition_id"]:
        print ("[remotecontrol_partition_unset] do host security unset !",  flush=True)
        # clear alarm_voice_flag
        app.host_lib.clear_alarm_voice_flag()
        app.host_lib.lcm_clear_and_update()
        result=remotecontrol_security_unset(rc_id)
        if not result:
          print("[remotecontrol_partition_unset][host security unset] failed !",  flush=True)
          audio_obj=app.audio.Audio() 
          # "system_unset_fail.mp3" takes 2.1 sec
          audio_obj.play(app.host_lib.get_service_dir() + "system_unset_fail.mp3", 6.3)
        
        return result
    
      
    item_found=False
    already_unset_flag=False
    dev_unique_list=set()
    for item in rc_list:
      #print("device_id=" + item["device_id"],  flush=True)
      if rc_id == item["device_id"]:
        matched_partition_list=item["partition_id"]
        
        # just in case 
        if len(matched_partition_list)==0:
          break
        
        for part_id in matched_partition_list:
          print("part_id=" + part_id,  flush=True)
          
          for part_item in total_partition_list:
            if part_item["partition_id"]==part_id:
              # change the status of  designated partition !!!
              if "status" in part_item:
                if part_item["status"]=="0":
                  print("[remotecontrol_partition_unset] partition(%s) is already unset" %part_id,  flush=True)
                  already_unset_flag=True
                  break
                
              else:
                # refer to host security state
                if not app.host_lib.check_host_alarm_set(keepalive_obj):
                  print("[remotecontrol_partition_unset] partition(%s) is already unset" %part_id,  flush=True)
                  part_item["status"]="0"
                  already_unset_flag=True
                  break
              
              part_item["status"]="0"
              for loop_id in part_item["loop_id"]:
                dev_id=check_device_id_from_loop_id(loop_id)
                print("[remotecontrol_partition_unset][%s,%s,%s] is ok to unset" %(part_id,loop_id,dev_id),  flush=True)
                dev_unique_list.add(dev_id)
                
              break

        item_found=True
        break                                             
        
    if item_found:
      # save current partition status
      with open(app.views.get_json_file_dir() + "Partition.json", "w") as f:
        json.dump(partition_file_content, f)

      if already_unset_flag:
        print("[remotecontrol_partition_unset] some partitions are already unset.",  flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "partition_already_unset.mp3")
      else:
        print("[remotecontrol_partition_unset] all partitions are unset ok !",  flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "partition_unset.mp3")       
        
      # all sensor security unset according to host/partition setting ...
      print("[remotecontrol_partition_unset] dev_unique_list=%s" %str(dev_unique_list),  flush=True)
      period=get_communication_time_by_security_state("0")
      for dev_id in dev_unique_list:
        if not app.device_lib.get_sensor_security_set(dev_id):
          data_obj=dict() 
          data_obj["device_id"]=dev_id
          data_obj["state"]=0
          device_command_api2('setSetUnsetConfig', data_obj)

          # 設定回報時間
          # socket.emit('setInterval', {device_id: extAddr, reporting: 10, polling: 2});
          data_obj=dict() 
          data_obj["device_id"]=dev_id
          data_obj["reporting"]=period
          data_obj["polling"]=2
          device_command_api2('setInterval', data_obj)
            
      # A:d05:KA04:無線遙控器分區設定
      # S:d05:GS05:無線遙控器分區解除
      # trigger to server
      signal_obj=dict()
      signal_obj["device_id"]=str(rc_id)
      signal_obj["type_id"]="d05"
      signal_obj["alert_catalog"]="S"
      signal_obj["abnormal_catalog"]="GS05"
      obj=app.server_lib.trigger_to_server(signal_obj)
  
    else:
      print("[remotecontrol_partition_unset] rc(%s) is NOT found in setting file ==> ignore this event" %(rc_id),  flush=True)
      audio_obj=app.audio.Audio() 
      audio_obj.play(app.host_lib.get_service_dir() + "rc_not_bind_to_part.mp3")
      return False
            
    return True

################################################################################
def remotecontrol_reader_partition_set(rc_id):
    print("[remotecontrol_reader_partition_set] rc_id=%s" %rc_id,  flush=True)

    # TODO: not sure
    '''
    # if alarm_voice_flag is set, skip this operation 
    flag,obj=app.host_lib.get_alarm_voice_flag()
    if flag:
      print("[remotecontrol_reader_partition_set] alarm_voice_flag is set => skip this operation !\n\n",  flush=True)
      return False
    '''
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
    except:
      print ("[remotecontrol_reader_partition_set] [KeepAlive.json] unknown error => skip this operation",  flush=True)
      return False
      
    try:
      with open(app.views.get_json_file_dir() + "RemoteControl_PartitionReader.json", "r") as f:
        file_content=json.load(f)
        rc_list=file_content["RemoteControl_PartitionReader"]
    except IOError:
      print ("RemoteControl_PartitionReader.json] file doesn't exist. skip it")
      return False
  
    try:
      with open(app.views.get_json_file_dir() + "Partition.json", "r") as f:
        partition_file_content=json.load(f)
        total_partition_list=partition_file_content["Partition"]
    except IOError:
      print ("[Partition.json] file doesn't exist. skip it",  flush=True)
      return False
    except:
      print ("[remotecontrol_reader_partition_set][Partition.json] unknown error => skip this operation",  flush=True)
      return False

    reader_id=get_reader_for_check_button()
    if  reader_id == "":
      print ("[remotecontrol_reader_partition_set] unable to get reader id => skip this operation",  flush=True)
      return False
    
    print ("[remotecontrol_reader_partition_set] reader_id=%s" %(reader_id),  flush=True)
      
    item_found=False
    already_set_flag=False
    set_ok_flag=dict()
    dev_unique_list=set()
    for item in rc_list:
      if rc_id == item["device_id"]:
        reader_array=item["reader_array"]
        for reader_item in reader_array:
          if reader_id == reader_item["reader_id"]:
            matched_partition_list=reader_item["partition_id"]
            
            # just in case 
            if len(matched_partition_list)==0:
              break

            # iterate all existing partitions
            if "0" in matched_partition_list:
              matched_partition_list.remove("0")
              for xx in total_partition_list:
                matched_partition_list.append(xx["partition_id"])
                            
            for part_id in matched_partition_list:
              print("part_id=" + part_id,  flush=True)
              set_ok_flag[part_id]=True
        
              for part_item in total_partition_list:
                if part_item["partition_id"]==part_id:
                  # change the status of designated partition !!!
                  if "status" in part_item:
                    if part_item["status"]=="1":
                      print("[remotecontrol_reader_partition_set] partition(%s) is already set" %part_id,  flush=True)
                      already_set_flag=True
                      break
                
                  else:
                    # refer to host security state
                    if app.host_lib.check_host_alarm_set(keepalive_obj):
                      print("[remotecontrol_reader_partition_set] partition(%s) is already set" %part_id,  flush=True)
                      part_item["status"]="1"
                      already_set_flag=True
                      break
                
                  # check sensor resetting status for loops(devices) in the this partition
                  for loop_id in part_item["loop_id"]:
                    dev_id=check_device_id_from_loop_id(loop_id)
                    if get_sensor_resetting_status(dev_id) == "1":
                      print("[remotecontrol_reader_partition_set][%s,%s,%s] is NOT in resetting state !" %(part_id,loop_id,dev_id),  flush=True)
                      set_ok_flag[part_id]=False
                      break
              
                  if set_ok_flag[part_id]:
                    # set current partition status:0=unset, 1=set
                    part_item["status"]="1"
                    for loop_id in part_item["loop_id"]:
                      dev_id=check_device_id_from_loop_id(loop_id)
                      print("[remotecontrol_reader_partition_set][%s,%s,%s] is ok to set" %(part_id,loop_id,dev_id),  flush=True)
                      dev_unique_list.add(dev_id)
                      
                  break
            
            item_found=True                  
            break
        
    if item_found:
      # save current partition status
      with open(app.views.get_json_file_dir() + "Partition.json", "w") as f:
        json.dump(partition_file_content, f)

      if already_set_flag:
        print("[remotecontrol_reader_partition_set] some partitions are already set.",  flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "partition_already_set.mp3")
      else:
        print("[remotecontrol_reader_partition_set] all partitions are set ok !",  flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "partition_set_ok.mp3")       
        
      
      # all sensor security set according to host/partition setting ...
      print("[remotecontrol_reader_partition_set] dev_unique_list=%s" %str(dev_unique_list),  flush=True)
      period=get_communication_time_by_security_state("1")
      for dev_id in dev_unique_list:
        if app.device_lib.get_sensor_security_set(dev_id):
          data_obj=dict() 
          data_obj["device_id"]=dev_id
          data_obj["state"]=1
          device_command_api2('setSetUnsetConfig', data_obj)

          # 設定回報時間
          # socket.emit('setInterval', {device_id: extAddr, reporting: 10, polling: 2});
          data_obj=dict() 
          data_obj["device_id"]=dev_id
          data_obj["reporting"]=period
          data_obj["polling"]=2
          device_command_api2('setInterval', data_obj)
      
      # A:d05:KA04:無線遙控器分區設定
      # S:d05:GS05:無線遙控器分區解除
      # trigger to server
      signal_obj=dict()
      signal_obj["device_id"]=str(rc_id)
      signal_obj["type_id"]="d05"
      signal_obj["alert_catalog"]="A"
      signal_obj["abnormal_catalog"]="KA04"
      obj=app.server_lib.trigger_to_server(signal_obj)
        
    else:
      print("[remotecontrol_reader_partition_set] rc(%s) and reader(%s) are NOT matched in setting file ==> ignore this event" %(rc_id, reader_id),  flush=True)
      audio_obj=app.audio.Audio() 
      audio_obj.play(app.host_lib.get_service_dir() + "rc_not_bind_to_part.mp3") #??
      return False
            
    return True
    

def remotecontrol_reader_partition_unset(rc_id):
    print("[remotecontrol_reader_partition_unset] rc_id=%s" %rc_id,  flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
    except:
      print ("[remotecontrol_reader_partition_unset] [KeepAlive.json] unknown error => skip this operation",  flush=True)
      return False
      
    try:
      with open(app.views.get_json_file_dir() + "RemoteControl_PartitionReader.json", "r") as f:
        file_content=json.load(f)
        rc_list=file_content["RemoteControl_PartitionReader"]
    except IOError:
      print ("RemoteControl_PartitionReader.json] file doesn't exist. skip it")
      return False
  
    try:
      with open(app.views.get_json_file_dir() + "Partition.json", "r") as f:
        partition_file_content=json.load(f)
        total_partition_list=partition_file_content["Partition"]
    except IOError:
      print ("[Partition.json] file doesn't exist. skip it",  flush=True)
      return False
    except:
      print ("[remotecontrol_reader_partition_unset][Partition.json] unknown error => skip this operation",  flush=True)
      return False

    reader_id=get_reader_for_check_button()
    if  reader_id == "":
      print ("[remotecontrol_reader_partition_unset] unable to get reader id => skip this operation",  flush=True)
      return False
    
    print ("[remotecontrol_reader_partition_unset] reader_id=%s" %(reader_id),  flush=True)
      
    item_found=False
    already_unset_flag=False
    dev_unique_list=set()
    for item in rc_list:
      if rc_id == item["device_id"]:
        reader_array=item["reader_array"]
        for reader_item in reader_array:
          if reader_id == reader_item["reader_id"]:
            matched_partition_list=reader_item["partition_id"]        
            
            # just in case 
            if len(matched_partition_list)==0:
              break
            
            # iterate all existing partitions
            if "0" in matched_partition_list:
              matched_partition_list.remove("0")
              for xx in total_partition_list:
                matched_partition_list.append(xx["partition_id"])
            
            for part_id in matched_partition_list:
              print("part_id=" + part_id,  flush=True)
        
              for part_item in total_partition_list:
                if part_item["partition_id"]==part_id:
                  # change the status of designated partition !!!
                  if "status" in part_item:
                    if part_item["status"]=="0":
                      print("[remotecontrol_reader_partition_unset] partition(%s) is already unset" %part_id,  flush=True)
                      already_unset_flag=True
                      break
                
                  else:
                    # refer to host security state
                    if not app.host_lib.check_host_alarm_set(keepalive_obj):
                      print("[remotecontrol_reader_partition_unset] partition(%s) is already unset" %part_id,  flush=True)
                      part_item["status"]="0"
                      already_unset_flag=True
                      break
                
                  part_item["status"]="0"
                  for loop_id in part_item["loop_id"]:
                    dev_id=check_device_id_from_loop_id(loop_id)
                    print("[remotecontrol_reader_partition_unset][%s,%s,%s] is ok to unset" %(part_id,loop_id,dev_id),  flush=True)
                    dev_unique_list.add(dev_id)


                  break
            
            item_found=True                  
            break
        
    if item_found:
      # save current partition status
      with open(app.views.get_json_file_dir() + "Partition.json", "w") as f:
        json.dump(partition_file_content, f)
  
      if already_unset_flag:
        print("[remotecontrol_reader_partition_unset] some partitions are already unset.",  flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "partition_already_unset.mp3")
      else:
        print("[remotecontrol_reader_partition_unset] all partitions are unset ok !",  flush=True)
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "partition_unset.mp3")       
        
      # all sensor security unset according to host/partition setting ...
      print("[remotecontrol_reader_partition_unset] dev_unique_list=%s" %str(dev_unique_list),  flush=True)
      period=get_communication_time_by_security_state("0")
      for dev_id in dev_unique_list:
        if not app.device_lib.get_sensor_security_set(dev_id):
          data_obj=dict() 
          data_obj["device_id"]=dev_id
          data_obj["state"]=0
          device_command_api2('setSetUnsetConfig', data_obj)
      
          # 設定回報時間
          # socket.emit('setInterval', {device_id: extAddr, reporting: 10, polling: 2});
          data_obj=dict() 
          data_obj["device_id"]=dev_id
          data_obj["reporting"]=period
          data_obj["polling"]=2
          device_command_api2('setInterval', data_obj)
            
      # A:d05:KA04:無線遙控器分區設定
      # S:d05:GS05:無線遙控器分區解除
      # trigger to server
      signal_obj=dict()
      signal_obj["device_id"]=str(rc_id)
      signal_obj["type_id"]="d05"
      signal_obj["alert_catalog"]="S"
      signal_obj["abnormal_catalog"]="GS05"
      obj=app.server_lib.trigger_to_server(signal_obj)
        
    else:
      print("[remotecontrol_reader_partition_unset] rc(%s) and reader(%s) are NOT matched in setting file ==> ignore this event" %(rc_id, reader_id),  flush=True)
      audio_obj=app.audio.Audio() 
      audio_obj.play(app.host_lib.get_service_dir() + "rc_not_bind_to_part.mp3") #??
      return False
            
    return True
                  
################################################################################
# device-connect event and then update to IOT service 
def device_connect(type_id, event_obj):
    dev_id=str(event_obj["device_id"])
    print("[device_connect] type_id=%s, device_id=%s" %(type_id, dev_id),  flush=True)
    
    identity=""
    if "identity" in event_obj: 
      identity=str(event_obj["identity"])
    
    table_name="Device"
    file_content=dict()
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
    except IOError:
      print ("[" + table_name + ".json] doesn't exist. create empty result!",  flush=True)
      file_content[table_name]=list()

    # check if the device is in device-list
    item_found=False
    prev_connection_status="1"
    device_list=file_content[table_name]
    for dev in device_list:
      if str(dev["device_id"]) == dev_id:
        item_found=True
        # update time stamp
        dev["time"]=str(datetime.datetime.now())
        # update identity
        dev["identity"]=str(identity)
        
        # get previous connection_status
        if "connection_status" in dev:
          prev_connection_status=dev["connection_status"]
        
        # update connection_status
        dev["connection_status"]="0"
        break
        
    if not item_found:
      # add new device
      print("[device_connect][%s] it's new device." %(dev_id),  flush=True) 
      new_dev=dict()
      new_dev["time"]=str(datetime.datetime.now())
      new_dev["type_id"]=str(type_id)
      new_dev["device_id"]=dev_id 
      new_dev["identity"]=str(identity)
      new_dev["connection_status"]="0"
      new_dev["abnormal_status"]="00000"
      device_list.append(new_dev)
      # save Device.json
      with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
        json.dump(file_content, f)
      
      # register device to IOT service      
      app.server_lib.register_device_to_server(dev_id)
      
    else:
      print("[device_connect][%s] it's not new device." %(dev_id),  flush=True)
      # save Device.json
      with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
        json.dump(file_content, f)
    
    # save WLxxx.json ...
    save_device_config_file(type_id, dev_id)
    
    # handle extend device(s)
    update_extend_device(event_obj)
    # note: no WLxxx config for extend device(d07)
    
    '''
    # temp, zl  
    # if this device is disconnected before,
    # sync host security(set/unset) status ( only for reed, doublebondtempxx, reader)
    if prev_connection_status=="1" and str(type_id) != "d05" and str(type_id) != "d06":
      print("[device_connect][%s] sync host security status." %(dev_id),  flush=True) 
      data_obj=dict() 
      data_obj["device_id"]=dev_id
      try:
        with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
          file_content=json.load(f)
          keepalive_obj=file_content["KeepAlive"][0]
          if app.host_lib.check_host_alarm_set(keepalive_obj):
            data_obj["state"]=1
          else:
            data_obj["state"]=0
          device_command_api2('setSetUnsetConfig', data_obj)
      except IOError:
        print ("[device_connect][KeepAlive.json] doesn't exist.")
      except:
        print ("[device_connect][KeepAlive.json] unknown error.")
    '''
    
    # check if this device(excludes rc and ipcam) is disconnected before
    if prev_connection_status=="1" and str(type_id) != "d05" and str(type_id) != "d06":
      # trigger "device-connect" event to server 
      trigger_status=""
      alarm_type="ResumeConnectToHostAlarm"
      result=trigger_silent(type_id,dev_id,trigger_status,alarm_type)
    
    # turn off LED8_ALM_G 
    print("[device_connect] turn off LED !",  flush=True)
    app.host_lib.off_led(LED8_ALM_G)
         
    return True

################################################################################
# device-disconnect event and then update to IOT service 
def device_disconnect(type_id, event_obj):
    dev_id=str(event_obj["device_id"])
    print("[device_disconnect] device_id=%s" %(dev_id),  flush=True)
    
    table_name="Device"
    file_content=dict()
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
    except IOError:
      print ("[" + table_name + ".json] doesn't exist. create empty result!",  flush=True)
      file_content[table_name]=list()

    # check if the device is in device-list
    item_found=False
    device_list=file_content[table_name]
    for dev in device_list:
      if str(dev["device_id"]) == dev_id:
        item_found=True
        dev["connection_status"]="1"
        # don't break
        
      # if extend_device exists..  
      if str(dev["device_id"]) == dev_id + "-1":
        dev["connection_status"]="1"
      if str(dev["device_id"]) == dev_id + "-2":
        dev["connection_status"]="1"  
        
    if item_found:
      # save Device.json
      with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
        json.dump(file_content, f)
    
      # trigger "device-disconnect" event to server
      trigger_status=""
      alarm_type="LostConnectToHostAlarm"
      result=trigger_silent(type_id, dev_id, trigger_status, alarm_type)
    
    
      # turn on LED8_ALM_G 
      print("[device_disconnect] trigger LED !",  flush=True)
      app.host_lib.set_led(LED8_ALM_G, 0.5) 
      
    return True

################################################################################    
# device update status     
def device_update_status(event_obj):
    dev_id=str(event_obj["device_id"])
    print("[device_update_status] device_id=%s" %(dev_id),  flush=True)
    
    
    #search the Device.json to find the matched device by device_id 
    try:
      with open(app.views.get_json_file_dir() + "Device.json", "r") as f:
        file_content=json.load(f)
        device_list=file_content["Device"]
    except IOError:
        print ("[Device.json] doesn't exist. Create file.",  flush=True)
        file_content=dict()
        file_content["Device"]=list()
        device_list=file_content["Device"]

    item_found=False
    item_obj=None
    for item in device_list:
        #print("id=%s, type=%s" %( item["device_id"], type(item["device_id"]) ),  flush=True)
        if dev_id==str(item["device_id"]):
          item_found=True
          item_obj=item
          break

    if item_found:
      #print("[device_update_status] event_obj=%s" %(str(event_obj)),  flush=True)
      event_obj.pop("trigger_type", None)
      for field, value in event_obj.items():
        #force value type of all is string 
        item_obj[field]=str(value)

            
      # get previous connection_status
      prev_connection_status="0"
      if "connection_status" in item_obj:
        prev_connection_status=item_obj["connection_status"]
            
      item_obj["time"]=str(datetime.datetime.now())
      item_obj["connection_status"]="0"
      
      app.views.save_file_content("Device", file_content)

      # handle extend device(s)
      update_extend_device(event_obj)
      # note: no WLxxx config for extend device(d07)
    
      if prev_connection_status=="1" and item_obj["type_id"] != "d05" and item_obj["type_id"] != "d06":
        # trigger "ResumeConnectToHost" event to server 
        trigger_status=""
        alarm_type="ResumeConnectToHostAlarm"
        result=trigger_silent(type_id,dev_id,trigger_status,alarm_type)
    
      # check wireless signal quality
      # if it is lower than 25% of the setting value, trigger alarm: "WLSignalAlarm"
      try:
        type_id=item_obj["type_id"]
        alarm_type="WLSignalAlarm"
        setting_value, result = setting_value_assign_check(type_id, dev_id, alarm_type)
        setting_value=setting_value[:-1] # remove the '%' at the end
        signal_power=item_obj["signal_power_status"]
        print("[device_update_status] device(%s) signal_power_status: %d, %d" %(dev_id, int(signal_power), int(setting_value)) ,  flush=True)
        # TODO: add condition: if manual-setting..
        if int(signal_power) < int(setting_value)*0.75:
          trigger_status=signal_power + "%"
          trigger_silent(type_id, dev_id, trigger_status, alarm_type)

      except:
        #print("[device_update_status] device(" + dev_id + ") WLSignalAlarm TEST",  flush=True)
        pass
        
      # for TemperatureAlarm, PowerAlarm, TerminatingImpedanceAlarm: trigger alarm just once
      #   if the value is back to normal, then clear the flag
      global trigger_alarm_flag
      try:
        if int(item_obj["temperature_status"]) < int(trigger_alarm_flag[dev_id]["TemperatureAlarm"]):
          trigger_alarm_flag[dev_id].pop("TemperatureAlarm", None)
          set_sensor_abnormal_status(dev_id, 1, 0)
      except:
        #print("[device_update_status] device(" + dev_id + ") TemperatureAlarm TEST",  flush=True)
        pass
        
      try:
        if int(item_obj["power_status"]) > int(trigger_alarm_flag[dev_id]["PowerAlarm"]):
          trigger_alarm_flag[dev_id].pop("PowerAlarm", None)
          set_sensor_abnormal_status(dev_id, 2, 0)
      except:
        #print("[device_update_status] device(" + dev_id + ") PowerAlarm TEST",  flush=True)
        pass
      
      # TODO: how to clear TerminatingImpedanceAlarm flag ?
        
      #print("[device_update_status] trigger_alarm_flag=%s" %(str(trigger_alarm_flag)),  flush=True)
      
    else:
      print("[device_update_status] device(" + dev_id + ") is NOT found. skip it.",  flush=True)
      
    return True

################################################################################

def check_device_id_from_loop_id(loop_id):
    #search the loopdevice_list to find the device_id
    device_id="" 
    try:
      with open(app.views.get_json_file_dir() + "LoopDevice.json", "r") as f:
        file_content=json.load(f)
        loopdevice_list=file_content["LoopDevice"]
    except IOError:
      print ("[LoopDevice.json] doesn't exist. Return error",  flush=True)
      return device_id

    item_found=False
    for item in loopdevice_list:
      if loop_id==str(item["loop_id"]):
        device_id=item["device_id"]
        item_found=True

    if not item_found:
      print("loop(" + loop_id + ") is NOT linked to any device!",  flush=True)

    return device_id

def check_loop_status_from_device_id(device_id):
    loop_id=check_loop_id_from_device_id(device_id)
    
    try:
      with open(app.views.get_json_file_dir() + "Loop.json", "r") as f:
        file_content=json.load(f)
        loop_list=file_content["Loop"]
    except:
      print("[check_loop_status_from_device_id][Loop.json] unknown error => assume default status",  flush=True)
      return loop_id,"0"

    item_found=False
    loop_status=""
    for item in loop_list:
      if loop_id==str(item["loop_id"]):
        loop_status=str(item["status"])
        item_found=True

    if not item_found:
      print("[check_loop_status_from_device_id] loop status is missing => assume default status",  flush=True)
      loop_status="0"

    return loop_id,loop_status
    
    
def check_loop_id_from_device_id(device_id):
    #search the loopdevice_list to find the matched loop by device_id 
    try:
      with open(app.views.get_json_file_dir() + "LoopDevice.json", "r") as f:
        file_content=json.load(f)
        loopdevice_list=file_content["LoopDevice"]
    except IOError:
      print ("[LoopDevice.json] doesn't exist. Return error",  flush=True)
      return ""

    item_found=False
    for item in loopdevice_list:
        if device_id==str(item["device_id"]):
          loop_id=item["loop_id"]
          item_found=True

    if not item_found:
          print("device(" + device_id + ") is NOT on any loop !",  flush=True)
          loop_id=""

    return str(loop_id)

################################################################################
#Update device to Device.json

def update_device_to_Device_json(device_id,type_id):
    #search the Device.json to find the matched device by device_id 
    try:
      with open(app.views.get_json_file_dir() + "Device.json", "r") as f:
        file_content=json.load(f)
        device_list=file_content["Device"]
    except IOError:
        print ("[Device.json] doesn't exist. Create file.",  flush=True)
        file_content=dict()
        file_content["Device"]=list()
        device_list=file_content["Device"]

    item_found=False
    for item in device_list:
        if device_id==str(item["device_id"]):
          item_found=True
          break
    print("update device to Device.json: item_found= " + str(item_found),  flush=True)

    if item_found==False:
      print("device(" + device_id + ") is new, add to Device.json !",  flush=True)
      new_device_item=dict()
      new_device_item["device_id"]=str(device_id)
      new_device_item["type_id"]=str(type_id)
      new_device_item["time"]=str(datetime.datetime.now())
      #new_device_item["name"]=""
      #new_device_item["identity"]=""
      
      if str(type_id) == "d06": #IPCam
        new_device_item["connection_way"]="4"
        
      file_content["Device"].append(new_device_item)
      app.views.save_file_content("Device", file_content)

    return True

################################################################################
#Alert type list for each device type


Alert_type={
    "d01":[ #靽銝餅?
        {
            "alert_catalog":"A",
            "abnormal_catalog":"GA03",
            "alarm_type":"GSensorAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"GA07",
            "alarm_type":"TemperatureAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"GA08",
            "alarm_type":"PowerAlarm"
        },
        
        {
            "alert_catalog":"A",
            "abnormal_catalog":"FAKE",
            "alarm_type":"TemperatureWarning"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"FAKE",
            "alarm_type":"PowerWarning"
        } 
    ],
    "d02":[ #?∠?蝤飢?葫??
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA02",
            "alarm_type":"ReedAlarm"     #trigger_type=="8"
        },
    
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA03",
            "alarm_type":"GSensorAlarm"     #trigger_type=="7"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA04",
            "alarm_type":"TemperatureAlarm" #trigger_type=="4"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA05",
            "alarm_type":"PowerAlarm"       #trigger_type=="6"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA06",
            "alarm_type":"WLSignalAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA07",
            "alarm_type":"TerminatingImpedanceAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA08",
            "alarm_type":"ResetAlarm"       #trigger_type=="1"
        },
        {
            "alert_catalog":"G",
            "abnormal_catalog":"DG01",
            "alarm_type":"SensorRecoveryEvent"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA01",
            "alarm_type":"LostConnectToHostAlarm"
        },
        {
            "alert_catalog":"G",
            "abnormal_catalog":"DG02",
            "alarm_type":"ResumeConnectToHostAlarm"
        },
        
        #A-d02-DA09-無線磁簧電壓接近異常 Y-N-N
        #A-d02-DA10-無線磁簧溫度接近異常 Y-N-N

        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA09",
            "alarm_type":"PowerWarning"
        },

        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA10",
            "alarm_type":"TemperatureWarning"
        }
         
    ],
    "d03":[ #?∠??撘?皜砍
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA02",
            "alarm_type":"MicrowaveAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA03",
            "alarm_type":"IrAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA04",
            "alarm_type":"GSensorAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA05",
            "alarm_type":"TemperatureAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA06",
            "alarm_type":"PowerAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA07",
            "alarm_type":"WLSignalAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA08",
            "alarm_type":"TerminatingImpedanceAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA09",
            "alarm_type":"ResetAlarm"
        },
        {
            "alert_catalog":"G",
            "abnormal_catalog":"AG01",
            "alarm_type":"SensorRecoveryEvent"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA01",
            "alarm_type":"LostConnectToHostAlarm"
        },
        {
            "alert_catalog":"G",
            "abnormal_catalog":"AG02",
            "alarm_type":"ResumeConnectToHostAlarm"
        },
        
        #A-d03-AA10-無線雙鍵式感測器電壓接近異常 Y-N-N
        #A-d03-AA11-無線雙鍵式感測器溫度接近異常 Y-N-N
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA10",
            "alarm_type":"PowerWarning"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"AA11",
            "alarm_type":"TemperatureWarning"
        } 
    ],
    "d04":[ #?∠???撘?閮
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA01",
            "alarm_type":"GSensorAlarm"     #trigger_type=="7"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA02",
            "alarm_type":"TemperatureAlarm" #trigger_type=="4"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA03",
            "alarm_type":"PowerAlarm"       #trigger_type=="6"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA04",
            "alarm_type":"WLSignalAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA05",
            "alarm_type":"TerminatingImpedanceAlarm"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA06",
            "alarm_type":"ResetAlarm"       #trigger_type=="1"
        },
        {
            "alert_catalog":"G",
            "abnormal_catalog":"CG01",
            "alarm_type":"SensorRecoveryEvent"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA07",
            "alarm_type":"LostConnectToHostAlarm"
        },
        {
            "alert_catalog":"G",
            "abnormal_catalog":"CG02",
            "alarm_type":"ResumeConnectToHostAlarm"
        },
        
        #A-d04-CA09-無線讀訊器電壓接近異常 Y-N-N
        #A-d04-CA10-無線讀訊器溫度接近異常 Y-N-N
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA09",
            "alarm_type":"PowerWarning"
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"CA10",
            "alarm_type":"TemperatureWarning"
        }
    ],
    
    # A/d07/DA11:擴充裝置1異常
    # A/d07/DA12:擴充裝置2異常
    # G/d07/DG03:擴充裝置1復置
    # G/d07/DG04:擴充裝置2復置
    "d07":[ # extend device
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA11",
            "alarm_type":"ExtendDevice1Alarm"     
        },
        {
            "alert_catalog":"A",
            "abnormal_catalog":"DA12",
            "alarm_type":"ExtendDevice2Alarm"     
        }
        ,
        {
            "alert_catalog":"G",
            "abnormal_catalog":"DG03",
            "alarm_type":"ExtendDevice1Reset"     
        },  
        {
            "alert_catalog":"G",
            "abnormal_catalog":"DG04",
            "alarm_type":"ExtendDevice2Reset"     
        }
        
    ]
}

################################################################################

Device_type={
    "d01":"HostSetting",    #靽銝餅?
    "d02":"WLReedSensor",   #?∠?蝤飢?葫??
    "d03":"WLDoubleBondBTemperatureSensor",   #?∠??撘?皜砍
    "d04":"WLReadSensor",   #?∠???撘?閮
    "d07":"NA",   
}

################################################################################
#Check if setting value is need 
def setting_value_assign_check(type_id,device_id,alarm_type):

    JSON_FILE_NAME = Device_type[type_id]
    #print("JSON_FILE_NAME= " + str(JSON_FILE_NAME),  flush=True)
    setting_value_assign=""
    check_result=False
    
    #Open device json file
    try:
      with open(app.views.get_json_file_dir() + JSON_FILE_NAME + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[JSON_FILE_NAME]
      for item in device_list:
        #print("item=%s\n" %str(item), flush=True)
        if item["device_id"] == device_id:
          if alarm_type=="TemperatureAlarm":
            setting_value_assign = item["temperature_sensing"]
          elif alarm_type=="PowerAlarm":
            setting_value_assign = item["battery_low_power_set"]
          elif alarm_type=="WLSignalAlarm":
            setting_value_assign = item["msg_send_strong_set_manual"]
          elif alarm_type=="TerminatingImpedanceAlarm":
            setting_value_assign = item["terminating_impedance"]
          else:
            print("alarm_type not found, setting value assigned NULL !",  flush=True)

          check_result=True
          break
          
    except IOError:
        print ("[setting_value_assign_check] JSON file doesn't exist.",  flush=True)
        #temp, zl
        check_result=True
        #check_result=False
        #return setting_value_assign,check_result
        #ending, zl
    except KeyError as e:
        print ("[setting_value_assign_check] KeyError",  flush=True)
        print (e,  flush=True)
        #temp, zl
        check_result=True
    #except:
    #    print ("[setting_value_assign_check] unknown error",  flush=True)
    #    #temp, zl
    #     check_result=True
    
    #print("[setting_value_assign_check] [%s,%s,%s] setting_value_assign=%s\n" %(type_id, device_id, alarm_type, setting_value_assign), flush=True)
        
    return setting_value_assign,check_result

################################################################################
#Assign alert type based on type_id & alarm_type
def alert_type_assign(type_id,device_id,alarm_type):
    trigger_result=True
    item_found=False
    alert_catalog_assign = ""
    abnormal_catalog_assign = ""
    
    print("alarm_type: " + str(alarm_type),  flush=True)
    print("type_id: " + str(type_id),  flush=True)
    print("device_id: " + str(device_id),  flush=True)
    setting_value_assign,check_result = setting_value_assign_check(type_id,device_id,alarm_type)
    print("check_result= " + str(check_result),  flush=True)
    if check_result==False:
        item_found=False
    else:
        alert_list=Alert_type[type_id]
        #print("alarm_type=%s" %(alarm_type), flush=True)
        for item in alert_list:
          #print("item[alarm_type]=%s" %(item["alarm_type"]), flush=True)  
          if alarm_type == item["alarm_type"]:
            alert_catalog_assign = item["alert_catalog"]
            abnormal_catalog_assign = item["abnormal_catalog"]
            item_found=True
            break
    
    if not item_found:
          print("alert_type not found !",  flush=True)
          trigger_result=False

    return alert_catalog_assign,abnormal_catalog_assign,setting_value_assign,trigger_result

################################################################################
# alarm processing:
def trigger_alarm(type_id,device_id,trigger_status,alarm_type):
    print("[trigger_alarm][%s] device(%s)(%s) is triggered !!!"  %(alarm_type, type_id, device_id),  flush=True)
    
    # it seems only reed have the resetting function, zl 
    #global sensor_resetting_status 
    #if device_id in sensor_resetting_status and sensor_resetting_status[device_id]=="0":
    #  print("device(%s) is already triggered. Ignore this event!"  %(device_id),  flush=True)
    #  return False
    # sensor_resetting_status[device_id]="0"
    
    # for tempertature-alarm, power-alarm, TerminatingImpedanceAlarm: trigger alarm just once
    #   if the value is alarmed, set the flag and ignore the following alarms
    global trigger_alarm_flag
    if alarm_type == "TemperatureAlarm" or alarm_type == "PowerAlarm" or alarm_type == "TerminatingImpedanceAlarm":
      print("[trigger_alarm] trigger_alarm_flag=%s" %(str(trigger_alarm_flag)),  flush=True)
      if device_id in trigger_alarm_flag and alarm_type in trigger_alarm_flag[device_id]:
        print("[trigger_alarm] ==> it is already triggered. Ignore this event!\n",  flush=True)
        return      
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
      
    #if not app.host_lib.check_host_alarm_set(keepalive_obj):
    if not app.device_lib.get_sensor_security_set(device_id):
      print("[trigger_alarm] sensor_security is unset:",  flush=True)
      # TODO: for some alarm types(TerminatingImpedanceAlarm ??), events should not be ignored...
      if alarm_type != "GSensorAlarm" and alarm_type != "TemperatureAlarm":
        print("[trigger_alarm][%s] is ignored !\n\n" %(alarm_type),  flush=True)
        return False
      else:
        print("[trigger_alarm][%s] CANNOT be ignored !" %(alarm_type),  flush=True)
        pass
        
    loop_id=str(check_loop_id_from_device_id(device_id))
    if loop_id=="":
      print("device(%s) is NOT on loop. Alarm is NOT issued to server" %(device_id), flush=True)
      return False

    if True:
      print("[trigger_alarm] make sound !!!\n\n")
      play_time=180
      try:
        hostsetting_obj=app.host_lib.open_hostsetting_json_file()
        play_time=int(hostsetting_obj["alarm_action_time_set"])
      except:
        pass
      audio_obj=app.audio.Audio() 
      audio_obj.play(app.host_lib.get_service_dir() + "event_happen.mp3", play_time)
      
      app.device_lib.ask_readers_make_sound(3, play_time)
      
      # set alarm voice flag
      # disaster = "[fire][gas][sos]"
      if alarm_type == "TemperatureAlarm":
        disaster_index=4
      else:
        disaster_index=1
      app.host_lib.set_alarm_voice_flag(loop_id, disaster_index)

      # display alarm info on LCM
      app.host_lib.lcm_display_alarm_info()
      
      print("[trigger_alarm] trigger LED !",  flush=True)
      app.host_lib.set_led(LED6_ALM_R, 0.5)
        
      print("[trigger_alarm] trigger optical-coupling operation !",  flush=True)
      app.host_lib.trigger_coupling_operation(device_id)
      
      print("[trigger_alarm] launch alarm-output action !",  flush=True)
      app.host_lib.launch_alarm_output_action()
      print("[trigger_alarm] launch 12v-output action !",  flush=True)
      app.host_lib.launch_12v_output_action()

      # check extend_out function, then take action ...
      output_function=get_extend_out_function(type_id, device_id)
      if len(output_function)==3:
        if output_function[0]=="1":
          print("[trigger_alarm][extend_out_function] sensor action ?",  flush=True)
          # TODO: send command to this device => need api definition
          pass  
        if output_function[1]=="1":
          print("[trigger_alarm][extend_out_function] customer notification ...",  flush=True)
          # TODO: customer notification ??? 
          pass
        if output_function[2]=="1":
          print("[trigger_alarm][extend_out_function] alarm ?",  flush=True)
          # TODO: send command to this device => need api definition
          pass

      else:
        print("[trigger_alarm] no extend out action !",  flush=True)
        pass
                                  
    #########################
    alert_catalog_assign,abnormal_catalog_assign,setting_value_assign,trigger_result=alert_type_assign(type_id,device_id,alarm_type)
    if trigger_result==False:
      print("[trigger_alarm] trigger_result: False",  flush=True)    
      return trigger_result
    
    if alarm_type == "TemperatureAlarm" or alarm_type == "PowerAlarm" or alarm_type == "TerminatingImpedanceAlarm":
      if not device_id in trigger_alarm_flag:
        # set new dict() only if device_id is absent in trigger_alarm_flag: avoid to clear the previous alarm of the other type 
        trigger_alarm_flag[device_id]=dict()
      
      if setting_value_assign=="":
        if alarm_type == "TemperatureAlarm":
          print("[trigger_alarm] [TemperatureAlarm] setting_value_assign='' ??",  flush=True)
          trigger_alarm_flag[device_id][alarm_type]="100"
        elif alarm_type == "PowerAlarm":
          print("[trigger_alarm] [PowerAlarm] setting_value_assign='' ??",  flush=True)
          trigger_alarm_flag[device_id][alarm_type]="0"
        elif alarm_type == "TerminatingImpedanceAlarm":
          print("[trigger_alarm] [TerminatingImpedanceAlarm] setting_value_assign='' ??",  flush=True)
          trigger_alarm_flag[device_id][alarm_type]="0"
      else:
        trigger_alarm_flag[device_id][alarm_type]=setting_value_assign
      
      if alarm_type == "TemperatureAlarm":  
        set_sensor_abnormal_status(device_id, 1, 1)
      elif alarm_type == "PowerAlarm":
        set_sensor_abnormal_status(device_id, 2, 1)
        
    alarm_obj=dict()
    alarm_obj["type_id"]=str(type_id)
    alarm_obj["abnormal_value"]=str(trigger_status)
    alarm_obj["alert_catalog"]=alert_catalog_assign
    alarm_obj["abnormal_catalog"]=abnormal_catalog_assign
    alarm_obj["setting_value"]=str(setting_value_assign)
    #print("alarm_obj:" + str(alarm_obj),  flush=True)
    alarm_obj["device_id"]=str(device_id)
    alarm_obj["loop_id"]=loop_id
    
    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
        trigger_result=True
    else:  
        trigger_result=False
    
    # notify IPCam to upload video
    app.ipcam_lib.ask_ipcam_to_upload_video(app.host_lib.get_event_id())
    
    return trigger_result

################################################################################
# warning processing:
def trigger_warning(type_id,device_id,trigger_status,warning_type):
    print("[trigger_warning][%s] device(%s)(%s) is triggered !!!"  %(warning_type, type_id, device_id),  flush=True)
    
    Device_name=Device_type[type_id]
    print("[trigger_warning] Device_name=%s" %(Device_name), flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
    
    #if not app.host_lib.check_host_alarm_set(keepalive_obj):
    if not app.device_lib.get_sensor_security_set(device_id):
      print("[trigger_warning] sensor event is ignored by host !\n\n",  flush=True)
      return False

    #########################
    alert_catalog_assign,abnormal_catalog_assign,setting_value_assign,trigger_result=alert_type_assign(type_id,device_id,warning_type)
    if trigger_result==False:
      return trigger_result

    alarm_obj=dict()
    alarm_obj["type_id"]=str(type_id)
    alarm_obj["abnormal_value"]=str(trigger_status)
    alarm_obj["alert_catalog"]=alert_catalog_assign
    alarm_obj["abnormal_catalog"]=abnormal_catalog_assign
    alarm_obj["setting_value"]=str(setting_value_assign)
    #print("alarm_obj:" + str(alarm_obj),  flush=True)

    alarm_obj["device_id"]=str(device_id)
    alarm_obj["loop_id"]=check_loop_id_from_device_id(device_id)
    #print("loop_id: " + str(alarm_obj["loop_id"]),  flush=True)
    if alarm_obj["loop_id"]=="":
      print("device(%s) is NOT on loop. Warning is NOT issued to server" %(device_id), flush=True)
      trigger_result=False
      return trigger_result

    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
        trigger_result=True
    else:  
        trigger_result=False
    
    
    return trigger_result

################################################################################
# silent event processing:
# no voice, LCM message, LED signal, IPCam recording
# not check security state
def trigger_silent(type_id,device_id,trigger_status,alarm_type):
    print("[trigger_silent][%s] device(%s)(%s) is triggered !!!"  %(alarm_type, type_id, device_id),  flush=True)
    
    # for tempertature-alarm, power-alarm, TerminatingImpedanceAlarm: trigger alarm just once
    #   if the value is alarmed, set the flag and ignore the following alarms
    global trigger_alarm_flag
    if alarm_type == "TemperatureAlarm" or alarm_type == "PowerAlarm" or alarm_type == "TerminatingImpedanceAlarm":
      print("[trigger_silent] trigger_alarm_flag=%s" %(str(trigger_alarm_flag)),  flush=True)
      if device_id in trigger_alarm_flag and alarm_type in trigger_alarm_flag[device_id]:
        print("[trigger_silent] ==> it is already triggered. Ignore this event!\n",  flush=True)
        return      
    '''
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
    '''
      
    loop_id=str(check_loop_id_from_device_id(device_id))
    if loop_id=="":
      print("device(%s) is NOT on loop. Alarm is NOT issued to server" %(device_id), flush=True)
      return False
  
    #########################
    alert_catalog_assign,abnormal_catalog_assign,setting_value_assign,trigger_result=alert_type_assign(type_id,device_id,alarm_type)
    if trigger_result==False:
      print("[trigger_silent][alert_type_assign] result: False",  flush=True)    
      return trigger_result
    
    if alarm_type == "TemperatureAlarm" or alarm_type == "PowerAlarm" or alarm_type == "TerminatingImpedanceAlarm":
      if not device_id in trigger_alarm_flag:
        # set new dict() only if device_id is absent in trigger_alarm_flag: avoid to clear the previous alarm of the other type 
        trigger_alarm_flag[device_id]=dict()
      
      if setting_value_assign=="":
        if alarm_type == "TemperatureAlarm":
          print("[trigger_silent] [TemperatureAlarm] setting_value_assign='' ??",  flush=True)
          trigger_alarm_flag[device_id][alarm_type]="100"
        elif alarm_type == "PowerAlarm":
          print("[trigger_silent] [PowerAlarm] setting_value_assign='' ??",  flush=True)
          trigger_alarm_flag[device_id][alarm_type]="0"
        elif alarm_type == "TerminatingImpedanceAlarm":
          print("[trigger_silent] [TerminatingImpedanceAlarm] setting_value_assign='' ??",  flush=True)
          trigger_alarm_flag[device_id][alarm_type]="0"
      else:
        trigger_alarm_flag[device_id][alarm_type]=setting_value_assign
      
      if alarm_type == "TemperatureAlarm":  
        set_sensor_abnormal_status(device_id, 1, 1)
      elif alarm_type == "PowerAlarm":
        set_sensor_abnormal_status(device_id, 2, 1)
      
        
    alarm_obj=dict()
    alarm_obj["type_id"]=str(type_id)
    alarm_obj["abnormal_value"]=str(trigger_status)
    alarm_obj["alert_catalog"]=alert_catalog_assign
    alarm_obj["abnormal_catalog"]=abnormal_catalog_assign
    alarm_obj["setting_value"]=str(setting_value_assign)
    alarm_obj["device_id"]=str(device_id)
    alarm_obj["loop_id"]=loop_id
    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
        trigger_result=True
    else:  
        trigger_result=False
    
    return trigger_result

################################################################################
def save_device_config_file(type_id, device_id):
    #print ("[save_device_config_file] ",  flush=True)
    if str(type_id) =="d02":
      table_name="WLReedSensor"
    elif str(type_id) =="d03":
      table_name="WLDoubleBondBTemperatureSensor"
    elif str(type_id) =="d04":
      table_name="WLReadSensor"
    elif str(type_id) =="d05":
      table_name="WLRemoteControl"
    else:
      print ("[save_device_config_file] type_id(%s) is NOT supported. Return" %(str(type_id)),  flush=True)
      return False 

    #print ("[save_device_config_file] table_name=%s" %(table_name),  flush=True)

    file_content=dict()
    file_content[table_name]=list()
    device_config_list=file_content[table_name]
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_config_list=file_content[table_name]
    except IOError:
      print ("[save_device_config_file][%s.json] doesn't exist." %(table_name),  flush=True)
      pass      

    # iterate the list
    item_found=False 
    for record in device_config_list:
      if device_id == record["device_id"]:
        item_found=True
        break
        
    if not item_found:
      new_record=dict()
      new_record["device_id"]=device_id
      # add some default settings...
      new_record["G_sensor_status "]="1"
      new_record["G_sensitivity"]="30%"
      new_record["temperature_sensing"]="50"
      new_record["temperature_sensing_gap"]="5"
      new_record["battery_low_power_set"]="20"
      new_record["battery_low_power_gap"]="5"
      new_record["msg_send_strong_set"]="0"
      new_record["msg_send_strong_set_manual"]="30%"
      new_record["msg_send_strong_set_manual_gap"]="5"
      new_record["terminating_impedance"]="10%"
      new_record["transmission_distance"]="0"
      new_record["transmission_distance_set_manual"]="1200"
      device_config_list.append(new_record)
      # save to json file
      with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
        json.dump(file_content, f)
    else:
      #print ("[save_device_config_file] device(%s) is already in %s.json" %(device_id, table_name),  flush=True)
      pass  
        
    return True
   
################################################################################
# add or update extend device to Device.json
def update_extend_device(event_obj):
    dev_id=str(event_obj["device_id"])
    print("[update_extend_device] main device_id=%s" %(dev_id),  flush=True)
    if not "extend_flag" in event_obj:
      print("[update_extend_device] extend_flag is absent => skip this operation",  flush=True)
      return
    
    extend_flag=str(event_obj["extend_flag"])
    
    # update main device's extend_flag
    set_device_value(dev_id, "extend_flag", extend_flag)
       
    # handle extend device(s)        
    table_name="Device"
    file_content=dict()
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
    except IOError:
      print ("[" + table_name + ".json] doesn't exist. create empty result!",  flush=True)
      file_content[table_name]=list()

    device_list=file_content[table_name]
    add_extend_dev1=True
    add_extend_dev2=True
    for item in device_list:
      # if extend_device exists, update its "connection_status" 
      if item["device_id"] == dev_id + "-1":
        item["connection_status"]="0"
        add_extend_dev1=False
      elif item["device_id"] == dev_id + "-2":
        item["connection_status"]="0"
        add_extend_dev2=False         
    
    try:
      if add_extend_dev1 and (extend_flag=="1" or extend_flag=="3"):
          extend1_dev=dict()
          extend1_dev["time"]=str(datetime.datetime.now())
          extend1_dev["type_id"]="d07"  
          extend1_dev["device_id"]=dev_id + "-1" 
          extend1_dev["connection_status"]="0"
          device_list.append(extend1_dev)
      if add_extend_dev2 and (extend_flag=="2" or extend_flag=="3"):
          extend2_dev=dict()
          extend2_dev["time"]=str(datetime.datetime.now())
          extend2_dev["type_id"]="d07"  
          extend2_dev["device_id"]=dev_id + "-2" 
          extend2_dev["connection_status"]="0"
          device_list.append(extend2_dev)  
    except:
      print("[update_extend_device][%s] extend device handling TEST" %(dev_id),  flush=True)
      pass

    # save Device.json
    with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
      json.dump(file_content, f)
      
    # register extend device(s) to IOT service
    try:
      if add_extend_dev1 and (extend_flag=="1" or extend_flag=="3"):
          app.server_lib.register_device_to_server(dev_id + "-1")
      
      if add_extend_dev2 and (extend_flag=="2" or extend_flag=="3"):
          app.server_lib.register_device_to_server(dev_id + "-2")  
    except:
      print("[update_extend_device][%s] extend device registering TEST" %(dev_id),  flush=True)
      pass

################################################################################
# set value to Device.json
def set_device_value(dev_id, field, value):
    print ("[set_device_value][%s] set %s=%s" %(dev_id, field, value),  flush=True)
    table_name="Device"
    file_content=dict()
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
    except:
      print ("[set_device_value] unknown error => skip this operation",  flush=True)
      return 
      
    device_list=file_content[table_name]
    for item in device_list:
      if item["device_id"] == dev_id:
        item[field]=value
        break         

    # save Device.json
    with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
      json.dump(file_content, f)

    
################################################################################
# get value from Device.json
def get_device_value(dev_id, field):
    print ("[get_device_value][%s][%s]" %(dev_id, field),  flush=True)
    table_name="Device"
    file_content=dict()
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
    except:
      print ("[get_device_value] unknown error => return ''",  flush=True)
      return "" 
    
    value=""  
    try:
      device_list=file_content[table_name]
      for item in device_list:
        if item["device_id"] == dev_id:
          if field in item:
            value=item[field]
          else:
            value=""  
          break
    except:
      pass
      
                   
    return value            
    
################################################################################
# 
def get_sensor_security_set(device_id):
    print("[get_sensor_security_status][%s][%s] " %(str(datetime.datetime.now()), device_id), flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except:
      print ("[get_sensor_security_set][KeepAlive.json] unknown error => assume unset",  flush=True)
      return False
         
    loop_id,loop_status=app.device_lib.check_loop_status_from_device_id(device_id)
    
    if loop_id == "":
      print ("[get_sensor_security_set] sensor is NOT on any loop =>  according to host security state",  flush=True)
      return app.host_lib.check_host_alarm_set(keepalive_obj)
      
    if loop_status != "0":  
      # 1. 感測器的迴路類別=1,2,3,4 => 感測器為設定狀態
      print("[get_sensor_security_set][%s,%s] loop_status=%s => security set" %(loop_id, device_id, loop_status), flush=True)
      return True
    else:
      # 2. 感測器在一般迴路(0)
      # check if the loop is associated with any partitions
      try:
        with open(app.views.get_json_file_dir() + "Partition.json", "r") as f:
          partition_file_content=json.load(f)
          total_partition_list=partition_file_content["Partition"]
      except:
        print ("[get_sensor_security_set][Partition.json] unknown error => according to host security state",  flush=True)
        return app.host_lib.check_host_alarm_set(keepalive_obj)
      '''
      {"Partition":[
  		    {"time:"", "partition_id":"1","loop_id":["1", "2", ...], "status":"0"},
          {"time:"", "partition_id":"2","loop_id":["1", "3", ...], "status":"1"},
          ...
      ]}
      '''
      loop_found_in_partition=False
      sensor_status=False
      for part_item in total_partition_list:
        if loop_id in part_item["loop_id"]:
          loop_found_in_partition=True
          if "status" in part_item and part_item["status"]=="1":
            sensor_status = sensor_status or True 
          else:                   
            sensor_status = sensor_status or False
      
      if loop_found_in_partition:            
        # 2.1 在分區內，其保全狀態:當感測器所在的所有分區均為解除狀態，感測器為解除狀態
        print("[get_sensor_security_set][%s,%s] sensor securtiy=%s" %(loop_id, device_id, str(sensor_status)), flush=True)
        return sensor_status
      else:
        # 2.2 不在任何分區，感測器狀態等於主機設解狀態
        print("[get_sensor_security_set][%s,%s] NOT in any partition => according to host security state" %(loop_id, device_id), flush=True)
        return app.host_lib.check_host_alarm_set(keepalive_obj)
          
    print("[get_sensor_security_set][%s] unknown situation => assume unset" %(device_id), flush=True)
    return False

################################################################################
#
def get_extend_in_function(table_name, dev_id, extend_index):
    print ("[get_extend_in_function][%s][%s]" %(dev_id, extend_index),  flush=True)
    result="1"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
        for item in device_list:
          if item["device_id"]==dev_id:
            if extend_index=="1":
              result=item["extend_in1_function"]
            elif extend_index=="2":
              result=item["extend_in2_function"]
                            
            break
    except:
      print("[get_extend_in_function][%s] unknown error => assume function enabled " %(table_name),  flush=True)
      pass
            
    return result    
    
################################################################################
#
def get_extend_out_function(type_id, dev_id):
    print ("[get_extend_out_function][%s, %s]" %(type_id, dev_id),  flush=True)
    
    if type_id=="d07":
      print("[get_extend_out_function] skip extend_in device !",  flush=True)
      return ""
      
    # check Device.json: extend_out
    table_name="Device"
    extend_out_status="0"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
        for item in device_list:
          if item["device_id"]==dev_id:
            extend_out_status=item["extend_out"]
            break
    except:
      print("[get_extend_out_function][%s] unknown error => assume extend_out does NOT exist" %(table_name),  flush=True)
      return ""
    
    if extend_out_status == "0":
      print("[get_extend_out_function][%s] extend_out does NOT exist" %(table_name),  flush=True)
      return ""
      
    # check WLxxx.json: extend_out_function
    table_name=Device_type[type_id]
    result="100"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
        for item in device_list:
          if item["device_id"]==dev_id:
            result=item["extend_out_function"]
            break
    except:
      print("[get_extend_out_function][%s] unknown error => assume alarm enabled " %(table_name),  flush=True)
      pass
    
    print("[get_extend_out_function][%s] result=%s" %(table_name, result),  flush=True)
    return result
    
################################################################################
# get communication time period according to security_state
def get_communication_time_by_security_state(state):
    print("[get_communication_time_by_security_state][%s] state=%s" %(str(datetime.datetime.now()), state), flush=True)
    period=30
    if state=="0":
      period=60  
    
    table_name="DeviceSetting"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        if state=="0":
          period=int(file_content[table_name][0]["sendMsg_time_when_disabled"])
        else:
          period=int(file_content[table_name][0]["sendMsg_time_when_enabled"])
    except:
      print("[get_communication_time_by_security_state][%s] unknown error => use default setting" %(table_name),  flush=True)
      pass
    
    print("[get_communication_time_by_security_state] period=%d" %(period),  flush=True)
    return period
    
################################################################################
# set communication time period to each sensor 
def set_communication_time_by_security_state(state):
    print("[set_communication_time_by_security_state][%s] state=%s" %(str(datetime.datetime.now()), state), flush=True)
    
    period=get_communication_time_by_security_state(state) 
    print("[set_communication_time_by_security_state] period=%d" %(period),  flush=True)
    
    table_name="Device"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
    except:
      print("[set_communication_time_by_security_state][%s] unknown error => skip this operation" %(table_name),  flush=True)
      return
    
    for dev_item in device_list:
      type_id=str(dev_item["type_id"])
      dev_id=str(dev_item["device_id"])
      if type_id=="d02" or type_id=="d03" or type_id=="d04":
        # 設定回報時間
        # socket.emit('setInterval', {device_id: extAddr, reporting: 10, polling: 2});
        data_obj=dict() 
        data_obj["device_id"]=dev_id
        data_obj["reporting"]=period
        data_obj["polling"]=2
        device_command_api2('setInterval', data_obj)

################################################################################
# ask readers to make sound 
def ask_readers_make_sound(mode, play_time):
    print("[ask_readers_make_sound][%s] mode=%d, time=%d" %(str(datetime.datetime.now()), mode, play_time), flush=True)
    # socket.emit('setMusicConfig', {device_id: extAddr, mode:2, time:3});
    # time: second
    # mode
    # invalid.. 0:勤務報到成功 1:勤務抵達 2:異常發生 3:系統解除 4:系統設定 5:裝置被移除   6:緊急事件     7:分區已解除 8:分區已設定
    # 20170306: 0:勤務報道成功 1:勤務抵達 2.溫度異常 3.異常發生 4.系統解除 5.系統設定失敗 6.系統設定成功 7.裝置被移除 
    table_name="Device"
    try:
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
    except:
      print ("[ask_readers_make_sound][%s.json] unknown error => skip this operation" %(table_name),  flush=True)
      return
      
    data_obj=dict()
    for item in device_list:
      if str(item["type_id"]) == "d04":
        data_obj["device_id"]=str(item["device_id"])
        data_obj["mode"]=mode
        data_obj["time"]=play_time
        device_command_api2("setMusicConfig", data_obj)


################################################################################
# device abnormal status: 
# [resetting][temperature][power][extend_in1][extend_in2]
# e.g. "00000"
# (0=normal)
def set_sensor_abnormal_status(device_id, index, value):
    value=str(value)
    print("[set_sensor_abnormal_status] device=%s, index=%d, value=%s" %(device_id, index, value),  flush=True)
    
    table_name="Device"
    try:
      resetting_status="0"
      with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
        file_content=json.load(f)
        device_list=file_content[table_name]
      
      item_found=False
      for dev_item in device_list:
        if dev_item["device_id"] == device_id:
          try:
            abnormal_status=dev_item["abnormal_status"]
          except:
            abnormal_status="00000"
            
          dev_item["abnormal_status"]=abnormal_status[0:index] + value + abnormal_status[index+1:]
          
          print("[set_sensor_abnormal_status] abnormal_status=%s" %(dev_item["abnormal_status"]),  flush=True)
          item_found=True
          break
      
      if item_found:  
        with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
          json.dump(file_content, f)
      else:
        print("[set_sensor_abnormal_status] device is NOT found ???",  flush=True)
        
    except IOError:
      print("[set_sensor_abnormal_status] [" + table_name + ".json] unknown error => skip this operation",  flush=True)
      return   
          