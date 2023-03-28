#-------------------------------------------------
#
# Project created by QtCreator 2017-01-03T14:11:00
#
#-------------------------------------------------
QT += quick
CONFIG += c++11
TARGET = RunnableThread
TEMPLATE = lib
CONFIG += staticlib
CONFIG += release
SOURCES += RunnableThread.cpp
HEADERS += RunnableThread.h

include(../ArkSdk.pri)
system(mkdir -p $$PWD/../Package/$$TARGET/$$OUTPUT)
system(cp RunnableThread.h $$PWD/../Package/$$TARGET)
