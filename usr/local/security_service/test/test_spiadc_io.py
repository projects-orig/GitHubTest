import threading
import spiadc_io

global spiadcio;
spiadcio = spiadc_io.SPIADCioControl()
spiadcio.start()


