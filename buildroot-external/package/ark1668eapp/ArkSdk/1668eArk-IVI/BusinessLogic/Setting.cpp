#include "Setting.h"
#include "BusinessLogic/ark_api.h"
#include "ArkApplication.h"
#include <QDebug>
#include <QTranslator>
#include <QFile>
static const QString LanguageConfig("/data/Language.ini");
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
class SettingPrivate
{
    Q_DISABLE_COPY(SettingPrivate)
public:
    explicit SettingPrivate(Setting* parent);
    ~SettingPrivate();
public:
    int m_MediaVolume;
    int m_NavigationVolume;
    int m_TelPhoneVolume;
    int m_VolumeType;
    int m_WifiOpenStatus;
    int m_WlanApStatus;
    int m_CarlifeTelephoneStatus;
    int m_LoaderInterfaceCount;
    QList<QString> m_SsidList;//wifi名称
private:
    Q_DECLARE_PUBLIC(Setting)
    Setting* const q_ptr;
};


Setting::Setting(QObject *parent)
    : QObject(parent),
      d_ptr(new SettingPrivate(this))
{

}

Setting::~Setting()
{

}
void Setting::setDisplayMode(int mode){
    arkapi_video_set_display_mode(mode);
}
void Setting::clearSsidList(){
    Q_D(Setting);
    d->m_SsidList.clear();
}

int Setting::wifiSsidListSize(){
    Q_D(Setting);
    return d->m_SsidList.size();
}

void Setting::appendSsidWifiSsidList(QString ssid){
    Q_D(Setting);
    d->m_SsidList.append(ssid);
}
QList<QString> Setting::getWifiSsidList(){
    Q_D(Setting);
    return d->m_SsidList;
}
void Setting::settingDataTime(){
    emit onDataTimeSetting();
}

void Setting::setMediaVolume(int value){
    Q_D(Setting);
    d->m_MediaVolume = value;
    return;
}
void Setting::setNavigationVolume(int value){
    Q_D(Setting);
    d->m_NavigationVolume = value;
    return;
}
void Setting::setTelPhoneVolume(int value){
    Q_D(Setting);
    d->m_TelPhoneVolume  = value;
    return;
}
void Setting::setVolumeType(int type)
{
    Q_D(Setting);
    d->m_VolumeType  = type;
    return;
}
int  Setting::getVolumeType(){
    Q_D(Setting);
    return d->m_VolumeType;
}
int Setting::getMediaVolume(){
    Q_D(Setting);
    return d->m_MediaVolume;
}
int Setting::getNavigationVolume(){
    Q_D(Setting);
    return d->m_NavigationVolume;
}

int  Setting::getTelPhoneVolume(){
    Q_D(Setting);
    return d->m_TelPhoneVolume;
}

void Setting::setLanguage(const LanguageType language)
{
    QString languagePath;
    languagePath.clear();
    switch (language) {
        case LT_Chinese: {
            languagePath = QString(":/Languages/zh_tr.qm");
            break;
        }
        case LT_TChinese:{
            languagePath = QString(":/Languages/Tzh_tr.qm");
            break;
        }
        case LT_English:
        default: {
            languagePath = QString(":/Languages/en_tr.qm");
            break;
        }
    }
    ArkApp->installTranslatorPath(languagePath);
    onLanguageChanged();
    QFile _LanguagecfgFile(LanguageConfig);
    QSettings *LanguagecfgsetFile = new QSettings(LanguageConfig,QSettings::IniFormat);
    if(!_LanguagecfgFile.exists())
    {
        qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"_LanguagecfgFile is not exist, creating...";
        executeShellCmd(QString((QString("touch ")+ LanguageConfig)).toLocal8Bit().constData());
        executeShellCmd("sync");
    }
    LanguagecfgsetFile->setValue("Language", QString::number(language));
    LanguagecfgsetFile->sync();
    executeShellCmd("sync");
    delete LanguagecfgsetFile;
}

