 # FileName: led_api.py
# Author  : Alex
# Version : 1.0 Nov 22, 2016  Beagle Bone Black version
#  

import time
import datetime
import os
import threading
import Adafruit_BBIO.ADC as ADC
import Adafruit_BBIO.GPIO as GPIO

#--- Initialize ---
BLINK_SPEED=0.5
led_mask=0xff
led_off=0xff


LED8_ALM_G = 8
LED7_ALM_B = 7
LED6_ALM_R = 6
LED5_SET_G = 5
LED4_SET_B = 4
LED3_SET_R = 3
LED2_SYS_B = 2
LED1_SYS_R = 1
on_list = {0:0xfe, 1:0xfd, 2:0xfb , 3:0xf7 , 4:0xef , 5:0xdf , 6:0xbf , 7:0x7f}
off_list = {0:0x01, 1:0x02, 2:0x04 , 3:0x08 , 4:0x10 , 5:0x20 , 6:0x40 , 7:0x80}
led_timer = [180, 0.5, 0, 0.5, 0.5, 0.5, 0.5 ,0.5 ,0.5] 
led_on_off = [0,0,0,0,0,0,0,0,0]#sets the led to desired value (1=on,0=off)
led_curr_timer = [0, 0, 0, 0, 0, 0, 0 ,0 ,0] 
led_off = [0, 0, 0, 0, 0, 0, 0 ,0 ,0] 
led_on_period = [0, 0, 0, 0, 0, 0, 0 ,0 ,0]

def SetLedBinary(bit):
      global led_mask     
      led=on_list[bit]     
      led_mask = led_mask & led     
      #print("on bit=%d,led_mask=0x%x" % (bit,led_mask))
      
def OffLedBinary(bit):
      global led_mask     
      led=off_list[bit]    
      led_mask = led_mask | led     
      #print("off bit=%d,led_mask=0x%x" % (bit,led_mask))
               
def SetLed(lednum,time):
      led_on_off[lednum-1]=1    
      led_timer[lednum-1]=time
      led_curr_timer[lednum-1]=led_timer[lednum-1]  
      print("led_on_off(%d)=%d" % (lednum,led_on_off[lednum-1]))
      # added, zl
      led_off[lednum-1]=led_on_period[lednum-1]
      
def OffLed(lednum):
      led_on_off[lednum-1]=0
      led_curr_timer[lednum-1]=0
      OffLedBinary(lednum-1)
      print("led_on_off(%d)=%d" % (lednum,led_on_off[lednum-1]))


