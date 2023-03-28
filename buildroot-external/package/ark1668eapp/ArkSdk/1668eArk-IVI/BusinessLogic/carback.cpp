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
#include "BusinessLogic/ark_api.h"
#include "ArkCar.h"
#include <linux/input.h>
#define CARBACK_IOCTL_BASE							0x9A
#define CARBACK_IOCTL_SET_APP_READY					_IO(CARBACK_IOCTL_BASE, 0)
#define CARBACK_IOCTL_APP_ENTER_DONE				_IO(CARBACK_IOCTL_BASE, 1)
#define CARBACK_IOCTL_APP_EXIT_DONE					_IO(CARBACK_IOCTL_BASE, 2)
#define CARBACK_IOCTL_GET_STATUS					_IOR(CARBACK_IOCTL_BASE, 3, int)
#define CARBACK_IOCTL_DETECT_SIGNAL					_IOR(CARBACK_IOCTL_BASE, 4, int)
#define CARBACK_IOCTL_SET_CARBACK_ENTRY				_IO(CARBACK_IOCTL_BASE, 5)
#define CARBACK_IOCTL_SET_CARBACK_EXIT				_IO(CARBACK_IOCTL_BASE, 6)
#define ARK_DVR_IOC_MAGIC			'n'
#define VIN_UPDATE_WINDOW		_IOWR(ARK_DVR_IOC_MAGIC,50, struct vin_screen)
static struct CarDeviceState device_state = {
    .reversing = ACS_Undefine,
    .reverse_detect = ACS_Undefine,
    .illlight = ACS_ILLLightOff,
    .brake = ACS_BrakeOff,
};
class CarbackPrivate
{
    Q_DISABLE_COPY(CarbackPrivate)
public:
    explicit CarbackPrivate(Carback* parent);
    ~CarbackPrivate();
public:
    void carbackStatusChange(int status);
    void enableGrabInput(const bool flag);
    void detectSignalstatus(int status);
    int  exitThread;
    int  carbackFd;
    int  m_InputFd;
    bool m_Flag;
    bool m_ArkUserInterfaceInit;
private:
    Q_DECLARE_PUBLIC(Carback)
    Carback* const q_ptr;
};

Carback::Carback(QObject *parent) :
    QObject(parent),
    d_ptr(new CarbackPrivate(this))
{

}

Carback::~Carback()
{

}

