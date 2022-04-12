#!/bin/bash

UBOOT_DIR=${BR2_EXTERNAL_ARK_PATH}/../u-boot
UBOOT_OUTPUT_DIR=${BR2_EXTERNAL_ARK_PATH}/../output/board/arkn141_dongle_sim/u-boot

#install modules to target
source ${BR2_EXTERNAL_ARK_PATH}/../env.source
cd ${BR2_EXTERNAL_ARK_PATH}/../output/board/arkn141_dongle_sim/linux
make INSTALL_MOD_PATH=${TARGET_DIR} modules_install

cd ${TARGET_DIR}/usr/bin
rm -rf hostapd test* live555* playSIP openRTSP vobStreamer registerRTSPStream
ln -s ../sbin/hostapd hostapd
cd ${TARGET_DIR}/lib
rm -rf debug/ libanl* libgfortran* libgomp*
cd ${TARGET_DIR}/usr/lib
rm -rf libnl-idiag* libnl-nf* libnl-xfrm*
cd ${TARGET_DIR}/usr/share/alsa
rm -rf cards/ pcm/ topology/ ucm/

cp -f ${UBOOT_OUTPUT_DIR}/tools/env/fw_printenv ${TARGET_DIR}/usr/bin/
cp ${UBOOT_DIR}/tools/env/fw_env.config ${TARGET_DIR}/etc
cd ${TARGET_DIR}/usr/bin/ && \
   ln -sf fw_printenv fw_setenv
