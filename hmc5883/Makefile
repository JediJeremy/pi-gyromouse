# You need to "sudo apt-get install libgtkmm-3.0-dev" to build the demo_3d binary

all: sensor-hmc5883

HDRS = 
CMN_OBJS = I2Cdev.o
SENSOR_OBJS = sensor-hmc5883.o

# Set DMP FIFO rate to 20Hz to avoid overflows on 3d demo.  See comments in
# MPU6050_6Axis_MotionApps20.h for details.

CXXFLAGS = -DDMP_FIFO_RATE=9 -Wall -g -O2 

$(CMN_OBJS) $(DMP_OBJS) $(RAW_OBJS) : $(HDRS)

sensor-hmc5883: $(CMN_OBJS) $(SENSOR_OBJS)
	$(CXX) -o $@ $^ -lm

clean:
	rm -f $(CMN_OBJS) $(SENSOR_OBJS) sensor-hmc5883

