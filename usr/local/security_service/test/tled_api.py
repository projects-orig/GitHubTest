import threading
import led_api

#LED9_SYS_G = 9
LED8_ALM_G = 8
LED7_ALM_B = 7
LED6_ALM_R = 6
LED5_SET_G = 5
LED4_SET_B = 4
LED3_SET_R = 3
LED2_SYS_B = 2
LED1_SYS_R = 1

def alarmCb(state):
    print("alarm_trigger", state)

def setUnsetCb():
    print("set_unset_trigger")
    
global led;
led = led_api.EventControl(alarmCb,setUnsetCb)
led.start()
global timer;
timer = led_api.LEDTimerControl()
timer.start()
led_api.SetLed(LED7_ALM_B,0.2)#led number,led speed
led_api.SetLed(LED8_ALM_G,0)#led number,led speed
led_api.SetLed(LED5_SET_G,0.1)#led number,led speed
led_api.SetLed(LED2_SYS_B,1)#led number,led speed
led_api.SetLed(LED4_SET_B,0.5)#led number,led speed