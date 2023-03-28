#include "AuxWidget.h"
#include "BusinessLogic/Widget.h"
#include "AutoConnect.h"
#include "BusinessLogic/ark_api.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/Audio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <QQmlProperty>
#include <QDebug>

class AuxWidgetPrivate
{
    Q_DISABLE_COPY(AuxWidgetPrivate)
public:
    explicit AuxWidgetPrivate(AuxWidget* parent);
    ~AuxWidgetPrivate();
    void initialize();
public:
    QObject* m_AuxWidgetObject;
    QObject* m_AuxLoaderObject;
    int m_AvInFd;
    int  m_Hthread;
    bool m_Visible;
    bool m_HthreadExitStatus;
private:
    Q_DECLARE_PUBLIC(AuxWidget)
    AuxWidget* const q_ptr;
};
AuxWidget::AuxWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new AuxWidgetPrivate(this))
{

}
void AuxWidget::setAuxWidgetObject(QObject* qmlObject){
    Q_D(AuxWidget);
    if(d->m_AuxWidgetObject == NULL)
    {
        d->m_AuxWidgetObject = qmlObject;
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_AuxWidgetObject, SIGNAL(visibleChanged()),
                     this,      SLOT(onVisibleChanged()),
                     type);
    QObject::connect(d->m_AuxWidgetObject, SIGNAL(auxWidgetClicked()),
                     this,      SLOT(onAuxWidgetClicked()),
                     type);
}

void AuxWidget::setAuxLoaderObject(QObject* qmlObject)
{
    Q_D(AuxWidget);
    if(d->m_AuxLoaderObject == NULL)
    {
        d->m_AuxLoaderObject = qmlObject;
    }
}
QObject* AuxWidget::getAuxWidgetObject(){
    Q_D(AuxWidget);
    if(d->m_AuxLoaderObject != NULL)
    {
        return d->m_AuxLoaderObject;
    }
}
void AuxWidget::setVisibleStatus(bool status)
{
    Q_D(AuxWidget);
    d->m_Visible = status;
}
bool AuxWidget::getVisibleStatus()
{
    Q_D(AuxWidget);
    return d->m_Visible;
}
void AuxWidget::onVisibleChanged()
{
    Q_D(AuxWidget);
    if(d->m_AuxLoaderObject->property("visible").toBool())
    {
        if(!d->m_Visible)
        {
            g_Widget->setPreemptiveWidget(1);
            d->initialize();
            d->m_Visible = true;
        }
    }
}

void AuxWidget::onAuxWidgetClicked()
{
    Q_D(AuxWidget);
    emit g_Widget->onWidgetTypeChange(Widget::T_Home, Widget::T_Aux,QString("show"));
}

void AuxWidget::setHthread(int value)
{
    Q_D(AuxWidget);
    d->m_Hthread = value;

}
bool AuxWidget::getHthreadExitStatus()
{
    Q_D(AuxWidget);
    qDebug()<<"++++++d->m_HthreadExitStatus+++++"<<d->m_HthreadExitStatus;
    return d->m_HthreadExitStatus;
}
AuxWidgetPrivate::AuxWidgetPrivate(AuxWidget *parent)
    : q_ptr(parent)
{
    m_AuxWidgetObject = NULL;
    m_AuxLoaderObject = NULL;
    m_AvInFd = -1;
    m_Hthread = 0;
    m_Visible = false;
    m_HthreadExitStatus = true;
}

