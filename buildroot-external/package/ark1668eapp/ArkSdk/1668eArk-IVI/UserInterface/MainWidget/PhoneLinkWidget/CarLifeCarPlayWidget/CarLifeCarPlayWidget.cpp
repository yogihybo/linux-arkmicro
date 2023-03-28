#include "CarLifeCarPlayWidget.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <stdio.h>
#include <QDebug>
static const QString BtPowerStatus("/data/BtPowerStatus.ini");
class CarLifeCarPlayWidgetPrivate
{
    Q_DISABLE_COPY(CarLifeCarPlayWidgetPrivate)
public:
    explicit CarLifeCarPlayWidgetPrivate(CarLifeCarPlayWidget* parent);
    ~CarLifeCarPlayWidgetPrivate();
    void initializeObject();
    void connectAllSlots();
    void setBtPowerStatus(int value);
public:
    QObject* m_CarLifeCarPlayWidgetObject;
    QObject* m_ParendObject;
    QObject* m_CarLifeBtnObject;
    QObject* m_CarPlayBtnObject;
    int m_LinkType;
    int m_LinkMode;
    int m_DbusSend;
    int m_CarLifeMusicPlayStatus;
    int m_BtStatus;
    bool m_ConnectedFlag;
    bool m_IsRunningBackGround;
private:
    Q_DECLARE_PUBLIC(CarLifeCarPlayWidget)
    CarLifeCarPlayWidget* const q_ptr;
};

CarLifeCarPlayWidget::CarLifeCarPlayWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new CarLifeCarPlayWidgetPrivate(this))
{

}

