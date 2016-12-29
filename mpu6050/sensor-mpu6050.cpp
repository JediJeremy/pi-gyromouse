#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "I2Cdev.h"
#include "MPU6050.h"


int fd; // output file descriptor
int sample_rate = 25; // default sample rate
int temp_rate = 1000; // rate in ms to report temperature at


// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

float temp_celcius(int16_t n) {
	return (float)n / 340 + 36.53;
}

int32_t time_ms() {
	// get the current time
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	// pull out seconds and milliseconds
	time_t  s  = spec.tv_sec;
	int32_t ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
	// add them and return
	return s * 1000 + ms;
}

char print_buffer[1024]; // common print buffer

void print_newline() {
	write(fd,"\n",1);
}

void print_float(float f) {
	sprintf(print_buffer,"%.6f",f);
	write(fd,print_buffer,strlen(print_buffer));
}

void print_index(int32_t now, int channel) {
	sprintf(print_buffer,"%d",now);
	write(fd,print_buffer,strlen(print_buffer));
	sprintf(print_buffer," %d",channel);
	write(fd,print_buffer,strlen(print_buffer));
	write(fd,"=",1);
}

void print_vector(float a, float b, float c) {
	print_float(a); write(fd," ",1);
	print_float(b); write(fd," ",1);
	print_float(c);
}

void print_temperature(int32_t now, float temp) {
	print_index(now, 2);
	print_float(temp);
	print_newline();
}

void print_sample(int32_t now, int channel, float a, float b, float c) {
	print_index(now, channel);
	print_vector(a,b,c);
	print_newline();
}

void setup() {
	// initialize device
	//printf("Initializing I2C devices...\n");
	accelgyro.initialize();
	// verify connection
	//printf("Testing device connections...\n");
	if(accelgyro.testConnection()) {
		//printf("MPU6050 connected\n" );
	} else {
		printf("MPU6050 connection failed\n");
    	exit(1);
	}
	/*
	// get the current temperate and output it
	float temp = temp_celcius( accelgyro.getTemperature() );
	int32_t now = time_ms();
	//
	//printf("Current temperature %.1f degrees C\n",temp);
	// printf("%d 2=%.1f\n",now,temp);
	print_temperature(now,temp);
	*/
}

int32_t temp_next = 0; // when is the next time we sample temperature

void sample(int32_t now) {
	// read raw accel/gyro measurements from device
	accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
	// display accel/gyro x/y/z values
	print_sample(now,0, ax,ay,az);
	print_sample(now,1, gx,gy,gz);
	// is it time to sample the temperature?
	if(temp_next <= now) {
		temp_next = now + temp_rate; // schedule next time
		float temp = temp_celcius( accelgyro.getTemperature() );
		print_temperature(now,temp);
	}
}

static struct option long_options[] =
{
	/* These options set a flag. */
	//{"verbose", no_argument,        &verbosemode, 1},
	//{"quiet",   no_argument,        &verbosemode, 0},
	/* These options don’t set a flag.	We distinguish them by their indices. */
	{"fifo",    required_argument,  0, 'f'},
	{"rate",    required_argument,  0, 'r'},
	{"trate",   required_argument,  0, 't'},
	{"help",    required_argument,  0, 'h'},
	{0, 0, 0, 0}
};

int main(int argc, char **argv)
{
	// start with stdout
	fd = fileno(stdout);

	int option_index = 0;
	int v;
	while(1) {
		// read next option
		int c = getopt_long(argc, argv, "f:r:h",  long_options, &option_index);
		// done yet?
		if (c == -1) break;
		// then parse the next
		switch (c) {
			case 'r':
				v = atoi(optarg);
				if(v>0) sample_rate = v;
				break;
			case 't':
				v = atoi(optarg);
				if(v>0) temp_rate = v;
				break;
			case 'f':
				printf("opening FIFO %s\n",optarg);
				// make sure the fifo exists
				mknod(optarg, S_IFIFO | 0666, 0);
				// open the fifo
				fd = open(optarg, O_WRONLY);
	    		break;
			case 'h':
				printf("Drives a MPU6050 accellerometer/gyro chip over I2C on the Raspberry Pi\n");
				printf("\n");
				printf("Usage: sensor-mpu6050 [-f <file>] [-h]\n");
				printf("  -f <file> FIFO filename (default <stdout>)\n");
				printf("  -r <number> sample rate in milliseconds (default 10) \n");
				printf("  -t <number> temperature sample rate (default 1000) \n");
				printf("  -h help.\n");
				printf("\n");
				printf("Writes lines to a file in the format \"<t> <n>=<x> <y> <z>\" where\n");
				printf("  <t> is the time (in unix milliseconds) the reading was taken.\n");
				printf("  <n> is the channel ID of the reading, in this case always '0'.\n");
				printf("  <x> <y> <z> values are floating point numbers measuring raw gauss values.\n");
				printf("\n");
				printf("If a FIFO filename is specified, the file will be created before use. If not given, stdout is used.\n");
				printf("\n");
				printf("This is the same 'generic' interface used by other sensor drivers,");
				printf("and is intended to be used to write to a /dev/sensor<n> FIFO.\n");
				printf("eg:\n");
				printf("  sensor-mpu6050 -f /dev/sensor0      # write to dev fifo for everyone to see\n");
				printf("  stdbuf -oL sensor-mpu6050 | analyze # low-latency stdout to another process\n");
				exit( -1 );
			default:
        	  printf("Unknown arg. Try %s -h for help.\n" , argv[0] );
        	  exit(-1);
	  }
	  // arg++;
	}

    setup();
    for (;;) {
       	int32_t now = time_ms();
       	sample(now);
       	usleep( (long)(sample_rate) * 1000 );
       }
    /*
    for (;;)
        loop();*/
}

