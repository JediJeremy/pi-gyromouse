#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define LIB_GPIOMEM
// #define LIB_SYSFS
// #define LIB_PIGPIO
// #define LIB_BCM2835

// left button on pin 29 - gpio 21 bcm 5
#define BUTTON0_GPIO 5
// right button on pin 31 - gpio 22 bcm 6
#define BUTTON1_GPIO 6

// #include "gpio.h"


#ifdef LIB_GPIOMEM
#include "gpiomem.h"
#endif

#ifdef LIB_PIGPIO
#include <pigpio.h>
#endif

#ifdef LIB_BCM2835
#include <bcm2835.h>
#endif

#ifdef LIB_SYSFS
#include "gpio.h"
#endif

int fd;

int verbose = 0;

void die(char * reason) {
	//printf("%s\n",reason);
	perror(reason);
	exit(1);
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

void send_syn() {
	struct input_event     syn_ev;
	// syn event
	memset(&syn_ev, 0, sizeof(struct input_event));
	syn_ev.type = EV_SYN;
	syn_ev.code = 0;
	syn_ev.value = 0;
	if(write(fd, &syn_ev, sizeof(struct input_event)) < 0) die("error: write");
}

void send_button(int btn_code, int value) {
	// printf("mouse %d %d\n",btn_code,value);
	struct input_event     btn_ev;
	// mouse button down
	memset(&btn_ev, 0, sizeof(struct input_event));
	btn_ev.type = EV_KEY;
	btn_ev.code = btn_code;
	btn_ev.value = value;
	if(write(fd, &btn_ev, sizeof(struct input_event)) < 0) die("error: write");
	// syn event
	send_syn();
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
	send_syn();
}

void go_active(void) {
	//system("/usr/src/gyromouse/screen_on.sh");
	system("sudo service lcdscreen stop");
}
void go_idle() {
	//system("/usr/src/gyromouse/screen_off.sh");
	system("sudo service lcdscreen start");
}


int activity_state = 0;
int32_t activity_timer = 0;
int32_t activity_timeout = 30 * 1000; // 30 seconds timeout
int32_t startup_timeout = 5 * 1000; // 5 second startup timeout

int user_activity(int32_t now) {
	// reset the timer
	activity_timer = now + activity_timeout;
	// which state are we in?
	switch(activity_state) {
		case 0: // idle state
			go_active();
			activity_state = 1;
			return 0; // we consumed the click
			break;
		case 1: break;
	}
	return 1;
}

void user_idle(int32_t now) {
	// which state are we in?
	switch(activity_state) {
		case 0: // idle state
			break;
		case 1: // active state
			// have we exceeded the timeout?
			if(activity_timer < now) {
				go_idle();
				activity_state = 0;
			}
			break;
	}
}

void debounce(int btn_code, int port, int32_t * bounce_time, int * last_level, int timeout, int32_t now) {
#ifdef LIB_PIGPIO
	int level = gpioRead(port);
#endif
#ifdef LIB_BCM2835
	uint8_t value = bcm2835_gpio_lev(port);
#endif
#ifdef LIB_SYSFS
	int level = GPIOReadBit(port);
#endif
#ifdef LIB_GPIOMEM
	int level = gpioRead(port);
#endif
	if(verbose) printf(" %d",level);
	if(level != *last_level) {
		if( *bounce_time < now ) {
			if(user_activity(now)) {
				send_button(btn_code, level ? 0 : 1 ); // flip level for button state
			}
			*last_level = level;
		}
		*bounce_time = now + timeout;
	}
}

int main(void) {
#ifdef LIB_PIGPIO
	if (gpioInitialise() < 0) die("error: pigpio failed");
	gpioSetMode(BUTTON0_GPIO, PI_INPUT);
	gpioSetMode(BUTTON1_GPIO, PI_INPUT);
	gpioSetPullUpDown(BUTTON0_GPIO, PI_PUD_UP);
	gpioSetPullUpDown(BUTTON1_GPIO, PI_PUD_UP);
    int gpio0 = BUTTON0_GPIO;
    int gpio1 = BUTTON1_GPIO;
#endif
#ifdef LIB_BCM2835
	if (!bcm2835_init()) die("error: bcm2835 failed");
    bcm2835_gpio_fsel(BUTTON0_GPIO, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(BUTTON1_GPIO, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(BUTTON0_GPIO, BCM2835_GPIO_PUD_UP);
    bcm2835_gpio_set_pud(BUTTON1_GPIO, BCM2835_GPIO_PUD_UP);
    int gpio0 = BUTTON0_GPIO;
    int gpio1 = BUTTON1_GPIO;
#endif
#ifdef LIB_SYSFS
    if (-1 == GPIOExport(BUTTON0_GPIO) ) die("error: gpio_export failed");
    if (-1 == GPIOExport(BUTTON1_GPIO) ) die("error: gpio_export failed");
    if (-1 == GPIODirection(BUTTON0_GPIO, 0) ) die("error: gpio_direction failed");
    if (-1 == GPIODirection(BUTTON1_GPIO, 0) ) die("error: gpio_direction failed");
    int gpio0 = GPIOOpenBit(BUTTON0_GPIO);
    int gpio1 = GPIOOpenBit(BUTTON1_GPIO);
#endif
#ifdef LIB_GPIOMEM
    if (gpioInitialise() < 0) return 1;
    gpioSetMode(BUTTON0_GPIO, PI_INPUT);
	gpioSetMode(BUTTON1_GPIO, PI_INPUT);
	gpioSetPullUpDown(BUTTON0_GPIO, PI_PUD_UP);
	gpioSetPullUpDown(BUTTON1_GPIO, PI_PUD_UP);
	int gpio0 = BUTTON0_GPIO;
	int gpio1 = BUTTON1_GPIO;
#endif
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
	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "piomouse");
	uidev.id.bustype = BUS_VIRTUAL; // BUS_USB;
	uidev.id.vendor  = 0x1;
	uidev.id.product = 0x2;
	uidev.id.version = 1;
	if(write(fd, &uidev, sizeof(uidev)) < 0) die("error: write");
	if(ioctl(fd, UI_DEV_CREATE) < 0) die("error: ioctl");
	
	// button debounce
	int32_t button0_bounce = 0;
	int32_t button1_bounce = 0;
	int button0_level = 1;
	int button1_level = 1;

	// assume we start inactive
	activity_state = 0;
	activity_timer = time_ms() + startup_timeout;

	// main loop.
	while(1) {
		usleep(25000);
		// get the current time
		int32_t now = time_ms();
		// debounce buttons
		debounce(BTN_LEFT, gpio0, &button0_bounce, &button0_level, 60, now );
		debounce(BTN_RIGHT, gpio1, &button1_bounce, &button1_level, 60, now );
		if(verbose) printf("\n");
		user_idle(now);
	}
	
	// destroy our mouse device
	if(ioctl(fd, UI_DEV_DESTROY) < 0) die("error: ioctl");
	close(fd);

	// close the gpio
#ifdef LIB_SYSFS
	GPIOCloseBit(gpio0);
	GPIOCloseBit(gpio1);
	GPIOUnexport(BUTTON0_GPIO);
	GPIOUnexport(BUTTON1_GPIO);
#endif

	return 0;
}
