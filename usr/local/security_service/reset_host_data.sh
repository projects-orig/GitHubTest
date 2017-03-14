#!/bin/bash

#echo "parameter =" $1

HOST_DATA_PATH=$1
DEFAULT_DATA_PATH=$2
if [ -z ${HOST_DATA_PATH} ]; then  
HOST_DATA_PATH="/public"
fi

DEFAULT_DATA_PATH=$2
if [ -z ${DEFAULT_DATA_PATH} ]; then  
DEFAULT_DATA_PATH="/public/security_service"
fi

rm -f ${HOST_DATA_PATH}/*.json  

# other errands...
# not needed, 20170113 cp ${DEFAULT_DATA_PATH}/Device.json ${HOST_DATA_PATH}/
cp ${DEFAULT_DATA_PATH}/KeepAlive.json ${HOST_DATA_PATH}/
cp ${DEFAULT_DATA_PATH}/HostSetting.json ${HOST_DATA_PATH}/
cp ${DEFAULT_DATA_PATH}/DeviceSetting.json ${HOST_DATA_PATH}/

echo "clear host data on " $HOST_DATA_PATH
