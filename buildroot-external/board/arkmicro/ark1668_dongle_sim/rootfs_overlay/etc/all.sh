#! /bin/sh

mount -t ramfs -n none /media
/sbin/mdev -s

#H264 and it565 driver, needed by media and carpaly etc.
cd /etc
./memalloc_load.sh
./driver_load.sh
./itu656_load.sh


#sd
insmod  /lib/modules/3.4.0/kernel/drivers/ark/sdmmc/ark_dw_mmc.ko

#audio
insmod  /lib/modules/3.4.0/kernel/drivers/ark/audio/ark-cs4334-codec.ko
insmod  /lib/modules/3.4.0/kernel/drivers/ark/audio/ark-sddac-codec.ko
insmod  /lib/modules/3.4.0/kernel/drivers/ark/audio/snd-soc-ark-i2s.ko
insmod  /lib/modules/3.4.0/kernel/drivers/ark/audio/snd-soc-ark-pcm-dma.ko
insmod  /lib/modules/3.4.0/kernel/drivers/ark/audio/snd-soc-ark-sddac.ko

#usb
insmod /lib/modules/3.4.0/kernel/drivers/usb/musb/musb_hdrc.ko
insmod /lib/modules/3.4.0/kernel/drivers/usb/musb/ark1680_musb.ko
insmod /lib/modules/3.4.0/kernel/drivers/usb/gadget/g_ncm.ko
echo otg > /sys/devices/platform/musb-ark1680.0/musb-hdrc.0/mode

#carplay
ifconfig lo up
ifconfig usb0 up
hostname CarPlay
mdnsd&

#2d
#insmod  /lib/modules/3.4.0/galcore.ko registerMemBase=0xE0F00000 irqLine=32 contiguousSize=0x400000 physSize=0x80000000 powerManagement=0

#ulimit -c unlimited
