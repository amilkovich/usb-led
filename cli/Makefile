all: usb-led-cli

usb-led-cli: usb-led-cli.c
	gcc -O -Wall usb-led-cli.c -I/usr/include/libusb-1.0 -lusb-1.0 -o usb-led-cli

clean:
	$(RM) usb-led-cli

.PHONY: all clean
