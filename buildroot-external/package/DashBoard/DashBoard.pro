QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = DashBoard
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += ECLINK  #亿恋互联库最新库异常，暂时屏蔽
DEFINES += __ARM__ #Arm端运行请打开此宏

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    logopainter.cpp \
    speedpainter.cpp \
    cornerlampwidget.cpp \
    linkwidget.cpp \
    BusinessLogic/carback.cpp \

HEADERS += \
    mainwindow.h \
    logopainter.h \
    speedpainter.h \
    cornerlampwidget.h \
    linkwidget.h \
    BusinessLogic/carback.h \

MOC_DIR = $$PWD/out
OBJECTS_DIR = $$PWD/out
RCC_DIR = $$PWD/out
DESTDIR = $$PWD/out/bin

SDK_OUTPUT_PATH = ../../../linux-arkmicro/output/board/ark1668e_devb/buildroot/staging/usr
INCLUDEPATH += include
INCLUDEPATH += $$SDK_OUTPUT_PATH/include/
LIBS += -L$$SDK_OUTPUT_PATH/lib -lmfc -lpthread -larkapi
if (contains(DEFINES, ECLINK)) {
LIBS += -L$$PWD/eclib/lib/ -leclinkplayer -lECSDK
}


RESOURCES += \
    #Resource/images.qrc #PC端运行解除屏蔽






