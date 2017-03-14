import subprocess, os
from subprocess import check_output
#import time
import datetime
import signal

class Audio:
    process=None
    
    @staticmethod
    def play_list(filePathList, play_time=-1):
        # [filePathList]:
        #   ["a.mp3", "b.mp3", "c.mp3"]  
        # [play_time]: 
        #  -1: playback to end of file 
        #   0: infinite loop
        # n>0: seconds of playback
        
        Audio.stop()
        
        filePath=" ".join(filePathList)
        #print("filePath=(%s)" %(filePath))
        
        # madplay -t <HH:MM:SS.DDD> -r <audio_file(s)>
        cmd_str=""
        if play_time == 0:
          cmd_str="madplay -r --no-tty-control " + filePath
          
        elif play_time > 0:
          # http://stackoverflow.com/questions/21520105/how-to-convert-seconds-into-hhmmss-in-python-3-x
          time_format=str(datetime.timedelta(seconds=play_time))
          cmd_str="madplay -t " + time_format + " -r --no-tty-control " + filePath
          
        else:
          cmd_str="madplay --no-tty-control " + filePath
        
        # http://stackoverflow.com/questions/4789837/how-to-terminate-a-python-subprocess-launched-with-shell-true
        Audio.process = subprocess.Popen(cmd_str, stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid) 
        
        # save pid to file
        try:
          with open("/tmp/madplay_shell.pid", "w") as f:
            f.write(str(Audio.process.pid))
        except:
          print("[audio][play] failed to record madplay_shell pid !!! \n\n",  flush=True)
          pass
    
    @staticmethod
    def play(filePath, play_time=-1):
        file_list=[filePath]
        Audio.play_list(file_list, play_time)

            
    @staticmethod
    def stop():
        if Audio.process != None:
          os.killpg(os.getpgid(Audio.process.pid), signal.SIGTERM)
          Audio.process=None
        
        # in case the audio device is used by other processes
        # get pid from file
        try:
          #print("[audio][stop] test",  flush=True)
          with open("/tmp/madplay_shell.pid", "r") as f:
            madplay_shell_pid=int(f.read())
            print("[audio][stop] madplay_shell_pid=%d" %(madplay_shell_pid),  flush=True)
            os.killpg(os.getpgid(madplay_shell_pid), signal.SIGTERM)
        except:
          #print("[audio][stop] madplay_shell.pid is not available",  flush=True)
          pass
          
            
'''    
    @staticmethod
    def play(filePath):
        Audio.stop()
        subprocess.Popen(["madplay","--no-tty-control", filePath, "&"], shell=False)
    
    @staticmethod
    def stop():
        p = check_output("ps -ef | grep madplay | grep -v grep | awk '{print $2}'", shell=True)
        pid = p.decode("utf-8").rstrip()
        if pid.strip():
            time.sleep(0.5)
            subprocess.Popen(["kill","-15",pid])
'''

#Audio.play("event_happen.mp3", 2)

#Audio.play("system_set_fail.mp3", 8.4)

#file_list=["system_set.mp3", "system_unset.mp3", "system_set_ok.mp3"]
#Audio.play_list(file_list, 0)


