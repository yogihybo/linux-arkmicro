#!/bin/bash

#cd ${TARGET_DIR}/usr/lib
#ln -sf libMali.so libOpenVG.so
#ln -sf libMali.so libOpenVGU.so
#ln -sf libMali.so libGLESv2.so.2.0
#ln -sf libMali.so libEGL.so.1.4
#ln -sf libGLESv2.so.2.0 libGLESv2.so.2
#ln -sf libGLESv2.so.2 libGLESv2.so
#ln -sf libEGL.so.1.4 libEGL.so.1
#ln -sf libEGL.so.1 libEGL.so

#install modules to target
UBOOT_DIR=${BR2_EXTERNAL_ARK_PATH}/../u-boot
UBOOT_OUTPUT_DIR=${BR2_EXTERNAL_ARK_PATH}/../output/board/ark1668e_devb/u-boot

source ${BR2_EXTERNAL_ARK_PATH}/../env.source
cd ${BR2_EXTERNAL_ARK_PATH}/../output/board/ark1668e_devb/linux
make INSTALL_MOD_PATH=${TARGET_DIR} modules_install

cp -f ${UBOOT_OUTPUT_DIR}/tools/env/fw_printenv ${TARGET_DIR}/usr/bin/
cp ${UBOOT_DIR}/tools/env/fw_env.config ${TARGET_DIR}/etc
cd ${TARGET_DIR}/usr/bin/ && \
ln -sf fw_printenv fw_setenv

mkdir -p ${TARGET_DIR}/data

#sed -i '/# GENERIC_SERIAL$/s~^.*#~::respawn:-/bin/sh #~' ${TARGET_DIR}/etc/inittab
