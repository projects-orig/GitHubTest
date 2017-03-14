import Adafruit_BBIO.GPIO as GPIO
import time
import threading

class PanelButton (threading.Thread):

		def __init__(self, alarmCb, setUnsetCb):
				threading.Thread.__init__(self)
				self.threadID = 1
				self.name = "panel_button_thread"
				self.alarmCb = alarmCb
				self.setUnsetCb = setUnsetCb
				self.running = True
        
		def run(self):    
				alarmState = False
				settingState = False 
				GPIO.setup("Setting_push", GPIO.IN)
				GPIO.setup("Alarm_push", GPIO.IN)
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
            
					threadLock.release()
            
					if alarmState != alarmValue:
						alarmState = alarmValue
						self.alarmCb(alarmValue)
            
					if settingState != settingValue:
						settingState = settingValue
						if settingValue:
							self.setUnsetCb()
                
					time.sleep(0.2)

				GPIO.cleanup()

		def stop(self):
			self.running = False

       