void CarLifeCarPlayWidget::setCarLifeCarPlayWidgetObject(QObject *qmlObject)
{
    Q_D(CarLifeCarPlayWidget);
    if(d->m_CarLifeCarPlayWidgetObject == NULL)
    {

        d->m_CarLifeCarPlayWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_CarLifeBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_CarPlayBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void CarLifeCarPlayWidget::setCarLifeCarPlayWidgetParendObject(QObject *qmlObject){
    Q_D(CarLifeCarPlayWidget);
    if(d->m_ParendObject == NULL)
    {
        d->m_ParendObject = qmlObject;
    }
}
void CarLifeCarPlayWidget::onToolButtonRelease(){
    Q_D(CarLifeCarPlayWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr== d->m_CarLifeBtnObject){
        qDebug()<<"===[CarLifeCarPlayWidget::onToolButtonRelease()]==="<<d->m_DbusSend;
        if(d->m_DbusSend == DBUS_CONNECTED ){
            if(d->m_IsRunningBackGround){
                printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                printf("linktype = %d, linkmode = %d\r\n", d->m_LinkType, d->m_LinkMode);
                g_Link->requestLink(d->m_LinkType, d->m_LinkMode, DBUS_REQUEST_FOREGROUND);
             }
        }
        else{
            QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(1);
            g_Link->setLinkType(Carlife);
        }
        g_Widget->setPhoneLinkStatus(1);
    }
    else if(ptr== d->m_CarPlayBtnObject){
        qDebug()<<"===[CarLifeCarPlayWidget::onToolButtonRelease():m_CarPlayBtnObject]==="<<d->m_DbusSend;
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
void CarLifeCarPlayWidget::onLinkStatus(int type, int mode, int status)
{
    Q_D(CarLifeCarPlayWidget);
    if(status == DBUS_CONNECTED){
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(2);
        d->m_DbusSend = status;
        d->m_LinkType = type;
        d->m_LinkMode = mode;
        qDebug()<<"+++++++++++msgWidgetVisible0000+++++++++++++";
        QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(0);
        if(type == Carlife){
           QQmlProperty(d->m_CarPlayBtnObject,"enabled").write(false);
           g_Audio->requestAudioSource(AS_CarlifeMusic);
           g_Bluetooth->disableBtMusic();
        }
        else if(type == Carplay){
           d->m_ConnectedFlag = true;
           QQmlProperty(d->m_CarLifeBtnObject,"enabled").write(false);
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
        if(type == Carlife){
           //g_Bluetooth->disableBtMusic();
           //g_Setting->mySleep(1000);
           g_Audio->requestAudioSource(AS_CarlifeMusic);
           if(d->m_CarLifeMusicPlayStatus != -1)
           {
               g_Link->requestKey(KEY_MUSIC_PLAY);
           }
        }
        else if(type == Carplay){
           g_Audio->requestAudioSource(AS_CarplayMusic);
        }
        g_Widget->setPreemptiveWidget(1);

    }
    else if(status == DBUS_BACKGROUND){
        //g_Bluetooth->connectBtMusic();
        g_Widget->setPreemptiveWidget(0);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
        d->m_IsRunningBackGround = true;
    }
    else if(status ==DBUS_MUSIC_STARTED){
        if(type == Carlife){
           g_Bluetooth->disableBtMusic();
           g_Audio->requestAudioSource(AS_CarlifeMusic);
           d->m_CarLifeMusicPlayStatus = 0;//播放状态
           //g_Link->requestKey(KEY_MUSIC_PLAY);
        }
        else if(type == Carplay){
           g_Audio->requestAudioSource(AS_CarplayMusic);
        }
    }
    else if(status ==DBUS_MUSIC_STOPPED){
        if(type == Carlife){
           d->m_CarLifeMusicPlayStatus = 1;//暂停状态
        }
    }
    else if(status == DBUS_NAVI_SOUND_STARTED){
        if(type == Carlife)
        {
            qDebug()<<"++++++++++DBUS_NAVI_SOUND_STARTED++++++++++++";
            g_Setting->setVolumeType(V_NavigationVolume);
            g_Audio->requestSetVolume(g_Setting->getNavigationVolume());
            g_Audio->open_amixer_mode(AUDIO_PHONE_MUSIC,CTRL_PHONE_MUSIC);
            g_Audio->set_amixersoftmaster_volume(90);//lower volume
        }
    }
    else if(status == DBUS_NAVI_SOUND_STOPPED){
        if(type == Carlife)
        {
            g_Setting->setVolumeType(V_MediaVolume);
            qDebug()<<"++++++++++DBUS_NAVI_SOUND_STOPPED++++++++++++";
            g_Audio->requestSetVolume(g_Setting->getMediaVolume());
            g_Audio->set_amixersoftmaster_volume(127);//max
            g_Audio->close_amixer_mode();
        }
    }
    else if(status == DBUS_PHONE_STARTED){
        if(type == Carlife){
           g_Audio->requestAudioSource(AS_CarlifePhone);
        }
        else if(type == Carplay){
           g_Audio->requestAudioSource(AS_CarplayPhone);
        }
    }
    else if(status == DBUS_PHONE_STOPPED){
        if(type == Carlife){
           g_Audio->releaseAudioSource(AS_CarlifePhone);
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
        QQmlProperty(d->m_CarLifeBtnObject,"enabled").write(true);
        QQmlProperty(d->m_CarPlayBtnObject,"enabled").write(true);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
        if(type == Carlife){
            if(g_Audio->getAudioSource() == AS_CarlifePhone)
                g_Audio->releaseAudioSource(AS_CarlifePhone);
            g_Bluetooth->connectBtMusic();
        }
        else if(type == Carplay){
            if(g_Audio->getAudioSource() == AS_CarplayPhone)
            {
                g_Audio->releaseAudioSource(AS_CarplayPhone);
            }
            if(d->m_BtStatus > 0 && d->m_ConnectedFlag == true)
            {
                g_Bluetooth->powerOn();
                g_Bluetooth->reConnectLastDevice();
                d->setBtPowerStatus(0);
            }
            g_Setting->onCarPlayExit();
            d->m_ConnectedFlag = false;
        }
        d->m_CarLifeMusicPlayStatus = -1;
    }
    printf("CarLifeCarPlayWidget::onLinkStatus :type = %d, mode =%d, status = %d\n", type, mode, status);
}
void CarLifeCarPlayWidget::onLinkDuckAudio(const int type, double durationSecs, double volume) //send carlink's audio duck volume
{
    qDebug()<<"++++++++++CarLifeCarPlayWidget::onLinkDuckAudio0000+++++++++"<<type;
    if(type == Carplay)
    {
        qDebug()<<"++++++++++DBUS_NAVI_SOUND_STARTED++++++++++++";
        g_Setting->setVolumeType(V_NavigationVolume);
        g_Audio->requestSetVolume(g_Setting->getNavigationVolume());
        g_Audio->open_amixer_mode(AUDIO_PHONE_MUSIC,CTRL_PHONE_MUSIC);
        g_Audio->set_amixersoftmaster_volume(90);//lower volume
    }
}


void CarLifeCarPlayWidget::onLinkUnduckAudio(const int type, double durationSecs)//send carlink's audio unduck volume
{
    qDebug()<<"++++++++++CarLifeCarPlayWidget::onLinkUnduckAudio+++++++++"<<type;
    if(type == Carplay)
    {
        g_Setting->setVolumeType(V_MediaVolume);
        qDebug()<<"++++++++++DBUS_NAVI_SOUND_STOPPED++++++++++++";
        g_Audio->requestSetVolume(g_Setting->getMediaVolume());
        g_Audio->set_amixersoftmaster_volume(127);//max
        g_Audio->close_amixer_mode();
    }
}

void CarLifeCarPlayWidget::onConnectStatusChange(int status)
{
    Q_D(CarLifeCarPlayWidget);
    if(status == 3)
    {
        if(g_Link->getDbusConnectStatus() == DBUS_BACKGROUND && g_Link->getLinkType() == Carlife && g_Setting->getCarlifeTelephoneStatus() == 0)
        {
            if(d->m_CarLifeMusicPlayStatus != -1)
            {
                g_Link->requestKey(KEY_MUSIC_PLAY);
            }
        }
    }
}

CarLifeCarPlayWidgetPrivate::CarLifeCarPlayWidgetPrivate(CarLifeCarPlayWidget *parent)
    : q_ptr(parent)
{
    m_CarLifeCarPlayWidgetObject = NULL;
    m_CarLifeBtnObject = NULL;
    m_CarPlayBtnObject = NULL;
    m_ParendObject     = NULL;
    m_LinkType = -1;
    m_LinkMode = -1;
    m_DbusSend = -1;
    m_CarLifeMusicPlayStatus = -1;
    m_IsRunningBackGround = false;
    m_BtStatus = 0;
    m_ConnectedFlag = false;
    connectAllSlots();
}
CarLifeCarPlayWidgetPrivate::~CarLifeCarPlayWidgetPrivate()
{

}
void CarLifeCarPlayWidgetPrivate::initializeObject()
{
    if(m_CarLifeCarPlayWidgetObject != NULL)
    {
        if(m_CarLifeBtnObject == NULL)
        {
            m_CarLifeBtnObject = m_CarLifeCarPlayWidgetObject->findChild<QObject*>("carLifeBtnObject");
        }

        if(m_CarPlayBtnObject == NULL)
        {
            m_CarPlayBtnObject = m_CarLifeCarPlayWidgetObject->findChild<QObject*>("carPlayBtnObject");
        }
    }
}
void CarLifeCarPlayWidgetPrivate::connectAllSlots()
{
    Q_Q(CarLifeCarPlayWidget);
    QObject::connect(g_Link, SIGNAL(onLinkStatus(int,int,int)),
                     q,   SLOT(onLinkStatus(int,int,int)));
    QObject::connect(g_Link, SIGNAL(onLinkDuckAudio(const int, double, double)),
                     q,   SLOT(onLinkDuckAudio(const int, double, double)));
    QObject::connect(g_Link, SIGNAL(onLinkUnduckAudio(const int, double)),
                     q,   SLOT(onLinkUnduckAudio(const int, double)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(int)));

}
void CarLifeCarPlayWidgetPrivate::setBtPowerStatus(int value)
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
