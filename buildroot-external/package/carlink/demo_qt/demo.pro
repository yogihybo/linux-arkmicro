QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_CXXFLAGS += -std=c++0x
CONFIG += c++11

#DEFINES += SCAN_QRCODE

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

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
INCLUDEPATH += ../UserInterface

INCLUDEPATH += ../include
INCLUDEPATH += ../include/user
INCLUDEPATH += ../include/carlife
INCLUDEPATH += ../include/carplay
INCLUDEPATH += ../include/auto
INCLUDEPATH += ../include/hicar
INCLUDEPATH += ../include/mirror
INCLUDEPATH += ../include/webrtc

if(contains(DEFINES,CQLW)){
    INCLUDEPATH += ../include/eclink/CQLW
}
if(contains(DEFINES,TYW02)){
    INCLUDEPATH += ../include/eclink/TYW02
}

SDK_OUTPUT_PATH =/media/zhouyu/work/bsp/linux-arkmicro/output/board/ark1668e_devb/buildroot/staging/usr/

INCLUDEPATH += $$SDK_OUTPUT_PATH/include/
INCLUDEPATH += $$SDK_OUTPUT_PATH/include/libusb-1.0/


if(contains(DEFINES, SCAN_QRCODE)) {
LIBS += -L./ -lqrencode
}

DESTDIR = ../../bin


LIBS += -L$$SDK_OUTPUT_PATH/lib -lmfc -larkapi -lpthread -lasound -lprotobuf -lfdk-aac

if(!contains(DEFINES,USE_EASYCONNECT)){
LIBS += -L$$SDK_OUTPUT_PATH/lib -lusb-1.0 -lcrypto -lssl
}

LIBS += -L$$PWD/../lib/user -lUserInterface

if(contains(DEFINES, AEC_DELAY)){
LIBS += -L$$PWD/../lib/user -lwebrtc_audio_processing -laudio_process
}

if(contains(DEFINES, USE_CARPLAY)){
LIBS += -L$$PWD/../lib/carplay  -lAirPlay -lAirPlaySupport -lAudioConverter -lAudioConverter_dummy -lAudioStream -lcarplaymisc -lCarplayWrapper -lCoreUtils -liap2link -lmyutils -lScreenStream
}
if(contains(DEFINES, USE_CARLIFE)){
LIBS += -L$$PWD/../lib/carlife  -lcarlifeplayer -lcarlifevehicle
}
if(contains(DEFINES, USE_AUTO)) {
LIBS += -L$$PWD/../lib/auto  -lAndroidAuto -larkCarlinkUtils
}
if(contains(DEFINES, USE_HICAR)) {
LIBS += -L$$PWD/../lib/hicar  -larkadapt -ldmsdpaudiohandler -lhicar -ldmsdp -ldmsdpcrypto -ldmsdpdvaudio -ldmsdpdvcamera -ldmsdpdvdevice -ldmsdpdvgps -ldmsdpdvinterface -ldmsdphisight -ldmsdpplatform -ldmsdpsec -lmanagement -lauthagent -lHisightSink -lsecurec -lHwDeviceAuthSDK -lHwKeystoreSDK -lnearby
}
if(contains(DEFINES, USE_MIRROR)) {
LIBS += -L$$PWD/../lib/mirror  -lmirrorplayer
}

if(contains(DEFINES, USE_EASYCONNECT)) {
    if(contains(DEFINES,CQLW)){
        LIBS += -L$$PWD/../lib/eclink/CQLW  -leclinkplayer -lECSDK -lECSDKFramework
    }
    if(contains(DEFINES,TYW02)){
        LIBS += -L$$PWD/../lib/eclink/TYW02  -leclinkplayer -lECSDK -lECSDKFramework
    }
}

SOURCES += \
    ../UserInterface/AutoLink.cpp \
    ../UserInterface/CarlifeLink.cpp \
    ../UserInterface/CarplayLink.cpp \
    ../UserInterface/EasyConnectLink.cpp \
    ../UserInterface/IUserLinkPlayer.cpp \
    main.cpp \
    mainwindow.cpp \
    ../UserInterface/CarplayLinkcbsImpl.cpp \
    ../UserInterface/CarplayAudioCtx.cpp \
    ../UserInterface/MirrorLink.cpp \
    qrcodewindow.cpp \
    ../UserInterface/WebrtcWrapper.cpp \
    ../UserInterface/webrtc.cpp \
    ../UserInterface/HiCarLink.cpp

