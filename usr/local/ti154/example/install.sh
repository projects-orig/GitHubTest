#!/bin/sh
echo "install"
curr_dir=$PWD
echo $curr_dir
cd /etc/init.d/
daemon_proc=collector/run_collector.sh
proc_name=ti154_collector_service
priority=92
ln -s $curr_dir/$daemon_proc $proc_name
update-rc.d $proc_name defaults $priority

rm -rf /etc/rc0.d/K$priority$proc_name
rm -rf /etc/rc1.d/K$priority$proc_name
rm -rf /etc/rc2.d/S$priority$proc_name
rm -rf /etc/rc3.d/S$priority$proc_name
rm -rf /etc/rc4.d/S$priority$proc_name
rm -rf /etc/rc6.d/K$priority$proc_name

daemon_proc=gateway/run_gateway.sh
proc_name=ti154_gateway_service
priority=93
ln -s $curr_dir/$daemon_proc $proc_name
update-rc.d $proc_name defaults $priority

rm -rf /etc/rc0.d/K$priority$proc_name
rm -rf /etc/rc1.d/K$priority$proc_name
rm -rf /etc/rc2.d/S$priority$proc_name
rm -rf /etc/rc3.d/S$priority$proc_name
rm -rf /etc/rc4.d/S$priority$proc_name
rm -rf /etc/rc6.d/K$priority$proc_name

cd -

