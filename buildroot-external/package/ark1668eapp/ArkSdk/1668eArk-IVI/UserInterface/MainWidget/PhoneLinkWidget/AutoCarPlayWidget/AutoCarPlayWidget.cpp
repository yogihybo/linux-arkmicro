#include "AutoCarPlayWidget.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include "BusinessLogic/Bluetooth.h"
#include <QQmlProperty>
#include <stdio.h>
#include <QDebug>
static const QString BtPowerStatus("/data/BtPowerStatus.ini");
class AutoCarPlayWidgetPrivate
{
    Q_DISABLE_COPY(AutoCarPlayWidgetPrivate)
public:
    explicit AutoCarPlayWidgetPrivate(AutoCarPlayWidget* parent);
    ~AutoCarPlayWidgetPrivate();
    void initializeObject();
    void connectAllSlots();
    void setBtPowerStatus(int value);
public:
    QObject* m_AutoCarPlayWidgetObject;
    QObject* m_ParendObject;
    QObject* m_AutoBtnObject;
    QObject* m_CarPlayBtnObject;
    int m_LinkType;
    int m_LinkMode;
    int m_DbusSend;
    int m_BtStatus;
    bool m_IsRunningBackGround;
    bool m_ConnectedFlag;
private:
    Q_DECLARE_PUBLIC(AutoCarPlayWidget)
    AutoCarPlayWidget* const q_ptr;
};

AutoCarPlayWidget::AutoCarPlayWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new AutoCarPlayWidgetPrivate(this))
{

}

