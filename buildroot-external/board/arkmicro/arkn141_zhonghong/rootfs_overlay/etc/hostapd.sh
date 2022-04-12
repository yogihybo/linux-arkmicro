#!/bin/sh

#kmem 0xe490005c 0x000000a6
#insmod  /lib/modules/3.4.0/kernel/drivers/ark/sdmmc/ark_dw_mmc.ko
mv /dev/random /dev/random.orig
ln -s /dev/urandom /dev/random
#insmod /lib/modules/4.14.88/kernel/drivers/net/wireless/realtek/rtl8189fs/wlan.ko
insmod /lib/modules/4.14.88/kernel/drivers/net/wireless/realtek/rtl8821cs/rtl8821cs.ko
#sleep 1
mkdir -p /var/lib/misc/
touch /var/lib/misc/udhcpd.leases
ifconfig wlan0 up
ifconfig wlan0 192.168.2.1 netmask 255.255.255.0
echo 1 > /proc/sys/net/ipv4/ip_forward 
echo 1 > /proc/sys/net/ipv6/conf/wlan0/disable_ipv6
udhcpd -f /etc/udhcpd.conf wlan0 &
hostapd -B /etc/hostapd/hostapd.conf
#route add default gw 192.168.2.1

echo 2097152 > /proc/sys/net/core/rmem_default
echo 2097152 > /proc/sys/net/core/rmem_max
echo 1048576 > /proc/sys/net/core/wmem_default
echo 1048576 > /proc/sys/net/core/wmem_max
echo 0 > /proc/sys/net/ipv4/tcp_timestamps
echo 1 > /proc/sys/net/ipv4/tcp_sack
echo 1 > /proc/sys/net/ipv4/tcp_fack
echo 1 > /proc/sys/net/ipv4/tcp_window_scaling

