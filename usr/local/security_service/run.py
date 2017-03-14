#!/usr/bin/python3

from app import app
import sys
import datetime

if __name__ == '__main__':
    # http://stackoverflow.com/questions/11146128/python-daemon-and-stdout
    #here = os.path.dirname(os.path.abspath(__file__))

    # redirect output of print to a TXT file 
    # http://stackoverflow.com/questions/4110891/python-how-to-simply-redirect-output-of-print-to-a-txt-file-with-a-new-line-crea
    # https://docs.python.org/3/library/functions.html
    #DEBUG_FILE_PATH = '/tmp/'
    #debug_file=DEBUG_FILE_PATH + "communication_debug.txt"
    #sys.stdout = open(debug_file, "a+")
    #print(str(datetime.datetime.now()) + " : debug output starts ...",  flush=True)

    app.run(host='0.0.0.0', debug=True, port=5001)
