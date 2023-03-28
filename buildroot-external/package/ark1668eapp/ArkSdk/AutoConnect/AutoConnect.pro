QT += quick
CONFIG += c++11
TARGET = AutoConnect
TEMPLATE = lib
CONFIG += staticlib
CONFIG += release
SOURCES += AutoConnect.cpp
HEADERS += AutoConnect.h

include(../ArkSdk.pri)
system(mkdir -p $$PWD/../Package/$$TARGET/$$OUTPUT)
system(cp AutoConnect.h $$PWD/../Package/$$TARGET)
