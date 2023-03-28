QT += qml quick xml dbus network
CONFIG += c++11
TEMPLATE = app
CONFIG += release
TARGET = Launcher

include(../ArkSdk.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
system(mkdir -p $$PWD/../Package/1668eArk-IVI/$$OUTPUT)
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += __ARM__
MOC_DIR = $$PWD/../Package/1668eArk-IVI/$$OUTPUT
OBJECTS_DIR = $$PWD/../Package/1668eArk-IVI/$$OUTPUT
RCC_DIR = $$PWD/../Package/1668eArk-IVI/$$OUTPUT
DESTDIR = $$PWD/../Package/1668eArk-IVI/$$OUTPUT
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    UserInterface/Launcher.cpp \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbImage/UsbImageModelData.cpp \
    UserInterface/QmlLauncher.cpp \
    BusinessLogic/Multimedia.cpp \
    BusinessLogic/Audio.cpp \
    UserInterface/MainWidget/MainWidget.cpp \
    UserInterface/MainWidget/DiskWidget/MultiMediaWidget.cpp \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbScanWidget.cpp \
    UserInterface/MainWidget/DiskWidget/MultiMediaPlayWIdget/MultiMediaPlayWidget.cpp \
    UserInterface/MainWidget/ToolWidget/ToolWiget.cpp \
    UserInterface/MainWidget/ToolWidget/StatusBar/StatusBar.cpp \
    UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.cpp \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbMusicListModel/UsbMusicListModel.cpp \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbMusic/UsbMusicListModelData.cpp \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbVideo/UsbVideoModelData.cpp \
    UserInterface/MainWidget/DiskWidget/MusicInformation/MusicInformationWidget.cpp \
    UserInterface/MainWidget/DiskWidget/MusicInformation/ImageProvider.cpp \
    UserInterface/MainWidget/DiskWidget/VideoWidget/VideoWidget.cpp \
    UserInterface/MainWidget/DiskWidget/VideoWidget/VideoListWidget/VideoListWidget.cpp \
    UserInterface/MainWidget/DiskWidget/VideoWidget/VideoToolBarWidget/VideoToolBarWidget.cpp \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdScanWidget.cpp \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdVideo/SdVideoModelData.cpp \
    UserInterface/MainWidget/DiskWidget/VideoWidget/ImageWidget/ImageWidget.cpp \
    UserInterface/MainWidget/DiskWidget/VideoWidget/ImageToolBarWidget/ImageToolBarWidget.cpp \
    UserInterface/MainWidget/DiskWidget/VideoWidget/ImageListWidget/ImageListWidget.cpp \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdMusic/SdMusicListModelDdata.cpp \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdImage/SdImageModelData.cpp \
    BusinessLogic/carback.cpp \
    UserInterface/MainWidget/SettingWidget/SettingWidget.cpp \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothSettingWidget.cpp \
    UserInterface/MainWidget/SettingWidget/VolumeSettingWidget.cpp/VolumeSettingWidget.cpp \
    BusinessLogic/Bluetooth.cpp \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothConnectWidget/BluetoothConnectWidget.cpp \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothConnectWidget/BluetoothNameModelData.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneWidget.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/PhoneBookModel.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/AllCallLogModel.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/PhoneBookModelData.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/AllCallLogModelData.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneInComingWidget/InComingWidget.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelephoneDialerWidget/DialerWidget.cpp \
    UserInterface/MainWidget/TelePhoneWidget/TelephoneOnCallWidget/OnCallWidget.cpp \
    BusinessLogic/carlink.cpp \
    BusinessLogic/carlinkproxy.cpp \
    UserInterface/MainWidget/PhoneLinkWidget/PhoneLinkWidget.cpp \
    BusinessLogic/HostApd.cpp \
    UserInterface/MainWidget/PhoneLinkWidget/CarLifeCarPlayWidget/CarLifeCarPlayWidget.cpp \
    UserInterface/MainWidget/PhoneLinkWidget/PhoneLinkMsgWidget/PhoneLinkMsgWidget.cpp \
    UserInterface/MainWidget/SettingWidget/MoreSettingWidget/MoreSettingWidget.cpp \
    UserInterface/MainWidget/SettingWidget/MoreSettingWidget/MoreSettingDataTimeWidget.cpp \
    UserInterface/MainWidget/SettingWidget/MoreSettingWidget/MoreSettingPhoneLinkWidget.cpp \
    UserInterface/MainWidget/SettingWidget/BrightnessSettingWidget/BrightnessSettingWidget.cpp \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingWidget.cpp \
    UserInterface/MainWidget/HomeWidget/HomeWidget.cpp \
    UserInterface/MainWidget/HomeWidget/BtTelMiniWidget/BtTelMiniWidget.cpp \
    UserInterface/MainWidget/HomeWidget/MusicMiniWidget/MusicMiniWidget.cpp \
    UserInterface/MainWidget/SettingWidget/VolumeSettingWidget.cpp/VolumeValueSettingWidget.cpp \
    UserInterface/MainWidget/SettingWidget/VolumeSettingWidget.cpp/VolumeBalanceSettingWidget.cpp \
    BusinessLogic/Setting.cpp \
    BusinessLogic/Widget.cpp \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingNativeInfoWidget/AboutSettingNativeInfoWidget.cpp \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingResetWidget/AboutSettingResetWidget.cpp \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingRecoveryWidget/AboutSettingRecoveryWidget.cpp \
    BusinessLogic/WiFiManager.cpp \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardWidget.cpp \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardFirstRowModelData.cpp \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardSecondRowModelData.cpp \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardThirdRowModelData.cpp \
    BusinessLogic/QmlWidget.cpp \
    UserInterface/MainWidget/SettingWidget/WifiSettingWidget/WifiSettingWidget.cpp \
    UserInterface/MainWidget/SettingWidget/WifiSettingWidget/WifiSettingWidgetModelData.cpp \
    UserInterface/MainWidget/HomeWidget/PhoneLinkMiniWidget/PhoneLinkMiniWidget.cpp \
    UserInterface/MainWidget/AuxWidget/AuxWidget.cpp \
    UserInterface/MainWidget/HomeWidget/AuxMiniWidget/AuxMiniWidget.cpp \
    UserInterface/MainWidget/PhoneLinkWidget/AutoCarPlayWidget/AutoCarPlayWidget.cpp \
    UserInterface/MainWidget/PhoneLinkWidget/EcLinkWidget/EcLinkWidget.cpp \
    UserInterface/MainWidget/PhoneLinkWidget/HicarWidget/HicarWidget.cpp \
    UserInterface/MainWidget/BackWidget.cpp \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothSwitchSettingWidget/BluetoothSwitchSettingWidget.cpp \
    UserInterface/MainWidget/PhoneLinkWidget/PhoneLinkMsgShowWidget.cpp
HEADERS += \
    UserInterface/Launcher.h \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbImage/UsbImageModelData.h \
    UserInterface/QmlLauncher.h \
    BusinessLogic/Multimedia.h \
    BusinessLogic/Audio.h \
    UserInterface/MainWidget/MainWidget.h \
    UserInterface/MainWidget/DiskWidget/MultiMediaWidget.h \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbScanWidget.h \
    UserInterface/MainWidget/DiskWidget/MultiMediaPlayWIdget/MultiMediaPlayWidget.h \
    UserInterface/MainWidget/ToolWidget/ToolWiget.h \
    UserInterface/MainWidget/ToolWidget/StatusBar/StatusBar.h \
    UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbMusicListModel/UsbMusicListModel.h \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbMusic/UsbMusicListModelData.h \
    UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbVideo/UsbVideoModelData.h \
    UserInterface/MainWidget/DiskWidget/MusicInformation/MusicInformationWidget.h \
    UserInterface/MainWidget/DiskWidget/MusicInformation/ImageProvider.h \
    UserInterface/MainWidget/DiskWidget/VideoWidget/VideoWidget.h \
    UserInterface/MainWidget/DiskWidget/VideoWidget/VideoListWidget/VideoListWidget.h \
    UserInterface/MainWidget/DiskWidget/VideoWidget/VideoToolBarWidget/VideoToolBarWidget.h \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdScanWidget.h \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdVideo/SdVideoModelData.h \
    UserInterface/MainWidget/DiskWidget/VideoWidget/ImageWidget/ImageWidget.h \
    UserInterface/MainWidget/DiskWidget/VideoWidget/ImageToolBarWidget/ImageToolBarWidget.h \
    UserInterface/MainWidget/DiskWidget/VideoWidget/ImageListWidget/ImageListWidget.h \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdMusic/SdMusicListModelDdata.h \
    UserInterface/MainWidget/DiskWidget/SdDiskWidget/SdImage/SdImageModelData.h \
    BusinessLogic/carback.h \
    BusinessLogic/ark_api.h \
    UserInterface/MainWidget/SettingWidget/SettingWidget.h \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothSettingWidget.h \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothSwitchSettingWidget/BluetoothSwitchSettingWidget.h \
    UserInterface/MainWidget/SettingWidget/VolumeSettingWidget.cpp/VolumeSettingWidget.h \
    BusinessLogic/Bluetooth.h \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothConnectWidget/BluetoothConnectWidget.h \
    UserInterface/MainWidget/SettingWidget/BluetoothSettingWidget/BluetoothConnectWidget/BluetoothNameModelData.h \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneWidget.h \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/PhoneBookModel.h \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/AllCallLogModel.h \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/PhoneBookModelData.h \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneModelData/AllCallLogModelData.h \
    UserInterface/MainWidget/TelePhoneWidget/TelePhoneInComingWidget/InComingWidget.h \
    UserInterface/MainWidget/TelePhoneWidget/TelephoneDialerWidget/DialerWidget.h \
    UserInterface/MainWidget/TelePhoneWidget/TelephoneOnCallWidget/OnCallWidget.h \
    BusinessLogic/carlink.h \
    BusinessLogic/carlinkproxy.h \
    UserInterface/MainWidget/PhoneLinkWidget/PhoneLinkWidget.h \
    BusinessLogic/HostApd.h \
    BusinessLogic/LinkBase.h \
    UserInterface/MainWidget/PhoneLinkWidget/CarLifeCarPlayWidget/CarLifeCarPlayWidget.h \
    UserInterface/MainWidget/PhoneLinkWidget/PhoneLinkMsgWidget/PhoneLinkMsgWidget.h \
    UserInterface/MainWidget/SettingWidget/MoreSettingWidget/MoreSettingWidget.h \
    UserInterface/MainWidget/SettingWidget/MoreSettingWidget/MoreSettingDataTimeWidget.h \
    UserInterface/MainWidget/SettingWidget/MoreSettingWidget/MoreSettingPhoneLinkWidget.h \
    UserInterface/MainWidget/SettingWidget/BrightnessSettingWidget/BrightnessSettingWidget.h \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingWidget.h \
    UserInterface/MainWidget/HomeWidget/HomeWidget.h \
    UserInterface/MainWidget/HomeWidget/BtTelMiniWidget/BtTelMiniWidget.h \
    UserInterface/MainWidget/HomeWidget/MusicMiniWidget/MusicMiniWidget.h \
    UserInterface/MainWidget/SettingWidget/VolumeSettingWidget.cpp/VolumeValueSettingWidget.h \
    UserInterface/MainWidget/SettingWidget/VolumeSettingWidget.cpp/VolumeBalanceSettingWidget.h \
    BusinessLogic/Setting.h \
    BusinessLogic/Widget.h \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingNativeInfoWidget/AboutSettingNativeInfoWidget.h \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingResetWidget/AboutSettingResetWidget.h \
    UserInterface/MainWidget/SettingWidget/AboutSettingWidget/AboutSettingRecoveryWidget/AboutSettingRecoveryWidget.h \
    BusinessLogic/wpa_ctrl.h \
    BusinessLogic/WiFiManager.h \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardWidget.h \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardFirstRowModelData.h \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardSecondRowModelData.h \
    UserInterface/MainWidget/KeyBoardModelData/KeyBoardThirdRowModelData.h \
    BusinessLogic/QmlWidget.h \
    UserInterface/MainWidget/SettingWidget/WifiSettingWidget/WifiSettingWidget.h \
    UserInterface/MainWidget/SettingWidget/WifiSettingWidget/WifiSettingWidgetModelData.h \
    UserInterface/MainWidget/HomeWidget/PhoneLinkMiniWidget/PhoneLinkMiniWidget.h \
    UserInterface/MainWidget/AuxWidget/AuxWidget.h \
    UserInterface/MainWidget/HomeWidget/AuxMiniWidget/AuxMiniWidget.h \
    UserInterface/MainWidget/PhoneLinkWidget/AutoCarPlayWidget/AutoCarPlayWidget.h \
    UserInterface/MainWidget/PhoneLinkWidget/EcLinkWidget/EcLinkWidget.h \
    UserInterface/MainWidget/PhoneLinkWidget/HicarWidget/HicarWidget.h \
    #BusinessLogic/CarLinkPlayer.h \
    UserInterface/MainWidget/BackWidget.h \
    BusinessLogic/display.h \
    BusinessLogic/ArkCar.h \
    UserInterface/MainWidget/PhoneLinkWidget/PhoneLinkMsgShowWidget.h
TRANSLATIONS += \
    ./Recource/Languages/en_tr.ts \
    ./Recource/Languages/zh_tr.ts \
    ./Recource/Languages/Tzh_tr.ts
RESOURCES += qml.qrc \
    #Recource/Devb/images.qrc
    Recource/languages.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

TAGLIB = $$PWD/../MultimediaService/TagLib/Header/
INCLUDEPATH += $$TAGLIB
DEPENDPATH += $$TAGLIB
INCLUDEPATH += $$TAGLIB/toolkit
DEPENDPATH += $$TAGLIB/toolkit
INCLUDEPATH += $$TAGLIB/flac
DEPENDPATH += $$TAGLIB/flac
INCLUDEPATH += $$TAGLIB/ape
DEPENDPATH += $$TAGLIB/ape
INCLUDEPATH += $$TAGLIB/mpeg
DEPENDPATH += $$TAGLIB/mpeg
INCLUDEPATH += $$TAGLIB/mpeg/id3v1
DEPENDPATH += $$TAGLIB/mpeg/id3v1
INCLUDEPATH += $$TAGLIB/mpeg/id3v2
DEPENDPATH += $$TAGLIB/mpeg/id3v2
INCLUDEPATH += $$TAGLIB/mpeg/id3v2/frames
DEPENDPATH += $$TAGLIB/mpeg/id3v2/frames
INCLUDEPATH += $$TAGLIB/ogg
DEPENDPATH += $$TAGLIB/ogg
INCLUDEPATH += $$TAGLIB/ogg/flac
DEPENDPATH += $$TAGLIB/ogg/flac
INCLUDEPATH += $$TAGLIB/ogg/opus
DEPENDPATH += $$TAGLIB/ogg/opus
INCLUDEPATH += $$TAGLIB/ogg/speex
DEPENDPATH += $$TAGLIB/ogg/speex
INCLUDEPATH += $$TAGLIB/ogg/vorbis
DEPENDPATH += $$TAGLIB/ogg/vorbis
INCLUDEPATH += $$TAGLIB/mp4
DEPENDPATH += $$TAGLIB/mp4
SDK_OUTPUT_PATH = $$PWD/../../../../../../linux-arkmicro/output/board/ark1668e_devb/buildroot/staging/usr
#SDK_OUTPUT_PATH = /home/lfp/Works/Ark1668eBsp/linux-arkmicro/output/board/ark1668e_devb/buildroot/staging/usr
INCLUDEPATH += $$SDK_OUTPUT_PATH/include/
INCLUDEPATH += $$SDK_OUTPUT_PATH/include/libusb-1.0/
LIBS += -L$$SDK_OUTPUT_PATH/lib -lmfc -lpthread -larkapi -lasound -lprotobuf -lssl -lcrypto -lfdk-aac -lusb-1.0 -lwpa_client
unix:!macx: LIBS += -L$$PWD/../Package/UserInterface/$$OUTPUT -lUserInterface
unix:!macx: LIBS += -L$$PWD/../Package/AutoConnect/$$OUTPUT -lAutoConnect
unix:!macx: LIBS += -L$$PWD/../Package/DbusService/$$OUTPUT -lDbusService
unix:!macx: LIBS += -L$$PWD/../Package/AudioService/$$OUTPUT -lAudioService
unix:!macx: LIBS += -L$$PWD/../Package/MultimediaService/$$OUTPUT -lMultimediaService
unix:!macx: LIBS += -L$$PWD/../Package/Utility/$$OUTPUT -lUtility
unix:!macx: LIBS += -L$$PWD/../Package/RunnableThread/$$OUTPUT -lRunnableThread
unix:!macx: LIBS += -L$$PWD/../Package/ArkApplication/$$OUTPUT -lArkApplication
ALSA = $$PWD/../AudioService/Alsa/Header
INCLUDEPATH += $$ALSA
DEPENDPATH  += $$ALSA
unix:!macx: LIBS += -L$$PWD/../AudioService/Alsa/Library -lasound
unix:!macx: LIBS += -ldl

#unix:!macx: LIBS += -L$$PWD/../ArkMicro/Library -lmupdf
#unix:!macx: LIBS += -L$$PWD/../ArkMicro/Library -lmupdf-third
unix:!macx: LIBS += -L$$PWD/../MultimediaService/TagLib/Library/arm -lConvert
unix:!macx: LIBS += -L$$PWD/../MultimediaService/TagLib/Library/arm -ltag

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target






