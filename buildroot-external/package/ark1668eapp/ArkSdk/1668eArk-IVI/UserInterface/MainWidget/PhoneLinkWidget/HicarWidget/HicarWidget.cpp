#include "HicarWidget.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "BusinessLogic/HostApd.h"
#include "BusinessLogic/WiFiManager.h"
#include "BusinessLogic/QmlWidget.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <stdio.h>
#include <QDebug>
#include <unistd.h>
#include <QTimer>
#include <QFile>
class HicarWidgetPrivate
{
    Q_DISABLE_COPY(HicarWidgetPrivate)
public:
    explicit HicarWidgetPrivate(HicarWidget* parent);
    ~HicarWidgetPrivate();
    void connectAllSlots();
    void linkChangeBTHandler(const int type, const bool bBT);
    void initializeTimer();
    void saveConfigString(const char* data,QString filename);
    void saveHicarConnectedMac();
    void waitForQuit();
public:
    QObject* m_HicarWidgetObject;
    QObject* m_ParendObject;
    QObject* m_HicarBtnObject;
    QObject* m_PinCodeObject;
    QObject* m_LoaderImageObject;
    QObject* m_AnimationObject;
    int m_LinkType;
    int m_LinkMode;
    int m_DbusSend;
    int m_Inserted;
    bool m_IsRunningBackGround;
    bool m_IsConnected ;
    QTimer* m_Timer;
private:
    Q_DECLARE_PUBLIC(HicarWidget)
    HicarWidget* const q_ptr;
};

