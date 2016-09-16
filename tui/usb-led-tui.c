#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <string.h>
#include <libusb.h>
#include <pthread.h>

#define VID 0x16c0
#define PID 0x05dc
#define USB_LED_SET_RED   0x0
#define USB_LED_GET_RED   0x1
#define USB_LED_SET_GREEN 0x2
#define USB_LED_GET_GREEN 0x3
#define USB_LED_SET_BLUE  0x4
#define USB_LED_GET_BLUE  0x5

#define WIN_MINX 20
#define WIN_MINY 7

WINDOW *window;
bool connected = false;
static libusb_device_handle *handle;
unsigned short red_bar = 0;
unsigned short green_bar = 0;
unsigned short blue_bar = 0;

enum selections {
	selection_red,
	selection_green,
	selection_blue
} selected;

void mvwaddstr_centered_x(WINDOW *window, int y, char *text) {
	mvwaddstr(window, y, getmaxx(window) / 2 - strlen(text) / 2, text);
}

void draw_window(bool with_refresh) {
	erase();
	box(window, 0, 0);

	attron(A_BOLD);
	mvwaddstr_centered_x(window, 0, "USB LED");
	attroff(A_BOLD);

	if (!connected) {
		mvwaddstr_centered_x(window, 1, "Disconnected");
	}

	if (selected == selection_red) attron(A_BOLD);
	mvwaddstr(window, getmaxy(window) / 2 - 2, 4, "Red");
	attroff(A_BOLD);
	if (selected == selection_green) attron(A_BOLD);
	mvwaddstr(window, getmaxy(window) / 2 - 0, 4, "Green");
	attroff(A_BOLD);
	if (selected == selection_blue) attron(A_BOLD);
	mvwaddstr(window, getmaxy(window) / 2 + 2, 4, "Blue");
	attroff(A_BOLD);

	move(getmaxy(window) / 2 - 2, 10);
	for (unsigned int i = 0; i < getmaxx(window) - 19; i++)
		addch(' ' | A_REVERSE);
	move(getmaxy(window) / 2 - 0, 10);
	for (unsigned int i = 0; i < getmaxx(window) - 19; i++)
		addch(' ' | A_REVERSE);
	move(getmaxy(window) / 2 + 2, 10);
	for (unsigned int i = 0; i < getmaxx(window) - 19; i++)
		addch(' ' | A_REVERSE);

	if (connected) {
		unsigned int red_fill =
				red_bar * (getmaxx(window) - 19) / 100;
		unsigned int green_fill =
				green_bar * (getmaxx(window) - 19) / 100;
		unsigned int blue_fill =
				blue_bar * (getmaxx(window) - 19) / 100;
		start_color();
		init_pair(1, COLOR_RED, COLOR_BLACK);
		init_pair(2, COLOR_GREEN, COLOR_BLACK);
		init_pair(3, COLOR_BLUE, COLOR_BLACK);

		move(getmaxy(window) / 2 - 2, 10);
		attron(COLOR_PAIR(1));
		for (unsigned int i = 0; i < red_fill; i++)
			addch(' ' | A_REVERSE);
		attroff(COLOR_PAIR(1));

		move(getmaxy(window) / 2 - 0, 10);
		attron(COLOR_PAIR(2));
		for (unsigned int i = 0; i < green_fill; i++)
			addch(' ' | A_REVERSE);
		attroff(COLOR_PAIR(2));

		move(getmaxy(window) / 2 + 2, 10);
		attron(COLOR_PAIR(3));
		for (unsigned int i = 0; i < blue_fill; i++)
			addch(' ' | A_REVERSE);
		attroff(COLOR_PAIR(3));

		use_default_colors();
	}

	char percentage[5];
	sprintf(percentage, "%hhu%%", connected ? red_bar : 0);
	if (selected == selection_red) attron(A_BOLD);
	mvwaddstr(window, getmaxy(window) / 2 - 2,
		  getmaxx(window) - strlen(percentage) - 4,
		  percentage);
	attroff(A_BOLD);

	sprintf(percentage, "%hhu%%", connected ? green_bar : 0);
	if (selected == selection_green) attron(A_BOLD);
	mvwaddstr(window, getmaxy(window) / 2 - 0,
		  getmaxx(window) - strlen(percentage) - 4,
		  percentage);
	attroff(A_BOLD);

	sprintf(percentage, "%hhu%%", connected ? blue_bar : 0);
	if (selected == selection_blue) attron(A_BOLD);
	mvwaddstr(window, getmaxy(window) / 2 + 2,
		  getmaxx(window) - strlen(percentage) - 4,
		  percentage);
	attroff(A_BOLD);

	switch (selected) {
		case selection_red:
			move(getmaxy(window) / 2 - 2, 2);
			addch('*' | A_BOLD);
			move(getmaxy(window) / 2 - 2, getmaxx(window) - 3);
			addch('*' | A_BOLD);
		break;

		case selection_green:
			move(getmaxy(window) / 2 - 0, 2);
			addch('*' | A_BOLD);
			move(getmaxy(window) / 2 - 0, getmaxx(window) - 3);
			addch('*' | A_BOLD);
		break;

		case selection_blue:
			move(getmaxy(window) / 2 + 2, 2);
			addch('*' | A_BOLD);
			move(getmaxy(window) / 2 + 2, getmaxx(window) - 3);
			addch('*' | A_BOLD);
		break;
	}

	if (with_refresh) refresh();
}

