import Adafruit_BBIO.GPIO as GPIO
import time
import os

#--- set IO ---
def IO_set (num,pull):

   text = "IO%d" % (num)
   #print(text)
   if pull==0:
       #print(pull)
       #GPIO.setup(text, GPIO.OUT)
       #time.sleep(0.1)
       GPIO.setup(text, GPIO.OUT)
       GPIO.output(text, GPIO.LOW)
   elif pull==1:
       #print(pull)
       #GPIO.setup(text, GPIO.OUT)
       #time.sleep(0.1)
       GPIO.setup(text, GPIO.OUT)
       GPIO.output(text, GPIO.HIGH)

def cc1310_reset():
    print("[cc1310_reset]",  flush=True)
    # { "GPIO2_1", "CC1310_RST", 65, -1, -1},
    text="65"
    
    # echo 45 > /sys/class/gpio/export
    command='echo {pin} > /sys/class/gpio/export'.format(pin=text)
    print("[cc1310_reset] command=" + command,  flush=True)    
    os.system(command)
     
    command='echo "out" > /sys/class/gpio/gpio{pin}/direction'.format(pin=text)
    print("[cc1310_reset] command=" + command,  flush=True)    
    os.system(command)
    
    command='echo 0 > /sys/class/gpio/gpio{pin}/value'.format(pin=text)    
    print("[cc1310_reset] command=" + command,  flush=True)
    os.system(command)
    
    time.sleep(1)

    command='echo 1 > /sys/class/gpio/gpio{pin}/value'.format(pin=text)    
    print("[cc1310_reset] command=" + command,  flush=True)
    os.system(command)
