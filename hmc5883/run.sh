#!/bin/sh

# make sure the fifo exists and is public
mkfifo /dev/sensor0
chmod go+w /dev/sensor0
# start the daemon
/usr/src/hmc5883/sensor-hmc5883 -f /dev/sensor0
