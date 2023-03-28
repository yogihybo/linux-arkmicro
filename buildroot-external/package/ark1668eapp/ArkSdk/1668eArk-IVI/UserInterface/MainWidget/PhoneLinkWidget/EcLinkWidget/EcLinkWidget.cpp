#include "EcLinkWidget.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <stdio.h>
#include <QDebug>
class EcLinkWidgetPrivate
{
    Q_DISABLE_COPY(EcLinkWidgetPrivate)
public:
    explicit EcLinkWidgetPrivate(EcLinkWidget* parent);
    ~EcLinkWidgetPrivate();
    void connectAllSlots();
    void linkChangeBTHandler(const int type, const bool bBT);
public:
    QObject* m_EcLinkWidgetObject;
    QObject* m_ParendObject;
    QObject* m_EcLinkBtnObject;
    int m_LinkType;
    int m_LinkMode;
    int m_DbusSend;
    bool m_IsRunningBackGround;
private:
    Q_DECLARE_PUBLIC(EcLinkWidget)
    EcLinkWidget* const q_ptr;
};

EcLinkWidget::EcLinkWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new EcLinkWidgetPrivate(this))
{

}
void EcLinkWidget::setEcLinkWidgetObject(QObject* qmlObject)
{
    Q_D(EcLinkWidget);
    if(d->m_EcLinkWidgetObject == NULL)
    {
        d->m_EcLinkWidgetObject = qmlObject;
    }
    if(d->m_EcLinkBtnObject == NULL)
    {
        d->m_EcLinkBtnObject = d->m_EcLinkWidgetObject->findChild<QObject*>("ecLinkBtnObject");
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_EcLinkBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void EcLinkWidget::onToolButtonRelease(){
    Q_D(EcLinkWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr== d->m_EcLinkBtnObject){
        qDebug()<<"===[EcLinkWidget::onToolButtonRelease()]==="<<d->m_DbusSend;
        if(d->m_DbusSend == DBUS_CONNECTED ){
            if(d->m_IsRunningBackGround){
                printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                printf("linktype = %d, linkmode = %d\r\n", d->m_LinkType, d->m_LinkMode);
                g_Link->requestLink(d->m_LinkType, d->m_LinkMode, DBUS_REQUEST_FOREGROUND);
             }
        }
        else{
            QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(1);
            g_Link->setLinkType(ECLink);
        }
        g_Widget->setPhoneLinkStatus(1);
    }
}
void EcLinkWidget::setEcLinkWidgetParentObject(QObject* qmlObject){
    Q_D(EcLinkWidget);
    if(d->m_ParendObject == NULL)
    {
        d->m_ParendObject = qmlObject;
    }
}
void EcLinkWidget::onLinkStatus(int type, int mode, int status)
{
    Q_D(EcLinkWidget);
    g_Link->setDbusConnectStatus(status);
    if(status == DBUS_CONNECTED){
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(2);
        d->m_DbusSend = status;
        d->m_LinkType = type;
        d->m_LinkMode = mode;
        QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(0);
        d->linkChangeBTHandler(ECLink,true);
        g_Widget->setPreemptiveWidget(1);
    }
    else if(status == DBUS_FOREGROUND){
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(2);
        d->m_IsRunningBackGround = false;
        d->linkChangeBTHandler(ECLink,true);
        g_Widget->setPreemptiveWidget(1);

    }
    else if(status == DBUS_BACKGROUND){
        g_Widget->setPreemptiveWidget(0);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
        d->m_IsRunningBackGround = true;
    }
    else if(status ==DBUS_MUSIC_STARTED){
        d->linkChangeBTHandler(ECLink,true);
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
    else if(status == DBUS_DISCONNECTED ||
            status == DBUS_FAILED ||
            status == DBUS_APP_EXIT ||
            status == DBUS_INTERRUPTED_BY_APP){
        g_Widget->setPreemptiveWidget(0);
        d->m_DbusSend = (DbusSend)status;
        QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(0);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
    }
    printf("EcLinkWidget::onLinkStatus :type = %d, mode =%d, status = %d\n", type, mode, status);
}
EcLinkWidgetPrivate::EcLinkWidgetPrivate(EcLinkWidget *parent)
    : q_ptr(parent)
{
    m_EcLinkWidgetObject = NULL;
    m_ParendObject = NULL;
    m_EcLinkBtnObject = NULL;
    m_LinkType = -1;
    m_LinkMode = -1;
    m_DbusSend = -1;
    m_IsRunningBackGround = false;
    connectAllSlots();
}
EcLinkWidgetPrivate::~EcLinkWidgetPrivate()
{

}
void EcLinkWidgetPrivate::linkChangeBTHandler(const int type, const bool bBT){
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << type << bBT << m_DbusSend;
    if (ECLink == type) {
        qDebug() << __PRETTY_FUNCTION__ << __LINE__;
        if (bBT && (Bluetooth::BCS_Connected == g_Bluetooth->connectStatus())) {
            qDebug() << __PRETTY_FUNCTION__ << __LINE__;
            if ((AS_CarplayMusic != g_Audio->getAudioSource())
                    && (AS_CarplayPhone != g_Audio->getAudioSource())
                    && (AS_CarlifeMusic != g_Audio->getAudioSource())
                    && (AS_CarlifePhone != g_Audio->getAudioSource())
                    && (AS_AutoMusic != g_Audio->getAudioSource())
                    && (AS_AutoPhone != g_Audio->getAudioSource())
                    && (AS_ECLinkMusic != g_Audio->getAudioSource())
                    && (AS_ECLinkBluetoothMusic != g_Audio->getAudioSource())) {
                g_Audio->faderOut();
            }
            if (AS_ECLinkBluetoothMusic != g_Audio->getAudioSource()) {
                g_Audio->requestAudioSource(AS_ECLinkBluetoothMusic);
            }
        } else {
            if ((AS_CarplayMusic != g_Audio->getAudioSource())
                    && (AS_CarplayPhone != g_Audio->getAudioSource())
                    && (AS_CarlifeMusic != g_Audio->getAudioSource())
                    && (AS_CarlifePhone != g_Audio->getAudioSource())
                    && (AS_AutoMusic != g_Audio->getAudioSource())
                    && (AS_AutoPhone != g_Audio->getAudioSource())
                    && (AS_ECLinkMusic != g_Audio->getAudioSource())
                    && (AS_ECLinkBluetoothMusic != g_Audio->getAudioSource())) {
                g_Audio->faderOut();
            }
            if (AS_ECLinkMusic != g_Audio->getAudioSource()) {
                g_Audio->requestAudioSource(AS_ECLinkMusic);
            }
        }
    }

}
void EcLinkWidgetPrivate::connectAllSlots()
{
    Q_Q(EcLinkWidget);
    QObject::connect(g_Link, SIGNAL(onLinkStatus(int,int,int)),
                     q,   SLOT(onLinkStatus(int,int,int)));

}
