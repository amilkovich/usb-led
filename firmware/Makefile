CFLAGS = -Wall -Os -Iusbdrv -mmcu=attiny85 -DF_CPU=16500000
OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usb-led-firmware.o

all: usb-led-firmware.hex

usb-led-firmware.hex: usb-led-firmware.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

usb-led-firmware.elf: $(OBJECTS)
	avr-gcc $(CFLAGS) $(OBJECTS) -o $@

%.o: %.c
	avr-gcc $(CFLAGS) -c $< -o $@

%.o: %.S
	avr-gcc $(CFLAGS) -x assembler-with-cpp -c $< -o $@

$(OBJECTS): usbdrv/usbconfig.h

flash: usb-led-firmware.hex
	micronucleus --run $<

clean:
	$(RM) *.o *.hex *.elf usbdrv/*.o

.PHONY: all flash clean
