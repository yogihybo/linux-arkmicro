#-------------------------------------------------
#
# Project created by QtCreator 2017-01-03T11:53:36
#
#-------------------------------------------------
QT += quick
CONFIG += c++11
TARGET = UserInterface
TEMPLATE = lib
CONFIG += staticlib
CONFIG += release
include(../ArkSdk.pri)
system(mkdir -p $$PWD/../Package/$$TARGET/$$OUTPUT)
system(cp UserInterfaceUtility.h $$PWD/../Package/$$TARGET)

HEADERS += \
    UserInterfaceUtility.h

SOURCES += \
    UserInterfaceUtility.cpp
