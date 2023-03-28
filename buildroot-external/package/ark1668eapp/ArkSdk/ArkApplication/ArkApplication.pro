#-------------------------------------------------
#
# Project created by QtCreator 2017-02-15T14:39:38
#
#-------------------------------------------------

QT      += qml quick
CONFIG  += c++11
TARGET = ArkApplication
TEMPLATE = lib
CONFIG += staticlib
#CONFIG += release
SOURCES += ArkApplication.cpp
HEADERS += ArkApplication.h

include(../ArkSdk.pri)
system(mkdir -p $$PWD/../Package/$$TARGET/$$OUTPUT)
system(cp ArkApplication.h $$PWD/../Package/$$TARGET)