void Setting::mySleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while(QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

void Setting::tmpDirHostapdIsExist()
{
    QString fileNmae = "/tmp/hostapd.conf";
    QFile file(fileNmae);
    if(!file.exists())
    {
        executeShellCmd(QString((QString("touch ")+ fileNmae)).toLocal8Bit().constData());
        QString strAll;
        QStringList strList;
        QFile readFile("/etc/hostapd/hostapd.conf");
        if(readFile.open((QIODevice::ReadOnly|QIODevice::Text)))
        {
            QTextStream stream(&readFile);
            strAll = stream.readAll();
        }
        readFile.close();
        if(file.open(QIODevice::WriteOnly|QIODevice::Text))
        {
            QTextStream stream(&file);
            if(strAll.size() > 1)
            {
                strList=strAll.split("\n");
                for(int i=0;i<strList.size();i++)
                {
                    if(i == strList.size()-1)
                    {
                        //最后一行不需要换行
                        stream<<strList.at(i);
                        break;
                    }
                    else
                    {
                        if(!QString(strList.at(i)).contains("ssid="))
                        {
                            stream<<strList.at(i)<<'\n';
                        }
                    }
                    if(QString(strList.at(i)).contains("ssid="))
                    {
                         QString tempStr  = strList.at(i);
                         QString random   = getRandomString();
                         QString wifiName = QString("ssid=ark1668e_wifi_") + random;
                         tempStr.replace(0,tempStr.length()+1,wifiName);
                         stream<<tempStr<<'\n';
                    }
                }
            }
        }
    }
    file.close();
}

QString Setting::getHostapdSsid(){
    QString strAll;
    QStringList strList;
    QFile readFile("/tmp/hostapd.conf");
    if(readFile.open((QIODevice::ReadOnly|QIODevice::Text)))
    {
        QTextStream stream(&readFile);
        strAll = stream.readAll();
    }
    readFile.close();
    strList=strAll.split("\n");
    QString ssid="";
    if(strList.size() > 1)
    {
        for(int i=0;i<strList.size();i++)
        {
            if(QString(strList.at(i)).contains("ssid="))
            {
                if(QString(strList.at(i)).size() > 5)
                {
                    ssid = QString(strList.at(i)).right(QString(strList.at(i)).size()-5);
                }
            }
        }
    }
    qDebug()<<"+++++++++getHostapdSsid end++++++++"<<ssid;
    return ssid;
}

int Setting::executeShellCmd(const char* cmd)
{
    FILE *fp;
    char cmdresult[256]={0};
    if((fp = popen(cmd,"r"))== NULL)
    {
        perror("why");
        printf("+++++++executeShellCmd:error++++++===\n");
        return -1;
    }
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("+++++executeShellCmd-cmdresult0000:%s+++++++++\n",cmdresult);
    }
    pclose(fp);
    return 0;
}
int Setting::backstageExecuteShellCmd(const char* cmd)
{
    FILE *fp;
    fp = popen(cmd,"r");
    if(fp == NULL)
    {
        perror("why");
        pclose(fp);
        return -1;
    }
     pclose(fp);
     return 0;
}
void Setting::setWlanApStatus(int status)
{
    Q_D(Setting);
    d->m_WlanApStatus = status;
}
int Setting::getWlanApStatus()
{
    Q_D(Setting);
    return d->m_WlanApStatus;
}

void Setting::setWifiOpenStatus(int status)
{
    Q_D(Setting);
    d->m_WifiOpenStatus = status;
}
int Setting::getwifiOpenStatus()
{
    Q_D(Setting);
    return d->m_WifiOpenStatus;
}

void Setting::setWifiConfig(int value)
{
    QFile BTcfgFile(Wificonfig);
    QSettings *BTcfgsetFile = new QSettings(Wificonfig,QSettings::IniFormat);
    if(!BTcfgFile.exists())
    {
        qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"Wificonfig is not exist, creating...";
        executeShellCmd(QString((QString("touch ")+ Wificonfig)).toLocal8Bit().constData());
        executeShellCmd("sync");
    }
    BTcfgsetFile->setValue("WifiStatus", QString::number(value));
    BTcfgsetFile->sync();
    executeShellCmd("sync");
    delete BTcfgsetFile;
}

void Setting::setCarlifeTelephoneStatus(int status)
{
    Q_D(Setting);
    d->m_CarlifeTelephoneStatus = status;
}
int Setting::getCarlifeTelephoneStatus()
{
    Q_D(Setting);
    return d->m_CarlifeTelephoneStatus;
}
void Setting::setLoaderInterfaceCount()
{
    Q_D(Setting);
    d->m_LoaderInterfaceCount++;
}
void Setting::loaderInterfaceCompleted()
{
    Q_D(Setting);
    if(d->m_LoaderInterfaceCount == LOADERCOMPLETED)
    {
        emit onLoaderCompleted();
    }
}

SettingPrivate::SettingPrivate(Setting *parent)
    : q_ptr(parent)
{
    m_MediaVolume = 20;
    m_NavigationVolume = 20;
    m_TelPhoneVolume = 20;
    m_VolumeType = V_MediaVolume;
    m_WifiOpenStatus = 0;
    m_WlanApStatus   = 0;
    m_CarlifeTelephoneStatus = 0;
    m_LoaderInterfaceCount = 0;
}

SettingPrivate::~SettingPrivate()
{

}


