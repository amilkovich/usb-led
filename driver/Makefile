obj-m += usb-led.o

ifndef KERNEL_DIR
KDIR='/lib/modules/$(shell uname -r)/build'
endif

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

.PHONY: all clean
