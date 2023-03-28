#! /bin/sh
QMAKE=/home/lfp/Works/1668eArmBuildroot/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/qmake
build()
{
	cd $1 && $QMAKE && make clean && make
	if [ $? -eq 0 ]; then
		cd ../
	fi
}
build UserInterface
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
build Utility
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
build RunnableThread
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
build ArkApplication
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
build AudioService
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
build AutoConnect
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
build DbusService
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
build MultimediaService
if [ $? != 0 ]; then
	echo build $1 error!
	exit 1
fi
echo ------------------------------------------------------------------------------------------------------------------------------------------
echo build sucess!
echo ------------------------------------------------------------------------------------------------------------------------------------------
