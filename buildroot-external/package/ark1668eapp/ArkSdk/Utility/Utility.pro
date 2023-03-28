#-------------------------------------------------
#
# Project created by QtCreator 2017-01-03T11:53:36
#
#-------------------------------------------------

QT += quick
CONFIG += c++11
TARGET = Utility
TEMPLATE = lib
CONFIG += staticlib
CONFIG += release
SOURCES += Utility.cpp

HEADERS += Utility.h

include(../ArkSdk.pri)
system(mkdir -p $$PWD/../Package/$$TARGET/$$OUTPUT)
system(cp Utility.h $$PWD/../Package/$$TARGET)
