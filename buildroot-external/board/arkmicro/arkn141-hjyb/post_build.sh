#!/bin/bash

#install modules to target
source ${BR2_EXTERNAL_ARK_PATH}/../env.source
cd ${BR2_EXTERNAL_ARK_PATH}/../output/board/arkn141-hjyb/linux
make INSTALL_MOD_PATH=${TARGET_DIR} modules_install
cd ${TARGET_DIR}/usr/bin
rm -rf hostapd test* live555* playSIP openRTSP vobStreamer registerRTSPStream
ln -s ../sbin/hostapd hostapd
cd ${TARGET_DIR}/lib
rm -rf debug/ libanl* libgfortran* libgomp* libnss*
cd ${TARGET_DIR}/usr/lib
rm -rf libnl-idiag* libnl-nf* libnl-xfrm*
cd ${TARGET_DIR}/usr/share/alsa
rm -rf cards/ pcm/ topology/ ucm/
