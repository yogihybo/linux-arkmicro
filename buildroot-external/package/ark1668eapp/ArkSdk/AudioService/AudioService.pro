QT += quick dbus
CONFIG += c++11
TARGET = AudioService
TEMPLATE = lib
CONFIG += staticlib
CONFIG += release
SOURCES += \
    AudioService.cpp \
    AudioServiceProxy.cpp \
    audiocontrol.cpp \
    AudioPersistent.cpp

HEADERS += \
    AudioService.h \
    AudioServiceProxy.h \
    audiocontrol.h \
    AudioPersistent.h

include(../ArkSdk.pri)
system(mkdir -p $$PWD/../Package/$$TARGET/$$OUTPUT)
system(cp AudioService.h $$PWD/../Package/$$TARGET)
system(cp AudioServiceProxy.h $$PWD/../Package/$$TARGET)
system(cp AudioPersistent.h $$PWD/../Package/$$TARGET)

#if (contains(DEFINES, $$COMPILER)) {
INCLUDEPATH += ./Alsa/Header
DEPENDPATH += ./Alsa/Header
unix:!macx: LIBS += -L./Alsa/Library -lasound
#}
