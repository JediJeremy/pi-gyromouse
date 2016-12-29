#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "I2Cdev.h"

#define HMC5883_I2C_ADDRESS 0x1e

// menu of possible config values
#define CONFIGA_RATE_0_75HZ   0b00000000
#define CONFIGA_RATE_1_5HZ    0b00000100
#define CONFIGA_RATE_3HZ      0b00001000
#define CONFIGA_RATE_7_5HZ    0b00001100
#define CONFIGA_RATE_15HZ     0b00010000
#define CONFIGA_RATE_30HZ     0b00010100
#define CONFIGA_RATE_75HZ     0b00011000

#define CONFIGA_SAMPLES_1     0b00000000
#define CONFIGA_SAMPLES_2     0b00100000
#define CONFIGA_SAMPLES_4     0b01000000
#define CONFIGA_SAMPLES_8     0b01100000

#define CONFIGA_BIAS_NONE     0b00000000
#define CONFIGA_BIAS_POSITIVE 0b00000001
#define CONFIGA_BIAS_NEGATIVE 0b00000010

#define CONFIGB_GAUSS_0_88    0b00000000
#define CONFIGB_GAUSS_1_3     0b00100000
#define CONFIGB_GAUSS_1_9     0b01000000
#define CONFIGB_GAUSS_2_5     0b01100000
#define CONFIGB_GAUSS_4_0     0b10000000
#define CONFIGB_GAUSS_4_7     0b10100000
#define CONFIGB_GAUSS_5_6     0b11000000
#define CONFIGB_GAUSS_8_1     0b11000000


float gain_range[8] = { 0.88, 1.3, 1.9, 2.5, 4.0, 4.7, 5.6, 8.1 };
float gain_gauss[8] = { 1370, 1090, 820, 660, 440, 390, 330, 230 };
float gain_resolution[8] = { 0.73, 0.92, 1.22, 1.52, 2.27, 2.56, 3.03, 4.35 };
float gain_config[8] = { CONFIGB_GAUSS_0_88,CONFIGB_GAUSS_1_3, CONFIGB_GAUSS_1_9, CONFIGB_GAUSS_2_5, CONFIGB_GAUSS_4_0, CONFIGB_GAUSS_4_7, CONFIGB_GAUSS_5_6, CONFIGB_GAUSS_8_1 };

int gain_index = 4;

// select from the menu
#define SETUP_RATE        CONFIGA_RATE_75HZ
#define SETUP_SAMPLES     CONFIGA_SAMPLES_2
#define SETUP_GAUSS       CONFIGB_GAUSS_1_3


void setup(void) {
	I2Cdev::writeByte(HMC5883_I2C_ADDRESS,0, SETUP_RATE | SETUP_SAMPLES | CONFIGA_BIAS_NONE); // config a register
	I2Cdev::writeByte(HMC5883_I2C_ADDRESS,1, gain_config[gain_index]); // config b register
	I2Cdev::writeByte(HMC5883_I2C_ADDRESS,2, 0b00000000); // start continuous sampling
}

int16_t sample_value(uint8_t * buffer, int index) {
	// get the sequential bytes
	uint8_t a = buffer[index];
	uint8_t b = buffer[index+1];
	// careful to msb first
	uint16_t c = (((uint16_t)a) << 8) | b;
	// and cast to result
	return *( (int16_t *)&c );
}


int fd; // file descriptor we are writing to
int rate = 25; // sampling rate, in milliseconds

char print_buffer[1024]; // common print buffer

void print_gauss(uint8_t * buffer, int index) {
	// get the sample integer value
	int16_t iv = sample_value(buffer, index);
	// is this the 'overload' value?
	if(iv == -4096) {
		write(fd,"null",4);
	} else {
		// convert to floating point
		float fv = (float)iv / (float)gain_gauss[gain_index];
		sprintf(print_buffer,"%.6f",fv);
		write(fd,print_buffer,strlen(print_buffer));
	}
}

void sample(int32_t now) {
	uint8_t buffer[8];
	// read data from device into buffer
	I2Cdev::readBytes(HMC5883_I2C_ADDRESS,3, 2, &buffer[0]);
	I2Cdev::readBytes(HMC5883_I2C_ADDRESS,5, 2, &buffer[2]);
	I2Cdev::readBytes(HMC5883_I2C_ADDRESS,7, 2, &buffer[4]);
	// write the sample row, reorder XZY
	sprintf(print_buffer,"%d 0=",now);
	write(fd,print_buffer,strlen(print_buffer));
	print_gauss(buffer,0); write(fd," ",1);
	print_gauss(buffer,4); write(fd," ",1);
	print_gauss(buffer,2);
	write(fd,"\n",1);
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



static struct option long_options[] =
{
	/* These options set a flag. */
	//{"verbose", no_argument,        &verbosemode, 1},
	//{"quiet",   no_argument,        &verbosemode, 0},
	/* These options don’t set a flag.	We distinguish them by their indices. */
	{"fifo",    required_argument,  0, 'f'},
	{"rate",    required_argument,  0, 'r'},
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
				if(v>0) rate = v;
				break;
			case 'f':
				printf("opening FIFO %s\n",optarg);
				// make sure the fifo exists
				mknod(optarg, S_IFIFO | 0666, 0);
				// open the fifo
				fd = open(optarg, O_WRONLY );
	    		break;
			case 'h':
				printf("Drives a HMC5883 magnetometer chip over I2C on the Raspberry Pi\n");
				printf("\n");
				printf("Usage: sensor-hmc5883 [-f <file>] [-h]\n");
				printf("  -f <file> fifo \n");
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
				printf("  sensor-hmc5883 -f /dev/sensor0      # write to dev fifo for everyone to see\n");
				printf("  stdbuf -oL sensor-hmc5883 | analyze # low-latency stdout to another process\n");
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
    	usleep( (long)(rate) * 1000 );
    }
}

