#!/bin/sh

insmod /lib/modules/3.4.0/kernel/drivers/usb/musb/musb_hdrc.ko
insmod /lib/modules/3.4.0/kernel/drivers/usb/musb/ark1680_musb.ko
insmod /lib/modules/3.4.0/kernel/drivers/usb/gadget/g_ncm.ko
echo otg > /sys/devices/platform/musb-ark1680.0/musb-hdrc.0/mode




