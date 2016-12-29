#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <wiringPi.h>

#include "parser.h"

int fd;

// #define MODE_0
// #define MODE_0_SCALE 1.0

#define MODE_1
#define MODE_1_SCALE 50.0


void die(char * reason) {
	//printf("%s\n",reason);
	perror(reason);
	exit(1);
}

void send_syn() {
	struct input_event     ev;
	// syn event
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = 0;
	ev.value = 0;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");
}

void send_mouse_abs(int mx, int my) {
	struct input_event     ev;
	//printf("mouse %d %d\n",mx,my);
	// mouse X movement
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_ABS;
	ev.code = ABS_X;
	ev.value = mx;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");
	// mouse Y movement
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_ABS;
	ev.code = ABS_Y;
	ev.value = my;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");
	// syn event
	send_syn();
}

void send_mouse_rel(int dx, int dy) {
	struct input_event     ev;
	//printf("mouse %d %d\n",mx,my);
	// mouse X movement
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_REL;
	ev.code = REL_X;
	ev.value = dx;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");
	// mouse Y movement
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_REL;
	ev.code = REL_Y;
	ev.value = dy;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");
	// syn event
	send_syn();
}


#ifdef MODE_0
/* MODE 0
 * direct linear scaling of the gauss readings to the [0..1023] range we said we'd do
 * during setup. The Z value is used as a 'level switch' to indicate the presence of
 * a magnetic stylus.
 *
 * This turns out to be not so useful, as there are always little eddy currents that wiggle
 * the pointer around, and make it impossible to click on anything. However, if you have a nice
 * enough setup of magnets, this might still be useful.
 */

int rescale_gauss(float g) {
	g = g * MODE_0_SCALING + 0.5;
	if( g<0 ) g = 0;
	if( g>1 ) g = 1;
	int v = g * 1024;
	if(v >= 1024) v = 1023;
}

void send_magneto(float gX, float gY, float gZ) {
	//printf("magneto %f %f %f\n",gX,gY,gZ);
	// is the gZ in 'mouse' territory?
	if( (gZ < -0.3) || (gZ == 0) ) {
		// map the x and y values to the absolute range
		send_mouse_abs( rescale_gauss(gX), rescale_gauss(gY) );
	}
}

#endif


#ifdef MODE_1
/* MODE 1
 * We set a Z-value which indicates the presence of a probe.
 * Beyond that, changes in the X-Y values compared to the previous reading will
 * be appended to pointer, scaled by the 'depth' of the probe.
 */

float last_x = 0;
float last_y = 0;

void send_magneto(float gX, float gY, float gZ) {
	//printf("magneto %f %f %f\n",gX,gY,gZ);
	// compute delta from last time
	float dx = (gX - last_x);
	float dy = (gY - last_y);
	last_x = gX; last_y = gY;
	// we mirror everything if we have to, to keep Z positive
	if(gZ < 0) {
		gX = -gX; dx = -dx; dy = -dy;
	}
	// is the probe nearby?
	if( (gZ > 0.3) ) {
		// scale the events by the depth
		float scale = sqrt(gZ - 0.3) * MODE_1_SCALE;
		// convert them to integers
		int ix = dx * scale; int iy = dy * scale;
		// if either were non-zero, send the update
		if( (ix != 0) || (iy != 0) ) {
			send_mouse_rel( ix,iy );
		}
	}
}

#endif

void parser_commit(char * parse_id, int parse_id_length, char * parse_value, int parse_value_length) {
	// printf("commit %s %s\n",parse_id,parse_value);
	// we have what looks like a well-formed line. convert and split up the id vector
	// std::vector<char *> id_vector = split_string(parse_id, ' ', parse_id_length);
	char * id_vector[16];
	split_string(parse_id, ' ', parse_id_length, id_vector, 16);
	// we really only care about the second value for the moment
	int sensor_id = parse_value_long( id_vector[1] ); // (id_vector[1] == null) ? -1 : atoi( id_vector[1] );
	// convert and split up the value vector
	// std::vector<char *> value_vector = split_string(parse_value, ' ', parse_value_length);
	char * value_vector[16];
	split_string(parse_value, ' ', parse_value_length, value_vector, 16);
	float gX,gY,gZ;
	switch(sensor_id) {
		case 0: // magnetometer
			gX = parse_value_float(value_vector[0]);
			gY = parse_value_float(value_vector[1]);
			gZ = parse_value_float(value_vector[2]);
			//
			send_magneto(gX, gY, gZ);
			break;
	}
}


int main(void) {
	// open the wiring ports for our buttons


	// open the uinput fifo
	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if(fd < 0) {
		printf("could not open /dev/uinput\n");
		return 1;
	}
	// enable the message types we're going to send
	if(ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE) < 0) die("error: ioctl");
#ifdef MODE_0
	if(ioctl(fd, UI_SET_EVBIT, EV_ABS) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_ABSBIT, ABS_X) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_ABSBIT, ABS_Y) < 0) die("error: ioctl");
#endif
#ifdef MODE_1
	if(ioctl(fd, UI_SET_EVBIT, EV_REL) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_RELBIT, REL_X) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_RELBIT, REL_Y) < 0) die("error: ioctl");
#endif
	if(ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) die("error: ioctl");
	// create our virtual mouse device
	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "gyromouse");
	uidev.id.bustype = BUS_VIRTUAL; // BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;
#ifdef MODE_0
	uidev.absmin[ABS_X] = 0;
	uidev.absmax[ABS_X] = 1023;
	uidev.absmin[ABS_Y] = 0;
	uidev.absmax[ABS_Y] = 1023;
#endif
	if(write(fd, &uidev, sizeof(uidev)) < 0) die("error: write");
	if(ioctl(fd, UI_DEV_CREATE) < 0) die("error: ioctl");
	//
	// wait for first connection
	printf("opening stdin...\n");
	int stdin_fd = fileno(stdin);
	// printf("got a writer\n");
	// wait for data
	int num;
	char buffer[1028];
	do {
		num = read(stdin_fd, buffer, 1024);
		// printf("read %d bytes: \"%s\"\n", num, buffer);
		if( num == -1 ) {
			perror("read");
		} else if( num == 0 ) {
			// eof.

		} else {
			buffer[num] = '\0';
			parse_block(buffer,num);
		}
	} while (num > 0);
	// } while(1);

	close(stdin_fd);

	// destroy our mouse device
	if(ioctl(fd, UI_DEV_DESTROY) < 0) die("error: ioctl");


	close(fd);

	return 0;
}
