#!/usr/bin/python3

import serial, time
#initialization and open the port

#possible timeout values:
#    1. None: wait forever, block call
#    2. 0: non-blocking mode, return immediately
#    3. x, x is bigger than 0, float allowed, timeout block call

ser = serial.Serial()
ser.port = "/dev/ttyO1"
#ser.port = "/dev/ttyS2"
ser.baudrate = 115200
ser.bytesize = serial.EIGHTBITS #number of bits per bytes
ser.parity = serial.PARITY_NONE #set parity check: no parity
ser.stopbits = serial.STOPBITS_ONE #number of stop bits
ser.timeout = 2              #timeout block read
ser.xonxoff = False     #disable software flow control
ser.rtscts = False     #disable hardware (RTS/CTS) flow control
ser.dsrdtr = False       #disable hardware (DSR/DTR) flow control
ser.writeTimeout = 2     #timeout for write

def line_detect():
  try: 
    ser.open()
  except Exception as e:
    print ("error open serial port: ")
    print (e)
    return False

  if ser.isOpen():
    try:
        ser.flushInput() #flush input buffer, discarding all its contents
        ser.flushOutput()#flush output buffer, aborting current output and discard all that is in buffer

        #write data
        ser.write(b'AT:R6C\r')
        print("write data: AT:R6C")
        #ser.write(b'AT\r')
        #print("write data: AT")
        
        time.sleep(0.5)  #give the serial port sometime to receive the data

        numOfLines = 0
        linebroken_candi = 0
        linebroken_real  = 0
        while True:
          response = ser.readline()
          #print("read data: " + response)->exception show up("Can't convert 'bytes' object to str implicitly",)
          print("read data: ")
          print(response)
          if response:
            print(response[0])
            print(response[1])
            if response[0]==48 and response[1]==48:
              linebroken_candi = 1
            if response[0]==79 and response[1]==75:
              linebroken_real = linebroken_candi
              linebroken_candi = 0
            if linebroken_real == 1:
              print ("line broken!!!!")
              break
          else:
            print("goodbye~")
            break
            
        ser.close()
        
        if linebroken_real == 1:        
          return False
        
        return True  
        
    except Exception as e1:
        print ("error communicating...: ")
        print (e1.args)
        ser.close()
        return False

  else:
    print ("cannot open serial port ")
    return False
    