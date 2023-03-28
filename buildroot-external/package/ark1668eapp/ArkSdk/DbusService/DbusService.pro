QT += quick dbus
CONFIG += c++11
TARGET = DbusService
TEMPLATE = lib
CONFIG += staticlib
CONFIG += release
SOURCES += \
    DbusService.cpp

HEADERS += \
    DbusService.h

include(../ArkSdk.pri)
system(mkdir -p $$PWD/../Package/$$TARGET/$$OUTPUT)
system(cp DbusService.h $$PWD/../Package/$$TARGET)