AuxWidgetPrivate::~AuxWidgetPrivate()
{

}
static void* readDetectSignal(void *arg)
{
    AuxWidgetPrivate* pThis = (AuxWidgetPrivate*)arg;
    bool _DetectSignal(false);
    bool _AvInConfig(true);
    int  _NoSignalCount = 0;
    int res = arkapi_vin_start(pThis->m_AvInFd);
    qDebug()<<"++++++++++readDetectSignal++++++++++";
    if(res < 0)
    {
        printf("+++[AuxWidget:] av in start error+++\n");
    }
    emit g_Widget->onWidgetTypeChange(Widget::T_Aux, Widget::T_Aux,QString("request"));
    pThis->m_HthreadExitStatus = false;
    while (1)
    {
        if(pThis->m_Hthread == 0)
        {
            qDebug()<<"++++++pThis->m_Hthread+++++++";
            arkapi_vin_stop(pThis->m_AvInFd);
            arkapi_close_vin(pThis->m_AvInFd);
            break;
        }
        int ret =  arkapi_vin_detect_signal(pThis->m_AvInFd);
        printf("+++[AuxWidget:] av in detect　signalr:%d+++\n",ret);
        sleep(1);
        if((ret == 1) && (!_DetectSignal))
        {
            _DetectSignal  = true;
            _NoSignalCount = 0;
            _AvInConfig    = true;
            g_Widget->setPreemptiveWidget(1);
            emit g_Widget->onWidgetTypeChange(Widget::T_Aux, Widget::T_Aux,QString("request"));
            g_Audio->requestAudioSource(AS_Aux);
        }
        else if(ret <= 0){
             _NoSignalCount++;
             if(_NoSignalCount > 3)
             {
                 if(_AvInConfig){
                     arkapi_vin_stop(pThis->m_AvInFd);
                     arkapi_close_vin(pThis->m_AvInFd);
                     _NoSignalCount = 0;
                     _DetectSignal  = false;
                     _AvInConfig    = false;
                     g_Widget->setPreemptiveWidget(1);
                     emit g_Widget->onWidgetTypeChange(Widget::T_Aux, Widget::T_Aux,QString("show"));
                     g_Audio->releaseAudioSource(AS_Aux);
                 }
                 else if(_AvInConfig == false && _NoSignalCount == 5){
                     pThis->m_AvInFd = arkapi_open_vin();
                     qDebug()<<"-------readDetectSignal:m_AvInFd-------"<<pThis->m_AvInFd;
                     if (pThis->m_AvInFd < 0) {
                         qDebug("readDetectSignal open AvIn fail.");
                     }
                     int progressive = 0;
                     int itu601en = 0;
                     int ret = arkapi_vin_config(pThis->m_AvInFd,progressive,itu601en);
                     if(ret < 0)
                     {
                         printf("+++[AuxWidget:readDetectSignal] av in config error+++\n");
                     }
                     else{
                         ret = arkapi_vin_switch_channel(pThis->m_AvInFd, DVR_SOURCE_AUX);
                         if(ret < 0){
                             printf("+++[AuxWidget:readDetectSignal] av in switch channel error+++\n");
                         }
                     }
                     int res = arkapi_vin_start(pThis->m_AvInFd);
                     if(res < 0)
                     {
                         printf("+++[AuxWidget:] av in start error+++\n");
                     }
                 }
             }
        }
    }
    pThis->m_HthreadExitStatus = true;
    return NULL;
}

void AuxWidgetPrivate::initialize()
{
    Q_Q(AuxWidget);
    //static bool _InitEnter(true);
    //if(_InitEnter){
    pthread_t pthead;
    m_AvInFd = arkapi_open_vin();
    qDebug()<<"-------initialize:m_AvInFd-------"<<m_AvInFd;
    if (m_AvInFd < 0) {
        qDebug("open AvIn fail.");
        return;
    }
    int progressive = 0;
    int itu601en = 0;
    int ret = arkapi_vin_config(m_AvInFd,progressive,itu601en);
    if(ret < 0)
    {
        printf("+++[AuxWidget:] av in config error+++\n");
    }
    else{
        qDebug()<<"=======arkapi_vin_switch_channel========";
        ret = arkapi_vin_switch_channel(m_AvInFd, DVR_SOURCE_AUX);
        if(ret < 0){
            printf("+++[AuxWidget:] av in switch channel error+++\n");
        }
    }
    ret = pthread_create(&pthead, NULL,readDetectSignal, this);
    if(ret != 0) {
        printf("pthread_create failed!\n");
    }
        //_InitEnter = false;
   // }


}
