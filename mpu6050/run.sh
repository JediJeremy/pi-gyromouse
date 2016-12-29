#!/bin/sh

# make sure the fifo exists and is public
mkfifo /dev/sensor1
chmod go+w /dev/sensor1
# start the daemon
/usr/src/mpu6050/sensor-mpu6050 -f /dev/sensor1
