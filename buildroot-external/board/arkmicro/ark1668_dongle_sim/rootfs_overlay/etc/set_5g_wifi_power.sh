#!/bin/sh
#get
rtwpriv wlan0 efuse_get rmap,22,14
usleep 20000
rtwpriv wlan0 efuse_get rmap,3A,11
usleep 20000

#set TX power index:5G
rtwpriv wlan0 efuse_set wmap,22,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,23,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,24,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,25,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,26,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,27,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,28,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,29,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,2A,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,2B,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,2C,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,2D,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,2E,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,2F,23
usleep 20000
rtwpriv wlan0 efuse_set wmap,30,02
usleep 20000

#set TX power index:2.4G
rtwpriv wlan0 efuse_set wmap,3A,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,3B,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,3C,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,3D,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,3E,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,3F,18
usleep 20000
rtwpriv wlan0 efuse_set wmap,40,20
usleep 20000
rtwpriv wlan0 efuse_set wmap,41,20
usleep 20000
rtwpriv wlan0 efuse_set wmap,42,20
usleep 20000
rtwpriv wlan0 efuse_set wmap,43,20
usleep 20000
rtwpriv wlan0 efuse_set wmap,44,20
usleep 20000
rtwpriv wlan0 efuse_set wmap,45,02
usleep 20000

#get
rtwpriv wlan0 efuse_get rmap,22,14
usleep 20000
rtwpriv wlan0 efuse_get rmap,3A,11
