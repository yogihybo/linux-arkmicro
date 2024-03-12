#!/bin/bash -e
SCRIPTS_DIR=`cd $(dirname $0); pwd -P`
if [ ! -f "$SCRIPTS_DIR/$1.config" ]; then
	echo "Error! No such board config file $1.config."
	exit -1
fi
source $SCRIPTS_DIR/$1.config
SDK_DIR=${SCRIPTS_DIR}/..
if [[ $BR_DIR == "" ]]; then
	BR_DIR=${SDK_DIR}/buildroot
	TOOLCHAIN_NAME=gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf
else
	BR_DIR=${SDK_DIR}/$BR_DIR
	TOOLCHAIN_NAME=gcc-linaro-7.3.1-2018.05-x86_64_arm-linux-gnueabihf
fi
BR_EXTERNAL_DIR=${SDK_DIR}/buildroot-external
TOOLCHAIN_DIR=${BR_EXTERNAL_DIR}/toolchain
TOOLCHAIN_PACKAGE_DIR=${BR_DIR}/dl/toolchain-external-linaro-arm
OUTPUT_DIR=${SDK_DIR}/output/board/${BOARD_TYPE}
UBOOT_OUTPUT_DIR=${OUTPUT_DIR}/u-boot
LINUX_OUTPUT_DIR=${OUTPUT_DIR}/linux
BR_OUTPUT_DIR=${OUTPUT_DIR}/buildroot
IMAGES_DIR=${OUTPUT_DIR}/images
ubootclean() {
	cd ${UBOOT_OUTPUT_DIR}
	make distclean
}
linuxclean() {
	cd ${LINUX_OUTPUT_DIR}
	make distclean
}
brclean() {
	cd ${BR_OUTPUT_DIR}
	make clean
	#cd ${BR_OUTPUT_DR}
	#rm -rf target
	#find ${BR_OUTPUT_DR}/ -name ".stamp_target_installed" | xargs rm -rf
}
ubootbuild() {
	cd $SDK_DIR/u-boot
        make O=${UBOOT_OUTPUT_DIR} ${UBOOT_CONFIG_FILE}
	make O=${UBOOT_OUTPUT_DIR}
	make O=${UBOOT_OUTPUT_DIR} envtools
	if [ -f "${UBOOT_OUTPUT_DIR}/spl/u-boot-spl.bin" ]; then
		cp -rf ${UBOOT_OUTPUT_DIR}/spl/u-boot-spl.bin $IMAGES_DIR/ubootspl.bin
	fi
	if [ -f "${UBOOT_OUTPUT_DIR}/u-boot.img" ]; then
		cp -rf ${UBOOT_OUTPUT_DIR}/u-boot.img $IMAGES_DIR
	fi
	cp -rf ${UBOOT_OUTPUT_DIR}/u-boot.bin $IMAGES_DIR
	touch $IMAGES_DIR/update-magic
	echo "ada7f0c6-7c86-11e9-8f9e-2a86e4085a59" > $IMAGES_DIR/update-magic
}
linuxbuild() {
	cd $SDK_DIR/linux
	make O=${LINUX_OUTPUT_DIR} ${LINUX_CONFIG_FILE}
	make O=${LINUX_OUTPUT_DIR}
	cp -rf ${LINUX_OUTPUT_DIR}/arch/arm/boot/dts/${DTB_FILE_NAME} $IMAGES_DIR
	cp -rf ${LINUX_OUTPUT_DIR}/arch/arm/boot/zImage $IMAGES_DIR
}
brbuild() {
	cd $BR_DIR
	make BR2_EXTERNAL=${BR_EXTERNAL_DIR} ${BR_CONFIG_FILE} O=${BR_OUTPUT_DIR}
	make O=${BR_OUTPUT_DIR}
	if [ -f "${BR_OUTPUT_DIR}/images/rootfs.ubi" ]; then
		cp -rf ${BR_OUTPUT_DIR}/images/rootfs.ubi $IMAGES_DIR
	fi
	if [ -f "${BR_OUTPUT_DIR}/images/rootfs.ext2" ]; then
		cp -rf ${BR_OUTPUT_DIR}/images/rootfs.ext2 $IMAGES_DIR
	fi
}
ubootconfig() {
	cd $SDK_DIR/u-boot
	make O=${UBOOT_OUTPUT_DIR} ${UBOOT_CONFIG_FILE}
	cd ${UBOOT_OUTPUT_DIR}
	make menuconfig
	make savedefconfig
	cp defconfig $SDK_DIR/u-boot/configs/${UBOOT_CONFIG_FILE}
}
linuxconfig() {
	cd $SDK_DIR/linux
	make O=${LINUX_OUTPUT_DIR} ${LINUX_CONFIG_FILE}
	cd ${LINUX_OUTPUT_DIR}
	make menuconfig
	make savedefconfig
	cp defconfig $SDK_DIR/linux/arch/arm/configs/${LINUX_CONFIG_FILE}
}
brconfig() {
	cd $BR_DIR
	make BR2_EXTERNAL=${BR_EXTERNAL_DIR} ${BR_CONFIG_FILE} O=${BR_OUTPUT_DIR}
	cd ${BR_OUTPUT_DIR}
	make menuconfig
	make savedefconfig
}
if [ "$2" == "clean" ] ; then
	if [ "$3" == "" ] ; then
		echo "clean all..."
		ubootclean
		linuxclean
		brclean
		exit $?
	elif [ "$3" == "uboot" ] ; then
		echo "clean u-boot..."
		ubootclean
		exit $?
	elif [ "$3" == "linux" ] ; then
		echo "clean linux..."
		linuxclean
		exit $?
	elif [ "$3" == "buildroot" ] ; then
		echo "clean buildroot..."
		brclean
		exit $?
	fi
