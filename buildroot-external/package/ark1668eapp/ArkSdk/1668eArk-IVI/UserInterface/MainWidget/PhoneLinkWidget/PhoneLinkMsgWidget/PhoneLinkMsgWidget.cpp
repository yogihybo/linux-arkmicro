#include "PhoneLinkMsgWidget.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/HostApd.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/WiFiManager.h"
#include "BusinessLogic/Setting.h"
#include "BusinessLogic/ark_api.h"
#include "BusinessLogic/QmlWidget.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <stdio.h>
#include <stdlib.h>
#include <QDebug>
#include <unistd.h>
#include <QTimer>
class PhoneLinkMsgWidgetPrivate
{
    Q_DISABLE_COPY(PhoneLinkMsgWidgetPrivate)
public:
    explicit PhoneLinkMsgWidgetPrivate(PhoneLinkMsgWidget* parent);
    ~PhoneLinkMsgWidgetPrivate();
    void initializeObject();
    void waitForQuit();
    void connectAllSlots();
public:
    QObject* m_PhoneLinkMsgWidgetObject;
    QObject* m_WireBtnObject;
    QObject* m_WirelessBtnObject;
    QObject* m_LoaderImageObject;
    QObject* m_AnimationObject;
    int m_Inserted;
    int m_PhoneType;
    int m_LinkType;
    int m_LinkMode;
    int m_DbusSend;
    int m_CallType;
    bool m_IsRunningBackGround;
    bool m_IsConnected;
private:
    Q_DECLARE_PUBLIC(PhoneLinkMsgWidget)
    PhoneLinkMsgWidget* const q_ptr;
};
PhoneLinkMsgWidget::PhoneLinkMsgWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new PhoneLinkMsgWidgetPrivate(this))
{

}
void PhoneLinkMsgWidget::setPhoneLinkMsgWidgetObject(QObject* qmlObject)
{
    Q_D(PhoneLinkMsgWidget);
    if(d->m_PhoneLinkMsgWidgetObject == NULL)
    {
        d->m_PhoneLinkMsgWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_WireBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_WirelessBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void PhoneLinkMsgWidget::onToolButtonRelease()
{
    Q_D(PhoneLinkMsgWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_WireBtnObject){
        qDebug()<<"====[PhoneLinkMsgWidget::onToolButtonRelease:m_DbusSend]===="<<d->m_DbusSend;
        qDebug()<<"====[PhoneLinkMsgWidget::onToolButtonRelease:m_Inserted]===="<<d->m_Inserted;
        qDebug()<<"====[PhoneLinkMsgWidget::onToolButtonRelease:getLinkType()]===="<<g_Link->getLinkType();
        if(d->m_Inserted != DBUS_DEVICE_ATTACHED)
        {
            emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("Phone not inserted")));
        }
        if(d->m_DbusSend == DBUS_CONNECTED)
        {
            emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("the Phone is connected")));
        }
        if(d->m_DbusSend != DBUS_CONNECTED ){
            if(d->m_Inserted == DBUS_DEVICE_ATTACHED){
                if (g_Link->getLinkType() == Carplay){
                    if(d->m_PhoneType == Phone_IOS && d->m_IsConnected == false)
                    {
                        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                        d->m_IsConnected = true;
                        g_Link->requestLink(Carplay, Wired, DBUS_REQUEST_CONNECT);
                        QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(1);
                        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(false);
                        QQmlProperty(d->m_AnimationObject,"running").write(true);
                    }
                    else
                    {
                        emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("Wrong phone type, please insert ios phone")));
                    }
                }
                else if(g_Link->getLinkType() == Android_Auto) {
                    if(d->m_PhoneType == Phone_Android && d->m_IsConnected == false)
                    {
                        d->m_IsConnected = true;
                        g_Link->requestLink(Android_Auto, Wired, DBUS_REQUEST_CONNECT);
                        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                        QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(1);
                        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(false);
                        QQmlProperty(d->m_AnimationObject,"running").write(true);
                    }
                    else
                    {
                        emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("Wrong phone type, please insert android phone")));
                    }
                }
                else if(g_Link->getLinkType() == Carlife){
                    if(d->m_PhoneType == Phone_Android && d->m_IsConnected == false)
                    {
                        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                        d->m_IsConnected = true;
                        g_Link->requestLink(Carlife, Wired, DBUS_REQUEST_CONNECT);
                        QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(1);
                        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(false);
                        QQmlProperty(d->m_AnimationObject,"running").write(true);
                    }
                    else
                    {
                        emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("Wrong phone type, please insert android phone")));
                    }
                }
            }
        }
    }
    if(ptr == d->m_WirelessBtnObject){
        if(d->m_DbusSend == DBUS_CONNECTED)
        {
            emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("the Phone is connected")));
        }
        if(d->m_DbusSend != DBUS_CONNECTED ){
            if(g_Link->getLinkType() == Carplay && d->m_IsConnected == false){
                if(g_Bluetooth->connectStatus() < 3)
                {
                    qDebug()<<"++++++++++g_Bluetooth->connectStatus()+++++++++"<<g_Bluetooth->connectStatus();
                    emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("Bt is not connected,please connect Bt first")));
                }
                if(g_Bluetooth->connectStatus() >= 3){
                    d->m_IsConnected = true ;
                    QFile _BwFile("/dev/bw_iap");
                    if(_BwFile.exists()){
                        qDebug()<<"/dev/bw_iap is exist";
                        QDir _SocketDirPath("/dev/socket");
                        if(_SocketDirPath.exists())
                        {
                            qDebug()<<"/dev/socket is exist";
                            g_Setting->executeShellCmd("rm -rf /dev/socket/");
                        }
                        g_Setting->executeShellCmd("mkdir -p /dev/socket/");
                        g_Setting->executeShellCmd("ln /dev/bw_iap /dev/socket/goc_rfcom");
                    }
                    if(g_Setting->getwifiOpenStatus() == 1)
                    {
                        g_Setting->executeShellCmd("killall udhcpd");
                        g_Setting->executeShellCmd("killall hostapd");
                        g_Setting->executeShellCmd("killall udhcpc");
                        g_Setting->executeShellCmd("killall wpa_supplicant");
                        g_Setting->executeShellCmd("ifconfig wlan0 down");
                        g_Setting->clearSsidList();
                        g_Setting->onWifiSsidListChanged();
                        g_WiFiManager->exitPthread();
                        g_HostApd->setInitHostapdStatus(1);
                        g_Setting->setWifiOpenStatus(0);
                        emit g_Setting->onWlanClose();
                    }
                    int _InitHostapdStatus = g_HostApd->getInitHostapdStatus();
                    qDebug()<<"++++++++++_InitHostapdStatusCarplay+++++++++++"<<_InitHostapdStatus;
                    if(_InitHostapdStatus == 1)
                    {
                        g_HostApd->CreatThreadInitHostApd();
                    }
                    for(int i= 0;i<50;i++)
                    {
                        usleep(100*1000);
                        QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(1);
                        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(false);
                        QQmlProperty(d->m_AnimationObject,"running").write(true);
                        _InitHostapdStatus = g_HostApd->getInitHostapdStatus();
                        if(_InitHostapdStatus == 0)
                        {
                            g_HostApd->restartHostApd();
                            QString ssid = g_Setting->getHostapdSsid();
                            g_Link->requestWifi(ssid.toStdString(), "12345678", "36");
                            qDebug()<<"======g_Bluetooth->getRemoteBtAddress().toStdString()======="<<g_Bluetooth->getRemoteBtAddress();
                            //AC1F74DB5BCA
                            QString _BtAddress;
                            QString _RemoteBtAddress = g_Bluetooth->getRemoteBtAddress();
                            if(_RemoteBtAddress.size() == 12)
                            {
                                _BtAddress = _RemoteBtAddress.mid(0,2) +(QString(":")) + _RemoteBtAddress.mid(2,2) +(QString(":"))+
                                             _RemoteBtAddress.mid(4,2) +(QString(":")) + _RemoteBtAddress.mid(6,2) +(QString(":"))+
                                             _RemoteBtAddress.mid(8,2) +(QString(":")) + _RemoteBtAddress.mid(10,2);

                            }
                            qDebug()<<"======_BtAddress=======" << _BtAddress;
                            g_Link->requestPhoneBTAddress(_BtAddress.toStdString());
                            g_Link->requestLink(Carplay, Wireless, DBUS_REQUEST_CONNECT);
                            break;
                        }
                    }
                }
            }
            else if(g_Link->getLinkType() == Android_Auto && d->m_IsConnected == false){
                if(g_Bluetooth->connectStatus() < 3)
                {
                    emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("Bt is not connected, please connect Bt first")));
                }
                if(g_Bluetooth->connectStatus() >= 3){
                    d->m_IsConnected = true;
                    if(g_Setting->getwifiOpenStatus() == 1)
                    {
                        g_Setting->executeShellCmd("killall udhcpd");
                        g_Setting->executeShellCmd("killall hostapd");
                        g_Setting->executeShellCmd("killall udhcpc");
                        g_Setting->executeShellCmd("killall wpa_supplicant");
                        g_Setting->executeShellCmd("ifconfig wlan0 down");
                        g_Setting->clearSsidList();
                        g_Setting->onWifiSsidListChanged();
                        g_WiFiManager->exitPthread();
                        g_HostApd->setInitHostapdStatus(1);
                        g_Setting->setWifiOpenStatus(0);
                        emit g_Setting->onWlanClose();
                    }
                    int _InitHostapdStatus = g_HostApd->getInitHostapdStatus();
                    qDebug()<<"++++++++++_InitHostapdStatusAndroid_Auto+++++++++++"<<_InitHostapdStatus;
                    if(_InitHostapdStatus == 1)
                    {
                        g_HostApd->CreatThreadInitHostApd();
                    }
                    for(int i= 0;i<50;i++)
                    {
                        usleep(100*1000);
                        QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(1);
                        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(false);
                        QQmlProperty(d->m_AnimationObject,"running").write(true);
                        _InitHostapdStatus = g_HostApd->getInitHostapdStatus();
                        if(_InitHostapdStatus == 0)
                        {
                            qDebug()<<"--------_InitHostapdStatus---------"<<i;
                            g_HostApd->restartHostApd();
                            QString ssid = g_Setting->getHostapdSsid();
                            qDebug()<<"+++++ssid0000++++++++"<<ssid;
                            g_Link->requestWifi(ssid.toStdString(), "12345678", "36");
                            qDebug()<<"======g_Bluetooth->getRemoteBtAddress().toStdString()======="<<g_Bluetooth->getRemoteBtAddress();
                            //AC1F74DB5BCA
                            QString _BtAddress;
                            QString _RemoteBtAddress = g_Bluetooth->getRemoteBtAddress();
                            if(_RemoteBtAddress.size() == 12)
                            {
                                _BtAddress = _RemoteBtAddress.mid(0,2) +(QString(":")) + _RemoteBtAddress.mid(2,2) +(QString(":"))+
                                             _RemoteBtAddress.mid(4,2) +(QString(":")) + _RemoteBtAddress.mid(6,2) +(QString(":"))+
                                             _RemoteBtAddress.mid(8,2) +(QString(":")) + _RemoteBtAddress.mid(10,2);

                            }
                            qDebug()<<"======_BtAddress=======" << _BtAddress;
                            g_Link->requestPhoneBTAddress(_BtAddress.toStdString());
                            g_Link->requestLink(Android_Auto, Wireless, DBUS_REQUEST_CONNECT);
                            break;
                        }
                    }
                }

            }
            else if(g_Link->getLinkType() == Carlife && d->m_IsConnected == false){
                if(g_WiFiManager->getWifiStatus() != WCS_Success)
                {
                    emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("wifi is not connected,please connect wifi first")));
                }
                else if(g_WiFiManager->getWifiStatus() == WCS_Success)
                {
                    d->m_IsConnected = true;
                    static string _IPAddress ;
                    char m_GateWay[32] = {0};
                    g_WiFiManager->wifiGetGateway(m_GateWay);
                    _IPAddress = QString(m_GateWay).toStdString();
                    printf("======m_GateWay: %s=====\n", m_GateWay);
                    g_Link->requestPhoneIPAddress(_IPAddress);
                    g_Link->requestLink(Carlife, Wireless, DBUS_REQUEST_CONNECT);
                    QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(1);
                    QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(false);
                    QQmlProperty(d->m_AnimationObject,"running").write(true);
                }
            }
        }
    }
}
void PhoneLinkMsgWidget::onLinkStatus(int type, int mode, int status)
{
    Q_D(PhoneLinkMsgWidget);
    printf("PhoneLinkMsgWidget::onLinkStatus :type = %d, mode =%d, status = %d\n", type, mode, status);
    g_Link->setDbusConnectStatus(status);
    if(status == DBUS_CONNECTED){
        //onUIChanged(false);
        g_Link->setLinkConnectStatus(status);
        d->m_DbusSend = status;
        d->m_LinkType = type;
        d->m_LinkMode = mode;
        g_Link->setLinkMode(mode);
        QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(0);
        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(true);
        g_Setting->setDisplayMode(DISP_PHONELINK);
        d->m_IsConnected = false;
    }
    else if(status == DBUS_FOREGROUND){
        g_Link->setLinkConnectStatus(status);
        d->m_IsRunningBackGround = false;
        g_Setting->setDisplayMode(DISP_PHONELINK);
        if(d->m_LinkType == Carlife)
        {
            if(d->m_CallType == 1)
            {
                g_Setting->onPhoneLinkTelePhone(d->m_CallType);
                d->m_CallType = -1;
            }
            if(g_Setting->getCarlifeTelephoneStatus() == 1)
            {
                g_Setting->setCarlifeTelephoneStatus(0);
            }
        }
    }
    else if(status == DBUS_BACKGROUND){
        g_Link->setLinkConnectStatus(0);
        d->m_IsRunningBackGround = true;
        if(d->m_LinkType == Carlife){
            if(d->m_CallType == 0)
            {
                g_Setting->onPhoneLinkTelePhone(d->m_CallType);
                d->m_CallType = -1;
            }
            if(g_Setting->getCarlifeTelephoneStatus() == 0)
            {
                emit g_Setting->onPhoneLinkTelePhone(-1);
            }
        }

        //onUIChanged(true);
    }
    else if(status == DBUS_DISCONNECTED ||
            status == DBUS_FAILED ||
            status == DBUS_APP_EXIT ||
            status == DBUS_INTERRUPTED_BY_APP){
        //onUIChanged(true);
        g_Link->setLinkConnectStatus(0);
        d->m_DbusSend = (DbusSend)status;
        QQmlProperty(d->m_PhoneLinkMsgWidgetObject,"showType").write(0);
        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(true);
        d->m_IsConnected = false;
        if(d->m_LinkType == Carlife){
            d->m_CallType = -1;
            g_Setting->onPhoneLinkTelePhone(d->m_CallType);
        }
        emit QmlWidget::instance()->onPhoneLinkMsgShowWidgetShow(QString(QObject::tr("phone connected exit")));
    }

}
void PhoneLinkMsgWidget::onPhoneType(int type, int inserted)
{
    Q_D(PhoneLinkMsgWidget);
    qDebug()<<"===[onPhoneType:inserted]==="<<inserted;
    if(type == 1){
        d->m_PhoneType = Phone_Android;
    }
    else if(type == 2){
        d->m_PhoneType = Phone_IOS;
    }
    if(inserted == 0){
        d->m_Inserted = DBUS_DEVICE_ATTACHED;
    }

    else if(inserted == 1){
         d->m_Inserted = DBUS_DEVICE_DEATTACHED;
    }
}
void PhoneLinkMsgWidget::onCarLinkVersion(const int type,  const QString ver)
{
    //qDebug()<<"===[PhoneLinkMsgWidget::onCarLinkVersion]===";
    printf("onCarLinkVersion :type = %d, ver =%s\n", type, ver.toStdString().data());
}

