from Adafruit_BBIO.SPI import SPI
import time
import threading

class SPIADCioControl (threading.Thread):

  def __init__(self):
      threading.Thread.__init__(self)
      self.name = "spiadcio_thread"
      self.running = True

  def run(self):
      #Only need to execute one of the following lines:
      #spi = SPI(bus, device) #/dev/spidev<bus>.<device>
      spi = SPI(0,0)  #/dev/spidev1.0
      spi.msh=2000000 # SPI clock set to 2000 kHz
      spi.bpw = 8  # bits/word
      spi.threewire = False
      spi.lsbfirst = False
      spi.mode = 1
      spi.cshigh = False  # chip select (active low)
      spi.open(0,0)
      print("spi... msh=" + str(spi.msh))

      gchannel=0
      buf0 = (7 << 3) | ((gchannel & 0x0f) >> 1) #(7<<3) for auto-2 mode
      buf1 = (gchannel & 0x01) << 7
      buf1 = buf1|0x40 #select 5v i/p range

      while(self.running):
          ret=spi.xfer2([buf0,buf1]);
          print("0x%x 0x%x" %(ret[0],ret[1]) )

          chanl=(ret[0]&0xf0)>>4
          adcval=((ret[0]&0x0f)<<4)+((ret[1]&0xf0)>>4)
          print(" -> chanl=%d adcval=0x%x" %(chanl,adcval) )

          time.sleep(1)

  def stop(self):
      self.running = False

