import Adafruit_BBIO.GPIO as GPIO
import time


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
  GPIO.setup('CC1310_RST', GPIO.OUT)
  GPIO.output('CC1310_RST', GPIO.LOW)
  time.sleep(1)
  GPIO.output('CC1310_RST', GPIO.HIGH)


