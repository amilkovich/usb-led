## usb-led

usb-led is a collection of projects that explore an rgb led through usb and the
hardware, firmware, Linux kernel driver/tui application/cli application/etc.
Here are some pictures of the terminal based software and the hardware
prototype.

![usb-led-disconnected.png](pictures/usb-led-disconnected.png?raw=true)
![usb-led-connected.png](pictures/usb-led-connected.png?raw=true)
![usb-led-on.png](pictures/usb-led-on.png?raw=true)
![usb-led-top.png](pictures/usb-led-top.png?raw=true)
![usb-led-bottom.png](pictures/usb-led-bottom.png?raw=true)

## FAQ

### What is usb-led?

usb-led is a small code repository for a small usb led hardware project based
on the Atmel ATtiny85 microcontroller.

### What license is usb-led release under?

usb-led driver is released under GPLv2, see driver/LICENSE for details.

All other usb-led projects are released under the MIT/X Consortium License, see
LICENSE file for details.

Currently, the library dependencies are:

micronucleus (LGPLv2, included as binary)
	http://www.obdev.at/vusb/
v-usb (LGPLv2, included as source)
	http://www.obdev.at/vusb/

### What bootloader is used for this project?

Currently micronucleus is used as the bootloader.

### Where can I find the schematic for this project?

This is under the TODO list if I decide to take this project any further.

### What is the description that shows up under lsusb?

It is planned if this project continues to be developed to get a custom
vid/pid. Currently, shared vid/pid are used.

bootloader usb description:

	$ lsusb | grep DigiSpark
	Bus 003 Device 008: ID 16d0:0753 MCS Digistump DigiSpark

v-usb firmware code usb description:

	$ lsusb | grep Ooijen
	Bus 003 Device 009: ID 16c0:05dc Van Ooijen Technische Informatica shared ID for use with libusb

### How do you use usb-led-cli?

Some common examples of running usb-led-cli would be as follows:

	$ ./usb-led-cli
	Usage: usb-led-cli get red/green/blue
	       usb-led-cli set red/green/blue brightness
	$ ./usb-led-cli get red
	0
	$ ./usb-led-cli set red 255

### How do you use usb-led-tui?

	$ ./usb-led-tui # arrow keys (up/down/left/right) to adjust colors :)
	$ # also page up/page down/home/end are also useful
