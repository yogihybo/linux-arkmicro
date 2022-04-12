#!/bin/bash

SCRIPTS_DIR=`cd $(dirname $0); pwd -P`
SDK_DIR=${SCRIPTS_DIR}/..
BR_DIR=${SDK_DIR}/buildroot
BR_EXTERNAL_DIR=${SDK_DIR}/buildroot-external
TOOLCHAIN_DIR=${BR_EXTERNAL_DIR}/toolchain
TOOLCHAIN_NAME=gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf
TOOLCHAIN_PACKAGE_DIR=${BR_DIR}/dl/toolchain-external-linaro-arm
OUTPUT_DIR=${SDK_DIR}/output/board/${BOARD_TYPE}
UBOOT_OUTPUT_DIR=${OUTPUT_DIR}/u-boot
LINUX_OUTPUT_DIR=${OUTPUT_DIR}/linux
BR_OUTPUT_DIR=${OUTPUT_DIR}/buildroot
IMAGES_DIR=${OUTPUT_DIR}/images

usage() {
	echo "Usage: add-board.sh src_board_type dst_board_type"
}

if [ $# -ne 2 ]; then
    usage
    exit
fi

if [ ! -f $1.config ]; then
	echo "src board is not exist"
	exit
fi

DST_BOARD_TYPE=$2
DST_BOARD_TYPE=${DST_BOARD_TYPE,,}
echo "$DST_BOARD_TYPE" | grep "-" | grep "_" > /dev/null
if [ $? -eq 0 ]; then
DST_BOARD_TYPE=${DST_BOARD_TYPE//-/_}
fi
SRC_CONFIG_FILE=$1.config
SRC_BOARD_TYPE=$(grep "BOARD_TYPE" $SRC_CONFIG_FILE | awk -F= '{print $2}')
SRC_BOOTSTRAP_DIR=$(grep "BOOTSTRAP_DIR" $SRC_CONFIG_FILE | awk -F= '{print $2}')
SRC_UBOOT_CONFIG_FILE=$(grep "UBOOT_CONFIG_FILE" $SRC_CONFIG_FILE | awk -F= '{print $2}')
SRC_LINUX_CONFIG_FILE=$(grep "LINUX_CONFIG_FILE" $SRC_CONFIG_FILE | awk -F= '{print $2}')
SRC_BR_CONFIG_FILE=$(grep "BR_CONFIG_FILE" $SRC_CONFIG_FILE | awk -F= '{print $2}')
SRC_DTB_FILE_NAME=$(grep "DTB_FILE_NAME" $SRC_CONFIG_FILE | awk -F= '{print $2}')
SRC_DTS_FILE_NAME=${SRC_DTB_FILE_NAME/dtb/dts}
CONFIG_FILE=$SCRIPTS_DIR/$DST_BOARD_TYPE.config

SRC_UBOOT_CONFIG_FILE_PATH=$SDK_DIR/u-boot/configs/$SRC_UBOOT_CONFIG_FILE
SRC_UBOOT_TARGET=$(grep "CONFIG_TARGET_" $SRC_UBOOT_CONFIG_FILE_PATH | awk -F= '{print $1}')
SRC_UBOOT_TARGET=${SRC_UBOOT_TARGET:14}
SRC_UBOOT_BOARD_DIR=${SRC_UBOOT_TARGET,,}
if [ ! -d $SDK_DIR/u-boot/board/arkmicro/$SRC_UBOOT_BOARD_DIR ];then
SRC_UBOOT_BOARD_DIR=${SRC_UBOOT_BOARD_DIR//_/-}
fi
SRC_UBOOT_BOARD_PATH=$SDK_DIR/u-boot/board/arkmicro/$SRC_UBOOT_BOARD_DIR

# board build config file
touch $CONFIG_FILE
echo "BOARD_TYPE=$DST_BOARD_TYPE" > $CONFIG_FILE
echo "BOOTSTRAP_DIR=$SRC_BOOTSTRAP_DIR" >> $CONFIG_FILE
echo "UBOOT_CONFIG_FILE=${DST_BOARD_TYPE}_defconfig" >> $CONFIG_FILE
echo "LINUX_CONFIG_FILE=${DST_BOARD_TYPE}_defconfig" >> $CONFIG_FILE
echo "BR_CONFIG_FILE=${DST_BOARD_TYPE}_defconfig" >> $CONFIG_FILE
echo "DTB_FILE_NAME=$DST_BOARD_TYPE.dtb" >> $CONFIG_FILE
sed -n /BR_DIR/p $1.config >> $CONFIG_FILE

DST_DTB_FILE_NAME=$DST_BOARD_TYPE.dtb
DST_DTS_FILE_NAME=$DST_BOARD_TYPE.dts
DST_UBOOT_TARGET=${DST_BOARD_TYPE^^}
DST_UBOOT_TARGET=${DST_UBOOT_TARGET//-/_}
DST_UBOOT_BOARD_PATH=$SDK_DIR/u-boot/board/arkmicro/$DST_BOARD_TYPE

# board bootstrap files is the same as the src

########### board u-boot ################################
# add u-boot board config file
cp -rf ${SDK_DIR}/u-boot/configs/${SRC_UBOOT_CONFIG_FILE} ${SDK_DIR}/u-boot/configs/${DST_BOARD_TYPE}_defconfig
sed -i s/CONFIG_TARGET_$SRC_UBOOT_TARGET/CONFIG_TARGET_$DST_UBOOT_TARGET/g ${SDK_DIR}/u-boot/configs/${DST_BOARD_TYPE}_defconfig
sed -i s/$SRC_DTB_FILE_NAME/$DST_DTB_FILE_NAME/g ${SDK_DIR}/u-boot/configs/${DST_BOARD_TYPE}_defconfig

# copy dts file and add in Makefile
UBOOT_DTS_PATH=${SDK_DIR}/u-boot/arch/arm/dts
if [ -f ${SDK_DIR}/u-boot/arch/arm/dts/$SRC_DTS_FILE_NAME ];then
	cp -rf $UBOOT_DTS_PATH/$SRC_DTS_FILE_NAME $UBOOT_DTS_PATH/$DST_DTS_FILE_NAME
fi
grep "$DST_DTB_FILE_NAME" $UBOOT_DTS_PATH/Makefile > /dev/null
if [ $? -ne 0 ]; then
	sed -i "/$SRC_DTB_FILE_NAME/{p;s/$SRC_DTB_FILE_NAME/$DST_DTB_FILE_NAME/}" $UBOOT_DTS_PATH/Makefile
	grep "$SRC_DTB_FILE_NAME" $UBOOT_DTS_PATH/Makefile | grep '\\' > /dev/null
	if [ $? -ne 0 ]; then
		sed -i "s/$SRC_DTB_FILE_NAME/$SRC_DTB_FILE_NAME \\\\/" $UBOOT_DTS_PATH/Makefile
	fi
fi

# modify arch/arm/mach-arkmicro/Kconfig file add source "board/arkmicro/$DST_BOARD_TYPE/Kconfig"
UBOOT_ARCH_KCONFIG=${SDK_DIR}/u-boot/arch/arm/mach-arkmicro/Kconfig
grep "TARGET_$DST_UBOOT_TARGET" $UBOOT_ARCH_KCONFIG > /dev/null
if [ $? -ne 0 ]; then
sed -i "/TARGET_$SRC_UBOOT_TARGET\>/,/^$/{
	H
	s/TARGET_$SRC_UBOOT_TARGET/TARGET_$DST_UBOOT_TARGET/
	s/bool \"[^\"]*\"/bool \"$DST_BOARD_TYPE board\"/
	/^$/g
}" $UBOOT_ARCH_KCONFIG
fi
grep "$DST_BOARD_TYPE\/Kconfig" $UBOOT_ARCH_KCONFIG > /dev/null
if [ $? -ne 0 ]; then
sed -i "/$SRC_UBOOT_BOARD_DIR\/Kconfig/{h;s/$SRC_UBOOT_BOARD_DIR/$DST_BOARD_TYPE/;G}" $UBOOT_ARCH_KCONFIG
fi

# modify arch/arm/mach-arkmicro/Makefile file
UBOOT_ARCH_MAKEFILE=${SDK_DIR}/u-boot/arch/arm/mach-arkmicro/Makefile
grep "TARGET_$DST_UBOOT_TARGET" $UBOOT_ARCH_MAKEFILE > /dev/null
if [ $? -ne 0 ]; then
	sed -i "/TARGET_$SRC_UBOOT_TARGET)/{p;s/$SRC_UBOOT_TARGET/$DST_UBOOT_TARGET/}" $UBOOT_ARCH_MAKEFILE
fi

# add board dir board/arkmicro/$DST_BOARD_TYPE
mkdir -p $DST_UBOOT_BOARD_PATH
cp -rf $SRC_UBOOT_BOARD_PATH/* $DST_UBOOT_BOARD_PATH/
echo -e "if TARGET_$DST_UBOOT_TARGET\n\nconfig SYS_BOARD\n\tdefault \"$DST_BOARD_TYPE\"\n\nconfig SYS_VENDOR\n\tdefault \"arkmicro\"\n\n\
config SYS_CONFIG_NAME\n\tdefault \"$DST_BOARD_TYPE\"\n\nendif" > $DST_UBOOT_BOARD_PATH/Kconfig

# add configs header file
SRC_UBOOT_CONFIG_HEADER_FILE=$(sed -n '/SYS_CONFIG_NAME/{n;p;}' $SRC_UBOOT_BOARD_PATH/Kconfig | awk '{print $2}')
SRC_UBOOT_CONFIG_HEADER_FILE=${SRC_UBOOT_CONFIG_HEADER_FILE//\"/}
cp ${SDK_DIR}/u-boot/include/configs/$SRC_UBOOT_CONFIG_HEADER_FILE.h ${SDK_DIR}/u-boot/include/configs/$DST_BOARD_TYPE.h
#########################################################

########### board linux #################################
DTS_PATH=${SDK_DIR}/linux/arch/arm/boot/dts
LINUX_CONFIG_PATH=${SDK_DIR}/linux/arch/arm/configs
# copy dts file and add in Makefile
cp $DTS_PATH/$SRC_DTS_FILE_NAME $DTS_PATH/$DST_DTS_FILE_NAME
grep "$DST_DTB_FILE_NAME" $DTS_PATH/Makefile > /dev/null
if [ $? -ne 0 ]; then
	sed -i "/$SRC_DTB_FILE_NAME/{p;s/$SRC_DTB_FILE_NAME/$DST_DTB_FILE_NAME/}" $DTS_PATH/Makefile
	grep "$SRC_DTB_FILE_NAME" $DTS_PATH/Makefile | grep '\\' > /dev/null
	if [ $? -ne 0 ]; then
		sed -i "s/$SRC_DTB_FILE_NAME/$SRC_DTB_FILE_NAME \\\\/" $DTS_PATH/Makefile
	fi
fi
# copy config file
cp $LINUX_CONFIG_PATH/$SRC_LINUX_CONFIG_FILE $LINUX_CONFIG_PATH/${DST_BOARD_TYPE}_defconfig

#########################################################

########### board buildroot #############################
BR_CONFIG_PATH=${SDK_DIR}/buildroot-external/configs
BR_BOARD_PATH=${SDK_DIR}/buildroot-external/board/arkmicro
# copy buildroot config file and modify to dst board
SRC_BR_BOARD_DIR=$(grep "BR2_ROOTFS_OVERLAY" $BR_CONFIG_PATH/$SRC_BR_CONFIG_FILE | awk -F / '{print $4}')
cp $BR_CONFIG_PATH/$SRC_BR_CONFIG_FILE $BR_CONFIG_PATH/${DST_BOARD_TYPE}_defconfig
sed -i "s/$SRC_BR_BOARD_DIR/$DST_BOARD_TYPE/g" $BR_CONFIG_PATH/${DST_BOARD_TYPE}_defconfig

# add board dir board/arkmicro/$DST_BOARD_TYPE
mkdir -p $BR_BOARD_PATH/$DST_BOARD_TYPE
cp -rf $BR_BOARD_PATH/$SRC_BR_BOARD_DIR/* $BR_BOARD_PATH/$DST_BOARD_TYPE
sed -i "s/$SRC_BR_BOARD_DIR/$DST_BOARD_TYPE/g" $BR_BOARD_PATH/$DST_BOARD_TYPE/post_build.sh

#########################################################
