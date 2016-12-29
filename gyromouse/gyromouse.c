#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <wiringPi.h>

#include "parser.h"

int fd;

void die(char * reason) {
	//printf("%s\n",reason);
	perror(reason);
	exit(1);
}

void send_button(int btn) {
	struct input_event     ev;
	printf("mouse click %d\n",btn);
	int btn_code;
	switch(btn) {
		case 2: btn_code = BTN_MIDDLE; break;
		case 1: btn_code = BTN_RIGHT; break;
		case 0: default: btn_code = BTN_LEFT;
	}
	// mouse button click
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_KEY;
	ev.code = btn_code;
	ev.value = 0;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");
	// syn event
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = 0;
	ev.value = 0;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");

}


void send_mouse(int dx, int dy) {
	struct input_event     ev;
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
	memset(&ev, 0, sizeof(struct input_event));
	ev.type = EV_SYN;
	ev.code = 0;
	ev.value = 0;
	if(write(fd, &ev, sizeof(struct input_event)) < 0) die("error: write");

}

// deadband...
int deadband(int v, int n) {
	if(v > n) {
		v -= n;
	} else if(v < -n) {
		v += n;
	} else {
		v = 0;
	}
	return v;
}

void send_gyro(long gX, long gY, long gZ) {
	// printf("gyro delta %d %d\n",gX,gY);
	// convert gyro values to mouse motion
	long mX = deadband(gX >> 8, 2);
	long mY = deadband(gY >> 8 ,2);
	if( (mX != 0) || (mY != 0) ) {
		// send a mouse control message
		send_mouse(mX, mY);
	}
}


void parser_commit(char * parse_id, int parse_id_length, char * parse_value, int parse_value_length) {
	// printf("commit %s %s\n",parse_id,parse_value);
	// we have what looks like a well-formed line. convert and split up the id vector
	// std::vector<char *> id_vector = split_string(parse_id, ' ', parse_id_length);
	char * id_vector[16];
	split_string(parse_id, ' ', parse_id_length, id_vector, 15);
	// we really only care about the second value for the moment
	int sensor_id = parse_value_long( id_vector[1] ); // (id_vector[1] == null) ? -1 : atoi( id_vector[1] );
	// convert and split up the value vector
	// std::vector<char *> value_vector = split_string(parse_value, ' ', parse_value_length);
	char * value_vector[16];
	split_string(parse_value, ' ', parse_value_length, value_vector, 15);
	long gX,gY,gZ;
	switch(sensor_id) {
		case 0: // accelerometer
			//printf("accel %s %s %s\n",value_vector[0],value_vector[1],value_vector[2]);
			break;
		case 1: // gyro
			//printf("gyro %s %s %s\n",value_vector[0],value_vector[1],value_vector[2]);
			gX = parse_value_long(value_vector[0]);
			gY = parse_value_long(value_vector[1]);
			gZ = parse_value_long(value_vector[2]);
			//
			send_gyro(gX, gY, gZ);
			break;
		case 2: // temperature
			//printf("temp %s\n",value_vector[0]);
			break;
	}
}


int main(void) {
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
	if(ioctl(fd, UI_SET_EVBIT, EV_REL) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_RELBIT, REL_X) < 0) die("error: ioctl");
	if(ioctl(fd, UI_SET_RELBIT, REL_Y) < 0) die("error: ioctl");
	//if(ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) die("error: ioctl");
	// create our virtual mouse device
	struct uinput_user_dev uidev;
	memset(&uidev, 0, sizeof(uidev));
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "gyromouse");
	uidev.id.bustype = BUS_VIRTUAL; // BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x1;
	uidev.id.version = 1;
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
