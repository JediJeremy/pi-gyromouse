#/bin/sh
#stdbuf -oL /usr/src/sensor-mpu6050/demo_raw | /usr/src/gyromouse/gyromouse &
#stdbuf -oL /usr/src/sensor-hmc5883/sensor | /usr/src/gyromouse/magnetomouse &
stdbuf -oL cat /dev/sensor0 | /usr/src/gyromouse/magnetomouse &
/usr/src/gyromouse/piomouse &

