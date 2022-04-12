#include "carback.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <QDebug>
#include "ark_api.h"

#define CARBACK_IOCTL_BASE							0x9A
#define CARBACK_IOCTL_SET_APP_READY					_IO(CARBACK_IOCTL_BASE, 0)
#define CARBACK_IOCTL_APP_ENTER_DONE				_IO(CARBACK_IOCTL_BASE, 1)
#define CARBACK_IOCTL_APP_EXIT_DONE					_IO(CARBACK_IOCTL_BASE, 2)
#define CARBACK_IOCTL_GET_STATUS					_IOR(CARBACK_IOCTL_BASE, 3, int)
#define CARBACK_IOCTL_DETECT_SIGNAL					_IOR(CARBACK_IOCTL_BASE, 4, int)
#define ARK_DVR_IOC_MAGIC			'n'
#define VIN_UPDATE_WINDOW		_IOWR(ARK_DVR_IOC_MAGIC,50, struct vin_screen)


Carback::Carback(QObject *parent) : QObject(parent)
{
    exitThread = 1;
    carbackFd = -1;
}

Carback::~Carback()
{
    exitThread = -1;
    carbackFd = -1;
}

void *Carback::readCarbckStatus(void *arg)
{
    Carback *pThis = (Carback *)arg;

    unsigned char data;
    int video0Fd = -1;
    struct vin_screen vin_para;

    video0Fd = open("/dev/video0",O_RDWR);
    if( video0Fd < 0 ) {
        qDebug("open /dev/video0 fail.");
    }
    vin_para.disp_x = 654;
    vin_para.disp_y = 214;
    vin_para.disp_width = 616;
    vin_para.disp_height = 436;

    while (pThis->exitThread) {
        if (1 == read(pThis->carbackFd, &data, 1)) {
            if (data == CBS_On) {
                arkapi_enter_carback();
                if (0 != ioctl(pThis->carbackFd, CARBACK_IOCTL_APP_ENTER_DONE)) {
                    qDebug("ioctl CARBACK_IOCTL_APP_ENTER_DONE fail.");
                }
                if (0!=ioctl(video0Fd, VIN_UPDATE_WINDOW, &vin_para)) {
                    qDebug("ioctl VIN_UPDATE_WINDOW fail.");
                }
                pThis->CarbackStatusChange(CBS_On);
            } else if (data == CBS_Off) {
                arkapi_exit_carback();
                if (0 != ioctl(pThis->carbackFd, CARBACK_IOCTL_APP_EXIT_DONE)) {
                    qDebug("ioctl CARBACK_IOCTL_APP_ENTER_DONE fail.");
                }
                pThis->CarbackStatusChange(CBS_Off);
            }
        } else {
            qDebug("read /dev/carback fail.");
        }
    }
}

void Carback::initialize()
{
    pthread_t pthead;
    int ret = -1;

    carbackFd = open("/dev/carback", O_RDONLY);
    if (carbackFd < 0) {
        qDebug("open /dev/carback fail.");
        return;
    }

    if (0 != ioctl(carbackFd, CARBACK_IOCTL_SET_APP_READY)) {
        qDebug("ioctl CARBACK_IOCTL_SET_APP_READY fail.");
        return;
    }

    ret = pthread_create(&pthead, NULL, readCarbckStatus, this);
    if(ret != 0) {
        printf("pthread_create failed!\n");
    }
}