class EventControl (threading.Thread):
    
  def __init__(self,alarmCb,setUnsetCb,dateButtonCb,timeButtonCb,funcButtonCb,resetButtonCb,coverOpenCb):
        threading.Thread.__init__(self)
        self.threadID = 2
        self.name = "event_thread"    
        self.alarmCb = alarmCb
        self.setUnsetCb = setUnsetCb
        
        self.dateButtonCb = dateButtonCb
        self.timeButtonCb = timeButtonCb    
        self.funcButtonCb = funcButtonCb
        self.resetButtonCb = resetButtonCb
        self.coverOpenCb = coverOpenCb
        
        ADC.setup()
        os.system('i2cset -f -y 0 0x24 0x9 0x01')
        self.running = True
    
       
  def run(self):    
        print("[%s][EventControl] test 1" %(str(datetime.datetime.now())),  flush=True)
        alarmState = False
        settingState = False 
        
        dateButtonState = False
        timeButtonState = False 
        funcButtonState = False
        resetButtonState = False
        
        #coverOpenState = False
        coverOpenState = True
        
        # get initial state of adapter
        adapterState = False
        if ADC.read_raw("P9_37") > 500:
            adapterState = True
        
        print("[%s][EventControl] test 2" %(str(datetime.datetime.now())),  flush=True)
        
        # get initial state of battery
        batteryState = False        
        value = ADC.read_raw("P9_39")
        if adapterState == False and value > 500:
            voltage = (value / 4095 * 1.8 * 3)
            if voltage < 3.7:
                batteryState = True
        if adapterState:
            OffLed(LED1_SYS_R)                                             
        elif batteryState :
            SetLed(LED1_SYS_R, 180)
        else:
            SetLed(LED1_SYS_R, 0)
        
        print("[%s][EventControl] test 3" %(str(datetime.datetime.now())),  flush=True)
            
        GPIO.setup("Setting_push", GPIO.IN)
        GPIO.setup("Alarm_push", GPIO.IN)
        print("[%s][EventControl] test 4" %(str(datetime.datetime.now())),  flush=True)
        GPIO.setup('Function_push', GPIO.IN)
        GPIO.setup('Time_push', GPIO.IN)
        GPIO.setup('Date_push', GPIO.IN)
        GPIO.setup('RST_default', GPIO.IN)
        GPIO.setup('install', GPIO.IN)
        
        print("[%s][EventControl] test 5" %(str(datetime.datetime.now())),  flush=True)
        
        #funcbutton_press_timestamp=0
        resetbutton_press_timestamp=0                             
        
        threadLock = threading.Lock()
        while(self.running):
            threadLock.acquire()

            if GPIO.input("Setting_push") == 1:
                settingValue = False 
            else:
                settingValue = True 
              
            if GPIO.input("Alarm_push") == 1:
                alarmValue = False 
            else:
                alarmValue = True
          
          
            if GPIO.input("Function_push") == 1:
                funcButtonValue = False
            else:
                funcButtonValue = True
                
            if GPIO.input("Time_push") == 1:
                timeButtonValue = False
            else:
                timeButtonValue = True
                
            if GPIO.input("Date_push") == 1:
                dateButtonValue = False
            else:
                dateButtonValue = True
                
            if GPIO.input("RST_default") == 1:
                resetButtonValue = False
            else:
                resetButtonValue = True
            
            if GPIO.input("install") == 0:
                coverOpenValue = False
            else:
                coverOpenValue = True
                        
            value = ADC.read_raw("P9_37")
            #print("power source value=%d" %(value), flush=True)
            if value < 500:
                adapterValue = False
            else:
                adapterValue = True
                
            value = ADC.read_raw("P9_39")
            #print("power status value=%d" %(value), flush=True)
            if adapterValue == False and value > 500:
                voltage = (value / 4095 * 1.8 * 3)
                if voltage < 3.7:
                  batteryValue = True                   
                else:
                  batteryValue = False
            else:
                batteryValue = False
            
            
            #print ("LEDControl")
            threadLock.release()
            
            if alarmState != alarmValue:
              alarmState = alarmValue
              self.alarmCb(alarmValue)
             
            if settingState != settingValue:
              settingState = settingValue
              if settingValue:
                self.setUnsetCb()
                
            if dateButtonState != dateButtonValue:
              dateButtonState = dateButtonValue
              if dateButtonState:
                self.dateButtonCb()
            
            if timeButtonState != timeButtonValue:
              timeButtonState = timeButtonValue
              if timeButtonState:
                self.timeButtonCb()
            
            if funcButtonState != funcButtonValue:
              funcButtonState = funcButtonValue
              if funcButtonState:
                self.funcButtonCb()
            
            '''
            # long-press detection test, zl
            if funcButtonState != funcButtonValue:
              funcButtonState = funcButtonValue
              if funcButtonState:
                # initial pressed
                funcbutton_press_timestamp=int(time.time())
              else:
                # button released
                funcbutton_press_timestamp=0
            else:
              if funcButtonState:
                # persist pressed
                current_timestamp=int(time.time())
                if funcbutton_press_timestamp > 0 and current_timestamp-funcbutton_press_timestamp > 5 :
                  funcbutton_press_timestamp=0 
                  self.funcButtonCb()
            '''                              
            
            ''''  
            if resetButtonState != resetButtonValue:
              resetButtonState = resetButtonValue
              if resetButtonState:
                self.resetButtonCb(99)
            '''
            # press time detection for reset-button 
            if resetButtonState != resetButtonValue:
              resetButtonState = resetButtonValue
              if resetButtonState:
                # initial pressed
                resetbutton_press_timestamp=float(time.time())
              else:
                # button released
                release_timestamp=float(time.time())
                self.resetButtonCb(release_timestamp-resetbutton_press_timestamp)
            
                  
            if coverOpenState != coverOpenValue:
              coverOpenState = coverOpenValue
              #if coverOpenState:
              self.coverOpenCb(coverOpenState)
              
              
                
            if batteryState != batteryValue:
              batteryState = batteryValue
              if batteryState :
                print("battery low power !",  flush=True)
                SetLed(LED1_SYS_R, 180)
              else:
                print("battery power ok",  flush=True)
                SetLed(LED1_SYS_R, 0)
            
            if adapterState != adapterValue:
              adapterState = adapterValue
              if adapterState:
                print("power adapter is plugged ==> turn off sys-red LED",  flush=True)
                OffLed(LED1_SYS_R)
              else:
                print("power adapter is absent !",  flush=True)
                # make sys-red LED refresh
                batteryState = not batteryState
                
              
            # test, zl
            time.sleep(0.2)
            #time.sleep(0.3)
            
        GPIO.cleanup()

  def stop(self):
      self.running = False
  
   



class LEDTimerControl (threading.Thread):
    
  def __init__(self):
        threading.Thread.__init__(self)
        self.threadID = 3
        self.name = "timer_thread"        
        self.running = True
        
  def run(self):    
        print("[%s][LEDTimerControl] test 1" %(str(datetime.datetime.now())),  flush=True)              
        threadLock = threading.Lock()
        print("[%s][LEDTimerControl] test 2" %(str(datetime.datetime.now())),  flush=True)
        while(self.running):
            threadLock.acquire()
            for i in range(0,9):                
                if led_on_off[i] ==1:   
                    #print("led_curr_timer[%d]=%f" % (i,led_curr_timer[i]))                          
                    if led_curr_timer[i] >0:
                        OffLedBinary(i) 
                        led_curr_timer[i]=led_curr_timer[i]-0.5
                    elif led_curr_timer[i]==0:
                        SetLedBinary(i) 
                                       
                        if led_off[i]>BLINK_SPEED:
                            led_off[i] -= 0.5
                        else:                             
                            led_off[i]=led_on_period[i]
                            led_curr_timer[i]=led_timer[i]
                
            text='i2cset -f -y 1 0x20 '+str(hex(led_mask))
            #print("text=%s" % (text))
            os.system(text)
            
            threadLock.release()           
            
            # test, zl
            #time.sleep(0.1)
            time.sleep(0.5)

      

  def stop(self):
      self.running = False

  