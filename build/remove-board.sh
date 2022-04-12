#!/bin/bash

usage() {
	echo "Usage: remove-board.sh board_type"
}

if [ $# -ne 1 ]; then
    usage
    exit
fi

if [ ! -f $1.config ]; then
	echo "board is not exist"
	exit
fi

SCRIPTS_DIR=`cd $(dirname $0); pwd -P`
SDK_DIR=${SCRIPTS_DIR}/..
CONFIG_FILE=$1.config
BOARD_TYPE=$(grep "BOARD_TYPE" $CONFIG_FILE | awk -F= '{print $2}')
UBOOT_CONFIG_FILE=$(grep "UBOOT_CONFIG_FILE" $CONFIG_FILE | awk -F= '{print $2}')
LINUX_CONFIG_FILE=$(grep "LINUX_CONFIG_FILE" $CONFIG_FILE | awk -F= '{print $2}')
BR_CONFIG_FILE=$(grep "BR_CONFIG_FILE" $CONFIG_FILE | awk -F= '{print $2}')
DTB_FILE_NAME=$(grep "DTB_FILE_NAME" $CONFIG_FILE | awk -F= '{print $2}')
DTS_FILE_NAME=${DTB_FILE_NAME/dtb/dts}

UBOOT_CONFIG_FILE_PATH=$SDK_DIR/u-boot/configs/$UBOOT_CONFIG_FILE
UBOOT_TARGET=$(grep "CONFIG_TARGET_" $UBOOT_CONFIG_FILE_PATH | awk -F= '{print $1}')
UBOOT_TARGET=${UBOOT_TARGET:14}
UBOOT_BOARD_DIR=${UBOOT_TARGET,,}
if [ ! -d $SDK_DIR/u-boot/board/arkmicro/$UBOOT_BOARD_DIR ];then
UBOOT_BOARD_DIR=${UBOOT_BOARD_DIR//_/-}
fi
UBOOT_BOARD_PATH=$SDK_DIR/u-boot/board/arkmicro/$UBOOT_BOARD_DIR
UBOOT_CONFIG_HEADER_FILE=$(sed -n '/SYS_CONFIG_NAME/{n;p;}' $UBOOT_BOARD_PATH/Kconfig | awk '{print $2}')
UBOOT_CONFIG_HEADER_FILE=${UBOOT_CONFIG_HEADER_FILE//\"/}.h

rm -rf $SCRIPTS_DIR/$CONFIG_FILE
rm -rf $SDK_DIR/u-boot/configs/$UBOOT_CONFIG_FILE
rm -rf $SDK_DIR/u-boot/board/arkmicro/$UBOOT_BOARD_DIR
rm -rf $SDK_DIR/u-boot/include/configs/$UBOOT_CONFIG_HEADER_FILE
rm -rf $SDK_DIR/u-boot/arch/arm/dts/$DTS_FILE_NAME
rm -rf $SDK_DIR/linux/arch/arm/configs/$LINUX_CONFIG_FILE
rm -rf $SDK_DIR/linux/arch/arm/boot/dts/$DTS_FILE_NAME
rm -rf $SDK_DIR/buildroot-external/configs/$BR_CONFIG_FILE
rm -rf $SDK_DIR/buildroot-external/board/arkmicro/$BOARD_TYPE

sed -i /TARGET_$UBOOT_TARGET/d $SDK_DIR/u-boot/arch/arm/mach-arkmicro/Makefile
sed -i "/TARGET_$UBOOT_TARGET\>/,/^$/{d}" ${SDK_DIR}/u-boot/arch/arm/mach-arkmicro/Kconfig
sed -i /$UBOOT_BOARD_DIR/d ${SDK_DIR}/u-boot/arch/arm/mach-arkmicro/Kconfig
sed -i /$DTB_FILE_NAME/d $SDK_DIR/u-boot/arch/arm/dts/Makefile
sed -i /$DTB_FILE_NAME/d $SDK_DIR/linux/arch/arm/boot/dts/Makefile