void PhoneLinkMsgWidget::onDateTime(const int type, const long long time)
{
    //qDebug()<<"===[PhoneLinkMsgWidget::onDateTime]===";
    time_t t = time;
    char *ptime = ctime(&t);
    printf("onDateTime :type: %d, time:%s\n", type, ptime);
}

void PhoneLinkMsgWidget::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder)
{
    Q_D(PhoneLinkMsgWidget);
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << oldHolder << newHolder;
    switch (newHolder) {
    case AS_Video: {
        if (-1 != d->m_DbusSend ) {
            qDebug()<<"++++++onHolderChange/////0000++++++++"<< d->m_LinkType << d->m_LinkMode;
            g_Link->requestLink(d->m_LinkType, d->m_LinkMode,DBUS_REQUEST_EXITED);
            switch (oldHolder) {
            case AS_CarplayPhone:
            case AS_CarlifePhone:
            case AS_AutoPhone:
            case AS_ECLinkMusic:
            case AS_ECLinkBluetoothMusic:{
                d->waitForQuit();
                break;
            }
            default: {
                break;
            }
            }
        }
        break;
    }
    case AS_Music:
    {
        switch (oldHolder) {
            case AS_CarlifeMusic:
            case AS_CarplayMusic:
            case AS_ECLinkMusic:
            case AS_HiCarMusic:
            case AS_HiCarBluetoothMusic:
                //qDebug()<<"+++++++++++KEY_MUSIC_PAUSE0000++++++++++";
                if(g_Link->getDbusConnectStatus() == DBUS_BACKGROUND)
                {
                    g_Link->requestKey(KEY_MUSIC_PAUSE);
                }
                break;
            default:
                break;
            }
            break;
    }
    default: {
        switch (oldHolder) {
        case AS_Aux: {
            switch (newHolder) {
            case AS_CarplayMusic:
            case AS_AutoMusic:
            case AS_CarlifeMusic:
            case AS_ECLinkMusic:
            case AS_ECLinkBluetoothMusic:{
                g_Audio->requestAudioSource(newHolder);
                break;
            }
            default: {
                break;
            }
            }
            break;
        }
        case AS_BluetoothPhone: {
            switch (newHolder) {
            case AS_CarlifeMusic:
            case AS_ECLinkMusic:
            case AS_ECLinkBluetoothMusic:{
                g_Audio->requestAudioSource(newHolder);
                break;
            }
            default: {
                break;
            }
            }
            break;
        }
        case AS_CarplayMusic: {
            if (AS_CarplayPhone != newHolder) {
                if (-1 != d->m_LinkType) {
                    qDebug() <<"++++0000+++++"<< __PRETTY_FUNCTION__ << __LINE__ << d->m_LinkType << newHolder;
                    if (AS_Aux != newHolder && AS_Music != newHolder) {
                        //g_Link->requestLink(Carplay, d->m_LinkMode, DBUS_REQUEST_EXITED);
                        if(g_Link->getDbusConnectStatus() == DBUS_BACKGROUND)
                        {
                            g_Link->requestKey(KEY_MUSIC_PAUSE);
                        }
                    }
                }
            }
            break;
        }
        case AS_AutoMusic: {
            if (AS_AutoPhone != newHolder) {
//                if (-1 != d->m_LinkType) {
//                    if (AS_Aux != newHolder && (AS_BluetoothMusic != newHolder)) {
//                        g_Link->requestLink(Carplay, d->m_LinkMode, DBUS_REQUEST_EXITED);
//                    }
//                }
            }
            break;
        }
        case AS_AutoPhone: {
            if (AS_AutoMusic == newHolder) {
                if (AS_AutoPhone != g_Audio->getAudioSource()) {
                    g_Audio->requestAudioSource(AS_AutoMusic);
                }
            } /*else {
                if (-1 != d->m_LinkType) {
                    g_Link->requestLink(Android_Auto, d->m_LinkMode, DBUS_REQUEST_EXITED);
                }
            }*/
            break;
        }
        case AS_CarlifeMusic: {
//            if (AS_BluetoothPhone != newHolder) {
//                if (-1 != d->m_LinkType) {
//                    if (AS_Aux != newHolder) {
//                        g_Link->requestLink(Carlife, d->m_LinkMode, DBUS_REQUEST_EXITED);
//                    }
//                }
//            }
            break;
        }
        case AS_CarlifePhone: {
            if (AS_CarlifeMusic == newHolder) {
                if (AS_CarlifePhone != g_Audio->getAudioSource()) {
                    g_Audio->requestAudioSource(AS_CarlifeMusic);
                }
            } /*else {
                if (-1 != d->m_LinkType) {
                    g_Link->requestLink(Carlife, d->m_LinkMode, DBUS_REQUEST_EXITED);
                }
            }*/
            break;
        }
        case AS_ECLinkMusic:
        case AS_ECLinkBluetoothMusic: {
//            if ((AS_BluetoothPhone != newHolder)
//                    && (AS_ECLinkMusic != newHolder)
//                    && (AS_ECLinkBluetoothMusic != newHolder)) {
//                if (-1 != d->m_LinkType) {
//                    if (AS_Aux != newHolder) {
//                        g_Link->requestLink(ECLink, d->m_LinkMode, DBUS_REQUEST_EXITED);
//                    }
//                }
//            }
            break;
        }
        default: {
            break;
        }
        }
        break;
    }
    }
}
void PhoneLinkMsgWidget::onTelephone(const int type, const QString name, const QString number)
{
    Q_D(PhoneLinkMsgWidget);
    qDebug()<<"++++++onTelephone::type++++++++++" << type;
    if(d->m_LinkType == Carlife)
    {
        g_Setting->setCarlifeTelephoneStatus(1);//来自于carlife的电话
        if(type == CALL_TYPE_DAIL)
        {
            g_Link->requestLink(d->m_LinkType, d->m_LinkMode, DBUS_REQUEST_BACKGROUND);
            d->m_CallType = 0;
        }
        else if(type == CALL_TYPE_HANG_UP)
        {
            g_Link->requestLink(d->m_LinkType, d->m_LinkMode, DBUS_REQUEST_FOREGROUND);
            d->m_CallType = 1;
        }
    }
}
PhoneLinkMsgWidgetPrivate::PhoneLinkMsgWidgetPrivate(PhoneLinkMsgWidget *parent)
    : q_ptr(parent)
{
    m_PhoneLinkMsgWidgetObject = NULL;
    m_WireBtnObject     = NULL;
    m_WirelessBtnObject = NULL;
    m_LoaderImageObject = NULL;
    m_AnimationObject   = NULL;
    m_LinkType = -1;
    m_LinkMode = -1;
    m_DbusSend = -1;
    m_IsRunningBackGround = false;
    m_Inserted = -1;
    m_PhoneType = -1;
    m_IsConnected = false;
    m_CallType = -1;
    connectAllSlots();
}
PhoneLinkMsgWidgetPrivate::~PhoneLinkMsgWidgetPrivate()
{

}

