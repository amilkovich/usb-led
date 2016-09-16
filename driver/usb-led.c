#include <linux/module.h>
#include <linux/slab.h>
#include <linux/usb.h>

#define VID 0x16c0
#define PID 0x05dc
#define USB_LED_SET_RED   0x0
#define USB_LED_GET_RED   0x1
#define USB_LED_SET_GREEN 0x2
#define USB_LED_GET_GREEN 0x3
#define USB_LED_SET_BLUE  0x4
#define USB_LED_GET_BLUE  0x5

static const struct usb_device_id id_table[] = {
	{ USB_DEVICE(VID, PID) },
	{ },
};
MODULE_DEVICE_TABLE(usb, id_table);

struct usb_led {
	struct usb_device *udev;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

static ssize_t show_red(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_led *led = usb_get_intfdata(intf);
	return sprintf(buf, "%hhu\n", led->red);
}

static ssize_t store_red(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_led *led = usb_get_intfdata(intf);
	unsigned char red;

	if (sscanf(buf, "%hhu", &red) == 1) {
		led->red = red;
		usb_control_msg(led->udev,
				usb_sndctrlpipe(led->udev, 0),
				USB_LED_SET_RED,
				USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
				red,
				0,
				NULL,
				0,
				USB_CTRL_GET_TIMEOUT);
	} else {
		return -EINVAL;
	}

	return count;
}

static ssize_t show_green(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_led *led = usb_get_intfdata(intf);
	return sprintf(buf, "%hhu\n", led->green);
}

static ssize_t store_green(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_led *led = usb_get_intfdata(intf);
	unsigned char green;

	if (sscanf(buf, "%hhu", &green) == 1) {
		led->green = green;
		usb_control_msg(led->udev,
				usb_sndctrlpipe(led->udev, 0),
				USB_LED_SET_GREEN,
				USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
				green,
				0,
				NULL,
				0,
				USB_CTRL_GET_TIMEOUT);
	} else {
		return -EINVAL;
	}

	return count;
}

static ssize_t show_blue(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_led *led = usb_get_intfdata(intf);
	return sprintf(buf, "%hhu\n", led->blue);
}

static ssize_t store_blue(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_led *led = usb_get_intfdata(intf);
	unsigned char blue;

	if (sscanf(buf, "%hhu", &blue) == 1) {
		led->blue = blue;
		usb_control_msg(led->udev,
				usb_sndctrlpipe(led->udev, 0),
				USB_LED_SET_BLUE,
				USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
				blue,
				0,
				NULL,
				0,
				USB_CTRL_GET_TIMEOUT);
	} else {
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(red, S_IRUGO | S_IWUSR, show_red, store_red);
static DEVICE_ATTR(green, S_IRUGO | S_IWUSR, show_green, store_green);
static DEVICE_ATTR(blue, S_IRUGO | S_IWUSR, show_blue, store_blue);

static int usb_led_probe(struct usb_interface *interface,
		     const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct usb_led *dev = NULL;
	int retval;
	unsigned char buffer[1];

	dev = kzalloc(sizeof(struct usb_led), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->udev = usb_get_dev(udev);

	usb_set_intfdata(interface, dev);

	retval = device_create_file(&interface->dev, &dev_attr_red);
	if (retval) {
		device_remove_file(&interface->dev, &dev_attr_red);
		usb_set_intfdata(interface, NULL);
		usb_put_dev(dev->udev);
		kfree(dev);
		return retval;
	}

	retval = device_create_file(&interface->dev, &dev_attr_green);
	if (retval) {
		device_remove_file(&interface->dev, &dev_attr_red);
		device_remove_file(&interface->dev, &dev_attr_green);
		usb_set_intfdata(interface, NULL);
		usb_put_dev(dev->udev);
		kfree(dev);
		return retval;
	}

	retval = device_create_file(&interface->dev, &dev_attr_blue);
	if (retval) {
		device_remove_file(&interface->dev, &dev_attr_red);
		device_remove_file(&interface->dev, &dev_attr_green);
		device_remove_file(&interface->dev, &dev_attr_blue);
		usb_set_intfdata(interface, NULL);
		usb_put_dev(dev->udev);
		kfree(dev);
		return retval;
	}

	usb_control_msg(dev->udev,
			usb_sndctrlpipe(dev->udev, 0),
			USB_LED_GET_RED,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
			0,
			0,
			(char *)buffer,
			sizeof(buffer),
			USB_CTRL_GET_TIMEOUT);

	dev->red = buffer[0];

	usb_control_msg(dev->udev,
			usb_sndctrlpipe(dev->udev, 0),
			USB_LED_GET_GREEN,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
			0,
			0,
			(char *)buffer,
			sizeof(buffer),
			USB_CTRL_GET_TIMEOUT);

	dev->green = buffer[0];

	usb_control_msg(dev->udev,
			usb_sndctrlpipe(dev->udev, 0),
			USB_LED_GET_BLUE,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_DIR_IN,
			0,
			0,
			(char *)buffer,
			sizeof(buffer),
			USB_CTRL_GET_TIMEOUT);

	dev->blue = buffer[0];

	dev_info(&interface->dev, "USB-LED device connected\n");

	return retval;
}

static void usb_led_disconnect(struct usb_interface *interface)
{
	struct usb_led *dev;

	dev = usb_get_intfdata(interface);

	device_remove_file(&interface->dev, &dev_attr_blue);
	device_remove_file(&interface->dev, &dev_attr_green);
	device_remove_file(&interface->dev, &dev_attr_red);

	usb_set_intfdata(interface, NULL);

	usb_put_dev(dev->udev);

	kfree(dev);

	dev_info(&interface->dev, "USB-LED device disconnected\n");
}

static struct usb_driver usb_led_driver = {
	.name = "usb-led",
	.probe = usb_led_probe,
	.disconnect = usb_led_disconnect,
	.id_table = id_table,
};

module_usb_driver(usb_led_driver);

MODULE_AUTHOR("Andrew Milkovich <amilkovich@gmail.com>");
MODULE_DESCRIPTION("USB-LED Driver");
MODULE_LICENSE("GPL");
