#!/bin/bash

if [ "$1" == "start" ]; then

echo "reset 1310 ....."

echo 65 > /sys/class/gpio/export  
sleep 1
echo "out" > /sys/class/gpio/gpio65/direction

echo 0 > /sys/class/gpio/gpio65/value
sleep 1
echo 1 > /sys/class/gpio/gpio65/value

else 
echo "skip reset 1310"

fi
