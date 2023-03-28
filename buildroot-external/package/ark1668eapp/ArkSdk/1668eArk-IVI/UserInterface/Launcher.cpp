#include "Launcher.h"
#include "AutoConnect.h"
#include "./UserInterface/MainWidget/MainWidget.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/HostApd.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Setting.h"
#include <QDebug>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <QSettings>
#include <QFile>
static const QString BTconfig("/data/Blutooth.ini");
static const QString PhoneLinkConfig("/data/PhoneLink.ini");
static const QString BtPowerStatus("/data/BtPowerStatus.ini");
static const QString Wificonfig("/data/Wifi.ini");
static QString getRandomString()
{
    int randValue;
    QString random;
    //设置一个随机数种子
    //srand((unsigned)time(NULL));
    qsrand(QTime(0,0,0).msecsTo(QTime::currentTime()));
    //获取[1000，10000)之间的随机数，保证得到的随机数是4位
    randValue = (qrand() % (10000 - 1000)) + 1000;
    random = QString("%1").arg(randValue);
    qDebug() << "random number:" << random;
    return random;
}
class LauncherPrivate
{
    Q_DISABLE_COPY(LauncherPrivate)
public:
    explicit LauncherPrivate(Launcher* parent);
    ~LauncherPrivate();
    void initMainWidget();
    void initialPlaymodefile();
    void initializeBTconfig();
    void initializePhoneLinkConfig();
public:
    MainWidget* m_MainWidget;
    QObject* m_LauncherObject;
private:
    Q_DECLARE_PUBLIC(Launcher)
    Launcher* const q_ptr;
};

Launcher::Launcher(QObject *parent) :
    QObject(parent)
  , d_ptr(new LauncherPrivate(this))
{

}
void Launcher::setLauncherObject(QObject* qmlObject)
{
    Q_D(Launcher);
    if(d->m_LauncherObject == NULL)
    {
        d->m_LauncherObject = qmlObject;
    }
    d->initializePhoneLinkConfig();
    g_Setting->tmpDirHostapdIsExist();
    d->initMainWidget();
    connectSignalAndSlotByNamesake(g_Setting, this, ARKRECEIVER(onLoaderCompleted()));
}

void Launcher::onLoaderCompleted()
{
    Q_D(Launcher);
    if(d->m_LauncherObject != NULL)
    {
        d->initializeBTconfig();
        g_Bluetooth->Blutooth_startThread();
        QFile _BtPowerStatusFile(BtPowerStatus);
        if(_BtPowerStatusFile.exists()== true)
        {
            QSettings* BtPowerStatusSetFile = new QSettings(BtPowerStatus,QSettings::IniFormat);
            int _BtPowerStatus  = BtPowerStatusSetFile->value("powerStatus").toString().toInt();
            if(_BtPowerStatus == 1)
            {
                g_Bluetooth->powerOn();
            }
            delete BtPowerStatusSetFile;
        }
        QFile _PhoneLinkcfgFile(PhoneLinkConfig);
        int _PhoneLinkType = 0;
        if(_PhoneLinkcfgFile.exists())
        {
            QSettings* PhoneLinkcfgSetFile = new QSettings(PhoneLinkConfig,QSettings::IniFormat);
            _PhoneLinkType  = PhoneLinkcfgSetFile->value("PhoneLink").toString().toInt();
            delete PhoneLinkcfgSetFile;
        }
        if(_PhoneLinkType == 3)
        {
            int _InitHostapdStatus = g_HostApd->getInitHostapdStatus();
            if(_InitHostapdStatus == 1)
            {
                g_HostApd->CreatThreadInitHostApd();
            }
            g_HostApd->restartHostApd();
            QString ssid = g_Setting->getHostapdSsid();
            g_Link->requestWifi(ssid.toStdString(), "12345678", "36");
            if(g_Link->getHicarInitStatus())
            {
                g_Link->setHicarInitStatus(false);
                g_Link->requestLink(HiCar, Wireless, DBUS_REQUEST_CONNECT);
            }
        }else{
            QFile _WificfgFile(Wificonfig);
            if(_WificfgFile.exists())
            {
                QSettings* WificfgSetFile = new QSettings(Wificonfig,QSettings::IniFormat);
                int _WifiStauts  = WificfgSetFile->value("WifiStatus").toString().toInt();
                delete WificfgSetFile;
                if(_WifiStauts == 1)
                {
                    emit g_Setting->onWlanOpen();
                }
            }
        }
        g_Setting->executeShellCmd("echo otg > /sys/devices/platform/soc/e0100000.usb/musb-hdrc.0/mode");
        g_Audio->requestAudioSource(AS_Idle);
    }
}
LauncherPrivate::LauncherPrivate(Launcher *parent)
    : q_ptr(parent)
{
    m_MainWidget = NULL;
    m_LauncherObject = NULL;
    initialPlaymodefile();
}