void AutoCarPlayWidget::setAutoCarPlayWidgetObject(QObject *qmlObject)
{
    Q_D(AutoCarPlayWidget);
    if(d->m_AutoCarPlayWidgetObject == NULL)
    {

        d->m_AutoCarPlayWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_AutoBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_CarPlayBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    d->connectAllSlots();
}
void AutoCarPlayWidget::setAutoCarPlayWidgetParendObject(QObject *qmlObject){
    Q_D(AutoCarPlayWidget);
    if(d->m_ParendObject == NULL)
    {
        d->m_ParendObject = qmlObject;
    }
}
void AutoCarPlayWidget::onToolButtonRelease(){
    Q_D(AutoCarPlayWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr== d->m_AutoBtnObject){
        qDebug()<<"===[AutoCarPlayWidget::onToolButtonRelease()]==="<<d->m_DbusSend;
        if(d->m_DbusSend == DBUS_CONNECTED ){
            if(d->m_IsRunningBackGround){
                printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                printf("linktype = %d, linkmode = %d\r\n", d->m_LinkType, d->m_LinkMode);
                g_Link->requestLink(d->m_LinkType, d->m_LinkMode, DBUS_REQUEST_FOREGROUND);
             }
        }
        else{
            QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(1);
            g_Link->setLinkType(Android_Auto);
        }
        g_Widget->setPhoneLinkStatus(1);
    }
    else if(ptr== d->m_CarPlayBtnObject){
        qDebug()<<"===[AutoCarPlayWidget::onToolButtonRelease():m_CarPlayBtnObject]==="<<d->m_DbusSend;
        if(d->m_DbusSend == DBUS_CONNECTED ){
            if(d->m_IsRunningBackGround){
                printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                printf("linktype = %d, linkmode = %d\r\n", d->m_LinkType, d->m_LinkMode);
                g_Link->requestLink(d->m_LinkType, d->m_LinkMode, DBUS_REQUEST_FOREGROUND);
             }
        }
        else{
            QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(1);
            g_Link->setLinkType(Carplay);
        }
        g_Widget->setPhoneLinkStatus(1);
    }

}
void AutoCarPlayWidget::onLinkStatus(int type, int mode, int status)
{
    Q_D(AutoCarPlayWidget);
    if(status == DBUS_CONNECTED){
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(2);
        d->m_DbusSend = status;
        d->m_LinkType = type;
        d->m_LinkMode = mode;
        QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(0);
        if(type == Android_Auto){
           QQmlProperty(d->m_CarPlayBtnObject,"enabled").write(false);
           g_Audio->requestAudioSource(AS_AutoMusic);
           g_Link->requestNightMode(false);//日间夜间模式
        }
        else if(type == Carplay){
           QQmlProperty(d->m_AutoBtnObject,"enabled").write(false);
           d->m_ConnectedFlag = true;
           d->m_BtStatus =  g_Bluetooth->getPowerStatus();
           if(d->m_BtStatus > 0)
           {
               d->setBtPowerStatus(1);
               g_Bluetooth->disconnectRemoteDevice();
               g_Bluetooth->powerOff();
           }
           g_Audio->requestAudioSource(AS_CarplayMusic);
           g_Setting->onCarPlayConnected();
        }
        g_Widget->setPreemptiveWidget(1);
    }
    else if(status == DBUS_FOREGROUND){
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(2);
        d->m_IsRunningBackGround = false;
        if(type == Android_Auto){
           g_Audio->requestAudioSource(AS_AutoMusic);
        }
        else if(type == Carplay){
           g_Audio->requestAudioSource(AS_CarplayMusic);
        }
        g_Widget->setPreemptiveWidget(1);
    }
    else if(status == DBUS_BACKGROUND){
        g_Widget->setPreemptiveWidget(0);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
        d->m_IsRunningBackGround = true;
    }
    else if(status == DBUS_MUSIC_STARTED){
        if(type == Android_Auto){
           g_Audio->requestAudioSource(AS_AutoMusic);
        }
        else if(type == Carplay){
           g_Audio->requestAudioSource(AS_CarplayMusic);
        }
    }
    else if(status == DBUS_NAVI_SOUND_STARTED){
        g_Setting->setVolumeType(V_NavigationVolume);
        g_Audio->requestSetVolume(g_Setting->getNavigationVolume());
        g_Audio->open_amixer_mode(AUDIO_PHONE_MUSIC,CTRL_PHONE_MUSIC);
        g_Audio->set_amixersoftmaster_volume(90);//lower volume
    }
    else if(status == DBUS_NAVI_SOUND_STOPPED){
        g_Setting->setVolumeType(V_MediaVolume);
        g_Audio->requestSetVolume(g_Setting->getMediaVolume());
        g_Audio->set_amixersoftmaster_volume(127);//max
        g_Audio->close_amixer_mode();
    }
    else if(status == DBUS_PHONE_STARTED){
        if(type == Android_Auto){
           g_Audio->requestAudioSource(AS_AutoPhone);
        }
        else if(type == Carplay){
           g_Audio->requestAudioSource(AS_CarplayPhone);
        }
    }
    else if(status == DBUS_PHONE_STOPPED){
        if(type == Android_Auto){
           g_Audio->releaseAudioSource(AS_AutoPhone);
        }
        else if(type == Carplay){
           g_Audio->releaseAudioSource(AS_CarplayPhone);
        }
    }
    else if(status == DBUS_DISCONNECTED ||
            status == DBUS_FAILED ||
            status == DBUS_APP_EXIT ||
            status == DBUS_INTERRUPTED_BY_APP){
        g_Widget->setPreemptiveWidget(0);
        d->m_DbusSend = (DbusSend)status;
        QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(0);
        QQmlProperty(d->m_AutoBtnObject,"enabled").write(true);
        QQmlProperty(d->m_CarPlayBtnObject,"enabled").write(true);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
        if(type == Android_Auto){
            if(g_Audio->getAudioSource() == AS_AutoPhone){
                g_Audio->releaseAudioSource(AS_AutoPhone);
            }
        }
        else if(type == Carplay){
            if(g_Audio->getAudioSource() == AS_CarplayPhone)
                g_Audio->releaseAudioSource(AS_CarplayPhone);
            if(d->m_BtStatus > 0 && d->m_ConnectedFlag == true)
            {
                g_Bluetooth->powerOn();
                g_Bluetooth->reConnectLastDevice();
                d->setBtPowerStatus(0);
            }
            g_Setting->onCarPlayExit();
            d->m_ConnectedFlag = false;
        }
    }
    printf("utoCarPlayWidget::onLinkStatus :type = %d, mode =%d, status = %d\n", type, mode, status);
}
AutoCarPlayWidgetPrivate::AutoCarPlayWidgetPrivate(AutoCarPlayWidget *parent)
    : q_ptr(parent)
{
    m_AutoCarPlayWidgetObject = NULL;
    m_AutoBtnObject = NULL;
    m_CarPlayBtnObject = NULL;
    m_ParendObject     = NULL;
    m_LinkType = -1;
    m_LinkMode = -1;
    m_DbusSend = -1;
    m_BtStatus = 0;
    m_IsRunningBackGround = false;
    m_ConnectedFlag = false;
}
AutoCarPlayWidgetPrivate::~AutoCarPlayWidgetPrivate()
{

}
void AutoCarPlayWidgetPrivate::initializeObject()
{
    if(m_AutoCarPlayWidgetObject != NULL)
    {
        if(m_AutoBtnObject == NULL)
        {
            m_AutoBtnObject = m_AutoCarPlayWidgetObject->findChild<QObject*>("autoBtnObject");
        }

        if(m_CarPlayBtnObject == NULL)
        {
            m_CarPlayBtnObject = m_AutoCarPlayWidgetObject->findChild<QObject*>("carPlayBtnObject");
        }
    }
}
void AutoCarPlayWidgetPrivate::connectAllSlots()
{
    Q_Q(AutoCarPlayWidget);
    QObject::connect(g_Link, SIGNAL(onLinkStatus(int,int,int)),
                     q,   SLOT(onLinkStatus(int,int,int)));

}
void AutoCarPlayWidgetPrivate::setBtPowerStatus(int value)
{
    QFile BTPowerFile(BtPowerStatus);
    QSettings *BTBTPowersetFile = new QSettings(BtPowerStatus,QSettings::IniFormat);
    if(!BTPowerFile.exists())
    {
        qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"BtPowerStatusfile is not exist, creating...";
        g_Setting->executeShellCmd(QString(QString("touch ")+ BtPowerStatus).toLocal8Bit().constData());
        g_Setting->executeShellCmd("sync");
    }
    BTBTPowersetFile->setValue("powerStatus", QString::number(value));
    BTBTPowersetFile->sync();
    g_Setting->executeShellCmd("sync");
    delete BTBTPowersetFile;
}
