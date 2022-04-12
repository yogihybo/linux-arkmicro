#!/bin/bash

UBOOT_DIR=${BR2_EXTERNAL_ARK_PATH}/../u-boot
UBOOT_OUTPUT_DIR=${BR2_EXTERNAL_ARK_PATH}/../output/board/tyw-dashboard/u-boot

#install modules to target
source ${BR2_EXTERNAL_ARK_PATH}/../env.source
cd ${BR2_EXTERNAL_ARK_PATH}/../output/board/tyw-dashboard/linux
make INSTALL_MOD_PATH=${TARGET_DIR} modules_install

cp -f ${UBOOT_OUTPUT_DIR}/tools/env/fw_printenv ${TARGET_DIR}/usr/bin/
cp ${UBOOT_DIR}/tools/env/fw_env.config ${TARGET_DIR}/etc
cd ${TARGET_DIR}/usr/bin/ && \
   ln -sf fw_printenv fw_setenv
