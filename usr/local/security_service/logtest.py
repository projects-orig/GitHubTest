#coding=UTF-8

import os
import sys, time
import signal
import threading, datetime
import random

def logtest_thread():
    interval=0.1
    print('[%s][logtest_thread] starts.' %str(datetime.datetime.now()),  flush=True)
    count=0
    
    while not interrupted:
      print("%06d[%s]: log stress test" %(count, str(datetime.datetime.now())),  flush=True)      
      
      debug_log_rotation("/tmp/logtest_daemon.log")
    
      count+=1
      time.sleep(interval)
    
    print("quit logtest_thread",  flush=True)

################################################################################
# debug log rotation to avoid enormous log file 
def debug_log_rotation(debug_log_file):
    try:
      statinfo = os.stat(debug_log_file)
      if statinfo.st_size > 10240*1024:
        command='cp {current_file_name} {old_file_name}'.format(current_file_name=debug_log_file, old_file_name=debug_log_file + ".1")    
        os.system(command)
        command='echo "debug start:" > {current_file_name}'.format(current_file_name=debug_log_file)    
        os.system(command)
    except FileNotFoundError:
      pass
    except:
      pass  
             

################################################################################           
def stop_all(*args):
    global interrupted
    interrupted=True
    
    
# lauch threads: mqtt, keep-alive
def main_func():
    global interrupted
    interrupted=False
        
    signal.signal(signal.SIGTERM, stop_all)
    signal.signal(signal.SIGQUIT, stop_all)
    signal.signal(signal.SIGINT,  stop_all)  # Ctrl-C
    
    t1=threading.Thread(target=logtest_thread)
    t1.start()
    
    
if __name__ == "__main__":
    main_func()
    sys.exit(0)
    
