#!/bin/sh

mv /dev/random /dev/random.orig
ln -s /dev/urandom /dev/random
#insmod /lib/modules/4.14.88/kernel/drivers/net/wireless/realtek/rtl8189fs/wlan.ko
insmod /lib/modules/4.14.88/kernel/drivers/net/wireless/realtek/rtl8821cs/rtl8821cs.ko
mkdir -p /var/lib/misc/
touch /var/lib/misc/udhcpd.leases
ifconfig wlan0 up
ifconfig wlan0 192.168.1.254 netmask 255.255.255.0
echo 1 > /proc/sys/net/ipv4/ip_forward 
udhcpd -f /etc/udhcpd.conf wlan0 &
hostapd -B /etc/hostapd.conf
route add default gw 192.168.1.254
