TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

#DEFINES += AEC_DELAY

if(contains(DEFINES, AEC_DELAY)){
DEFINES += WEBRTC_POSIX
DEFINES += WEBRTC_AUDIO_PROCESSING_ONLY_BUILD
}

DEFINES += USE_CARPLAY
DEFINES += USE_AUTO
DEFINES += USE_CARLIFE
DEFINES += USE_HICAR
#DEFINES += USE_MIRROR
#DEFINES += USE_EASYCONNECT

#DEFINES += CQLW
DEFINES += TYW02

INCLUDEPATH += ../UserInterface

INCLUDEPATH += ../../include
INCLUDEPATH += ../../include/user
INCLUDEPATH += ../../include/webrtc
INCLUDEPATH += ../../include/carlife
INCLUDEPATH += ../../include/carplay
INCLUDEPATH += ../../include/auto
INCLUDEPATH += ../../include/hicar
INCLUDEPATH += ../../include/mirror
INCLUDEPATH += ../../cmd
if(contains(DEFINES,CQLW)){
    INCLUDEPATH += ../../include/eclink/CQLW
}
if(contains(DEFINES,TYW02)){
    INCLUDEPATH += ../../include/eclink/TYW02
}


SDK_OUTPUT_PATH =/media/zhouyu/work/bsp/linux-arkmicro/output/board/ark1668e_devb/buildroot/staging/usr/


INCLUDEPATH += $$SDK_OUTPUT_PATH/include/
INCLUDEPATH += $$SDK_OUTPUT_PATH/include/dbus-1.0
INCLUDEPATH += $$SDK_OUTPUT_PATH/include/libusb-1.0/



LIBS += -L$$SDK_OUTPUT_PATH/lib -lmfc -larkapi -lpthread -lasound -lprotobuf  -lfdk-aac -ldbus-1

if(!contains(DEFINES,USE_EASYCONNECT)){
LIBS += -L$$SDK_OUTPUT_PATH/lib -lusb-1.0 -lcrypto -lssl
}

LIBS += -L$$PWD/../../lib/user -lUserInterface

if(contains(DEFINES, AEC_DELAY)){
LIBS += -L$$PWD/../../lib/user -lwebrtc_audio_processing -laudio_process
}

if(contains(DEFINES, USE_CARPLAY)){
LIBS += -L$$PWD/../../lib/carplay  -lAirPlay -lAirPlaySupport -lAudioConverter -lAudioConverter_dummy -lAudioStream -lcarplaymisc -lCarplayWrapper -lCoreUtils -liap2link -lmyutils -lScreenStream
}
if(contains(DEFINES, USE_CARLIFE)){
LIBS += -L$$PWD/../../lib/carlife  -lcarlifeplayer -lcarlifevehicle
}
if(contains(DEFINES, USE_AUTO)) {
LIBS += -L$$PWD/../../lib/auto  -lAndroidAuto -larkCarlinkUtils
}
if(contains(DEFINES, USE_HICAR)) {
LIBS += -L$$PWD/../../lib/hicar  -larkadapt -ldmsdpaudiohandler -lhicar -ldmsdp -ldmsdpcrypto -ldmsdpdvaudio -ldmsdpdvcamera -ldmsdpdvdevice -ldmsdpdvgps -ldmsdpdvinterface -ldmsdphisight -ldmsdpplatform -ldmsdpsec -lmanagement -lauthagent -lHisightSink -lsecurec -lHwDeviceAuthSDK -lHwKeystoreSDK -lnearby
}
if(contains(DEFINES, USE_MIRROR)) {
LIBS += -L$$PWD/../../lib/mirror  -lmirrorplayer
}
if(contains(DEFINES, USE_EASYCONNECT)) {
    if(contains(DEFINES,CQLW)){
        LIBS += -L$$PWD/../../lib/eclink/CQLW  -leclinkplayer -lECSDK -lECSDKFramework
    }
    if(contains(DEFINES,TYW02)){
        LIBS += -L$$PWD/../../lib/eclink/TYW02  -leclinkplayer -lECSDK -lECSDKFramework
    }
}
SOURCES += main.c \
    ../../UserInterface/WebrtcWrapper.cpp \
    ../../UserInterface/webrtc.cpp \
    ../../UserInterface/MirrorLink.cpp \
    ../../UserInterface/IUserLinkPlayer.cpp \
    ../../UserInterface/HiCarLink.cpp \
    ../../UserInterface/EasyConnectLink.cpp \
    ../../UserInterface/CarplayLinkcbsImpl.cpp \
    ../../UserInterface/CarplayLink.cpp \
    ../../UserInterface/CarplayAudioCtx.cpp \
    ../../UserInterface/CarlifeLink.cpp \
    ../../cmd/CarLinkPlayer.cpp \
    ../../cmd/CarLinkWrapper.cpp \
    ../../UserInterface/AutoLink.cpp

DISTFILES +=

HEADERS += \
    ../../cmd/CarLinkWrapper.h \
    ../../cmd/CarLinkPlayer.h