HicarWidget::HicarWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new HicarWidgetPrivate(this))
{

}
void HicarWidget::setHicarWidgetObject(QObject* qmlObject){
    Q_D(HicarWidget);
    if(d->m_HicarWidgetObject == NULL)
    {
        d->m_HicarWidgetObject = qmlObject;
    }
    if(d->m_HicarBtnObject == NULL)
    {
        d->m_HicarBtnObject = d->m_HicarWidgetObject->findChild<QObject*>("hicarBtnObject");
    }
    if(d->m_PinCodeObject == NULL)
    {
        d->m_PinCodeObject = d->m_HicarWidgetObject->findChild<QObject*>("pinCodeObject");
    }
    if(d->m_LoaderImageObject == NULL)
    {
        d->m_LoaderImageObject = d->m_HicarWidgetObject->findChild<QObject*>("loaderImageObject");
    }
    if(d->m_AnimationObject == NULL)
    {
        d->m_AnimationObject = d->m_LoaderImageObject->findChild<QObject*>("animationObject");
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_HicarBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void HicarWidget::setHicarWidgetParentObject(QObject* qmlObject){
    Q_D(HicarWidget);
    if(d->m_ParendObject == NULL)
    {
        d->m_ParendObject = qmlObject;
    }
}
void HicarWidget::onCarLinkInitDone(const int type){
    //qDebug()<<"+++++++onCarLinkInitDone000++++++"<<type;
    if(type == HiCar)
    {
        QString fileName("/data/hicarConnectedMac");
        QFile file(fileName);
        QString _RemoteBtAddress = g_Bluetooth->getHicarBackLinkMac();
        qDebug()<<"++++_RemoteBtAddress+++++"<<_RemoteBtAddress;
        bool fileExists(false);
        if(file.exists())
        {
            qDebug()<<"+++++++file is exists++++++++";
            if(!file.open(QIODevice::ReadOnly)){
                qDebug()<<"+++++++file open fail++++++++";
                return ;
            }  // 以读的方式打开文件
            while(!file.atEnd()) // 判断文件是否结束
            {
                // 每读取一行数据，游标seek会自动往下跳，所以当seek到达末尾是atEnd()返回true
                QString data =  file.readLine();
                if(data.left(12) == _RemoteBtAddress){
                    fileExists = true;
                    break;
                }
            }
            file.close();
        }
        else {
            qDebug()<<"+++++++file is not exists++++++++";
        }
        qDebug()<<"++++fileExists+++++"<<fileExists;
        if(fileExists){
             g_Link->requestBluetoothCmd(g_Bluetooth->getHicarBackLinkCmd().toStdString());
        }
    }
}
void HicarWidget::onToolButtonRelease(){
    Q_D(HicarWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr== d->m_HicarBtnObject){
        qDebug()<<"===[HicarWidget::onToolButtonRelease()]==="<<d->m_Inserted;
        if(d->m_Inserted != 0 && d->m_IsConnected == false)
        {
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
            qDebug()<<"++++++_InitHostapdStatusHicar++++++++++++"<<_InitHostapdStatus;
            if(_InitHostapdStatus == 1)
            {
                g_HostApd->CreatThreadInitHostApd();
            }
            for(int i= 0;i<50;i++)
            {
                usleep(100*1000);
                _InitHostapdStatus = g_HostApd->getInitHostapdStatus();
                if(_InitHostapdStatus == 0)
                {
                    qDebug()<<"--------_InitHostapdStatus---------"<<i;
                    g_HostApd->restartHostApd();
                    QString ssid = g_Setting->getHostapdSsid();
                    g_Link->requestWifi(ssid.toStdString(), "12345678", "36");
                    qDebug()<<"======g_Bluetooth->getRemoteBtAddress().toStdString()======="<<g_Bluetooth->getRemoteBtAddress();
                    //AC1F74DB5BCA
                    QString _BtAddress;
                    QString _RemoteBtAddress = g_Bluetooth->getLocalMacAddress();
                    if(_RemoteBtAddress.size() == 12)
                    {
                        _BtAddress = _RemoteBtAddress.mid(0,2) +(QString(":")) + _RemoteBtAddress.mid(2,2) +(QString(":"))+
                                     _RemoteBtAddress.mid(4,2) +(QString(":")) + _RemoteBtAddress.mid(6,2) +(QString(":"))+
                                     _RemoteBtAddress.mid(8,2) +(QString(":")) + _RemoteBtAddress.mid(10,2);

                    }
                    qDebug()<<"======_BtAddress=======" << _BtAddress;
                    QFile addressfile(QString("/data/bluetoothaddress"));
                    if (addressfile.open(QFile::WriteOnly)) {
                        QString localAddress = _BtAddress;
                        addressfile.write(localAddress.toLocal8Bit().data(), localAddress.length());
                        addressfile.flush();
                        addressfile.close();
                    }
                    if(g_Link->getHicarInitStatus())
                    {
                        g_Link->setHicarInitStatus(false);
                        g_Link->requestCarBluetooth(QString("").toStdString(),_BtAddress.toStdString(),QString("").toStdString());
                        g_Link->requestLink(HiCar, Wireless, DBUS_REQUEST_CONNECT);
                    }
                    //qDebug()<<"======xxx1111lfp=======";
                    g_Link->requestBroadcast(true);
                    g_Widget->setPhoneLinkStatus(1);
                    //qDebug()<<"======xxx0000lfp=======";
                    QQmlProperty(d->m_LoaderImageObject,"visible").write(true);
                    QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(false);
                    QQmlProperty(d->m_AnimationObject,"running").write(true);
                    break;
                }
            }
        }
    }
}
void HicarWidget::onLinkStatus(int type, int mode, int status)
{
    Q_D(HicarWidget);
    printf("HicarWidget::onLinkStatus :type = %d, mode =%d, status = %d\n", type, mode, status);
    static bool _InitHicarNAVISOUND(true);
    if(status == DBUS_CONNECTED){
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(2);
        d->m_DbusSend = status;
        d->m_LinkType = type;
        d->m_LinkMode = mode;
        QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(0);
        d->linkChangeBTHandler(HiCar,true);
        g_Widget->setPreemptiveWidget(1);
        g_Link->setLinkConnectStatus(status);
        g_Link->setLinkMode(mode);
        g_Setting->setDisplayMode(DISP_PHONELINK);
        QQmlProperty(d->m_LoaderImageObject,"visible").write(false);
        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(true);
        QQmlProperty(d->m_AnimationObject,"running").write(false);
        d->initializeTimer();
        if(d->m_Timer->isActive())
            d->m_Timer->stop();
        d->saveHicarConnectedMac();
        d->m_IsConnected = false;
    }
    else if(status == DBUS_CONNECTTING){
        d->initializeTimer();
        d->m_Timer->start();
    }
    else if(status == DBUS_FOREGROUND){
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(2);
        d->m_IsRunningBackGround = false;
        d->linkChangeBTHandler(HiCar,true);
        g_Widget->setPreemptiveWidget(1);
        g_Link->setLinkConnectStatus(status);
        g_Setting->setDisplayMode(DISP_PHONELINK);
    }
    else if(status == DBUS_BACKGROUND){
        g_Widget->setPreemptiveWidget(0);
        g_Link->setLinkConnectStatus(0);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
        d->m_IsRunningBackGround = true;
    }
    else if(status ==DBUS_MUSIC_STARTED){
        d->linkChangeBTHandler(HiCar,true);
    }
    else if(status == DBUS_NAVI_SOUND_STARTED){
        qDebug()<<"==========DBUS_NAVI_SOUND_STARTED============";
        if(_InitHicarNAVISOUND)
        {
            _InitHicarNAVISOUND = false;
            g_Setting->setVolumeType(V_NavigationVolume);
            g_Audio->requestSetVolume(g_Setting->getNavigationVolume());
            g_Audio->open_amixer_mode(AUDIO_PHONE_MUSIC,CTRL_PHONE_MUSIC);
            g_Audio->set_amixersoftmaster_volume(90);//lower volume
        }
    }
    else if(status == DBUS_NAVI_SOUND_STOPPED){
        qDebug()<<"==========DBUS_NAVI_SOUND_STOPPED============";
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
        g_Link->setLinkConnectStatus(0);
        QQmlProperty(d->m_ParendObject,"msgWidgetVisible").write(0);
        QQmlProperty(d->m_ParendObject,"phoneLinkStatus").write(g_Widget->getPhoneLinkStatus());
        QQmlProperty(d->m_LoaderImageObject,"visible").write(false);
        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(true);
        QQmlProperty(d->m_AnimationObject,"running").write(false);
        d->m_IsConnected = false;
    }
}
void HicarWidget::onTimeout(){
    Q_D(HicarWidget);
    QTimer* ptr = static_cast<QTimer*>(sender());
    if(ptr == d->m_Timer)
    {
        if(d->m_Timer->isActive())
            d->m_Timer->stop();
        QQmlProperty(d->m_LoaderImageObject,"visible").write(false);
        QQmlProperty(d->m_LoaderImageObject,"scanFinish").write(true);
        QQmlProperty(d->m_AnimationObject,"running").write(false);
    }
}
void HicarWidget::onPinCode(const int type, const QString pincode){
    Q_D(HicarWidget);
//    qDebug()<<"=====onPinCode==type======"<<type;
//    qDebug()<<"=======pincode======"<<pincode;
    if(type == HiCar)
    {
        QQmlProperty(d->m_PinCodeObject,"text").write(QString("Huawei HiCar 连接码:")+pincode);
    }
}
void HicarWidget::onPhoneType(int type, int inserted)
{
    Q_D(HicarWidget);
    qDebug()<<"++++++inserted++++++++"<<inserted;
    if(inserted == 0)
    {
        g_Link->setLinkType(HiCar);
        g_Widget->setPhoneLinkStatus(0);
        g_Link->requestLink(HiCar, Wired, DBUS_REQUEST_CONNECT);
        d->m_Inserted = DBUS_DEVICE_ATTACHED;
    }
    else if(inserted == 1){
        d->m_Inserted = DBUS_DEVICE_DEATTACHED;
    }
}
void HicarWidget::onBlueToothCmd(const int type, const QString cmd){
    Q_D(HicarWidget);
    qDebug()<<"=======cmd======"<<cmd;
    if(type == HiCar){
        QStringList _CmdList = cmd.split("#");
        QString _CmdAt = QString(_CmdList.at(1)).left(2);
        int size       = QString(_CmdList.at(1)).size();
        QString _Data  = QString(_CmdList.at(1)).right(size-2);
        _Data = _Data.left(_Data.size()-2);
        g_Bluetooth->sendHicarDataToblueware(_CmdAt,_Data);
    }
}
void HicarWidget::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder)
{
    Q_D(HicarWidget);
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << oldHolder << newHolder;
    switch (newHolder) {
    case AS_Video: {
        if (-1 != d->m_DbusSend ) {
            g_Link->requestLink(d->m_LinkType, d->m_LinkMode, DBUS_REQUEST_EXITED);
            switch (oldHolder) {
            case AS_HiCarMusic:
            case AS_HiCarBluetoothMusic:{
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
    default: {
        switch (oldHolder) {
        case AS_Aux: {
            switch (newHolder) {
            case AS_HiCarMusic:
            case AS_HiCarBluetoothMusic:{
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
            case AS_HiCarMusic:
            case AS_HiCarBluetoothMusic:{
                g_Audio->requestAudioSource(newHolder);
                break;
            }
            default: {
                break;
            }
            }
            break;
        }
        case AS_HiCarMusic:
        case AS_HiCarBluetoothMusic: {
            if ((AS_BluetoothPhone != newHolder)
                    && (AS_HiCarMusic != newHolder)
                    && (AS_HiCarBluetoothMusic != newHolder)
                    && (AS_BluetoothMusic != newHolder)) {
                if (-1 != d->m_LinkType) {
                    if (AS_Aux != newHolder) {
                        g_Link->requestLink(HiCar, d->m_LinkMode, DBUS_REQUEST_EXITED);
                    }
                }
            }
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
HicarWidgetPrivate::HicarWidgetPrivate(HicarWidget *parent)
    : q_ptr(parent)
{
    m_HicarWidgetObject = NULL;
    m_ParendObject = NULL;
    m_HicarBtnObject = NULL;
    m_PinCodeObject = NULL;
    m_LoaderImageObject = NULL;
    m_AnimationObject = NULL;
    m_Timer = NULL;
    m_LinkType = -1;
    m_LinkMode = -1;
    m_DbusSend = -1;
    m_Inserted = -1;
    m_IsRunningBackGround = false;
    m_IsConnected = false;
    connectAllSlots();
}

HicarWidgetPrivate::~HicarWidgetPrivate()
{

}
void HicarWidgetPrivate::linkChangeBTHandler(const int type, const bool bBT){
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << type << bBT << m_DbusSend;
    if (HiCar == type) {
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
                    && (AS_ECLinkBluetoothMusic != g_Audio->getAudioSource())
                    && (AS_HiCarMusic != g_Audio->getAudioSource())
                    && (AS_HiCarBluetoothMusic != g_Audio->getAudioSource())) {
                g_Audio->faderOut();
            }
            if (AS_HiCarBluetoothMusic != g_Audio->getAudioSource()) {
                g_Audio->requestAudioSource(AS_HiCarBluetoothMusic);
            }
        } else {
            if ((AS_CarplayMusic != g_Audio->getAudioSource())
                    && (AS_CarplayPhone != g_Audio->getAudioSource())
                    && (AS_CarlifeMusic != g_Audio->getAudioSource())
                    && (AS_CarlifePhone != g_Audio->getAudioSource())
                    && (AS_AutoMusic != g_Audio->getAudioSource())
                    && (AS_AutoPhone != g_Audio->getAudioSource())
                    && (AS_ECLinkMusic != g_Audio->getAudioSource())
                    && (AS_ECLinkBluetoothMusic != g_Audio->getAudioSource())
                    && (AS_HiCarMusic != g_Audio->getAudioSource())
                    && (AS_HiCarBluetoothMusic != g_Audio->getAudioSource())) {
                    g_Audio->faderOut();
            }
            if (AS_HiCarMusic != g_Audio->getAudioSource()) {
                g_Audio->requestAudioSource(AS_HiCarMusic);
            }
        }
    }

}
void HicarWidgetPrivate::initializeTimer(){
    Q_Q(HicarWidget);
    if(m_Timer == NULL)
    {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(10000);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer,SIGNAL(timeout()),q,SLOT(onTimeout()));
    }
}

void HicarWidgetPrivate::saveConfigString(const char* data,QString filename)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    file.write(data);
    file.close();
}
void HicarWidgetPrivate::saveHicarConnectedMac(){
    QString fileName("/data/hicarConnectedMac");
    QString _RemoteBtAddress = g_Bluetooth->getRemoteBtAddress() + QString("\n");
    QFile file(fileName);
    bool fileExists(false);
    if(file.exists())
    {
        if(!file.open(QIODevice::ReadOnly)){
             qDebug()<<"======open file fail========";
             return ;
        }  // 以读的方式打开文件
        else{
            qDebug()<<"======open file sucess========";
        }
        while(!file.atEnd()) // 判断文件是否结束
        {
            // 每读取一行数据，游标seek会自动往下跳，所以当seek到达末尾是atEnd()返回true
            QString data =  file.readLine();
            if(data == _RemoteBtAddress){
                fileExists = true;
                break;
            }
        }
        file.close();
    }
    if(!fileExists){
         string _RemoteBtMac = _RemoteBtAddress.toStdString();
         saveConfigString(_RemoteBtMac.c_str(),fileName);
    }
}
void HicarWidgetPrivate::waitForQuit()
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
void HicarWidgetPrivate::connectAllSlots()
{
    Q_Q(HicarWidget);
    QObject::connect(g_Link, SIGNAL(onLinkStatus(int,int,int)),
                     q,   SLOT(onLinkStatus(int,int,int)));

    QObject::connect(g_Link, SIGNAL(onPinCode(const int ,const QString)),
                     q,   SLOT(onPinCode(const int ,const QString)));

    QObject::connect(g_Link, SIGNAL(onBlueToothCmd(const int ,const QString)),
                     q,   SLOT(onBlueToothCmd(const int ,const QString)));

    QObject::connect(g_Link, SIGNAL(onPhoneType(int,int)),
                     q,   SLOT(onPhoneType(int,int)));

    QObject::connect(g_Link, SIGNAL(onCarLinkInitDone(const int)),
                     q,   SLOT(onCarLinkInitDone(const int)));
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onHolderChange(const int, const int)));

}

