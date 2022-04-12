mount -t configfs none /sys/kernel/config
cd /sys/kernel/config/usb_gadget/
mkdir -p g1
cd g1
echo "0x05ac"  > idVendor
echo "0x12a8" > idProduct
echo "0x1001" > bcdDevice
mkdir strings/0x409
echo "6a001f5ae423f030c687441ff4dbcb7dbf3e8b26" > strings/0x409/serialnumber
echo "Apple Inc."  > strings/0x409/manufacturer
echo "iPhone"  > strings/0x409/product
mkdir configs/c.1
echo 0xc0 > configs/c.1/bmAttributes
mkdir functions/ptp.usb0
mkdir configs/c.1/strings/0x409
echo "PTP" > configs/c.1/strings/0x409/configuration
ln -s functions/ptp.usb0 configs/c.1
mkdir configs/c.2
echo 0xc0 > configs/c.2/bmAttributes
mkdir functions/audio_sim.usb0
mkdir functions/hid.usb0
mkdir configs/c.2/strings/0x409
echo "iPod USB Interface" > configs/c.2/strings/0x409/configuration
ln -s functions/audio_sim.usb0 configs/c.2
ln -s functions/hid.usb0 configs/c.2
mkdir configs/c.3
echo 0xc0 > configs/c.3/bmAttributes
mkdir functions/ptp.usb0
mkdir functions/mux.usb0
mkdir configs/c.3/strings/0x409
echo "PTP + Apple Mobile Device" > configs/c.3/strings/0x409/configuration
ln -s functions/ptp.usb0 configs/c.3
ln -s functions/mux.usb0 configs/c.3
mkdir configs/c.4
echo 0xc0 > configs/c.4/bmAttributes
mkdir functions/ptp.usb0
mkdir functions/mux.usb0
mkdir functions/vsc.usb0
mkdir configs/c.4/strings/0x409
echo "PTP + Apple Mobile Device + Apple USB Ethernet" > configs/c.4/strings/0x409/configuration
ln -s functions/ptp.usb0 configs/c.4
ln -s functions/mux.usb0 configs/c.4
ln -s functions/vsc.usb0 configs/c.4
#echo musb-hdrc.0 > UDC
#echo peripheral > /sys/devices/platform/ahb/e0100000.usb/musb-hdrc.0/mode
echo musb-hdrc.1 > UDC
echo peripheral > /sys/devices/platform/ahb/e0400000.usb/musb-hdrc.1/mode


