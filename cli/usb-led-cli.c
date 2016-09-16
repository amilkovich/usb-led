#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <libgen.h>
#include <libusb.h>

#define VID 0x16c0
#define PID 0x05dc
#define USB_LED_SET_RED   0x0
#define USB_LED_GET_RED   0x1
#define USB_LED_SET_GREEN 0x2
#define USB_LED_GET_GREEN 0x3
#define USB_LED_SET_BLUE  0x4
#define USB_LED_GET_BLUE  0x5

bool validate_args(int argc, char **argv, unsigned char *brightness) {
	if (argc != 3 && argc != 4)
		return false;

	if (argc == 3) {
		if (strcmp(argv[1], "get") != 0)
			return false;
		if (strcmp(argv[2], "red") != 0 &&
		    strcmp(argv[2], "green") != 0 &&
		    strcmp(argv[2], "blue") != 0)
			return false;
	} else if (argc == 4) {
		if (strcmp(argv[1], "set") != 0)
			return false;
		if (strcmp(argv[2], "red") != 0 &&
		    strcmp(argv[2], "green") != 0 &&
		    strcmp(argv[2], "blue") != 0)
			return false;
		if (sscanf(argv[3], "%hhu", brightness) != 1)
			return false;
	}

	return true;
}

int main(int argc, char **argv) {
	unsigned char brightness;

	if (!validate_args(argc, argv, &brightness)) {
		fprintf(stderr, "Usage: %s get red/green/blue\n       "
				"%s set red/green/blue brightness\n",
			basename(argv[0]), basename(argv[0]));
		exit(1);
	}

	libusb_init(NULL);

	libusb_device_handle *handle =
		libusb_open_device_with_vid_pid(NULL, VID, PID);

	if (handle == NULL) {
		fprintf(stderr, "Error: unable to open usb device!\n");
		libusb_exit(NULL);
		exit(1);
	}

	libusb_claim_interface(handle, 0);

	if (argc == 3) {
		uint8_t request;
		if (argv[2][0] == 'r') request = USB_LED_GET_RED;
		else if (argv[2][0] == 'g') request = USB_LED_GET_GREEN;
		else request = USB_LED_GET_BLUE;

		unsigned char buffer[1];
		libusb_control_transfer(handle,
			LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
				LIBUSB_ENDPOINT_IN,
			request,
			0,
			0,
			(unsigned char *)buffer,
			sizeof(buffer),
			0);
		printf("%hhu\n", buffer[0]);
	} else if (argc == 4) {
		uint8_t request;
		if (argv[2][0] == 'r') request = USB_LED_SET_RED;
		else if (argv[2][0] == 'g') request = USB_LED_SET_GREEN;
		else request = USB_LED_SET_BLUE;

		libusb_control_transfer(handle,
			LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
				LIBUSB_ENDPOINT_IN,
			request,
			brightness,
			0,
			NULL,
			0,
			0);
	}

	libusb_release_interface(handle, 0);

	libusb_exit(NULL);

	return 0;
}
