#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <math.h>

void emit(int fd, int type, int code, int val) {

	struct input_event ie;

	ie.type = type;
	ie.code = code;
	ie.value = val;
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	write(fd, &ie, sizeof(ie));
};


void convDecToBin(int n, char *buf) {
	*(buf+5) = '\0';
  	int mask = 0x10 << 1;
  	while(mask >>= 1)
    	*buf++ = !!(mask & n) + '0';
}

int main() {

	struct uinput_user_dev uud;

	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
        ioctl(fd, UI_SET_KEYBIT, BTN_JOYSTICK);
	ioctl(fd, UI_SET_EVBIT, EV_ABS);
	ioctl(fd, UI_SET_ABSBIT, ABS_X);
	ioctl(fd, UI_SET_EVBIT, EV_ABS);
	ioctl(fd, UI_SET_ABSBIT, ABS_Y);

	memset(&uud, 0, sizeof(uud));
	uud.id.bustype = BUS_USB;
	uud.id.vendor = 39546;
	uud.id.product = 47639;
	snprintf(uud.name, UINPUT_MAX_NAME_SIZE, "Chompstick");
	write(fd, &uud, sizeof(uud));

	ioctl(fd, UI_DEV_CREATE);

	libusb_device_handle *dev_handle;
	libusb_context *ctx = NULL;

	int r;

	r = libusb_init(&ctx);

	if(r < 0) {
		printf("Init Error");
		return 1;
	}

	libusb_set_debug(ctx, 3);

	dev_handle = libusb_open_device_with_vid_pid(ctx, 39546, 47639);

	if(dev_handle == NULL) {
		printf("ERROR CONNECTING\n");
	} else {
		printf("Successfully connected to device\n");
	}

	r = libusb_claim_interface(dev_handle, 0);
       	if(r < 0) {
        	printf("Error claiming interface");
                return 1;
        }
       	printf("Claimed Interface\n");

	unsigned char data[1];
        int actual;
	bool user_has_not_requested_exit = true;
	int button, xaxis, yaxis;

	while(user_has_not_requested_exit) {
		data[0] = '\0';
		r = libusb_bulk_transfer(dev_handle, 129 , data, sizeof(data), &actual, 0);
		char buf[5];
		convDecToBin((int)data[0], buf);
		if(buf[0] == '1') {
			button = 1;
			emit(fd, EV_KEY, BTN_JOYSTICK, 1);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		} else {
			button = 0;
			emit(fd, EV_KEY, BTN_JOYSTICK, 0);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}

		if(buf[1] == '0' && buf[2] == '1') {
			xaxis = -32767;
		} else if(buf[1] == '1' && buf[2] == '0') {
			xaxis = 0;
		} else if(buf[1] == '1' && buf[2] == '1') {
			xaxis = 32767;
		}

		if(buf[3] == '0' && buf[4] == '1') {
			yaxis = 32767;
		} else if (buf[3] == '1' && buf[4] == '0') {
			yaxis = 0;
		} else if (buf[3] == '1' && buf[4] == '1') {
			yaxis = -32767;
		}

		if(button == 1) {
			emit(fd, EV_ABS, ABS_X, xaxis);
                	emit(fd, EV_SYN, SYN_REPORT, 0);
			emit(fd, EV_ABS, ABS_Y, yaxis);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		} else {
			emit(fd, EV_ABS, ABS_X, 0);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		        emit(fd, EV_ABS, ABS_Y, 0);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
		}
	}
	r = libusb_release_interface(dev_handle, 0);
	ioctl(fd, UI_DEV_DESTROY);
        close(fd);
	return 0;
}

