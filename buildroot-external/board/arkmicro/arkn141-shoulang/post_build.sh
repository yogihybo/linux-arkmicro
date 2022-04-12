#!/bin/bash

#install modules to target
source ${BR2_EXTERNAL_ARK_PATH}/../env.source
cd ${BR2_EXTERNAL_ARK_PATH}/../output/board/ark-shoulang/linux
make INSTALL_MOD_PATH=${TARGET_DIR} modules_install
cd ${TARGET_DIR}/usr/bin
rm -rf hostapd
ln -s ../sbin/hostapd hostapd
