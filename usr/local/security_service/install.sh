#!/bin/sh
echo "install"

DATA_DIR=/usr/local/share/security_service/
mkdir -p $DATA_DIR
chmod 777 $DATA_DIR

curr_dir=$PWD
echo $curr_dir
cd /etc/init.d/


daemon_proc=reset_1310.sh
chmod 755 $curr_dir/$daemon_proc
proc_name=reset_1310
ln -s $curr_dir/$daemon_proc $proc_name
update-rc.d $proc_name defaults 80


daemon_proc=run_daemon.py
chmod 755 $curr_dir/$daemon_proc
proc_name=communication_service
ln -s $curr_dir/$daemon_proc $proc_name
update-rc.d $proc_name defaults 81

daemon_proc=keepalive_daemon.py
chmod 755 $curr_dir/$daemon_proc
proc_name=keepalive_service
ln -s $curr_dir/$daemon_proc $proc_name
update-rc.d $proc_name defaults 82
cd -