LauncherPrivate::~LauncherPrivate()
{


}
void LauncherPrivate::initialPlaymodefile()
{
    if(access("/etc/playmode",F_OK) != 0)
    {
        g_Setting->executeShellCmd("touch /etc/playmode");
        g_Setting->executeShellCmd("chmod 777 /etc/playmode");
    }else{
        qDebug()<<Q_FUNC_INFO<<"/etc/playmode is existed";
    }
    if(access("/etc/videoplaymode",F_OK) != 0)
    {
        g_Setting->executeShellCmd("touch /etc/videoplaymode");
        g_Setting->executeShellCmd("chmod 777 /etc/videoplaymode");
    }else{
        qDebug()<<Q_FUNC_INFO<<"/etc/videoplaymode is existed";
    }
    QString path = QString("/data/MultiMediaFile");
    QDir dir(path);
    if(!dir.exists()){
        mkdir("/data/MultiMediaFile",0777);
    }

}
void LauncherPrivate::initializeBTconfig()
{
    QFile BTcfgFile(BTconfig);
    QSettings *BTcfgsetFile = new QSettings(BTconfig,QSettings::IniFormat);
    if(!BTcfgFile.exists())
    {
        qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"BTconfigfile is not exist, creating...";
        g_Setting->executeShellCmd(QString(QString("touch ")+ BTconfig).toLocal8Bit().constData());
        g_Setting->executeShellCmd("sync");
        QString random = getRandomString();
        QString deviceName = QString("CARKIT_")+random;
        BTcfgsetFile->beginGroup("General");
        BTcfgsetFile->setValue("DeviceName", deviceName);
        BTcfgsetFile->setValue("PinCode", QString("0000"));
        BTcfgsetFile->endGroup();
        BTcfgsetFile->sync();
        g_Setting->executeShellCmd("sync");
    }
    g_Setting->mySleep(10);
    g_Bluetooth->setLocalDeviceName(BTcfgsetFile->value("General/DeviceName").toString());
    g_Bluetooth->setPincode(BTcfgsetFile->value("General/PinCode").toString());
    delete BTcfgsetFile;
}

void LauncherPrivate::initializePhoneLinkConfig(){
    QFile PhoneLinkcfgFile(PhoneLinkConfig);
    QSettings *PhoneLinkcfgsetFile = new QSettings(PhoneLinkConfig,QSettings::IniFormat);
    if(!PhoneLinkcfgFile.exists())
    {
        qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"PhoneLinkcfgFile is not exist, creating...";
        g_Setting->executeShellCmd(QString(QString("touch ")+ PhoneLinkConfig).toLocal8Bit().constData());
        g_Setting->executeShellCmd("sync");
        PhoneLinkcfgsetFile->setValue("PhoneLink", QString("0"));
        PhoneLinkcfgsetFile->sync();
        g_Setting->executeShellCmd("sync");
    }
    delete PhoneLinkcfgsetFile;
}
void LauncherPrivate::initMainWidget()
{
    Q_Q(Launcher);
    if(m_MainWidget == NULL)
    {
        m_MainWidget = new MainWidget(q);
        QObject* mainWidgetObject = m_LauncherObject->findChild<QObject*>("mainWidgetObject");
        m_MainWidget->setMainWidgetObject(mainWidgetObject);
    }
}
