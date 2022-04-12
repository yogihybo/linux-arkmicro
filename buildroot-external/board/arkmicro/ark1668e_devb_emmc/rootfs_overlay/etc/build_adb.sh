#!/bin/sh

if [ $# -eq 0 ];
then
    usb_idx=0
else
	usb_idx=$1
fi


# config adb function
KER_CONF="`grep '/sys/kernel/config' /proc/mounts`"
if  [ -z "$KER_CONF" ];  then
    mount -t configfs none /sys/kernel/config
fi

if  [ ! -d /sys/kernel/config/usb_gadget/g1 ];  then
    mkdir /sys/kernel/config/usb_gadget/g1
fi

if  [ ! -d /sys/kernel/config/usb_gadget/g1/strings/0x409 ];  then
    mkdir /sys/kernel/config/usb_gadget/g1/strings/0x409
fi

if  [ ! -d /sys/kernel/config/usb_gadget/g1/functions/adb.g1 ];  then
    mkdir /sys/kernel/config/usb_gadget/g1/functions/adb.g1
fi

if  [ ! -d /sys/kernel/config/usb_gadget/g1/configs/c.1 ];  then
    mkdir /sys/kernel/config/usb_gadget/g1/configs/c.1
fi

if  [ ! -d /sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409 ];  then
    mkdir /sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409
fi

echo "0x18d1" > /sys/kernel/config/usb_gadget/g1/idVendor
echo "0x0002" > /sys/kernel/config/usb_gadget/g1/idProduct

echo "012345678adcdef" > /sys/kernel/config/usb_gadget/g1/strings/0x409/serialnumber
echo "Google.Inc" > /sys/kernel/config/usb_gadget/g1/strings/0x409/manufacturer
echo "adb device debug" > /sys/kernel/config/usb_gadget/g1/strings/0x409/product

echo 0xc0 > /sys/kernel/config/usb_gadget/g1/configs/c.1/bmAttributes
echo 500 > /sys/kernel/config/usb_gadget/g1/configs/c.1/MaxPower

if  [ ! -L /sys/kernel/config/usb_gadget/g1/configs/c.1/adb.g1 ];  then
    ln -s /sys/kernel/config/usb_gadget/g1/functions/adb.g1/ /sys/kernel/config/usb_gadget/g1/configs/c.1/adb.g1
fi

# start adbd daemon
killall adbd
adbd &

# enable udc
echo musb-hdrc.$usb_idx > /sys/kernel/config/usb_gadget/g1/UDC
if [ $usb_idx -eq 0 ];
then
	echo peripheral > /sys/devices/platform/soc/e0100000.usb/musb-hdrc.$usb_idx/mode
else
	echo peripheral > /sys/devices/platform/soc/e0400000.usb/musb-hdrc.$usb_idx/mode
fi

