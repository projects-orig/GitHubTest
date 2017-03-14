
import datetime
import os

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

'''
input("press Function_push button and input enter key for next test: ")
value = read_func_button()
print('Function_push press value:%d\n' % int(value))
input("press Function_push button and input enter key for next test: ")
value = read_func_button()
print('Function_push press value:%d\n' % int(value))
print('Function_push test finish\n\n')


input("press Time_push button and input enter key for next test: ")
value = read_time_button()
print('Time_push press value:%d\n' % int(value))
input("press Time_push button and input enter key for next test: ")
value = read_time_button()
print('Time_push press value:%d\n' % int(value))
print('Time_push test finish\n\n')


input("press Date_push button and input enter key for next test: ")
value = read_date_button()
print('Date_push press value:%d\n' % int(value))
input("press Date_push button and input enter key for next test: ")
value = read_date_button()
print('Date_push press value:%d\n' % int(value))
print('Date_push test finish\n\n')
'''

input("press reset button and input enter key for next test: ")
print("reset_button=%s" %get_gpio("59"))




