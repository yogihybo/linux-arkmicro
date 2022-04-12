#!/bin/sh

#Qt5 environment
export QTDIR=/usr/lib/qt5.15.2
export QT_QPA_PLATFORM_PLUGIN_PATH=$QTDIR/plugins
export QT_QPA_PLATFORM=linuxfb:tty=/dev/fb0
export QT_QPA_FONTDIR=/usr/share/fonts
export QT_QPA_GENERIC_PLUGINS=evdevtouch:/dev/input/event0
#export QT_LOGGING_RULES=qt.qpa.input=true
export LD_LIBRARY_PATH=/usr/lib:/lib:/usr/lib/qt5.15.2:$LD_LIBRARY_PATH

#decoder 
insmod /lib/modules/4.14.88/kernel/drivers/soc/arkmicro/hx170dec/hx170dec.ko

#wifi bluetooth
/etc/hostapd.sh
gocsdk >/dev/null 2>&1 &

#UI
Launcher &