elif [ "$2" == "build" ] ; then
	if [ ! -d $TOOLCHAIN_DIR/$TOOLCHAIN_NAME ] ; then
		mkdir -p $TOOLCHAIN_DIR
		xz -vkd $TOOLCHAIN_PACKAGE_DIR/$TOOLCHAIN_NAME.tar.xz
		tar -xvf $TOOLCHAIN_PACKAGE_DIR/$TOOLCHAIN_NAME.tar -C $TOOLCHAIN_DIR
		rm -rf $TOOLCHAIN_PACKAGE_DIR/$TOOLCHAIN_NAME.tar
	fi
	source $SDK_DIR/env.source
	mkdir -p $IMAGES_DIR
	cp -rf $SDK_DIR/bootstrap/${BOOTSTRAP_DIR}/* $IMAGES_DIR
	cp -rf $SDK_DIR/tools/crc/crc.sh   $IMAGES_DIR
	cp -rf $SDK_DIR/tools/crc/crc32app $IMAGES_DIR
	if [ "$3" == "" ] ; then
		echo "build all..."
		ubootbuild
		linuxbuild
		brbuild
		cd $IMAGES_DIR
		./crc.sh
		rm -rf $IMAGES_DIR/crc.sh
		rm -rf $IMAGES_DIR/crc32app
		exit $?
	elif [ "$3" == "uboot" ] ; then
		echo "build u-boot..."
		ubootbuild
		cd $IMAGES_DIR
		./crc.sh
		rm -rf $IMAGES_DIR/crc.sh
		rm -rf $IMAGES_DIR/crc32app
		exit $?
	elif [ "$3" == "linux" ] ; then
		echo "build linux..."
		linuxbuild
		cd $IMAGES_DIR
		./crc.sh
		rm -rf $IMAGES_DIR/crc.sh
		rm -rf $IMAGES_DIR/crc32app
		exit $?
	elif [ "$3" == "buildroot" ] ; then
		echo "build buildroot..."
		brbuild
		cd $IMAGES_DIR
		./crc.sh
		rm -rf $IMAGES_DIR/crc.sh
		rm -rf $IMAGES_DIR/crc32app
		exit $?
	fi
elif [ "$2" == "config" ] ; then
	if [ ! -d $TOOLCHAIN_DIR/$TOOLCHAIN_NAME ] ; then
		mkdir -p $TOOLCHAIN_DIR
		xz -vkd $TOOLCHAIN_PACKAGE_DIR/$TOOLCHAIN_NAME.tar.xz
		tar -xvf $TOOLCHAIN_PACKAGE_DIR/$TOOLCHAIN_NAME.tar -C $TOOLCHAIN_DIR
		rm -rf $TOOLCHAIN_PACKAGE_DIR/$TOOLCHAIN_NAME.tar
	fi
	source $SDK_DIR/env.source
	if [ "$3" == "uboot" ] ; then
		echo "config u-boot..."
		ubootconfig
		exit $?
	elif [ "$3" == "linux" ] ; then
		echo "config linux..."
		linuxconfig
		exit $?
	elif [ "$3" == "buildroot" ] ; then
		echo "config buildroot..."
		brconfig
		exit $?
	fi
fi