HEADERS += \
    ../UserInterface/AudioRecord.h \
    ../UserInterface/AutoLink.h \
    ../UserInterface/CarlifeLink.h \
    ../UserInterface/CarplayLink.h \
    ../UserInterface/ConfigParser.h \
    ../UserInterface/EasyConnectLink.h \
    ../UserInterface/IUserLinkPlayer.h \
    ../UserInterface/LinkAssist.h \
    ../UserInterface/LinkBase.h \
    ../UserInterface/Thread.h \
    ../UserInterface/UsbHostService.h \
    ../UserInterface/Util.h \
    ../UserInterface/VideoDecoder.h \
    ../include/auto/AndroidAuto.h \
    ../include/auto/IUserAutoCbs.h \
    ../include/carlife/CCarLifeLibWrapper.h \
    ../include/carlife/adbcommand.h \
    ../include/carlife/aoamode.h \
    ../include/carlife/arkIphoneusbtethering.h \
    ../include/carlife/arkusbtetheringcallbacks.h \
    ../include/carlife/audiocontrol.h \
    ../include/carlife/bluetoothcontrol.h \
    ../include/carlife/carlife.h \
    ../include/carlife/carlifeplayer.h \
    ../include/carlife/cmdchannel.h \
    ../include/carlife/eapchannel.h \
    ../include/carlife/event.h \
    ../include/carlife/mediachannel.h \
    ../include/carlife/mediadecode.h \
    ../include/carlife/miccapture.h \
    ../include/carlife/msgqueue.h \
    ../include/carlife/thread.h \
    ../include/carlife/timer.h \
    ../include/carlife/ttschannel.h \
    ../include/carlife/types.h \
    ../include/carlife/usb_vendors.h \
    ../include/carlife/usbmanager.h \
    ../include/carlife/util.h \
    ../include/carlife/videochannel.h \
    ../include/carlife/videodecode.h \
    ../include/carlife/vrchannel.h \
    ../include/carplay/carplayAudioWrapper.h \
    ../include/carplay/carplayVideoWrapper.h \
    ../include/carplay/carplayWrapper.h \
    mainwindow.h \
    ../include/user/AutoLink.h \
    ../include/user/CarlifeLink.h \
    ../include/user/CarplayAudioCtx.h \
    ../include/user/CarplayLink.h \
    ../include/user/CarplayLinkCbsImpl.h \
    ../include/user/EasyConnectLink.h \
    ../include/user/MirrorLink.h \
    ../include/user/IUserLinkPlayer.h \
    ../include/user/Thread.h \
    ../include/user/UsbHostService.h \
    ../include/user/Util.h \
    ../include/user/VideoDecoder.h \
    ../include/user/AudioDecoder.h \
    ../include/user/AudioRecord.h \
    ../include/user/ConfigParser.h \
    ../include/user/LinkAssist.h \
    ../include/user/LinkBase.h \
    ../include/mirror/thread.h \
    ../include/mirror/stream.h \
    ../include/mirror/server.h \
    ../include/mirror/net.h \
    ../include/mirror/mirrorplayer.h \
    ../include/mirror/keycodes.h \
    ../include/mirror/inputconvertnormal.h \
    ../include/mirror/inputconvertbase.h \
    ../include/mirror/input.h \
    ../include/mirror/devicemanage.h \
    ../include/mirror/device.h \
    ../include/mirror/controlmsg.h \
    ../include/mirror/controller.h \
    ../include/mirror/common.h \
    ../include/mirror/command.h \
    ../include/mirror/bufferutil.h \
    ../include/mirror/adbcommand.h \
    qrencode.h \
    qrcodewindow.h \
    ../include/eclink/APPListener.h \
    ../include/eclink/AudioPlayer.h \
    ../include/eclink/AudioRecoder.h \
    ../include/eclink/EapDevice.h \
    ../include/eclink/ECSDKAPPManager.h \
    ../include/eclink/ECSDKAudioManager.h \
    ../include/eclink/ECSDKFramework.h \
    ../include/eclink/ECSDKMirrorManager.h \
    ../include/eclink/ECSDKOTAManager.h \
    ../include/eclink/ECSDKToolKit.h \
    ../include/eclink/ECSDKTouchKeyManager.h \
    ../include/eclink/ECSDKTypes.h \
    ../include/eclink/ECSETypes.h \
    ../include/eclink/MirrorListener.h \
    ../include/eclink/OTAListener.h \
    ../include/eclink/SDKListener.h \
    ../include/eclink/ToolKitListener.h \
    ../include/eclink/VideoPlayer.h \
    ../include/user/HiCarLink.h \
    ../include/hicar/ark_hicar_api.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target



