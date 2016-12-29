# You need to "sudo apt-get install libgtkmm-3.0-dev" to build the demo_3d binary

all: gyromouse magnetomouse piomouse

HDRS = 
CMN_OBJS = 
GYRO_OBJS = gyromouse.o parser.o
PIO_OBJS = piomouse.o
MAG_OBJS = magnetomouse.o parser.o

# Set DMP FIFO rate to 20Hz to avoid overflows on 3d demo.  See comments in
# MPU6050_6Axis_MotionApps20.h for details.

CXXFLAGS = -DDMP_FIFO_RATE=9 -Wall -g -O2 

$(CMN_OBJS) $(DMP_OBJS) $(RAW_OBJS) : $(HDRS)

gyromouse: $(CMN_OBJS) $(GYRO_OBJS)
	$(CXX) -o $@ $^ -lm

magnetomouse: $(CMN_OBJS) $(MAG_OBJS)
	$(CXX) -o $@ $^ -lm

piomouse: $(CMN_OBJS) $(PIO_OBJS)
#	$(CXX) -o $@ $^ -lm -lpigpio -lrt -lpthread // for LIB_PIGPIO option
	$(CXX) -o $@ $^ -lm


clean:
	rm -f $(CMN_OBJS) $(GYRO_OBJS) $(PIO_OBJS) gyromouse magnetomouse piomouse

