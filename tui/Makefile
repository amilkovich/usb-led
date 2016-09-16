all: usb-led-tui

usb-led-tui: usb-led-tui.c
	gcc -O -Wall usb-led-tui.c -lcurses -I/usr/include/libusb-1.0 -lusb-1.0 -lpthread -o usb-led-tui

clean:
	$(RM) usb-led-tui

.PHONY: all clean
