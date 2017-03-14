#!/usr/bin/python3
from app import app

import os
import sys, time
from daemon import daemon

class MyDaemon(daemon):
    def run(self):
        app.run(host='0.0.0.0', port=5001)

if __name__ == "__main__":
  
        daemon = MyDaemon('/tmp/communication_service.pid')
                                                           
        if len(sys.argv) == 2:
                if 'start' == sys.argv[1]:
                        daemon.start()
                elif 'stop' == sys.argv[1]:
                        daemon.stop()
                elif 'restart' == sys.argv[1]:
                        daemon.restart()
                else:
                        print("Unknown command")
                        sys.exit(2)
                sys.exit(0)
        else:
                print("usage: %s start|stop|restart" % sys.argv[0])
                sys.exit(2)