void PhoneLinkMsgWidgetPrivate::initializeObject()
{
    Q_Q(PhoneLinkMsgWidget);
    if(m_PhoneLinkMsgWidgetObject != NULL)
    {
        if(m_WireBtnObject == NULL)
        {
            m_WireBtnObject = m_PhoneLinkMsgWidgetObject->findChild<QObject*>("wireBtnObject");
        }
        if(m_WirelessBtnObject == NULL)
        {
            m_WirelessBtnObject = m_PhoneLinkMsgWidgetObject->findChild<QObject*>("wirelessBtnObject");
        }
        if(m_LoaderImageObject == NULL)
        {
            m_LoaderImageObject = m_PhoneLinkMsgWidgetObject->findChild<QObject*>("loaderImageObject");
        }
        if(m_AnimationObject == NULL)
        {
            m_AnimationObject = m_LoaderImageObject->findChild<QObject*>("animationObject");
        }
    }
}

void PhoneLinkMsgWidgetPrivate::connectAllSlots()
{
    Q_Q(PhoneLinkMsgWidget);
    QObject::connect(g_Link, SIGNAL(onLinkStatus(int,int,int)),
                     q,   SLOT(onLinkStatus(int,int,int)));

    QObject::connect(g_Link, SIGNAL(onCarLinkVersion(int,QString)),
                     q,   SLOT(onCarLinkVersion(int,QString)));

    QObject::connect(g_Link, SIGNAL(onPhoneType(int,int)),
                     q,   SLOT(onPhoneType(int,int)));

    QObject::connect(g_Link, SIGNAL(onDateTime(int,long long)),
                     q,   SLOT(onDateTime(int,long long)));

    QObject::connect(g_Link, SIGNAL(onTelephone(int, QString, QString)),
                     q,   SLOT(onTelephone(int, QString, QString)));
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onHolderChange(const int, const int)));
}

void PhoneLinkMsgWidgetPrivate::waitForQuit()
{
    QTime time;
    time.start();
    while (time.elapsed() < 3000) {
        qApp->processEvents();
        if (DBUS_REQUEST_EXITED == m_DbusSend) {
            qDebug() << "wait status break";
            break;
        }
    }
    if (time.elapsed() >= 3000) {
        qDebug() << "wait for quit timeout!";
    }
}
