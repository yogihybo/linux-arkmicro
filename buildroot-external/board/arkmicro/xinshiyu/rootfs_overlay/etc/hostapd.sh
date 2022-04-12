#!/bin/sh
#kmem 0xe490005c 0x000000a6
mv /dev/random /dev/random.orig
ln -s /dev/urandom /dev/random
#insmod /lib/modules/3.4.0/ark_wlan.ko
insmod ./lib/modules/4.14.88/kernel/drivers/net/wireless/realtek/rtl8821cs/rtl8821cs.ko
#sleep 1
mkdir -p /var/lib/misc/
touch /var/lib/misc/udhcpd.leases
ifconfig wlan0 up
ifconfig wlan0 192.168.43.1 netmask 255.255.255.0
echo 1 > /proc/sys/net/ipv4/ip_forward 
echo 1 > /proc/sys/net/ipv6/conf/wlan0/disable_ipv6
udhcpd -f /etc/udhcpd.conf wlan0 &
hostapd -B /etc/hostapd/hostapd.conf
#route add default gw 192.168.2.1

