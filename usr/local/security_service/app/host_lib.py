#coding=UTF-8

import threading
import requests
import os
import json
import datetime, time
from uuid import getnode


import app.views 
import app.server_lib
import app.audio
import app.ipcam_lib

try: 
  import app.led_api
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)
except:
  print("[led_api] unknown error.",  flush=True)

try:
  import app.gpio_io_api
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)

try:
  import Adafruit_BBIO
except ImportError as e:
  print("import error:",  flush=True)
  print(e,  flush=True)

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

# for TemperatureAlarm and PowerAlarm: trigger alarm just once
trigger_alarm_flag=dict()

EVENT_ID=""

def get_event_id():
  return EVENT_ID
   
def set_event_id():
  global EVENT_ID
  datetime_obj=datetime.datetime.now()
  EVENT_ID=app.views.get_customer_number() + "_" + datetime_obj.strftime("%Y%m%d%H%M%S%f")
  return EVENT_ID
  

def get_service_dir():
    #return "/public/security_service/"
    app_dir=os.path.dirname(os.path.abspath(__file__))
    service_dir=os.path.abspath(app_dir + "/../") + "/"
    #print("service_dir=%s" %(service_dir), flush=True)
    return service_dir
    
def get_host_mac():
    host_mac = hex(getnode())[2:]
    return host_mac

def get_default_gateway_linux():
    """Read the default gateway directly from /proc."""
    import socket, struct
    with open("/proc/net/route") as fh:

      for line in fh:
        fields = line.strip().split()
        if fields[1] != '00000000' or not int(fields[3], 16) & 2:
          continue
          
        return socket.inet_ntoa(struct.pack("<L", int(fields[2], 16)))

def get_wifi_scan_result():
    gateway_ip=get_default_gateway_linux()
    print("gateway_ip=%s" %(gateway_ip), flush=True)
    try:
        #<1> post to /boafrm/formWlSiteSurvey
        #    refresh="Site Survey"
        url = "http://" + gateway_ip + '/boafrm/formWlSiteSurvey'
        print(url,  flush=True)
        payload = 'refresh="Site Survey"'
        print("payload=" + str(payload),  flush=True)
        r = requests.post(url, data=payload, timeout=10)
        #print("server message:",  flush=True) 
        #print(r.content.decode("utf-8"),  flush=True)
        
        #<2> get 192.168.1.254/sapido_survey.htm
        url = "http://" + gateway_ip + '/sapido_survey.htm'
        print(url,  flush=True)
        r = requests.get(url, timeout=10)
        #print("server message:",  flush=True) 
        #print(r.content.decode("utf-8"),  flush=True)
        '''
        {
             "wifisurvey": [
                {
                        "SSID" : "SAPIDO_IPJC1n_d6d41c",
                        "BSSID" : "00:d0:41:d6:d4:1b",
                        "Channel" : "6 (B+G+N)",
                        "Type" : "AP",
                        "Encrypt" : "no",
                        "Signal" : "42"
                },
                ...
                {}
        }
        '''
        ap_list=r.json()["wifisurvey"]
        file_content=list()
        for ap_info in ap_list:
          #print(ap_info)
          ap_info2=dict()
          # in case the empty dict exists
          if "SSID" in ap_info and "Signal" in ap_info and "Encrypt" in ap_info:
            ap_info2["ssid"]=str(ap_info["SSID"])
            ap_info2["signal"]=int(ap_info["Signal"])
            ap_info2["security"]=str(ap_info["Encrypt"])
            ap_info2["channel"]=str(ap_info["Channel"])
            file_content.append(ap_info2)
            
        #wifi_scan_result.json: [{"ssid":"xx", "signal":0,"security":"", "channel":""},...]
        # write file_content to file
        with open(app.views.get_json_file_dir() + "wifi_scan_result.json", "w") as f:
          json.dump(file_content, f)
    
    except ValueError as e:
        print("error: %s", str(e))
    except requests.exceptions.ConnectionError as e:
        print("error: %s", str(e))    
    except requests.exceptions.RequestException as e:
        print("error: %s", str(e))
    except UnicodeEncodeError as e:
        print("error: %s", str(e))
    except:         
        print("[get_wifi_scan_result] Unknown error")    
    
          
################################################################################

