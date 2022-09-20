PROJECT = carlink_c
STAGING_DIR := /media/zhouyu/work/bsp/linux-arkmicro/output/board/ark1668e_devb/buildroot/staging
CROSS_COMPILE = /opt/arm-buildroot-linux-gnueabihf_sdk-buildroot/bin/arm-linux-gnueabihf-

DEFINES = -DUSE_CARPLAY -DUSE_AUTO
CFLAGS = -Wall -fPIC -o2 -pipe $(DEFINES) -I. -I../../include/webrtc -I../../include/auto -I../../include/carlife -I../../include/hicar -I../../include/carplay -I../../include/eclink -I$(STAGING_DIR)/usr/include/libusb-1.0 -I../../include/mirror -I../../include/user -I../../cmd

LDFLAG = -L$(STAGING_DIR)/usr/lib -lmfc -larkapi -lasound -lprotobuf -lssl -lcrypto -lfdk-aac -lusb-1.0 -L../../lib/carplay -lAirPlay -lAirPlaySupport -lAudioConverter -lAudioConverter_dummy -lAudioStream -lcarplaymisc -lCarplayWrapper -lCoreUtils -liap2link -lmyutils -lScreenStream -L../../lib/auto -lAndroidAuto -larkCarlinkUtils -L../../lib/carlife -lcarlifeplayer -lcarlifevehicle  -L../../lib/eclink -L../../lib/mirror -L../../lib/user -lUserInterface -lstdc++



GCC = $(CROSS_COMPILE)gcc
SRCS = $(wildcard *.cpp ../../UserInterface/*.cpp ../../cmd/CarLinkPlayer.cpp ../../cmd/CarLinkWrapper.cpp ./main.c)
OBJS = $(patsubst %.cpp, %.o, $(SRCS))

$(PROJECT):$(OBJS)
	$(GCC) $(LDFLAG) $^ -o $@
%.o:%.cpp
	$(GCC) $(CFLAGS) -c $< -o $@

clean:
	rm -f ../../UserInterface/*.o ../../cmd/*.o ./*.o
	rm -f carlink_c
