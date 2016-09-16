VERSION=0.5

obj-m += driver/usb-led.o
ifndef KERNEL_DIR
KDIR='/lib/modules/$(shell uname -r)/build'
endif

all: usb-led-bootloader.hex usb-led-firmware.hex usb-led-cli usb-led-tui usb-led.ko

usb-led-bootloader.hex: bootloader/usb-led-bootloader.hex
	cp bootloader/usb-led-bootloader.hex .

usb-led-firmware.hex: firmware/usb-led-firmware.hex
	cp firmware/usb-led-firmware.hex .

firmware/usb-led-firmware.hex: firmware/usb-led-firmware.c
	@make --no-print-directory -C firmware

usb-led-cli: cli/usb-led-cli
	cp cli/usb-led-cli .

cli/usb-led-cli: cli/usb-led-cli.c
	@make --no-print-directory -C cli

usb-led-tui: tui/usb-led-tui
	cp tui/usb-led-tui .

tui/usb-led-tui: tui/usb-led-tui.c
	@make --no-print-directory -C tui

usb-led.ko: driver/usb-led.ko
	cp driver/usb-led.ko .

driver/usb-led.ko: driver/usb-led.c
	make -C $(KDIR) M=$(PWD)/driver modules

flashbootloader: usb-led-bootloader.hex
	avrdude -c usbtiny -p attiny85 -U flash:w:usb-led-bootloader.hex:i -B 20

flashfuse:
	avrdude -c usbtiny -p attiny85 -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m -B 20

flash: usb-led-firmware.hex
	micronucleus --run usb-led-firmware.hex

release: all
	@echo creating release tarball
	@mkdir -p release/usb-led-${VERSION}
	@cp release/Makefile.release release/usb-led-${VERSION}/Makefile
	@cp usb-led-bootloader.hex usb-led-firmware.hex usb-led-cli usb-led-tui usb-led.ko release/usb-led-${VERSION}
	@tar -C release -cf release/usb-led-${VERSION}.tar usb-led-${VERSION}/
	@gzip release/usb-led-${VERSION}.tar
	@rm -rf release/usb-led-${VERSION}

clean:
	$(RM) usb-led-bootloader.hex
	@make --no-print-directory -C firmware clean
	$(RM) usb-led-firmware.hex
	@make --no-print-directory -C cli clean
	$(RM) usb-led-cli
	@make --no-print-directory -C tui clean
	$(RM) usb-led-tui
	@make -C $(KDIR) M=$(PWD)/driver clean
	$(RM) usb-led.ko

.PHONY: all flashbootloader flashfuse flash release clean