void *usb_function(void *arg) {
	while (1) {
		libusb_handle_events_completed(NULL, NULL);
	}

	return 0;
}

void set_led(uint8_t request, uint16_t value) {
	libusb_control_transfer(handle,
		LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
			LIBUSB_ENDPOINT_IN,
		request,
		value,
		0,
		NULL,
		0,
		0);
}

unsigned char get_led(uint8_t request) {
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
	return buffer[0];
}

int usb_hotplug_callback(struct libusb_context *ctx, struct libusb_device *dev,
		     libusb_hotplug_event event, void *user_data) {
	if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
		if (libusb_open(dev, &handle) != LIBUSB_SUCCESS) {
			fprintf(stderr, "Error: could not open USB device\n");
		} else {
			libusb_claim_interface(handle, 0);

			connected = true;
			red_bar = (get_led(USB_LED_GET_RED) + 1) * 100 / 255;
			green_bar = (get_led(USB_LED_GET_GREEN) + 1) * 100 / 255;
			blue_bar = (get_led(USB_LED_GET_BLUE) + 1) * 100 / 255;
			draw_window(true);
		}
	} else if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
		if (handle) {
			libusb_release_interface(handle, 0);
			libusb_close(handle);
			handle = NULL;

			connected = false;
			red_bar = 0;
			green_bar = 0;
			blue_bar = 0;
			draw_window(true);
		}
	} else {
		printf("Warning: unhandled event %d\n", event);
	}

	return 0;
}