static void* readCarbckStatus(void *arg)
{
    CarbackPrivate *pThis = (CarbackPrivate *)arg;
    unsigned char data;
    int video0Fd = -1;
    struct vin_screen vin_para;
    static int reverse_detect_filter = 0;
    video0Fd = open("/dev/video0",O_RDWR);
    if( video0Fd < 0 ) {
        qDebug("open /dev/video0 fail.");
    }
    vin_para.disp_x = 0;
    vin_para.disp_y = 0;
    vin_para.disp_width = 1920;
    vin_para.disp_height = 720;

    if (pThis->m_ArkUserInterfaceInit) {
        pThis->m_ArkUserInterfaceInit = false;
        if (ioctl(pThis->carbackFd, CARBACK_IOCTL_SET_APP_READY) != 0) {
            printf("CARBACK_IOCTL_SET_APP_READY fail.\n");
        }
        int data = 0;
        if (ioctl(pThis->carbackFd, CARBACK_IOCTL_GET_STATUS, &data) != 0) {
            printf("CARBACK_IOCTL_GET_STATUS fail.\n");
            data = 0;
        }
        enum ArkCarStatus status = (data > 0)? ACS_ReversingOn : ACS_ReversingOff;
        device_state.reversing = status;
        if (ACS_ReversingOn == data) {
            printf("REVERSING ON\n");
            arkapi_enter_carback();
            if (0 != ioctl(pThis->carbackFd, CARBACK_IOCTL_APP_ENTER_DONE)) {
                qDebug("ioctl CARBACK_IOCTL_APP_ENTER_DONE fail.");
            }
            if (0!=ioctl(video0Fd, VIN_UPDATE_WINDOW, &vin_para)) {
                qDebug("ioctl VIN_UPDATE_WINDOW fail.");
            }
            pThis->carbackStatusChange(Carback::CBS_On);
            pThis->enableGrabInput(true);
            if (ioctl(pThis->carbackFd, CARBACK_IOCTL_DETECT_SIGNAL, &data) != 0) {
                printf("CARBACK_IOCTL_DETECT_SIGNAL fail.\n");
                data = 0;
                status = (0 == data)? ACS_NoDetectSignal: ACS_DetectSignal;
                if (device_state.reverse_detect != status) {
                    device_state.reverse_detect = status;
                    pThis->detectSignalstatus(data);
                    printf("%s,%d, status = %d\n", __PRETTY_FUNCTION__, __LINE__, device_state.reverse_detect);
                }
            }
        } else {
            printf("REVERSING OFF\n");
//            arkapi_exit_carback();
//            if (0 != ioctl(pThis->carbackFd, CARBACK_IOCTL_APP_EXIT_DONE)) {
//                qDebug("ioctl CARBACK_IOCTL_APP_ENTER_DONE fail.");
//            }
//            pThis->carbackStatusChange(Carback::CBS_Off);
//            pThis->enableGrabInput(false);

        }
    }

    while (pThis->exitThread) {
        if (1 == read(pThis->carbackFd, &data, 1)) {
            if (ACS_Undefine != device_state.reversing) {
                enum ArkCarStatus status = (data > 0)? ACS_ReversingOn : ACS_ReversingOff;
                if (status != device_state.reversing) {
                    device_state.reversing = status;
                    reverse_detect_filter = 0;
                    if (data == Carback::CBS_On) {
                        printf("REVERSING ON\n");
                        arkapi_enter_carback();
                        if (0 != ioctl(pThis->carbackFd, CARBACK_IOCTL_APP_ENTER_DONE)) {
                            qDebug("ioctl CARBACK_IOCTL_APP_ENTER_DONE fail.");
                        }
//                        if (0!=ioctl(video0Fd, VIN_UPDATE_WINDOW, &vin_para)) {
//                            qDebug("ioctl VIN_UPDATE_WINDOW fail.");
//                        }
                        pThis->carbackStatusChange(Carback::CBS_On);
                        pThis->enableGrabInput(true);
                        device_state.reverse_detect = ACS_Undefine;
                    }
                    else if (data == Carback::CBS_Off) {
                        printf("REVERSING OFF\n");
                        arkapi_exit_carback();
                        if (0 != ioctl(pThis->carbackFd, CARBACK_IOCTL_APP_EXIT_DONE)) {
                            qDebug("ioctl CARBACK_IOCTL_APP_ENTER_DONE fail.");
                        }
                        pThis->carbackStatusChange(Carback::CBS_Off);
                        pThis->enableGrabInput(false);
                        if (pThis->m_Flag) {
                            pThis->m_Flag = false;
                            nice(-20);
                        }
                     }
                }
            }
        } else {
            printf("read reversing_fd error\n");
            perror("read");
        }
        if (ACS_ReversingOn == device_state.reversing) {
            if (pThis->carbackFd >= 0) {
                int data;
                if (ioctl(pThis->carbackFd, CARBACK_IOCTL_DETECT_SIGNAL, &data) != 0) {
                    printf("CARBACK_IOCTL_DETECT_SIGNAL fail.\n");
                    data = 0;
                }
                enum ArkCarStatus status = (0 == data)? ACS_NoDetectSignal: ACS_DetectSignal;
                if (device_state.reverse_detect != status) {
                    if (reverse_detect_filter++ > 3) {
                        reverse_detect_filter = 0;
                        device_state.reverse_detect = status;
                        pThis->detectSignalstatus(data);
                    }
                }
            }
        }
    }
}




void Carback::initialize()
{
    Q_D(Carback);
    pthread_t pthead;
    int ret = -1;
    d->carbackFd = open("/dev/carback", O_RDONLY);
    if (d->carbackFd < 0) {
        qDebug("open /dev/carback fail.");
        return;
    }
    if (0 != ioctl(d->carbackFd, CARBACK_IOCTL_SET_APP_READY)) {
        qDebug("ioctl CARBACK_IOCTL_SET_APP_READY fail.");
        return;
    }
    ret = pthread_create(&pthead, NULL,readCarbckStatus, d);
    if(ret != 0) {
        printf("pthread_create failed!\n");
    }
}

void Carback::localDeviceName()
{
    Q_D(Carback);
    qDebug() << __PRETTY_FUNCTION__ <<__LINE__;
}
CarbackPrivate::CarbackPrivate(Carback *parent)
    : q_ptr(parent)
{
    exitThread = 1;
    carbackFd = -1;
    m_InputFd = -1;
    m_Flag = true;
    m_ArkUserInterfaceInit =true;
}

CarbackPrivate::~CarbackPrivate()
{

}
void CarbackPrivate::carbackStatusChange(int status){
    Q_Q(Carback);
    qDebug()<<"==[Carback::carbackStatusChange:status]=="<<status;
    emit q->onCarbackStatusChange(status);
}
void CarbackPrivate::detectSignalstatus(int status){
    Q_Q(Carback);
    qDebug()<<"==[Carback::detectSignalstatus:status]=="<<status;
    emit q->onDetectSignal(status);
}
void CarbackPrivate::enableGrabInput(const bool flag)
{
    if (flag) {
        if (-1 == m_InputFd) {
            m_InputFd = open("/dev/input/event0", O_WRONLY);
            if (m_InputFd >= 0) {
                ioctl(m_InputFd, EVIOCGRAB, 1);
            }
        }
    } else {
        if (-1 != m_InputFd) {
            ioctl(m_InputFd, EVIOCGRAB, 0);
            close(m_InputFd);
            m_InputFd = -1;
        }
    }
}