def modem_dial_func():
    import subprocess
    print("[%s][modem_dial_func] starts." %(str(datetime.datetime.now())),  flush=True)
    modem_timestamp = int(time.mktime(datetime.datetime.now().timetuple()))
    with open("/tmp/modem_timestamp", 'w+') as f:
      f.write(str(modem_timestamp))

    # if the modem connection is ready, just use it
    command="ifconfig | grep 'P-t-P' | cut -d ':' -f 3 | awk '{print $1}'"
    proc=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
    ppp_gateway=proc.stdout.readline().decode('ascii').rstrip("\n")
    if ppp_gateway != "":
      print("[%s][modem_dial_func] modem connection is ready => just return" %(str(datetime.datetime.now())),  flush=True)
      return
      
     
    # wvdial:  modem dial program
    # just in case: kill previous 'wvdial' process(es) ...
    app.host_lib.kill_wvdial_process()
      
    # note#1: it seems 'wvdial' only dumps message to stderr ...
    # note#2: 'wvdial' will be resident if the dialup is ok
    # note#3: 'wvdial' will be resident if 'No carrier' happens
    # note#4: 'wvdial' will quit if 'No dial tone' happens
    
    wvdial_proc=subprocess.Popen(["wvdial"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    timer = 0
    dial_result=False
    ppp_gateway=""
    while not wvdial_proc.poll() or timer <= 30: # 30 second timeout
      time.sleep(1)
      timer += 1
      line=wvdial_proc.stderr.readline()
      print("[modem_dial_func][stderr]: %s" %(line), flush=True)
      if 'No dial tone' in str(line):
        dial_result=False            
        #wvdial_proc.wait()    # when finished, get the exit code
        break
    
      # try to get p-t-p by ifconfig
      command="ifconfig | grep 'P-t-P' | cut -d ':' -f 3 | awk '{print $1}'"
      #print("[modem_dial_thread] command=" + command,  flush=True)
      proc2=subprocess.Popen(command, stdout=subprocess.PIPE, shell=True)
      ppp_gateway=proc2.stdout.readline().decode('ascii').rstrip("\n")
      if ppp_gateway != "":
        print("[modem_dial_func] ppp_gateway=" + ppp_gateway,  flush=True)
        dial_result=True            
        break

    print("[modem_dial_func] timer=%d" %(timer), flush=True)
    print("[%s][modem_dial_func] test 1" %(str(datetime.datetime.now())),  flush=True)
        
    # temporary procedure ...  
    if dial_result:
      print("[modem_dial_func] => modem dial is ok." , flush=True)

      # just in case: kill previous dhclient process(es) ...
      app.host_lib.kill_dhclient_process()                           
      
      # del the default gw of eth0
      command="route del -net default" 
      print("[modem_dial_func] command=" + command,  flush=True)    
      os.system(command)
      
      # add the default gw of ppp0  
      command="route add default gw " + ppp_gateway 
      print("[modem_dial_func] command=" + command,  flush=True)    
      os.system(command)
    else:
      print("[modem_dial_func] => dialup failed !" , flush=True)
      os.killpg(os.getpgid(wvdial_proc.pid), signal.SIGTERM)
      
      
        
################################################################################
# save HostSetting's modem config(phone-number/username/password) to "/etc/wvdial.conf" 
def modem_config():
    print("[modem_config] starts.", flush=True)
    try:
      with open(app.views.get_json_file_dir() + "HostSetting.json", "r") as f:
        file_content=json.load(f)
        host_setting_obj=file_content["HostSetting"][0]
        
        phone_number=host_setting_obj["phone_number"]
        phone_account=host_setting_obj["phone_account"]
        phone_password=host_setting_obj["phone_password"]
        
        # create new config file (/etc/wvdial.conf) according to the above stuff
        # https://docs.python.org/3/library/configparser.html
        '''
        [Dialer Defaults]
        Dial Command = ATDT
        Init1 = ATZ
        Init2 = ATQ0 V1 E1 S0=0 &C1 &D2 +FCLASS=0
        Modem Type = Analog Modem
        Buad = 115200
        Modem = /dev/ttyO1
        Phone = 40661234
        Username = mychat
        Password = mychat
        New PPPD = yes
        Auto Reconnect = 1
        '''
        import configparser
        config = configparser.ConfigParser()
        # to mae the option-name case-sensitive when it is saved  
        config.optionxform = str

        config['Dialer Defaults'] = {
          "Dial Command":"ATDT",
          "Init1":"ATZ",
          "Init2":"ATQ0 V1 E1 S0=0 &C1 &D2 +FCLASS=0",
          "Modem Type":"Analog Modem", 
          "Baud":"115200", 
          "Modem":"/dev/ttyO1", 
          "New PPPD":"yes", 
          "Auto Reconnect":"1",
        }
        config['Dialer Defaults']['Phone']=phone_number 
        config['Dialer Defaults']['Username']=phone_account
        config['Dialer Defaults']['Password']=phone_password
        
        with open('/etc/wvdial.conf', 'w') as configfile:
          config.write(configfile)

    except:
      print("[modem_config] unknown error. jsut return.",  flush=True)
      return False
    
    
    return True    
    
################################################################################                
def start_network_config():
    try:
      with open(app.views.get_json_file_dir() + "HostSetting.json", "r") as f:
        file_content=json.load(f)
        host_setting_obj=file_content["HostSetting"][0]
        network_set=host_setting_obj["network_set"]
    except IOError as e:
      print("[start_network_config] HostSetting file doesn't exist.",  flush=True)
      return False
    except KeyError as e:
      print(e)
      print("[start_network_config] no such key. jsut return.",  flush=True)
      return False
    except:
      print("[start_network_config] unknown error. jsut return.",  flush=True)
      return False
      
    print("network_set=%s" %(network_set),  flush=True)
    
    gateway_ip=get_default_gateway_linux()
    print("gateway_ip=%s" %(gateway_ip), flush=True)
    #url = "http://" + gateway_ip + '/boafrm/formWanTcpipSetup'
    url = "http://" + gateway_ip + '/boafrm/formEtopSetMibs'
            
    print(url,  flush=True)

    try: 
      if network_set=="1":
        # pppoe
        pppoe_id=host_setting_obj["pppoe_id"]
        pppoe_pass=host_setting_obj["pppoe_pass"]
        '''
        payload = {
          "pppConnect":"Connect",
          "wanType":"ppp",
          "pppUserName":pppoe_id,
          "pppPassword":pppoe_pass,
          "pppConnectType":"0",
        }
        '''
        #payload = "pppConnect=Connect&wanType=ppp&pppConnectType=0&pppUserName=" + pppoe_id + "&pppPassword=" + pppoe_pass
        payload = "action=reinit&mibname0=WAN_DHCP&mibvalue0=3&mibname1=PPP_USER_NAME&mibvalue1=" + pppoe_id + "&mibname2=PPP_PASSWORD&mibvalue2=" + pppoe_pass + "&mibname3=REPEATER_ENABLED1&mibvalue3=1&mibname4=OP_MODE&mibvalue4=0"

      elif network_set=="2":
        # static ip
        ipaddr=host_setting_obj["static_ip"]
        netmask=host_setting_obj["static_mask"]
        gateway=host_setting_obj["static_gate"]
        dns=host_setting_obj["dns"]
        #payload = "pppConnect=Connect&dnsMode=dnsManual&wanType=fixedIp&wan_ip=" + ipaddr + "&wan_mask=" + netmask + "&wan_gateway=" + gateway
        payload = "action=reinit&mibname0=WAN_DHCP&mibvalue0=0&mibname1=WAN_IP_ADDR&mibvalue1=" + ipaddr + "&mibname2=WAN_SUBNET_MASK&mibvalue2=" + netmask + "&mibname3=WAN_DEFAULT_GATEWAY&mibvalue3=" + gateway + "&mibname4=DNS1&mibvalue4=" + dns + "&mibname5=REPEATER_ENABLED1&mibvalue5=1&mibname6=OP_MODE&mibvalue6=0"
         
      elif network_set=="3":
        # dhcpclient
        #payload = "pppConnect=Connect&dnsMode=dnsAuto&wanType=autoIp"  
        payload = "action=reinit&mibname0=WAN_DHCP&mibvalue0=1&mibname1=REPEATER_ENABLED1&mibvalue1=1&mibname2=OP_MODE&mibvalue2=0"
         
      elif network_set=="4":
        # 4=WiFi
        # wifi site survey
        # skip this operation to save time... get_wifi_scan_result()

        # HostSetting.json: "wifi_ssid":"","wifi_pass":""
        host_wifi_ssid=str(host_setting_obj["wifi_ssid"])
        host_wifi_pass=str(host_setting_obj["wifi_pass"])
        
        # wifi_scan_result.json: [{"ssid":"xx", "signal":0,"security":"", "channel":""},...]
        ap_list=list()
        try:
          with open(app.views.get_json_file_dir() + "wifi_scan_result.json", "r") as f:
            ap_list=json.load(f)
        except:
            print("[start_network_config] wifi site survey fails...",  flush=True)
            pass
            
        ap_found=False  
        target_ap_obj=None
        for ap_info in ap_list:
          if host_wifi_ssid==str(ap_info["ssid"]):
            target_ap_obj=ap_info 
            ap_found=True          
            break 
        if not ap_found:
          print("[start_network_config] ssid(%s) is NOT found !" %(host_wifi_ssid),  flush=True)
          return False
        
        print("AP details:")
        print(target_ap_obj)

        # http://stackoverflow.com/questions/5749195/how-can-i-split-and-parse-a-string-in-python        
        channel=str(target_ap_obj["channel"].split(" ")[0])
        print("channel=%s" %(channel),  flush=True)
        
        # http://stackoverflow.com/questions/7361253/how-to-determine-whether-a-substring-is-in-a-different-string
        if "WPA" in target_ap_obj["security"]:
          payload = "action=reinit&mibname0=OP_MODE&mibvalue0=2&mibname1=REPEATER_ENABLED1&mibvalue1=1&mibname2=REPEATER_SSID1&mibvalue2=" + host_wifi_ssid + "&mibname3=WLAN0_CHANNEL&mibvalue3=" + channel + "&mibname4=WLAN0_CONTROL_SIDEBAND&mibvalue4=1&mibname5=WLAN0_VAP4_SSID&mibvalue5=" + host_wifi_ssid + "&mibname6=WLAN0_VAP4_WLAN_DISABLED&mibvalue6=0&mibname7=WLAN0_VAP4_MODE&mibvalue7=1&mibname8=WLAN0_VAP4_ENCRYPT&mibvalue8=6&mibname9=WLAN0_VAP4_WPA_CIPHER_SUITE&mibvalue9=3&mibname10=WLAN0_VAP4_WPA_PSK&mibvalue10=" + host_wifi_pass + "&mibname11=WLAN0_VAP4_WPA2_CIPHER_SUITE&mibvalue11=3"
        else:
          # no security
          payload = "action=reinit&mibname0=OP_MODE&mibvalue0=2&mibname1=REPEATER_ENABLED1&mibvalue1=1&mibname2=REPEATER_SSID1&mibvalue2=" + host_wifi_ssid + "&mibname3=WLAN0_CHANNEL&mibvalue3=" + channel + "&mibname4=WLAN0_CONTROL_SIDEBAND&mibvalue4=1&mibname5=WLAN0_VAP4_SSID&mibvalue5=" + host_wifi_ssid + "&mibname6=WLAN0_VAP4_WLAN_DISABLED&mibvalue6=0&mibname7=WLAN0_VAP4_MODE&mibvalue7=1&mibname8=WLAN0_VAP4_ENCRYPT&mibvalue8=0&mibname9=WLAN0_VAP4_WPA_CIPHER_SUITE&mibvalue9=3"

        #else:
        #  print("[start_network_config] NOT implemented yet.")
        #  return False
          
      elif network_set=="6":
        # 6=3G
        # "sim_password":"SIM卡密碼","pin_code":"PIN碼" ??
        # ? host_setting_obj["pin_code"]
        # ? host_setting_obj["sim_password"]
        usb3g_user=""
        usb3g_pass=""
        payload = "action=reinit&mibname0=WAN_DHCP&mibvalue0=16&mibname1=PPP_CONNECT_COUNT&mibvalue1=0&mibname2=USB3G_USER&mibvalue2=" + usb3g_user + "&mibname3=USB3G_PASS&mibvalue3=" + usb3g_pass + "&mibname4=USB3G_APN&mibvalue4=internet&mibname5=USB3G_DIALNUM&mibvalue5=*99#&mibname6=REPEATER_ENABLED1&mibvalue6=1&mibname7=OP_MODE&mibvalue7=0" 
    
      else:
        print("[start_network_config] unknown network_set(%s)" %(network_set),  flush=True)
        return False           
  
      print("payload=" + str(payload),  flush=True)
      try:
        r = requests.post(url, data=payload, timeout=30)
        print("server message:",  flush=True) 
        print(r.content.decode("utf-8"),  flush=True)
        # just in case the router LAN config is changed
        
        # kill previous dhclient process(es) ...
        app.host_lib.kill_dhclient_process()
        
        command="route del -net default" 
        print("[start_network_config] command=" + command,  flush=True)
        os.system(command)
        command="dhclient eth0"
        print("[start_network_config] command=" + command,  flush=True)
        os.system(command)
    
      except ValueError as e:
        print("error: %s", str(e))
        return False
      except requests.exceptions.ConnectionError as e:
        print("error: %s", str(e))
        return False    
      except requests.exceptions.RequestException as e:
        print("error: %s", str(e))
        return False
      except UnicodeEncodeError as e:
        print("error: %s", str(e))
        return False
      except:         
        print("Unknown error")    
        return False
          
    except KeyError as e:
      print(e)
      print("[start_network_config] key error.")
      return False
    #except:
    #  print("[start_network_config] network setting fails.")
    #  return False


    #update KeepAlive.json
    
    
    return True
    

def start_antenna_wifi_config():
    try:
      with open(app.views.get_json_file_dir() + "HostSetting.json", "r") as f:
        file_content=json.load(f)
        host_setting_obj=file_content["HostSetting"][0]
        antenna_type=host_setting_obj["antenna_wifi_type"]
        
        print("[start_antenna_wifi_config] antenna_type=%s" %(antenna_type),  flush=True)
        
        gateway_ip=get_default_gateway_linux()
        print("gateway_ip=%s" %(gateway_ip), flush=True)
        url = "http://" + gateway_ip + '/boafrm/formEtopSetMibs'
        print(url,  flush=True)
        payload = "action=reinit&mibname0=ANT_OUT&mibvalue0=" + antenna_type
      
        print("payload=" + str(payload),  flush=True)
        try:
          r = requests.post(url, data=payload, timeout=10)
          print("server message:",  flush=True) 
          print(r.content.decode("utf-8"),  flush=True)
          
        except ValueError as e:
          print("error: %s", str(e))
        except requests.exceptions.ConnectionError as e:
          print("error: %s", str(e))    
        except requests.exceptions.RequestException as e:
          print("error: %s", str(e))
        except UnicodeEncodeError as e:
          print("error: %s", str(e))
        except:         
          print("Unknown error")
        
    except IOError as e:
      print("[start_antenna_wifi_config] HostSetting file doesn't exist.",  flush=True)
      return False 
    except KeyError as e:
      print(e)
      print("[start_antenna_wifi_config] key error",  flush=True)
      return False
    except:
      print("[start_antenna_wifi_config] unknown error.",  flush=True)
      return False

    return True
        
def start_antenna_sub1g_config():
    try:
      with open(app.views.get_json_file_dir() + "HostSetting.json", "r") as f:
        file_content=json.load(f)
        host_setting_obj=file_content["HostSetting"][0]
        antenna_type=host_setting_obj["antenna_sub1g_type"]
        
        print("[start_antenna_sub1g_config] antenna_type=%s" %(antenna_type),  flush=True)
        # socket.emit('setAntennaConfig', {state:1});
        # state : 1 = external , 2 = internal
        data_obj=dict()
        if antenna_type == "1" :
          data_obj["state"]=1
        else:
          data_obj["state"]=2
        app.device_lib.device_command_api2('setAntennaConfig', data_obj)
        
    except IOError as e:
      print("[start_antenna_sub1g_config] HostSetting file doesn't exist.",  flush=True)
      return False 
    except KeyError as e:
      print(e)
      print("[start_antenna_sub1g_config] key error",  flush=True)
      return False
    except:
      print("[start_antenna_sub1g_config] unknown error.",  flush=True)
      return False
      
    return True
    
################################################################################
def save_validate_code(code):
    with open(app.views.get_json_file_dir() + "validate_code.json", "w") as f:
      file_content=dict()
      file_content["validate_code"]=code
      json.dump(file_content, f)


def get_validate_code():
    try:
      with open(app.views.get_json_file_dir() + "validate_code.json", "r") as f:
        file_content=json.load(f)
        code=file_content["validate_code"]
    except:
      print("[login_process] [validate_code.json] error.")
      code=""        
  
    return code

################################################################################
def host_reboot():
    print("[%s][host_reboot] starts." %(str(datetime.datetime.now())),  flush=True)
    command='reboot'
    print("[host_reboot] command=" + command,  flush=True)
    os.system(command)          
    
def host_reset_and_reboot():
    print("[%s][host_reset_and_reboot] starts." %(str(datetime.datetime.now())),  flush=True)
    
    app.host_lib.set_led(LED5_SET_G, 0.5)
    
    # send reset-to-default event to IOT service
    app.host_lib.send_host_reset_signal()
    
    
    # remove all json files (reset to default)
    command='bash {service_dir}reset_host_data.sh {data_dir} {service_dir}'.format(service_dir=get_service_dir(), data_dir=app.views.get_json_file_dir())
    print("[host_reset_and_reboot] command=" + command,  flush=True)
    os.system(command)
    
    # send command to sensor daemon
    # reset to default: socket.emit('clean', {}); 
    app.device_lib.device_command_api2('clean', {})
        
    
    time.sleep(3)
    app.host_lib.off_led(LED5_SET_G)
    
    # reboot
    command='reboot'
    print("[host_reset_and_reboot] command=" + command,  flush=True)
    os.system(command)          
    
    
################################################################################
def login_process(validate_code, username, password):
    login_result=False
    #clear current user_token
    app.host_lib.save_user_token("")
    
    print("[login_process]: %s,%s,%s" %(validate_code,username,password),  flush=True)
    if validate_code=="9876" and username=="" and password=="":
      #1. backup files 
      #table_list=["Device", "WLReadSensor", "WLDoubleBondBTemperatureSensor", "WLReedSensor", "WLIPCam"]
      # just Device.json is preserved, zl 
      table_list=["Device"]
      try:
        for table_name in table_list:
          command='mv {data_dir}{file_name} /tmp/'.format(data_dir=app.views.get_json_file_dir(), file_name=table_name + ".json")
          print("[login_process] command=" + command,  flush=True)
          os.system(command)
      except:
        print("[login_process] backup unknown error.",  flush=True)
        pass
          
      #2. launch reset_host_data.sh: remove all json files and reset HostSetting.json, KeepAlive.json
      #package_dir=os.path.dirname(os.path.abspath(__package__))
      #print("package path=%s" %(package_dir), flush=True)
      command='bash {service_dir}reset_host_data.sh {data_dir} {service_dir}'.format(service_dir=get_service_dir(), data_dir=app.views.get_json_file_dir())
      #get_service_dir()
      print("[login_process] command=" + command,  flush=True)
      os.system(command)
      
      #3. restore files
      try:
        for table_name in table_list:
          command='mv /tmp/{file_name} {data_dir}'.format(data_dir=app.views.get_json_file_dir(), file_name=table_name + ".json")
          print("[login_process] command=" + command,  flush=True)
          os.system(command)
      except:
        print("[login_process] restore unknown error.",  flush=True)
        pass
      
      login_result=True

    else:
      if app.server_lib.check_network_status():
        # network/server status is ok 
        data_obj=dict()
        
        # check if input validate_code is matched with current host validate_code
        host_validate_code=get_validate_code()
        if host_validate_code!="" and host_validate_code!=validate_code:
          print("[login_process] validate_code is NOT matched. login failed !\n\n")
          return False
        else:
          pass
        
        # IOT service just checks username/password
        data_obj["name"]=username
        data_obj["pass"]=password
        result_obj=app.server_lib.send_request_to_server("server_login", data_obj)
        print("result_obj=" + str(result_obj))
        if "status" in result_obj and result_obj["status"]==200:
          try:
            app.host_lib.save_login_username(data_obj["name"])
            app.host_lib.save_user_token(result_obj["token"])
          except:
            print("[login_process]: unknown error",  flush=True)
            pass
            
          # save current login-info for use when network disconnection happens ...
          data_obj["time"]=str(datetime.datetime.now())       
          data_obj["tokenid"]=result_obj["token"]   
          with open(app.views.get_json_file_dir() + "login_info.json", "w") as f:
            json.dump(data_obj, f)
          login_result=True
        else:
          login_result=False
      else:
        # network disconnection or server failure happens
        try:
          with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
            file_content=json.load(f)
            usable_validate_code=file_content["KeepAlive"][0]["admin_password"]
        except (IOError, KeyError):
          usable_validate_code=""
        
        try:
          with open(app.views.get_json_file_dir() + "login_info.json", "r") as f:
            login_info=json.load(f)
            print(login_info,  flush=True)
            usable_username=login_info["name"]
            usable_password=login_info["pass"]
        except (IOError, KeyError):
            usable_username=""
            usable_password=""
        print("[login_process] [network disconnection]: %s,%s,%s" %(usable_validate_code,usable_username,usable_password),  flush=True)
        if validate_code==usable_validate_code and username==usable_username and password==usable_password:
          login_result=True
        else:
          login_result=False
       
    return login_result
    
# 客戶帳號是否開通
def check_account_open(data):
    customer_status=False
    if "customer_status" in data and (data["customer_status"]=="1" or data["customer_status"]=="01"):
      customer_status=True
    return customer_status 

def check_security_always_set_enabled(data):
    always_set=False
    if "always_set" in data and (data["always_set"]=="1" or data["always_set"]=="Y"):
      always_set=True
    return always_set 

# 保全是否設定中
def check_host_alarm_set(data):
    start_host_set=False
    if "start_host_set" in data and (data["start_host_set"]=='Y' or data["start_host_set"]=='1'):
      start_host_set=True
    return start_host_set

def check_early_lift(data):
    print("\ncheck if alert-lift happens too early.",  flush=True)
    # https://www.cyberciti.biz/faq/howto-get-current-date-time-in-python/
    today = datetime.date.today()
    #print("today=" + str(today),  flush=True)  
    #print((int(time.mktime(today.timetuple()))),  flush=True)
    
    try:
      preservation_set_time=data["preservation_set_time"]
    except:
      preservation_set_time="00:00"

    print("preservation_set_time=" + preservation_set_time,  flush=True)
    assigned_set_time=datetime.datetime.strptime(preservation_set_time, "%H:%M")
    tt = datetime.datetime(today.year, today.month, today.day, assigned_set_time.hour, assigned_set_time.minute, 0)
    alarm_set_timestamp=int(time.mktime(tt.timetuple())) 
        
    
    try:
      preservation_lift_time=data["preservation_lift_time"]
    except:
      preservation_lift_time="00:00"
    
    print("preservation_lift_time=" + preservation_lift_time,  flush=True)
     
    #if "preservation_lift_time_change" in data and data["preservation_lift_time_change"] != "":
    try:
      # 保全解除時間_暫改 preservation_lift_time_change
      preservation_lift_time_change = data["preservation_lift_time_change"]
      print("[check_early_lift] preservation_lift_time_change=" + preservation_lift_time_change,  flush=True) 
      preservation_lift_time_change_datetime=datetime.datetime.strptime(preservation_lift_time_change, "%Y/%m/%d %H:%M")
      preservation_lift_time_change_timestamp=int(time.mktime(preservation_lift_time_change_datetime.timetuple()))
      
      preservation_lift_time_change_date=datetime.date(preservation_lift_time_change_datetime.year, preservation_lift_time_change_datetime.month, preservation_lift_time_change_datetime.day)
    #else:
    except:
      print("[check_early_lift] preservation_lift_time_change does not exist or is not valid formatted",  flush=True) 
      preservation_lift_time_change_timestamp=0
      preservation_lift_time_change_date=None
            
    print("preservation_lift_time_change_timestamp=" + str(preservation_lift_time_change_timestamp),  flush=True)
    
    # http://stackoverflow.com/questions/1759455/how-can-i-account-for-period-am-pm-with-datetime-strptime
    assigned_lift_time=datetime.datetime.strptime(preservation_lift_time, "%H:%M")
    #print(assigned_lift_time)
    
    # https://www.tutorialspoint.com/python/time_strftime.htm
    # http://www.linuxquestions.org/questions/programming-9/python-datetime-to-epoch-4175520007/
    tt = datetime.datetime(today.year, today.month, today.day, assigned_lift_time.hour, assigned_lift_time.minute, 0)
    alert_lift_timestamp= int(time.mktime(tt.timetuple()))
    print("alert_lift_timestamp=" + str(alert_lift_timestamp),  flush=True)
    
    current_timestamp=int(time.time())
    print("current_timestamp=" + str(current_timestamp),  flush=True)
    
    # check if today is the same as the date of 保全解除時間_暫改
    if today == preservation_lift_time_change_date:
      # check if the current time is before 保全解除時間_暫改
      if alarm_set_timestamp < preservation_lift_time_change_timestamp: 
        if current_timestamp > alarm_set_timestamp and current_timestamp < preservation_lift_time_change_timestamp:                          
          return True
      else:                                         
        if current_timestamp < preservation_lift_time_change_timestamp or current_timestamp > alarm_set_timestamp:
          return True
    else:
      # check if the current time is earlier than the assigned alert-lift_time
      if alarm_set_timestamp < alert_lift_timestamp:
        if current_timestamp > alarm_set_timestamp and current_timestamp < alert_lift_timestamp:
          return True
      else:
        if current_timestamp < alert_lift_timestamp or current_timestamp > alarm_set_timestamp:
          return True
      
    return False

def check_holiday_lift(data):
    print("[check_holiday_lift] check if alert-lift happens on holiday.",  flush=True)
    
    try:
      # http://stackoverflow.com/questions/1759455/how-can-i-account-for-period-am-pm-with-datetime-strptime
      assigned_holiday=datetime.datetime.strptime(data["holiday"], "%Y/%m/%d")
    except:
      assigned_holiday="0000/00/00"
    
    print("holiday=" + str(assigned_holiday),  flush=True)
    #print(type(assigned_holiday),  flush=True)
    
    # https://www.cyberciti.biz/faq/howto-get-current-date-time-in-python/
    tt = datetime.date.today()
    today = datetime.datetime(tt.year, tt.month, tt.day, 0, 0, 0)
    print("today=" + str(today),  flush=True)  
    #print(type(today),  flush=True)
    
    # check if today is holiday 
    if today == assigned_holiday:
      return True
      
    return False

# 保全設定時間_延後設定 DelayTime   2016/11/4 18:40         preservation_set_time_delay_set 
def check_delay_set2(data):
    print("\ncheck preservation_set_time_delay_set",  flush=True)
    '''
    if "preservation_set_time_delay_set" in data and data["preservation_set_time_delay_set"] != "" :
      preservation_set_time_delay_set=data["preservation_set_time_delay_set"]
    else:
      preservation_set_time_delay_set="1970/01/01 00:00"
    print("preservation_set_time_delay_set=" + preservation_set_time_delay_set,  flush=True) 
    
    preservation_set_time_delay_set_datetime=datetime.datetime.strptime(preservation_set_time_delay_set, "%Y/%m/%d %H:%M")
    '''
    try:
      print("[check_delay_set2] preservation_set_time_delay_set=" + data["preservation_set_time_delay_set"],  flush=True) 
      preservation_set_time_delay_set=data["preservation_set_time_delay_set"]
      preservation_set_time_delay_set_datetime=datetime.datetime.strptime(preservation_set_time_delay_set, "%Y/%m/%d %H:%M")
    except:
      print("[check_delay_set2] error: preservation_set_time_delay_set does not exist or is not valid formatted",  flush=True)
      preservation_set_time_delay_set="1970/01/01 00:00"
      preservation_set_time_delay_set_datetime=datetime.datetime.strptime(preservation_set_time_delay_set, "%Y/%m/%d %H:%M")
    
    print("preservation_set_time_delay_set=" + preservation_set_time_delay_set,  flush=True) 
    
    
    
    
    current_timestamp=int(time.time())
    print("current_timestamp=" + str(current_timestamp),  flush=True)
    
    alarm_set_timestamp= int(time.mktime(preservation_set_time_delay_set_datetime.timetuple())) 
    print("alarm_set_timestamp=" + str(alarm_set_timestamp),  flush=True)

    today = datetime.date.today()
    print("today=" + str(today),  flush=True)  
    today_timestamp = int(time.mktime(today.timetuple()))
    print(today_timestamp,  flush=True)
    
    if alarm_set_timestamp >= today_timestamp and current_timestamp >= alarm_set_timestamp:
      return True
  
    return False
    

# 保全設定時間_不設定   NonSetup    2016/8/20               preservation_set_time_no_set
def allow_delay_set(data):
    print("\ncheck preservation_set_time_no_set",  flush=True)
    try:
      preservation_set_time_no_set=data["preservation_set_time_no_set"]
    except:
      preservation_set_time_no_set="1970/01/01"
    
    print("preservation_set_time_no_set=" + preservation_set_time_no_set,  flush=True) 
    
    today = datetime.date.today()
    #print("today=" + str(today),  flush=True)  
    #print((int(time.mktime(today.timetuple()))),  flush=True)
    today_datetime = datetime.datetime(today.year, today.month, today.day, 0, 0, 0)
    preservation_set_time_no_set_datetime=datetime.datetime.strptime(preservation_set_time_no_set, "%Y/%m/%d")
    print(today_datetime)
    print(preservation_set_time_no_set_datetime)
    if today_datetime==preservation_set_time_no_set_datetime:    
      return False
    
    return True
    
def check_delay_set_over_30_min(data, base_datetime_obj):
    print("[check_delay_set_over_30_min]",  flush=True)
    
    alarm_set_timestamp= int(time.mktime(base_datetime_obj.timetuple())) 
    print("alarm_set_timestamp=" + str(alarm_set_timestamp),  flush=True)
    
    current_timestamp=int(time.time())
    print("current_timestamp=" + str(current_timestamp),  flush=True)
      
    if not app.host_lib.check_host_alarm_set(data):
      # check if the current time is over the assigned (base time + delay_time + 1800 sec)
      if current_timestamp > (alarm_set_timestamp + 1800):
        return True
    return False
    
def check_delay_set_over_x_min(data, base_datetime_obj):
    print("[check_delay_set_over_x_min]",  flush=True)
    
    try:
      standard_delay_set_time=data["standard_delay_set_time"]
    except:
      standard_delay_set_time="60"

    print("standard_delay_set_time=" + standard_delay_set_time,  flush=True)

    alarm_set_timestamp= int(time.mktime(base_datetime_obj.timetuple())) 
    print("alarm_set_timestamp=" + str(alarm_set_timestamp),  flush=True)
    
    current_timestamp=int(time.time())
    print("current_timestamp=" + str(current_timestamp),  flush=True)
      
    if not app.host_lib.check_host_alarm_set(data):
      # check if the current time is over the assigned (base time + standard_delay_set_time*60 sec)
      if current_timestamp > (alarm_set_timestamp + int(standard_delay_set_time)*60):
        return True
    return False
    
def check_delay_set(data):
    print("[check_delay_set] check if alarm is NOT set yet when the alarm-set time is up",  flush=True)
    
    # https://www.cyberciti.biz/faq/howto-get-current-date-time-in-python/
    today = datetime.date.today()
    #print("today=" + str(today),  flush=True)  
    #print((int(time.mktime(today.timetuple()))),  flush=True)
    # https://www.tutorialspoint.com/python/time_strftime.htm
    # http://www.linuxquestions.org/questions/programming-9/python-datetime-to-epoch-4175520007/
    
    try:
      preservation_lift_time=data["preservation_lift_time"]
    except:
      preservation_lift_time="00:00"
    print("preservation_lift_time=" + preservation_lift_time,  flush=True)

    assigned_lift_time=datetime.datetime.strptime(preservation_lift_time, "%H:%M")
    #print(assigned_lift_time)
    # https://www.tutorialspoint.com/python/time_strftime.htm
    # http://www.linuxquestions.org/questions/programming-9/python-datetime-to-epoch-4175520007/
    tt = datetime.datetime(today.year, today.month, today.day, assigned_lift_time.hour, assigned_lift_time.minute, 0)
    alert_lift_timestamp= int(time.mktime(tt.timetuple()))
    print("alert_lift_timestamp=" + str(alert_lift_timestamp),  flush=True)
    
    try:
      preservation_set_time=data["preservation_set_time"]
    except:
      preservation_set_time="00:00"

    print("preservation_set_time=" + preservation_set_time,  flush=True) 
    
    try:
      preservation_delay_time=data["preservation_delay_time"]
    except:
      preservation_delay_time="30"
      
    print("preservation_delay_time=" + preservation_delay_time,  flush=True)
    
    #if "preservation_set_time_change" in data and data["preservation_set_time_change"]!="":
    try:
      # 保全設定時間_暫改
      preservation_set_time_change = data["preservation_set_time_change"]
      print("preservation_set_time_change=" + preservation_set_time_change,  flush=True)
      preservation_set_time_change_datetime=datetime.datetime.strptime(preservation_set_time_change, "%Y/%m/%d %H:%M")
      preservation_set_time_change_timestamp=int(time.mktime(preservation_set_time_change_datetime.timetuple()))
      
      preservation_set_time_change_date=datetime.date(preservation_set_time_change_datetime.year, preservation_set_time_change_datetime.month, preservation_set_time_change_datetime.day)
    #else:
    except:
      preservation_set_time_change_timestamp=0              
      preservation_set_time_change_date=None
      
    print("preservation_set_time_change_timestamp=" + str(preservation_set_time_change_timestamp),  flush=True)
    
    # http://stackoverflow.com/questions/1759455/how-can-i-account-for-period-am-pm-with-datetime-strptime
    assigned_set_time=datetime.datetime.strptime(preservation_set_time, "%H:%M")
    #print(assigned_set_time,  flush=True)
    
    
    tt = datetime.datetime(today.year, today.month, today.day, assigned_set_time.hour, assigned_set_time.minute, 0)
    alarm_set_timestamp= int(time.mktime(tt.timetuple())) + int(preservation_delay_time)*60
    print("alarm_set_timestamp=" + str(alarm_set_timestamp),  flush=True)
    
    current_timestamp=int(time.time())
    print("current_timestamp=" + str(current_timestamp),  flush=True)
      
    if not app.host_lib.check_host_alarm_set(data):
      # check if today is the same as the date of 保全設定時間_暫改
      if today == preservation_set_time_change_date:
        # check if the current time is after 保全設定時間_暫改
        if preservation_set_time_change_timestamp < alert_lift_timestamp:
          if current_timestamp > preservation_set_time_change_timestamp and current_timestamp < alert_lift_timestamp:
            return True
        else:
          if current_timestamp < alert_lift_timestamp or current_timestamp > preservation_set_time_change_timestamp:
            return True
      else:
        # check if the current time is over the assigned (alarm-set time + delay_time)
        if alarm_set_timestamp < alert_lift_timestamp:
          if current_timestamp > alarm_set_timestamp and current_timestamp < alert_lift_timestamp:
            return True
        else:
          if current_timestamp < alert_lift_timestamp or current_timestamp > alarm_set_timestamp:
            return True
            
    return False

################################################################################
# modem (RJ11) init
def modem_init():
    set_gpio(15, 0)
    value = open('/sys/class/gpio/gpio57/value').read()
    print("[modem_init] Modem_RST:=%s\n" % value)

    set_gpio(15, 1)
    value = open('/sys/class/gpio/gpio57/value').read()
    print("[modem_init] Modem_RST:=%s\n" % value) 
    
################################################################################
def handle_host_event(event_obj):
    result=True
    trigger_type=str(event_obj["trigger_type"])
    try:
        time_stamp=event_obj["time"]
        print("[handle_host_event] time: " + str(time_stamp))
    except :
        print("[handle_host_event] No timestamp exist, ignore!!!")


    if trigger_type=="1":
      # 1=host is ready to work (sensor daemon is ready)
      print("[start_network_config] Host is ready to work !",  flush=True)
      audio_obj=app.audio.Audio() 
      audio_obj.stop()
      audio_obj.play(app.host_lib.get_service_dir() + "boot_complete.mp3")      
      
      # set sub1g antenna when sensor daemon is ready  
      app.host_lib.start_antenna_sub1g_config()
      
      # gpio related stuff init
      app.host_lib.init_12v_output()
      app.host_lib.init_alarm_output()
      app.host_lib.stop_coupling_operation()
      app.host_lib.modem_init()
      
      security_set=False
      try:
        with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
          file_content=json.load(f)
          keepalive_obj=file_content["KeepAlive"][0]
          security_set=app.host_lib.check_host_alarm_set(keepalive_obj)
      except:
        print("[KeepAlive.json] unknown error => assume security is unset",  flush=True)
        pass
          
      if security_set:
        app.host_lib.host_setunset_gpio("1")
      else:
        app.host_lib.host_setunset_gpio("0")
      
      # send reboot event to IOT service
      app.host_lib.send_reboot_signal()

    elif trigger_type=="2":
        print("Host switch to modem(GA09)~~~")
        # update KeepAlive.json
        update_keepalive_json_file("now_connection_way", "5")
        
        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="A"
        signal_obj["abnormal_catalog"]="GA09"

        obj=app.server_lib.trigger_to_server(signal_obj)
        
        '''
        #test only
        tt=threading.Thread(target=modem_dial_func)
        # http://stackoverflow.com/questions/17888802/is-the-python-non-daemon-thread-a-non-detached-thread-when-is-its-resource-free
        # Daemons are cleaned up by Python, Non-daemonic threads are not - you have to signal them to stop.
        tt.daemon=True
        tt.start()
        '''
        
    elif trigger_type=="3":
        print("Host backup communication error(GA18)!!!")

        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="A"
        signal_obj["abnormal_catalog"]="GA18"

        obj=app.server_lib.trigger_to_server(signal_obj)
    
    elif trigger_type=="4":
        print("Host 3G network communication(GA11)")

        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="A"
        signal_obj["abnormal_catalog"]="GA11"

        obj=app.server_lib.trigger_to_server(signal_obj)

    elif trigger_type=="5":
        print("Host ADSL communication(GA13)")

        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="A"
        signal_obj["abnormal_catalog"]="GA13"

        obj=app.server_lib.trigger_to_server(signal_obj)

    elif trigger_type=="6":
        print("Host ADSL network disconnect(GA15)!!!")

        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="A"
        signal_obj["abnormal_catalog"]="GA15"

        obj=app.server_lib.trigger_to_server(signal_obj)

    elif trigger_type=="7":
        print("Host 3G network disconnect(GA16)!!!")

        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="A"
        signal_obj["abnormal_catalog"]="GA16"

        obj=app.server_lib.trigger_to_server(signal_obj)

    elif trigger_type=="8":
        print("Host ADSL network reconnect(GG03)!!!")
        # update KeepAlive.json: now_connection_way="?"
        try:
          hostsetting_obj=open_hostsetting_json_file()
          update_keepalive_json_file("now_connection_way", hostsetting_obj["network_set"])
        except:
          print ("[handle_host_event][hostSetting] network_set error.")
          update_keepalive_json_file("now_connection_way", "3")
          
        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="G"
        signal_obj["abnormal_catalog"]="GG03"

        obj=app.server_lib.trigger_to_server(signal_obj)
      
    elif trigger_type=="9":
        print("Host 3G network reconnect(GG04)!!!")
        # update KeepAlive.json: now_connection_way="6"
        update_keepalive_json_file("now_connection_way", "6")

        signal_obj=dict()
        signal_obj["type_id"]="d01"
        signal_obj["alert_catalog"]="G"
        signal_obj["abnormal_catalog"]="GG04"

        obj=app.server_lib.trigger_to_server(signal_obj)
        
    else:
        print("[handle_host_event] Invalid trigger_type(%s) !" %(trigger_type),  flush=True)
        result=False
        
    return result
    
################################################################################
# save keepalive message to log file
'''
[
(...},
...
]
'''
def save_to_keepalive_log(log_obj):
    # log separation by date to reduce the size of each log file
    today = datetime.date.today()
    log_file_name="keepalive_log_" + today.strftime("%Y%m%d") +".json"
    
    # read the corresponding log file
    try:
      with open("/var/log/" + log_file_name, "r") as f:
        log_list=json.load(f)
        #print(log_list,  flush=True)
    except IOError:
      log_list=list()        

    # write back to the corresponding log file
    with open("/var/log/" + log_file_name, "w") as f:
      f.write('[\n')
      for item in log_list: 
        item_str = json.dumps(item, sort_keys=False, separators=(', ', ':'))
        # to append a newline for each log                                                         
        f.write(item_str + ',\n')
      
      # wrtie log_obj to file!
      log_str = json.dumps(log_obj, sort_keys=False, separators=(', ', ':'))                                                         
      f.write(log_str + '\n')
      
      f.write(']\n')        
      

################################################################################
# save signal/event to log file
'''
[
{"abnormal_catalog":"GA07", "send_service_status":"Y", "type_id":"d01", "abnormal_value":"50", "event_id":"xx", "alert_catalog":"A", "customer_number":"11345", "setting_value":"80", "send_customer_status":"Y", "time":"2016-12-02 11:21:49.604127"},
...
]
'''
def save_to_event_log(log_obj):
    # log separation by month to reduce the size of each log file
    today = datetime.date.today()
    log_file_name="event_log_" + today.strftime("%Y%m%d") +".json"
    
    # read the corresponding log file
    try:
      with open("/var/log/" + log_file_name, "r") as f:
        log_list=json.load(f)
        #print(log_list)
    except IOError:
      log_list=list()        
    except:
      print("[save_to_event_log] unknown error",  flush=True)
      return
      
    # write back to the corresponding log file
    with open("/var/log/" + log_file_name, "w") as f:
      f.write('[\n')
      for item in log_list: 
        item_str = json.dumps(item, sort_keys=False, separators=(', ', ':'))
        # to append a newline for each log                                                         
        f.write(item_str + ',\n')
      
      # wrtie log_obj to file!
      log_str = json.dumps(log_obj, sort_keys=False, separators=(', ', ':'))                                                         
      f.write(log_str + '\n')
      
      f.write(']\n')        
      

################################################################################
def trigger_coupling_operation(device_id):
    print("[trigger_coupling_operation][%s] start" %(str(datetime.datetime.now())),  flush=True)
    print("[trigger_coupling_operation] device_id=%s" %(device_id),  flush=True)
    
    # find out the matched loop_id
    loop_id=app.device_lib.check_loop_id_from_device_id(device_id)
    if loop_id=="":
      print("[trigger_coupling_operation][%s] is NOT on any loop!" %(str(device_id)),  flush=True)
      return False
    print("[trigger_coupling_operation] loop_id=%s" %(loop_id),  flush=True)
      
    # iterate the "CouplingLoop" list
    coupling_loop_list=list()
    try:
      with open(app.views.get_json_file_dir() + "CouplingLoop.json", "r") as f:
        file_content=json.load(f)
        coupling_loop_list=file_content["CouplingLoop"]
        for coupling_item in coupling_loop_list:
          print(coupling_item)
          optical_coupling_id=coupling_item["optical_coupling"]
          optical_coupling_status=coupling_item["status"]   
          loop_list=coupling_item["loop_id"]
          for loop_item in loop_list:
            if loop_id == str(loop_item): 
                print("[trigger_coupling_operation] optical_coupling_id[%s] is triggered !" %(optical_coupling_id),  flush=True)
              
                if optical_coupling_status == "0":   # APP and host : 0=NO->NC, 1=NC->NO
                  #app.gpio_io_api.IO_set(int(optical_coupling_id), 1) # gpio API: 1=NC, 0=NO
                  #text="IO" + optical_coupling_id
                  #Adafruit_BBIO.GPIO.setup(text, Adafruit_BBIO.GPIO.OUT)
                  #Adafruit_BBIO.GPIO.output(text, Adafruit_BBIO.GPIO.HIGH)
                  set_gpio(int(optical_coupling_id), 1)  
                else:
                  #app.gpio_io_api.IO_set(int(optical_coupling_id), 0) # gpio API: 1=NC, 0=NO
                  #text="IO" + optical_coupling_id
                  #Adafruit_BBIO.GPIO.setup(text, Adafruit_BBIO.GPIO.OUT)
                  #Adafruit_BBIO.GPIO.output(text, Adafruit_BBIO.GPIO.LOW)
                  set_gpio(int(optical_coupling_id), 0)
    except IOError:
      print("[trigger_coupling_operation] CouplingLoop.json file doesn't exist.",  flush=True)
      return False 
    #except:
    #  print("[trigger_coupling_operation] unknown error.",  flush=True)
    #  return False
    
    print("[trigger_coupling_operation][%s] end" %(str(datetime.datetime.now())),  flush=True)
    return True
    
def stop_coupling_operation():
    print("[stop_coupling_operation][%s] start" %(str(datetime.datetime.now())),  flush=True)
      
    # iterate the "CouplingLoop" list
    coupling_loop_list=list()
    try:
      with open(app.views.get_json_file_dir() + "CouplingLoop.json", "r") as f:
        file_content=json.load(f)
        coupling_loop_list=file_content["CouplingLoop"]
        for coupling_item in coupling_loop_list:
          print(coupling_item)
          optical_coupling_id=coupling_item["optical_coupling"]
          optical_coupling_status=coupling_item["status"]   
          print("[stop_coupling_operation] optical_coupling_id[%s]" %(optical_coupling_id),  flush=True)
          try:
            if optical_coupling_status == "0":   # APP and host : 0=NO->NC, 1=NC->NO
              #app.gpio_io_api.IO_set(int(optical_coupling_id), 0) # gpio API: 1=NC, 0=NO
              #text="IO" + optical_coupling_id
              #Adafruit_BBIO.GPIO.setup(text, Adafruit_BBIO.GPIO.OUT)
              #Adafruit_BBIO.GPIO.output(text, Adafruit_BBIO.GPIO.LOW)
              set_gpio(int(optical_coupling_id), 0)  
            else:
              #app.gpio_io_api.IO_set(int(optical_coupling_id), 1) # gpio API: 1=NC, 0=NO
              #text="IO" + optical_coupling_id
              #Adafruit_BBIO.GPIO.setup(text, Adafruit_BBIO.GPIO.OUT)
              #Adafruit_BBIO.GPIO.output(text, Adafruit_BBIO.GPIO.HIGH)
              set_gpio(int(optical_coupling_id), 1)
              
          except:
            print("[stop_coupling_operation][app.gpio_io_api.IO_set] unknown error.",  flush=True)
              
    except IOError:
      print("[stop_coupling_operation] CouplingLoop.json file doesn't exist.",  flush=True)
      return False 
    except:
      print("[stop_coupling_operation] unknown error.",  flush=True)
      return False
      
    print("[stop_coupling_operation][%s] end" %(str(datetime.datetime.now())),  flush=True)
    return True


################################################################################
def sync_host_setting_to_keepalive():
    print("[sync_host_setting_to_keepalive]",  flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!")
      return False
    except:
      print ("[KeepAlive.json] unknown error.")
      return False
    
    hostsetting_obj=open_hostsetting_json_file()
    try:
      keepalive_obj["now_connection_way"] = hostsetting_obj["network_set"]
    except:
      print ("[hostsetting_obj][now_connection_way] error.")
      pass
    try:
      keepalive_obj["temperature_control"] = hostsetting_obj["action_temperature_sensing"]
    except:
      print ("[hostsetting_obj][temperature_control] error.")
      pass
      
    try:
      keepalive_obj["low_power_control"] = hostsetting_obj["battery_low_power_set"]
    except:
      print ("[hostsetting_obj][low_power_control] error.")
      pass  

    with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
      json.dump(file_content, f)

    return True

################################################################################    
def open_hostsetting_json_file():
    print("[open_hostsetting_json_file]",  flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "HostSetting.json", "r") as f:
        file_content=json.load(f)
    except IOError:
      print("[HostSetting.json] doesn't exist. The account is NOT opened!",  flush=True)
      return None 
    except:
      print("[open_hostsetting_json_file] unknown error.",  flush=True)
      return None
      
    return file_content["HostSetting"][0]
    
################################################################################
def get_host_urgent_sound_set():
    print("[get_host_urgent_sound_set]",  flush=True)
    enable_flag=True
    try:
      hostsetting_obj=open_hostsetting_json_file()
      if hostsetting_obj["urgent_bt_alarm_sound_set"] == "0":
        enable_flag=False
    except:
      print("[get_host_urgent_sound_set] unknown error => assume alarm_sound is enabled",  flush=True)
      pass
      
    return enable_flag


################################################################################
def get_power_source():
    print("[get_power_source]",  flush=True)
    # 0=battery, 1=AC
    value = ADC.read_raw("P9_37")
    #print ("[get_power_source] value=%d" %(value),  flush=True)
    if value < 500:
      #print ("[get_power_source] 12V_SEN AIN2 is disabled !",  flush=True)
      return 0 
    else:     
      #print ("[get_power_source] 12V_SEN AIN2 is enabled !",  flush=True)
      return 1    
        
    
def get_power_status():
    print("[get_power_status]",  flush=True)
    value = ADC.read_raw("P9_39")
    if value > 500:
      voltage = (value / 4095 * 1.8 * 3)
      percentage = ((voltage -3.5)/(4.2-3.5))* 100
      #print (" BAT_DET AIN0 value: %f - voltage: %fv - percentage: %d" % (value, voltage,percentage))  
      if percentage < 0:
        return 0
      elif percentage > 100:
        return 100   
      else:
        return int(percentage)
    else:
      #print (" BAT_DET AIN0 is disable!")  
      return 0    

def get_temperature_status():
    print("[get_temperature_status]",  flush=True)
    os.system('i2cset -y -f 1 0x40 0xf3')
    a=os.popen("i2cget -y -f 1 0x40").read()
    b=a.rstrip('\n')
    #print(a.rstrip('\n'))
    h=int(b, 16) << 8
    #print(h)   
    t=h+136  
    #print(t)  
    temperature= ((t * 175.72)/65536)-46.85     
    #print("[get_temperature_status] temperature=%d" %(int(temperature)),  flush=True)
    return int(temperature)
    
    
################################################################################
#         
def set_led(led_number, off_time):
    print("[set_led] led_number=%d, off_time=%1.1f" %(led_number, off_time),  flush=True)
    try:
      pid = str(os.getpid())
      #print("[set_led] pid=%s" %(pid),  flush=True)
      from subprocess import check_output
      process_name = check_output(" ps -ax | grep " + pid  + " | grep -v  grep|awk '{print $6}'", shell=True).decode("utf-8")
      #print("[set_led] process_name=%s" %(process_name),  flush=True)
      if "keepalive" in process_name :
        print("[set_led] call led_api directly ..." ,  flush=True)
        app.led_api.SetLed(led_number, off_time) 
       
      else:
        print("[set_led] call led_api via socket of keepalive_service ..." ,  flush=True)
        import socket
        import sys
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect(('localhost', 5002))
        command_obj=dict() 
        command_obj["command"]="set_led"
        command_obj["arg1"]=led_number
        command_obj["arg2"]=off_time  
        client_socket.send(json.dumps(command_obj).encode('utf-8'))
        client_socket.close()
    except:
      print("[set_led] unknown error",  flush=True)
      pass
      
def off_led(led_number):
    print("[off_led] led_number=%d" %(led_number),  flush=True)
    try:
      pid = str(os.getpid())
      #print("[off_led] pid=%s" %(pid),  flush=True)
      from subprocess import check_output
      process_name = check_output(" ps -ax | grep " + pid  + " | grep -v  grep|awk '{print $6}'", shell=True).decode("utf-8")
      #print("[off_led] process_name=%s" %(process_name),  flush=True)
      if "keepalive" in process_name :
        print("[off_led] call led_api directly ..." ,  flush=True)
        app.led_api.OffLed(led_number) 
       
      else:
        print("[off_led] call led_api via socket of keepalive_service ..." ,  flush=True)
        import socket
        import sys
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect(('localhost', 5002))
        command_obj=dict() 
        command_obj["command"]="off_led"
        command_obj["arg1"]=led_number
        client_socket.send(json.dumps(command_obj).encode('utf-8'))
        client_socket.close()
    except:
      print("[off_led] unknown error",  flush=True)
      pass
    
################################################################################
def read_date_button():
    return get_gpio("112")
    
def read_time_button():
    return get_gpio("113")

def read_func_button():
    return get_gpio("116")

        
def get_gpio(text):
    #print("[get_gpio][%s] start" %(str(datetime.datetime.now())),  flush=True)
    
    try:
      value = open('/sys/class/gpio/gpio' + text + '/value').read()
      #print("[get_gpio] value=" + value,  flush=True)
    except:
      print("[get_gpio] failed ==> try to config gpio " + text,  flush=True)
      command='echo {pin} > /sys/class/gpio/export'.format(pin=text)
      #print("[get_gpio] " + command,  flush=True)    
      os.system(command)
     
      command='echo "in" > /sys/class/gpio/gpio{pin}/direction'.format(pin=text)
      #print("[get_gpio] " + command,  flush=True)    
      os.system(command)
      value = open('/sys/class/gpio/gpio' + text + '/value').read()
    
    #print("[get_gpio][%s] end" %(str(datetime.datetime.now())),  flush=True)
    
    return value.strip()
    
def set_gpio(io_number, value):
    #print("[set_gpio][%s] start" %(str(datetime.datetime.now())),  flush=True)
    #print("[set_gpio] io_number=%d, value=%d" %(io_number, value),  flush=True)
    '''
    { "GPIO1_13", "IO1", 45, -1, -1},
    { "GPIO1_14", "IO2", 46, -1, -1},
    { "GPIO1_15", "IO3", 47, -1, -1},
    { "GPIO1_16", "IO4", 48, -1, -1},
    { "GPIO1_17", "IO5", 49, -1, -1},
    { "GPIO1_18", "IO6", 50, -1, -1},
    { "GPIO1_19", "IO7", 51, -1, -1},
    { "GPIO1_20", "IO8", 52, -1, -1},
    { "GPIO1_24", "IO9", 56, -1, -1},
    { "GPIO1_26", "IO10", 58, -1, -1},
    { "GPIO1_28", "IO11", 60, -1, -1},
    { "GPIO2_2", "IO12", 66, -1, -1},
    { "GPIO2_1", "CC1310_RST", 65, -1, -1},
    { "GPIO2_4", "IO14", 68, -1, -1},
    { "GPIO2_22", "IO16", 86, -1, -1},
    { "GPIO1_25", "Modem_RST", 57, -1, -1},

    '''
    
    text=""
    if io_number == 1:
      text="45"
    elif io_number == 2:
      text="46"
    elif io_number == 3:
      text="47"
    elif io_number == 4:
      text="48"
    elif io_number == 5:
      text="49"
    elif io_number == 6:
      text="50"
    elif io_number == 7:
      text="51"
    elif io_number == 8:
      text="52"
    elif io_number == 9:
      text="56"
    elif io_number == 10:
      text="58"
    elif io_number == 11:
      text="60"
    elif io_number == 12:
      text="66"
    
    # non-coupling number definition
    elif io_number == 13:
      # IO14
      text="68"
    elif io_number == 14:
      # IO16
      text="86"   
    elif io_number == 15:
      # Modem_RST
      text="57"   
    
    else:
      print("[set_gpio] invalid IO number!",  flush=True)
      return  
    
    # echo 45 > /sys/class/gpio/export  
    # echo "out" > /sys/class/gpio/gpio45/direction
    # echo 1 > /sys/class/gpio/gpio45/value
    # echo 0 > /sys/class/gpio/gpio45/value
    try:
      xxx = open('/sys/class/gpio/gpio' + text + '/value').read()
      command='echo {val} > /sys/class/gpio/gpio{pin}/value'.format(val=str(value), pin=text)    
      print("[set_gpio] " + command,  flush=True)
      os.system(command)
    except:
      print("[set_gpio] failed ==> try to config gpio " + text,  flush=True)
      command='echo {pin} > /sys/class/gpio/export'.format(pin=text)
      print("[set_gpio] " + command,  flush=True)    
      os.system(command)
     
      command='echo "out" > /sys/class/gpio/gpio{pin}/direction'.format(pin=text)
      print("[set_gpio] " + command,  flush=True)    
      os.system(command)
      
      command='echo {val} > /sys/class/gpio/gpio{pin}/value'.format(val=str(value), pin=text)    
      print("[set_gpio] " + command,  flush=True)
      os.system(command)
    
    #print("[set_gpio][%s] end" %(str(datetime.datetime.now())),  flush=True)
    

################################################################################
def set_alarm_voice_flag(loop_id, disaster_index):
    print("[set_alarm_voice_flag]",  flush=True)
    
    # disaster = "[fire][gas][sos]"
    disaster_list=["000", "001","010","011","100","101","110","111"]
    
    flag,obj=get_alarm_voice_flag()
    if flag==True:
      # alarm(s) already exist
      # add the loop_id to loop_list if it is not in the loop_list
      try:
        obj["time"]=str(datetime.datetime.now())
        
        new_disaster_index = disaster_index | disaster_list.index(obj["disaster"])
        obj["disaster"]=disaster_list[new_disaster_index]
        print("new disaster=%s" %(obj["disaster"]),  flush=True)
                  
        item_found=False
        for item in obj["loop_list"]:
          if str(item) == loop_id:
             item_found=True
             break
        if not item_found:
          obj["loop_list"].append(loop_id)
        
        with open("/tmp/alarm_voice_flag", "w") as f:
          json.dump(obj, f)
      except:
        print("[set_alarm_voice_flag][add more alarms] unknown error",  flush=True)
        
    else:
      # first alarm 
      try:
        with open("/tmp/alarm_voice_flag", "w") as f:
          alarm_obj=dict()
          alarm_obj["time"]=str(datetime.datetime.now())
          
          alarm_obj["disaster"]=disaster_list[disaster_index]
          alarm_obj["loop_list"]=list()
          alarm_obj["loop_list"].append(loop_id)
          json.dump(alarm_obj, f)    
      except:
        print("[set_alarm_voice_flag][first alarm] unknown error",  flush=True)
      
      
def clear_alarm_voice_flag():
    print("[clear_alarm_voice_flag]",  flush=True)
    try:
      os.remove("/tmp/alarm_voice_flag")
    except:
      print("[clear_alarm_voice_flag] no need to clear it",  flush=True)
         

def get_alarm_voice_flag():
    print("[get_alarm_voice_flag]",  flush=True)
    try:
      with open("/tmp/alarm_voice_flag", "r") as f:
        alarm_obj=json.load(f)    
        print("[get_alarm_voice_flag] alarm_obj=%s" %(str(alarm_obj)),  flush=True)
      return True, alarm_obj  
    except IOError:
      print("[get_alarm_voice_flag] alarm_voice_flag is not set",  flush=True)
      return False, None 
    except:
      print("[get_alarm_voice_flag] unknown error",  flush=True)
      return False, None 

################################################################################
def lcm_display_normal_info():
    try:
      # display date
      app.lcm_api.lcm_date()
    
      # display temperature
      # temperature format : -9 ~ 99
      temperature=get_temperature_status()
      if temperature >= -9 and temperature <= 99:
        app.lcm_api.lcm_temperature(str(temperature))
      else:
        app.lcm_api.lcm_temperature("--")
    except:
      print("[lcm_display_normal_info] LCM opertation failed",  flush=True)
      pass
     
def lcm_clear_and_update():
    try:
      app.lcm_api.lcm_clear() 
      lcm_display_normal_info()
      # to display the border lines ...
      app.lcm_api.lcm_disaster("000")
    except:
      print("[lcm_clear_and_update] LCM opertation failed",  flush=True)
      pass
      
def lcm_display_alarm_info():
    # disaster = "[fire][gas][sos]"
    # disaster = str(random.randrange(2)) + str(random.randrange(2)) + str(random.randrange(2))
    
    #if True:
    try:
      result,alarm_info_obj=app.host_lib.get_alarm_voice_flag()
      loop_list=alarm_info_obj["loop_list"]
      loop_list_size=len(loop_list)
      for i in range(16-loop_list_size):
        loop_list.append("")
      app.lcm_api.lcm_circuit2(*loop_list)

      # disaster = "[fire][gas][sos]"
      disaster=alarm_info_obj["disaster"]
      app.lcm_api.lcm_disaster(disaster)
        
      app.lcm_api.lcm_temperature(str(app.host_lib.get_temperature_status()))
    except:
      print("[lcm_display_alarm_info] LCM opertation failed",  flush=True)
      pass
          
################################################################################
def kill_dhclient_process():
    import subprocess
    from subprocess import check_output
                     
    p = check_output("ps -ef | grep dhclient | grep -v grep | awk '{print $2}'", shell=True)
    pid = p.decode("utf-8").rstrip()
    if pid.strip():
      # replace the newlines with spaces for 'kill' command's parameter
      pid = pid.replace('\n', ' ').replace('\r', '')

      pid_list=pid.split()
      for pp in pid_list:  
        #print("kill pid:%s" %pp)
        subprocess.Popen(["kill","-15",pp])

################################################################################
def kill_wvdial_process():
    import subprocess
    from subprocess import check_output
                     
    p = check_output("ps -ef | grep wvdial | grep -v grep | awk '{print $2}'", shell=True)
    pid = p.decode("utf-8").rstrip()
    if pid.strip():
      # replace the newlines with spaces for 'kill' command's parameter
      pid = pid.replace('\n', ' ').replace('\r', '')

      pid_list=pid.split()
      for pp in pid_list:  
        #print("kill pid:%s" %pp)
        subprocess.Popen(["kill","-15",pp])

################################################################################
# check host environment status: power, temperature
def check_env_status():
    print("[check_env_status]",  flush=True)
    
    power_source=get_power_source()
    power_status=get_power_status()
    temperature_status=get_temperature_status()
    print("[check_env_status] host power source=%d, battery=%d%%, temperature=%d(C)" %(power_source, power_status, temperature_status),  flush=True)
    
    previous_power_source=power_source # just initial value
    # update value to KeepAlive.json
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
        
        # get previous status
        previous_power_source=int(keepalive_obj["power_source"])
        
    except IOError:
      print ("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False
    except:
      print ("[KeepAlive.json] unknown error.",  flush=True)
      return False
    
    keepalive_obj["power_source"]=str(power_source)
    keepalive_obj["power_status"]=str(power_status)
    keepalive_obj["temperature_status"]=str(temperature_status)
    with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
      json.dump(file_content, f)
    
    # power_source: 1 -> 0 : A, GA01
    if previous_power_source==1 and power_source==0:
      alarm_obj=dict()
      alarm_obj["type_id"]="d01"
      alarm_obj["alert_catalog"]="A"
      alarm_obj["abnormal_catalog"]="GA01"
      alarm_obj["device_id"]=get_host_mac()
      #print("alarm_obj:" + str(alarm_obj),  flush=True)
      obj = app.server_lib.trigger_to_server(alarm_obj)
      if obj['status']==200:
        trigger_result=True
      else:  
        trigger_result=False
      
      
    # power_source: 0 -> 1 : G, GG01
    if previous_power_source==0 and power_source==1:
      alarm_obj=dict()
      alarm_obj["type_id"]="d01"
      alarm_obj["alert_catalog"]="G"
      alarm_obj["abnormal_catalog"]="GG01"
      alarm_obj["device_id"]=get_host_mac()
      #print("alarm_obj:" + str(alarm_obj),  flush=True)
      obj = app.server_lib.trigger_to_server(alarm_obj)
      if obj['status']==200:
        trigger_result=True
      else:  
        trigger_result=False
      
    
    # for TemperatureAlarm and PowerAlarm: trigger alarm just once
    # if the value is back to normal, then clear the flag
    global trigger_alarm_flag
    try:
      if temperature_status < int(trigger_alarm_flag["TemperatureAlarm"]):
        trigger_alarm_flag.pop("TemperatureAlarm", None)
    except:
      print("[check_env_status] TemperatureAlarm TEST",  flush=True)
      pass
    try:
      if power_status > int(trigger_alarm_flag["PowerAlarm"]):
        trigger_alarm_flag.pop("PowerAlarm", None)
    except:
      print("[check_env_status] PowerAlarm TEST",  flush=True)
      pass
    print("[check_env_status] trigger_alarm_flag=%s" %(str(trigger_alarm_flag)),  flush=True)
    
    # if temperature is alarmed before, ignore the following alarms
    if "TemperatureAlarm" in trigger_alarm_flag:
      print("[check_env_status] TemperatureAlarm is already triggered. Ignore this event!\n",  flush=True)
    else:        
      # check if temperature alarm should be triggered
      if temperature_status > int(keepalive_obj["temperature_control"]):
          print ("[check_env_status] host temperature alarm(%d > %s(C)) !" %(temperature_status, keepalive_obj["temperature_control"]),  flush=True)
          trigger_alarm_flag["TemperatureAlarm"]=temperature_status 

          print("[check_env_status] make sound !!!\n\n")
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
          app.host_lib.set_alarm_voice_flag("0", 4)

          # display alarm info on LCM
          app.host_lib.lcm_display_alarm_info()
          
          print("[check_env_status][TemperatureAlarm] trigger LED !",  flush=True)
          app.host_lib.set_led(LED6_ALM_R, 0.5)
          
          alarm_obj=dict()
          alarm_obj["type_id"]="d01"
          alarm_obj["abnormal_value"]=str(temperature_status)
          alarm_obj["alert_catalog"]="A"
          alarm_obj["abnormal_catalog"]="GA07"
          alarm_obj["setting_value"]=str(keepalive_obj["temperature_control"])
          alarm_obj["device_id"]=get_host_mac()
          #print("alarm_obj:" + str(alarm_obj),  flush=True)
          obj = app.server_lib.trigger_to_server(alarm_obj)
          if obj['status']==200:
            trigger_result=True
          else:  
            trigger_result=False

          # notify IPCam to upload video
          app.ipcam_lib.ask_ipcam_to_upload_video(app.host_lib.set_event_id())
      else:
          print ("[check_env_status] host temperature is ok (%d < %s(C))" %(temperature_status, keepalive_obj["temperature_control"]),  flush=True)
          pass
            
    # if power is alarmed before, ignore the following alarms
    if "PowerAlarm" in trigger_alarm_flag:
      print("[check_env_status] PowerAlarm is already triggered. Ignore this event!\n",  flush=True)
    else:        
      # check if power alarm should be triggered
      if power_status < int(keepalive_obj["low_power_control"]):
          print ("[check_env_status] host low power alarm (%d%% < %s%%) !" %(power_status, keepalive_obj["low_power_control"]),  flush=True) 
          trigger_alarm_flag["PowerAlarm"]=power_status
          print("[check_env_status] No sound and led for PowerAlarm\n\n",  flush=True)          
      
          alarm_obj=dict()
          alarm_obj["type_id"]="d01"
          alarm_obj["abnormal_value"]=str(power_status)
          alarm_obj["alert_catalog"]="A"
          alarm_obj["abnormal_catalog"]="GA08"
          alarm_obj["setting_value"]=str(keepalive_obj["low_power_control"])
          alarm_obj["device_id"]=get_host_mac()
          #print("alarm_obj:" + str(alarm_obj),  flush=True)
          obj = app.server_lib.trigger_to_server(alarm_obj)
          if obj['status']==200:
            trigger_result=True
          else:  
            trigger_result=False
      else:
        print ("[check_env_status] host battery is ok (%d%% > %s%%)" %(power_status, keepalive_obj["low_power_control"]),  flush=True)
        pass
    
################################################################################
def check_switch_without_network():
    result=True
    hostsetting_obj=open_hostsetting_json_file()
    try:
      # 0:no, 1:yes
      if hostsetting_obj["switch_without_network"] == "0":
        result=False
    except:
      pass    
    
    return result
    
################################################################################
def host_security_set():
    print("[host_security_set]",  flush=True)
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      print("[host_security_set] FAILED.",  flush=True)
      return False 
    
    # check if security is already set
    if app.host_lib.check_host_alarm_set(keepalive_obj):
      print("[host_security_set] security is already set.\n\n",  flush=True)
      
      # if alarm_voice_flag is not set, make voice prompt 
      flag,obj=app.host_lib.get_alarm_voice_flag()
      if flag==False:
        audio_obj=app.audio.Audio() 
        audio_obj.play(app.host_lib.get_service_dir() + "already_set.mp3")
      else:
        print("[host_security_set] alarm_voice_flag is set ==> do NOT interrupt alarm(event_happen) voice !\n\n",  flush=True)
        pass
        
      return True
    
    # check resetting status of reeds
    if app.device_lib.check_all_sensor_resetting_status() == False:
      print("[host_security_set] FAILED.",  flush=True)
      return False
    
    # check current connection status and switch_without_network config
    try:
      hostsetting_obj=open_hostsetting_json_file() 
      if keepalive_obj["now_connection_status"]=="1" and hostsetting_obj["switch_without_network"]=="0":
        print("[host_security_set] network fails and switch_without_network is NOT allowed !",  flush=True)
        return False
    except:
      pass
        
    signal_obj={"type_id": "d01"}
    if True:
      print("new host state=set",  flush=True)
      security_set='1'
      alert_catalog='S'
      abnormal_catalog='GS01'
      signal_obj["device_id"]=app.host_lib.get_host_mac()
      signal_obj["abnormal_value"]=""
      
    keepalive_obj["start_host_set"]=str(security_set)
    # save new security setting:
    with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
      json.dump(file_content, f)
    
    print("[security set] make sound !!!\n\n",  flush=True)
    audio_obj=app.audio.Audio() 
    audio_obj.play(app.host_lib.get_service_dir() + "system_set_ok.mp3")
    
    app.device_lib.ask_readers_make_sound(6, 1)
         
    app.device_lib.sensor_security_set(1)

    # update partition status
    app.host_lib.update_partition_status("1")

    app.host_lib.host_setunset_gpio("1")
                   

    # binding IPCams
    app.ipcam_lib.ipcam_bind("0")
    print("IPCam binding~~~")

    # PIR setting
    app.ipcam_lib.PIR_setting()
               
    # trigger to server
    signal_obj["alert_catalog"]=alert_catalog
    signal_obj["abnormal_catalog"]=abnormal_catalog
    
    app.host_lib.set_event_id()
    
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
        trigger_result=True
        app.server_lib.send_keepalive_to_server()
    else:  
        trigger_result=False
    
    #return trigger_result
    return True
    

def host_security_unset():
    print("[host_security_unset]",  flush=True)
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except IOError:
      print("[KeepAlive.json] doesn't exist. The account is NOT opened!",  flush=True)
      return False 
    except:
      print("[host_security_unset] unknown error...",  flush=True)
      return False
    
    # check if security is already unset
    if not app.host_lib.check_host_alarm_set(keepalive_obj):
      print("[host_security_unset] security is already unset.",  flush=True)
      audio_obj=app.audio.Audio() 
      audio_obj.stop()
      audio_obj.play(app.host_lib.get_service_dir() + "already_unset.mp3")
      return True

    # check current connection status and switch_without_network config
    try:
      hostsetting_obj=open_hostsetting_json_file() 
      if keepalive_obj["now_connection_status"]=="1" and hostsetting_obj["switch_without_network"]=="0":
        print("[host_security_unset] network fails and switch_without_network is NOT allowed !",  flush=True)
        return False
    except:
      pass
      
    print("new host state=unset",  flush=True)
    security_set='0'  
    
    keepalive_obj["start_host_set"]=str(security_set)
    # save new security setting:
    with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
      json.dump(file_content, f)

    audio_obj=app.audio.Audio()
    audio_obj.play(app.host_lib.get_service_dir() + "system_unset.mp3")
    app.device_lib.ask_readers_make_sound(4, 1)
        
    app.device_lib.sensor_security_set(0)
    
    # update partition status
    app.host_lib.update_partition_status("0")
    
    app.host_lib.host_setunset_gpio("0")
    # unbind IPCams
    app.ipcam_lib.ipcam_bind("1")
    print("IPCam unbinding!!!")
    
    print("[host_security_unset] stop LED.",  flush=True)
    app.host_lib.off_led(LED6_ALM_R)
    
      
    app.host_lib.stop_coupling_operation()
    
    signal_obj={"type_id": "d01"}
    alert_catalog='S'
    abnormal_catalog='GS02'
    signal_obj["device_id"]=app.host_lib.get_host_mac()
    signal_obj["abnormal_value"]=""
    signal_obj["alert_catalog"]=alert_catalog
    signal_obj["abnormal_catalog"]=abnormal_catalog
    
    app.host_lib.set_event_id()
    
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
        signal_obj["device_id"]=app.host_lib.get_host_mac()
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
        signal_obj["device_id"]=app.host_lib.get_host_mac()
        signal_obj["alert_catalog"]=alert_catalog
        signal_obj["abnormal_catalog"]=abnormal_catalog
        obj=app.server_lib.trigger_to_server(signal_obj)

    #return trigger_result
    return True


################################################################################
def update_partition_status(status_value):
    print("[update_partition_status] status_value=%s" %str(status_value),  flush=True)
    
    try:
      with open(app.views.get_json_file_dir() + "Partition.json", "r") as f:
        partition_file_content=json.load(f)
        total_partition_list=partition_file_content["Partition"]
        for part in total_partition_list:
          part["status"]=str(status_value)
    except IOError:
      print("[update_partition_status][Partition.json] file doesn't exist. skip it",  flush=True)
      return False
    except:
      print ("[update_partition_status] unknown error",  flush=True)
      return False
      
    # save partition status
    with open(app.views.get_json_file_dir() + "Partition.json", "w") as f:
      json.dump(partition_file_content, f)

################################################################################
LOGIN_USERNAME=""
USER_TOKEN=""

def save_login_username(username):
    global LOGIN_USERNAME
    LOGIN_USERNAME=username
                      

def get_login_username():
    return LOGIN_USERNAME

def save_user_token(token):
    global USER_TOKEN
    USER_TOKEN=token
                      
def get_user_token():
    return USER_TOKEN
    

################################################################################
# how to remove a sensor:
#   user: select sensor to be removed in APP
#   APP: send delete request 
#   host: remove the corresponding item (by device_id) in json files
def delete_device_list(dev_id_list):
    for device_id in dev_id_list:
      delete_device(device_id)
      
      
def delete_device(dev_id):
    print("[delete_device][%s]" %(dev_id),  flush=True)
    
    # delete the loop record in Loop.json if the device is bound to a certain loop
    loop_id=str(app.device_lib.check_loop_id_from_device_id(dev_id))
    if loop_id != "" :
      table_name="Loop"
      try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
          file_content=json.load(f)
          loop_list=file_content[table_name]
          for item in loop_list:
            if item["loop_id"]==loop_id:
              loop_list.remove(item)
              break
              
        with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
          json.dump(file_content, f)
            
      except:
        print("[delete_device][%s] unknown error. skip table search-and-delete" %(table_name),  flush=True)
        pass
           
    table_list=["Device", "LoopDevice", "HostCamPir", "RemoteControl_Partition", 
      "RemoteControl_PartitionReader", "RemoteControl_LockReader", "WLReadSensor", 
      "WLDoubleBondBTemperatureSensor", "WLReedSensor", "WLIPCam"]
    for table_name in table_list:
      try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
          file_content=json.load(f)
          device_list=file_content[table_name]
          item_found=False
          for item in device_list:
            if item["device_id"]==dev_id:
              device_list.remove(item)
              item_found=True
              break
          
          # delete extend_device#1      
          for item in device_list:
            #print("[delete_device] item=%s" %str(item),  flush=True)
            #print("[delete_device] extend_dev1=%s" %(dev_id + "-1"),  flush=True)
            if item["device_id"] == dev_id + "-1":
              print("[delete_device][%s] extend_dev1(%s) is FOUND !!!!!!" %(table_name, dev_id + "-1"),  flush=True)
              device_list.remove(item)
              item_found=True
              break
          
          # delete extend_device#2    
          for item in device_list:
            if item["device_id"] == dev_id + "-2":
              print("[delete_device][%s] extend_dev2(%s) is FOUND !!!!!!" %(table_name, dev_id + "-2"),  flush=True)
              device_list.remove(item)
              item_found=True
              break
              
        if item_found:              
          with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
            json.dump(file_content, f)    

      except:
        print("[delete_device][%s] unknown error. skip table search-and-delete" %(table_name),  flush=True)
        pass  
    
    
    # remove readsensor-related setting("reader_id") 
    '''
    RemoteControl_PartitionReader.json: 
      {"RemoteControl_PartitionReader": [{"device_id": "5149013183024120", "version": "", "reader_array": [{"reader_id": "5149013205209794", "partition_id": ["2", "1"]}], "time": "2017-02-09 16:06:59.133234"}]}
    '''
    table_name="RemoteControl_PartitionReader"
    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
          file_content=json.load(f)
          device_list=file_content[table_name]
          item_found=False
          for item in device_list:
            for reader_obj in item["reader_array"]:
              if reader_obj["reader_id"]==dev_id:
                item["reader_array"].remove(reader_obj)
                item_found=True
                break
                
        if item_found:              
          with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
            json.dump(file_content, f)    
    except:
        print("[delete_device][%s] unknown error. skip table search-and-delete" %(table_name),  flush=True)
        pass  

    '''
    RemoteControl_LockReader.json: 
      {"RemoteControl_LockReader": [{"reader_id": ["5149013205209794"], "device_id": "5149013183024120", "version": "", "time": "2017-02-13 17:54:53.272186"}]}
    '''
    table_name="RemoteControl_LockReader"
    try:
        with open(app.views.get_json_file_dir() + table_name + ".json", "r") as f:
          file_content=json.load(f)
          device_list=file_content[table_name]
          item_found=False
          for item in device_list:
            for reader_id in item["reader_id"]:
              if reader_id==dev_id:
                item["reader_id"].remove(reader_id)
                item_found=True
                break
                
        if item_found:              
          with open(app.views.get_json_file_dir() + table_name + ".json", "w") as f:
            json.dump(file_content, f)    
    except:
        print("[delete_device][%s] unknown error. skip table search-and-delete" %(table_name),  flush=True)
        pass  

    # only send del_device request to IOT service (IOT DB sync is supported by Sam, 20170215)
    data_obj=dict()
    data_obj["device_id"]=dev_id
    app.server_lib.send_request_to_server("server_del_device", data_obj)
    
    
################################################################################
# set default PIR(enable) in HostCamPir.json only if the current setting is absent! 
def init_host_cam_pir(cam_dev_id):
    print("[init_host_cam_pir][%s]" %(cam_dev_id),  flush=True)         
    '''
    {"HostCamPir":[	
      {"device_id":"xx","enable_pir":"1"},
      ...
    ]}
    '''
    try:
      with open(app.views.get_json_file_dir() + "HostCamPir.json", "r") as f:  
        pir_file_content=json.load(f)
        pir_file_content_list=pir_file_content["HostCamPir"]
    except:
      print ("[ipcam_open_port][HostCamPir.json] create EMPTY list",  flush=True)
      pir_file_content=dict()
      pir_file_content["HostCamPir"]=list()
      pir_file_content_list=pir_file_content["HostCamPir"]
    
    item_found=False
    for item in pir_file_content_list:
      if item["device_id"]==cam_dev_id:
        item_found=True
        break
        
    if item_found==False:                               
      print("[init_host_cam_pir][%s] is absent => add default setting." %(cam_dev_id),  flush=True)
      setting_obj=dict()  
      setting_obj["device_id"]=cam_dev_id
      setting_obj["enable_pir"]="1"
      pir_file_content_list.append(setting_obj)
      with open(app.views.get_json_file_dir() + "HostCamPir.json", "w") as f:
        json.dump(pir_file_content, f)
    else:
      print("[init_host_cam_pir][%s] already exist => skip setting." %(cam_dev_id),  flush=True)
      pass

################################################################################      
def send_reboot_signal():
    signal_obj=dict()
    signal_obj["type_id"]="d01"
    signal_obj["device_id"]=app.host_lib.get_host_mac()
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="GA02"
    
    app.host_lib.set_event_id()
    
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
      print("trigger to server ok",  flush=True)
      pass

################################################################################
# sen host 'reset to default' event
def send_host_reset_signal():
    print ("[send_host_reset_signal][%s]" %(str(datetime.datetime.now())), flush=True)
    signal_obj=dict()
    signal_obj["type_id"]="d01"
    signal_obj["device_id"]=get_host_mac()
    signal_obj["alert_catalog"]="O"
    signal_obj["abnormal_catalog"]="GO01"
    
    app.host_lib.set_event_id()
    
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
      print("trigger to server ok",  flush=True)
      pass

################################################################################
def restore_alarm_output_func(*args):
    value=int(args[0])
    print ("[restore_alarm_output_func][%s] value=%d" %(str(datetime.datetime.now()), value), flush=True)
    set_gpio(13, value)

#  
def init_alarm_output():
    hostsetting_obj=open_hostsetting_json_file()
    try:
      alarm_contact_output_set = hostsetting_obj["alarm_contact_output_set"]
    except:
      print("[init_alarm_output] alarm_contact_output_set error.", flush=True)
      alarm_contact_output_set = "1"  

    if alarm_contact_output_set == "0":
      print("[init_alarm_output] alarm_contact_output is DISABLED !", flush=True)
      return 
    
    try:
      alarm_contact_output_action_status = hostsetting_obj["alarm_contact_output_action_status"]
    except:
      print("[init_alarm_output] alarm_contact_output_action_status error.", flush=True)
      alarm_contact_output_action_status = "1"
        
    if alarm_contact_output_action_status == "0":
      # 0=NO->NC
      set_gpio(13, 0)
    else:
      # 1=NC->NO
      set_gpio(13, 1)

# 
def launch_alarm_output_action():
    print("[launch_alarm_output_action][%s] starts" %(str(datetime.datetime.now())), flush=True)
    hostsetting_obj=open_hostsetting_json_file()
    try:
      alarm_contact_output_set = hostsetting_obj["alarm_contact_output_set"]
    except:
      print("[launch_alarm_output_action] alarm_contact_output_set error.", flush=True)
      alarm_contact_output_set = "1"  

    if alarm_contact_output_set == "0":
      print("[launch_alarm_output_action] alarm_contact_output is DISABLED !", flush=True)
      return 
    
    try:
      alarm_contact_output_action_time = hostsetting_obj["alarm_contact_output_action_time"]
    except:
      print("[launch_alarm_output_action] alarm_contact_output_action_time error.", flush=True)
      alarm_contact_output_action_time = "30"  
      
    try:
      alarm_contact_output_action_status = hostsetting_obj["alarm_contact_output_action_status"]
    except:
      print("[launch_alarm_output_action] alarm_contact_output_action_status error.", flush=True)
      alarm_contact_output_action_status = "1"
        
    if alarm_contact_output_action_status == "0":
      # 0=NO->NC
      set_gpio(13, 1)
      timer = threading.Timer(int(alarm_contact_output_action_time), restore_alarm_output_func, [0])
      timer.start()               
    else:
      # 1=NC->NO
      set_gpio(13, 0)
      timer = threading.Timer(int(alarm_contact_output_action_time), restore_alarm_output_func, [1])
      timer.start()
    
    
################################################################################
def restore_12v_output_func(*args):
    value=int(args[0])
    print ("[restore_12v_output_func][%s] value=%d" %(str(datetime.datetime.now()), value), flush=True)
    set_gpio(13, value)
 
def init_12v_output():
    print("[init_12v_output][%s] starts" %(str(datetime.datetime.now())), flush=True)
    hostsetting_obj=open_hostsetting_json_file()
    try:
      v_contact_output_set = hostsetting_obj["v_contact_output_set"]
    except:
      print("[init_12v_output] v_contact_output_set error.", flush=True)
      v_contact_output_set = "1"  

    if v_contact_output_set == "0":
      print("[init_12v_output] 12v_contact_output is DISABLED !", flush=True)
      return 
    
    # just in case 12v_output and alarm_output are enabled at the same time
    try:
      if hostsetting_obj["alarm_contact_output_set"]=="1":
        print("[init_12v_output] alarm_contact_output is ebabled => 12v_contact_output operation is ignored !", flush=True)
        return
    except:
      print("[init_12v_output] get alarm_contact_output_set error => 12v_contact_output operation is ignored !", flush=True)
      return  

    try:
      v_output_contact_action_status = hostsetting_obj["v_output_contact_action_status"]
    except:
      print("[init_12v_output] v_output_contact_action_status error.", flush=True)
      v_output_contact_action_status = "1"
        
    if v_output_contact_action_status == "0":
      # 0=NO->NC
      set_gpio(13, 0)
    else:
      # 1=NC->NO
      set_gpio(13, 1)
    

def launch_12v_output_action():
    print("[launch_12v_output_action][%s] starts" %(str(datetime.datetime.now())), flush=True)
    hostsetting_obj=open_hostsetting_json_file()
    try:
      v_contact_output_set = hostsetting_obj["v_contact_output_set"]
    except:
      print("[launch_12v_output_action] v_contact_output_set error.", flush=True)
      v_contact_output_set = "1"  

    if v_contact_output_set == "0":
      print("[launch_12v_output_action] 12v_contact_output is DISABLED !", flush=True)
      return 
    
    # just in case 12v_output and alarm_output are enabled at the same time
    try:
      if hostsetting_obj["alarm_contact_output_set"]=="1":
        print("[launch_12v_output_action] alarm_contact_output is ebabled => 12v_contact_output operation is ignored !", flush=True)
        return
    except:
      print("[launch_12v_output_action] get alarm_contact_output_set error => 12v_contact_output operation is ignored !", flush=True)
      return  

    try:
      v_output_contact_action_time = hostsetting_obj["v_output_contact_action_time"]
    except:
      print("[launch_12v_output_action] v_output_contact_action_time error.", flush=True)
      v_output_contact_action_time = "3"  
      
    try:
      v_output_contact_action_status = hostsetting_obj["v_output_contact_action_status"]
    except:
      print("[launch_12v_output_action] v_output_contact_action_status error.", flush=True)
      v_output_contact_action_status = "1"
        
    if v_output_contact_action_status == "0":
      # 0=NO->NC
      set_gpio(13, 1)
      timer = threading.Timer(int(v_output_contact_action_time), restore_12v_output_func, [0])
      timer.start()               
    else:
      # 1=NC->NO
      set_gpio(13, 0)
      timer = threading.Timer(int(v_output_contact_action_time), restore_12v_output_func, [1])
      timer.start()

################################################################################
# host setunset gpio config : { "GPIO2_22", "IO16", 86, -1, -1}, 
def host_setunset_gpio(state):
    print ("[host_setunset_gpio][%s] state=%s" %(str(datetime.datetime.now()), state), flush=True)
    # TODO: not sure 0/1
    if state=="1":
      set_gpio(14,0)
    else:
      set_gpio(14,1)
    
      
      
################################################################################
# send network init event
'''
def send_network_init_event():
    print("[send_network_init_event][%s]" %(str(datetime.datetime.now())), flush=True)
    try:
      hostsetting_obj=app.host_lib.open_hostsetting_json_file()
      network_set=hostsetting_obj["network_set"]
    except:
      print("[send_network_init_event] unknown error => skip this operation", flush=True)
      return False
      
    alarm_obj=dict()
    alarm_obj["type_id"]="d01"
    alarm_obj["device_id"]=get_host_mac()
    alarm_obj["alert_catalog"]="A"  
    if network_set == "1" or network_set == "2" or network_set == "3":  
      alarm_obj["abnormal_catalog"]="GA13"
    elif network_set == "6":  
      alarm_obj["abnormal_catalog"]="GA11"      
    else:
      print("[send_network_init_event] network_set=%s => skip this operation" %(network_set), flush=True)
      return False
    
    #print("alarm_obj:" + str(alarm_obj),  flush=True)
    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
      trigger_result=True
    else:  
      trigger_result=False
      
    return trigger_result      
'''

################################################################################
# send network recovery event
'''
def send_network_recovery_event():
    print("[send_network_recovery_event][%s]" %(str(datetime.datetime.now())), flush=True)
    try:
      hostsetting_obj=app.host_lib.open_hostsetting_json_file()
      network_set=hostsetting_obj["network_set"]
    except:
      print("[send_network_recovery_event] unknown error => skip this operation", flush=True)
      return False
      
    alarm_obj=dict()
    alarm_obj["type_id"]="d01"
    alarm_obj["device_id"]=get_host_mac()
    alarm_obj["alert_catalog"]="G"  
    if network_set == "1" or network_set == "2" or network_set == "3":
      alarm_obj["abnormal_catalog"]="GG03"
    elif network_set == "6":  
      alarm_obj["abnormal_catalog"]="GG04"      
    else:
      print("[send_network_recovery_event] network_set=%s => skip this operation" %(network_set), flush=True)
      return False
    
    #print("alarm_obj:" + str(alarm_obj),  flush=True)
    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
      trigger_result=True
    else:  
      trigger_result=False
      
    return trigger_result      
'''

################################################################################
# send network lost event
# note: this event is actulally preserved, and then sent out after network works again ...
'''
def send_network_lost_event():
    print("[send_network_lost_event][%s]" %(str(datetime.datetime.now())), flush=True)
    try:
      hostsetting_obj=app.host_lib.open_hostsetting_json_file()
      network_set=hostsetting_obj["network_set"]
    except:
      print("[send_network_lost_event] unknown error => skip this operation", flush=True)
      return False
      
    alarm_obj=dict()
    alarm_obj["type_id"]="d01"
    alarm_obj["device_id"]=get_host_mac()
    alarm_obj["alert_catalog"]="A"  
    if network_set == "1":  
      alarm_obj["abnormal_catalog"]="GA15"
    elif network_set == "6":  
      alarm_obj["abnormal_catalog"]="GA16"      
    else:
      print("[send_network_lost_event] network_set=%s => skip this operation" %(network_set), flush=True)
      return False
            
    #print("alarm_obj:" + str(alarm_obj),  flush=True)
    obj = app.server_lib.trigger_to_server(alarm_obj)
    if obj['status']==200:
      trigger_result=True
    else:  
      trigger_result=False
      
    return trigger_result      
'''

################################################################################      
def send_enter_hostsetting_signal():
    signal_obj=dict()
    signal_obj["type_id"]="d01"
    signal_obj["device_id"]=get_host_mac()
    signal_obj["alert_catalog"]="A"
    signal_obj["abnormal_catalog"]="GA06"
    signal_obj["abnormal_value"]="user_XXX"

    app.host_lib.set_event_id()
    
    obj=app.server_lib.trigger_to_server(signal_obj)
    if obj['status']==200:
      print("trigger to server ok",  flush=True)
      pass

################################################################################
def sync_server_address_to_keepalive():
    print("[sync_server_address_to_keepalive][%s]" %(str(datetime.datetime.now())), flush=True)
    try:
      with open(app.views.get_json_file_dir() + "ServerAddress.json", "r") as f:
        data_obj=json.load(f)
        server_address_dict=data_obj["ServerAddress"][0]
    except:
      print("[sync_server_address_to_keepalive][ServerAddress.json] unknown error => return False.", flush=True)
      return False

    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        keepalive_file_content=json.load(f)
        keepalive_obj=keepalive_file_content["KeepAlive"][0]
    except:
      print("[sync_server_address_to_keepalive][KeepAlive.json] unknown error => return False")
      return False
            
    try:    
      keepalive_obj["send_message_major_url"]=server_address_dict["server1"]["addr"]
    except:
      print("[sync_server_address_to_keepalive] server1 error")
    
    try:    
      keepalive_obj["send_message_minor_url"]=server_address_dict["server2"]["addr"]  
    except:
      print("[sync_server_address_to_keepalive] server2 error")
        
    try:    
      keepalive_obj["send_message_third_url"]=server_address_dict["server3"]["addr"]
    except:
      print("[sync_server_address_to_keepalive] server3 error")
          
    try:    
      keepalive_obj["send_message_fourth_url"]=server_address_dict["server4"]["addr"]
    except:
      print("[sync_server_address_to_keepalive] server4 error")
        
    # save to json file
    with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
      json.dump(keepalive_file_content, f)

    return True

################################################################################
def update_hostsetting_json_file(field, value):
    print("[update_hostsetting_json_file] set %s=%s" %(field, str(value)),  flush=True)
    try:
      with open(app.views.get_json_file_dir() + "HostSetting.json", "r") as f:
        file_content=json.load(f)
        hostsetting_obj=file_content["HostSetting"][0]
    except: 
      print("[update_hostsetting_json_file] unknown error => skip this operation",  flush=True)
      return
  
    try:
      hostsetting_obj[field]=value
      with open(app.views.get_json_file_dir() + "HostSetting.json", "w") as f:
        json.dump(file_content, f)
    except: 
      print("[update_hostsetting_json_file] unknown error => update failed.",  flush=True)
      pass
    

################################################################################
def update_keepalive_json_file(field, value):
    print("[update_keepalive_json_file] set %s=%s" %(field, str(value)),  flush=True)
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except: 
      print("[update_keepalive_json_file] unknown error => skip this operation",  flush=True)
      return
  
    try:
      keepalive_obj[field]=value
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "w") as f:
        json.dump(file_content, f)
    except: 
      print("[update_keepalive_json_file] unknown error => update failed.",  flush=True)
      pass
    

################################################################################
def get_keepalive_json_property(field):
    print("[get_keepalive_json_property] get value of '%s'" %(field),  flush=True)
    try:
      with open(app.views.get_json_file_dir() + "KeepAlive.json", "r") as f:
        file_content=json.load(f)
        keepalive_obj=file_content["KeepAlive"][0]
    except: 
      print("[get_keepalive_json_property][KeepAlive.json] unknown error => return empty string",  flush=True)
      return ""
  
    try:
      return keepalive_obj[field]
    except: 
      print("[get_keepalive_json_property] unknown error => return empty string",  flush=True)
      return ""
      
################################################################################
def battery_config(value):
    print("[battery_config] value=%s" %(value),  flush=True)
    app.host_lib.update_keepalive_json_file("low_power_control", value)
    app.host_lib.update_hostsetting_json_file("battery_low_power_set", value)
    
    try:
      hostsetting_obj=open_hostsetting_json_file()
      # update host setting to server
      app.server_lib.update_to_server("HostSetting", hostsetting_obj)            
    except:
      print("[battery_config] unknown error => skip this operation",  flush=True)
      pass
      
      