int main(void) {
	libusb_hotplug_callback_handle handle_hotplug;

	libusb_init(NULL);

	if (libusb_hotplug_register_callback(NULL,
				LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
				LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
				0,
				VID, PID,
				LIBUSB_HOTPLUG_MATCH_ANY,
				usb_hotplug_callback,
				NULL,
				&handle_hotplug) != LIBUSB_SUCCESS) {
		fprintf(stderr, "Error: unable to create hotplug callback\n");
		libusb_exit(NULL);
		exit(1);
	}

	handle = libusb_open_device_with_vid_pid(NULL, VID, PID);
	if (handle) {
		libusb_claim_interface(handle, 0);
		connected = true;
		red_bar = (get_led(USB_LED_GET_RED) + 1) * 100 / 255;
		green_bar = (get_led(USB_LED_GET_GREEN) + 1) * 100 / 255;
		blue_bar = (get_led(USB_LED_GET_BLUE) + 1) * 100 / 255;
	}

	pthread_t usb_thread;
	if (pthread_create(&usb_thread, NULL, usb_function, NULL)) {
		fprintf(stderr, "Error: unable to create thread\n");
		exit(1);
	}

	if ((window = initscr()) == NULL) {
		fprintf(stderr, "Error: unable to initialize ncurses\n");
		exit(1);
	}

	if (getmaxx(window) < WIN_MINX || getmaxy(window) < WIN_MINY) {
		goto cleanup;
	}

	curs_set(0);
	noecho();
	keypad(window, TRUE);

	draw_window(false);

	int key;
	while ((key = getch()) != 'q') {
		switch (key) {
			case KEY_RESIZE:
				if (getmaxx(window) < WIN_MINX ||
				    getmaxy(window) < WIN_MINY) {
					goto cleanup;
				}
				draw_window(false);
			break;

			case KEY_UP:
				if (selected > 0) selected--;
				draw_window(false);
			break;

			case KEY_DOWN:
				if (selected < 2) selected++;
				draw_window(false);
			break;

			case KEY_PPAGE:
				if (!connected) break;
				switch (selected) {
				case selection_red:
					if (red_bar < 90) red_bar += 10;
					else red_bar = 100;
					set_led(USB_LED_SET_RED,
						red_bar * 255 / 100);
				break;

				case selection_green:
					if (green_bar < 90) green_bar += 10;
					else green_bar = 100;
					set_led(USB_LED_SET_GREEN,
						green_bar * 255 / 100);
				break;

				case selection_blue:
					if (blue_bar < 90) blue_bar += 10;
					else blue_bar = 100;
					set_led(USB_LED_SET_BLUE,
						blue_bar * 255 / 100);
				break;
				}
				draw_window(false);
			break;

			case KEY_NPAGE:
				if (!connected) break;
				switch (selected) {
				case selection_red:
					if (red_bar > 10) red_bar -= 10;
					else red_bar = 0;
					set_led(USB_LED_SET_RED,
						red_bar * 255 / 100);
				break;

				case selection_green:
					if (green_bar > 10) green_bar -= 10;
					else green_bar = 0;
					set_led(USB_LED_SET_GREEN,
						green_bar * 255 / 100);
				break;

				case selection_blue:
					if (blue_bar > 10) blue_bar -= 10;
					else blue_bar = 0;
					set_led(USB_LED_SET_BLUE,
						blue_bar * 255 / 100);
				break;
				}
				draw_window(false);
			break;

			case KEY_HOME:
				if (!connected) break;
				switch (selected) {
				case selection_red:
					red_bar = 0;
					set_led(USB_LED_SET_RED, 0);
				break;

				case selection_green:
					green_bar = 0;
					set_led(USB_LED_SET_GREEN, 0);
				break;

				case selection_blue:
					blue_bar = 0;
					set_led(USB_LED_SET_BLUE, 0);
				break;
				}
				draw_window(false);
			break;

			case KEY_END:
				if (!connected) break;
				switch (selected) {
				case selection_red:
					red_bar = 100;
					set_led(USB_LED_SET_RED, 255);
				break;

				case selection_green:
					green_bar = 100;
					set_led(USB_LED_SET_GREEN, 255);
				break;

				case selection_blue:
					blue_bar = 100;
					set_led(USB_LED_SET_BLUE, 255);
				break;
				}
				draw_window(false);
			break;

			case KEY_LEFT:
			case KEY_RIGHT:
				if (!connected) break;
				switch (selected) {
				case selection_red:
					if (key == KEY_LEFT) {
						if (red_bar > 0)
							red_bar--;
					} else {
						if (red_bar < 100)
							red_bar++;
					}
					set_led(USB_LED_SET_RED,
						red_bar * 255 / 100);
				break;

				case selection_green:
					if (key == KEY_LEFT) {
						if (green_bar > 0)
							green_bar--;
					} else {
						if (green_bar < 100)
							green_bar++;
					}
					set_led(USB_LED_SET_GREEN,
						green_bar * 255 / 100);
				break;

				case selection_blue:
					if (key == KEY_LEFT) {
						if (blue_bar > 0)
							blue_bar--;
					} else {
						if (blue_bar < 100)
							blue_bar++;
					}
					set_led(USB_LED_SET_BLUE,
						blue_bar * 255 / 100);
				break;
				}
				draw_window(false);
			break;
		}
	}

cleanup:
	if (handle) {
		libusb_release_interface(handle, 0);
		libusb_close(handle);
		handle = NULL;
	}

	pthread_cancel(usb_thread);
	pthread_join(usb_thread, NULL);

	delwin(window);
	endwin();

	libusb_hotplug_deregister_callback(NULL, handle_hotplug);
	libusb_exit(NULL);

	return 0;
}
