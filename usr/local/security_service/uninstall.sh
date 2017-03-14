#!/bin/sh
echo "Uninstall !"
proc_name=communication_service
find /etc/ -name "*${proc_name}" -exec rm -f {} \;

proc_name=keepalive_service
find /etc/ -name "*${proc_name}" -exec rm -f {} \;
echo "Done"
