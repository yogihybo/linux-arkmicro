#include "Bluetooth.h"
#include "AutoConnect.h"
#include "RunnableThread.h"
#include "Multimedia.h"
#include "UserInterfaceUtility.h"
#include "carlink.h"
#include "Audio.h"
#include <QList>
#include <QString>
#include <QChar>
#include <QMap>
#include <unistd.h>
#include <stdio.h>
#include <QTime>
#include <QTimer>
#include <QFile>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <QTextCodec>
#include "BusinessLogic/Setting.h"
static const QString BTconfig("/data/Blutooth.ini");
static bool IsNotChinese(QString str)
{
    QByteArray ba = str.toLatin1();
    const char *ch = ba.data();
    while (*ch) {
        if((*ch >= 'A' && *ch <= 'Z')||(*ch >= 'a' && *ch <= 'z') || (*ch >= '0' && *ch <= '9')){

        }else {
            return false;
        }
        ch++;
    }
    return true;
}

static bool IsChinese(QString str)
{
    int count = str.count();
    for(int i = 0; i < count; i++){
        QChar ch = str.at(i);
        ushort us = ch.unicode();
        if(us >= 0x4E00 && us <= 0x9FA5){

        }else {
            return false;
        }
    }
    return true;
}

bool In(wchar_t start, wchar_t end, wchar_t code)
{
    if (code >= start && code <= end)
    {
        return true;
    }
    return false;
}

char convert(int n)
{
    if (In(0xB0A1,0xB0C4,n)) return 'a';
    if (In(0XB0C5,0XB2C0,n)) return 'b';
    if (In(0xB2C1,0xB4ED,n)) return 'c';
    if (In(0xB4EE,0xB6E9,n)) return 'd';
    if (In(0xB6EA,0xB7A1,n)) return 'e';
    if (In(0xB7A2,0xB8c0,n)) return 'f';
    if (In(0xB8C1,0xB9FD,n)) return 'g';
    if (In(0xB9FE,0xBBF6,n)) return 'h';
    if (In(0xBBF7,0xBFA5,n)) return 'j';
    if (In(0xBFA6,0xC0AB,n)) return 'k';
    if (In(0xC0AC,0xC2E7,n)) return 'l';
    if (In(0xC2E8,0xC4C2,n)) return 'm';
    if (In(0xC4C3,0xC5B5,n)) return 'n';
    if (In(0xC5B6,0xC5BD,n)) return 'o';
    if (In(0xC5BE,0xC6D9,n)) return 'p';
    if (In(0xC6DA,0xC8BA,n)) return 'q';
    if (In(0xC8BB,0xC8F5,n)) return 'r';
    if (In(0xC8F6,0xCBF0,n)) return 's';
    if (In(0xCBFA,0xCDD9,n)) return 't';
    if (In(0xCDDA,0xCEF3,n)) return 'w';
    if (In(0xCEF4,0xD188,n)) return 'x';
    if (In(0xD1B9,0xD4D0,n)) return 'y';
    if (In(0xD4D1,0xD7F9,n)) return 'z';
    return '\0';
}
class BluetoothPrivate
{
public:
    explicit BluetoothPrivate(Bluetooth* parent);
    ~BluetoothPrivate();
    void connectAllSlots();
    void cancelSync();
    void syncPhoneBook();
    void syncAllCallLog();
    void syncInComingCallLog();
    void syncOutComingCallLog();
    void syncMissedCallLog();
    void initializeAutoCancelTimer();
    void getBwSerialData(std::string data);
    void getAddrMac();
    QString getChineseSpell(QString src);
public:
    std::ifstream readStream;
private:
    Bluetooth* const q_ptr;
    Q_DECLARE_PUBLIC(Bluetooth)
    QString m_Pincode;
    QString m_LocalDeviceName;
    QString m_RemoteDeviceName;
    QString m_RemoteBtAddress;
    QString m_LocalAddress;
    int     m_BluetoothConnectStatus;
    int     m_LastBluetoothConnectStatus;
    QTimer* m_AutoCancelTimer;
    BluetoothVoiceMode m_BluetoothVoiceMode;
    BluetoothPairedMode m_BluetoothPairedMode;
    int  m_BluetoothPowerStatus;
    int  m_BluetoothAutoConnect;
    int  m_BluetoothAutoAnswer;
    QString m_LastNumber;
    QList<struct PhoneBookInfo> m_PhoneBookList;
    QList<struct CallLogInfo> m_AllCallList;
    QList<struct RemoteDeviceInfo>m_RemoteDeviceInfoList;
    QMap<QString, QString> m_PairedMap;
    int  m_SyncType;
    bool m_Unmute;
    int  m_index;
    int  m_HFPCFGBit;
    int  m_A2DPStatus;
    int  m_BluetoothPlayerStatus;
    int  m_BtMicMuteStatus;
    int  m_BtFirstEnterStatus;
    int  m_LastBtStatus;
    int  m_MusicType;
    QString m_BtMusicTitle;
    QString m_BtMusicArtist;
    QString m_BtMusicAlbum;
    QString m_HicarBackLinkMac;
    QString m_HicarBackLinkCmd;
    QString m_LastOnCallingNum;
};
Bluetooth::Bluetooth(QObject *parent)
    : QObject(parent)
    , d_ptr(new BluetoothPrivate(this))
{

}

Bluetooth::~Bluetooth()
{
    if (NULL != d_ptr) {
        delete d_ptr;
    }
}

QString Bluetooth::pinCode()
{
    Q_D(Bluetooth);
    QFile BTcfgFile(BTconfig);
    if(BTcfgFile.exists()){
        QSettings * BTcfgSetFile = new QSettings(BTconfig,QSettings::IniFormat);
        d->m_Pincode = BTcfgSetFile->value("General/PinCode").toString();
        delete BTcfgSetFile;
        qDebug() << __PRETTY_FUNCTION__ << d->m_Pincode;
    }
    return d->m_Pincode;
}

QString Bluetooth::localDeviceName()
{
    Q_D(Bluetooth);
    QSettings * BTcfgSetFile = new QSettings(BTconfig,QSettings::IniFormat);
    d->m_LocalDeviceName = BTcfgSetFile->value("General/DeviceName").toString();
    delete BTcfgSetFile;
    qDebug() << __PRETTY_FUNCTION__ << d->m_LocalDeviceName;
    return d->m_LocalDeviceName;
}

QString Bluetooth::remoteDeviceName()
{
    Q_D(Bluetooth);
    return d->m_RemoteDeviceName;
}

QString Bluetooth::remoteDeviceAddress()
{
    Q_D(Bluetooth);
}

int Bluetooth::connectStatus()
{
    Q_D(Bluetooth);
    return d->m_BluetoothConnectStatus;
}

const QList<PhoneBookInfo> &Bluetooth::getRecordList()
{
    Q_D(Bluetooth);
    return d->m_PhoneBookList;
}
QList<struct CallLogInfo>& Bluetooth::getCallLogInfo(){
    Q_D(Bluetooth);
    return d->m_AllCallList;
}

const QMap<QString, QString> &Bluetooth::getPairedList()
{
    Q_D(Bluetooth);
    return d->m_PairedMap;
}

void Bluetooth::musicToggle()
{
    if (BPS_PowerOn != getPowerStatus()) {
        return;
    }
    Q_D(Bluetooth);
    QString type;
    qDebug()<<__PRETTY_FUNCTION__ <<"LINE:"<<__LINE__ <<"m_BluetoothPlayerStatus ="<<d->m_BluetoothPlayerStatus;
    if (Bluetooth::BtMusic_Playing == d->m_BluetoothPlayerStatus) {
        type = QString("PAUSE");// ABT_MusicPause;
    } else{
        type = QString("PLAY");//ABT_MusicPlay;
    }
    QString CMD = type;
    Send_AT_CMD(CMD);
}

void Bluetooth::musicPause()
{
    if (BPS_PowerOn != getPowerStatus()) {
        return;
    }
    QString CMD = "PAUSE" ;
    Send_AT_CMD(CMD);
}
void Bluetooth::musicStop()
{
    if (BPS_PowerOn != getPowerStatus()) {
        return;
    }
    QString CMD = "STOP" ;
    Send_AT_CMD(CMD);
}

void Bluetooth::musicPlay()
{
    if (BPS_PowerOn != getPowerStatus()) {
        return;
    }
    QString CMD = "PLAY";
    Send_AT_CMD(CMD);
}

void Bluetooth::musicPrevious()
{
    if (BPS_PowerOn != getPowerStatus()) {
        return;
    }
    QString CMD = "FORWARD" ;
    Send_AT_CMD(CMD);
}

void Bluetooth::musicNext()
{
    if (BPS_PowerOn != getPowerStatus()) {
        return;
    }
    QString CMD = "BACKWARD" ;
    Send_AT_CMD(CMD);
}

void Bluetooth::disconnectRemoteDevice()
{
    if (BPS_PowerOn != getPowerStatus()) {
        return;
    }
    Q_D(Bluetooth);
    QString AT_CMD = QString("DSCA");
    Send_AT_CMD(AT_CMD);
//    d->m_BluetoothConnectStatus = BCS_Idle;
//    emit onConnectStatusChange(d->m_BluetoothConnectStatus);

}

void Bluetooth::deleteRemoteDevice(const unsigned short index)
{
    Q_D(Bluetooth);
    QString AT_CMD = QString("PLIST=") + QString::number(index);
    Send_AT_CMD(AT_CMD);
}

int Bluetooth::getSynchronizeType()
{
    Q_D(Bluetooth);
    return d->m_SyncType;
}

void Bluetooth::cancelSynchronize(BluetoothRecordType type)
{
    Q_D(Bluetooth);
    if (d->m_SyncType == type) {
        d->cancelSync();
    }
}

void Bluetooth::synchronizePhoneBook()
{
   // g_Radio->setRadioIdle();
    Q_D(Bluetooth);
    d->cancelSync();
    d->m_PhoneBookList.clear();
    d->syncPhoneBook();
    d->initializeAutoCancelTimer();
    d->m_AutoCancelTimer->start();
}
void Bluetooth::synchronizeAllCallLog(){
    Q_D(Bluetooth);
    d->cancelSync();
    d->m_AllCallList.clear();
    d->syncInComingCallLog();
    d->initializeAutoCancelTimer();
    d->m_AutoCancelTimer->start();
}
void Bluetooth::setPincode(const QString &pincode)
{
    Q_D(Bluetooth);
    QString PINCODE = QString("PIN=") + pincode;
    Send_AT_CMD(PINCODE);
    d->m_Pincode = pincode;
    emit onPinCodeChange(pincode);
    QFile BTcfgFile(BTconfig);
    if(BTcfgFile.exists()){
        QSettings *BTcfgSetFile = new QSettings(BTconfig,QSettings::IniFormat);
        BTcfgSetFile->beginGroup("General");
        BTcfgSetFile->setValue("PinCode", pincode);
        BTcfgSetFile->endGroup();
        BTcfgSetFile->sync();
        g_Setting->executeShellCmd("sync");
        delete BTcfgSetFile;
    }
}

void Bluetooth::setLocalDeviceName(const QString &devicename)
{
    Q_D(Bluetooth);
    QString DEVNAME = QString("NAME=") + devicename;
    Send_AT_CMD(DEVNAME);
    d->m_LocalDeviceName = devicename;
    emit onLocalDeviceNameChange(devicename);
    QSettings *BTcfgFile = new QSettings(BTconfig,QSettings::IniFormat);
    BTcfgFile->beginGroup("General");
    BTcfgFile->setValue("DeviceName", devicename);
    BTcfgFile->endGroup();
    BTcfgFile->sync();
    g_Setting->executeShellCmd("sync");
    delete BTcfgFile;
}

void Bluetooth::dialPhone(const QString &phone)
{
    QString CALLPHONE = QString("HFPDIAL=") + phone;
    Send_AT_CMD(CALLPHONE);
}

void Bluetooth::dialNumber(const QString &number)
{
    QString CALLNUMBER = QString("HFPDTMF=") + number;
    Send_AT_CMD(CALLNUMBER);
}

BluetoothVoiceMode Bluetooth::getVoiceMode()
{
    Q_D(Bluetooth);
    return d->m_BluetoothVoiceMode;
}

void Bluetooth::voiceToggleSwitch()
{
    Q_D(Bluetooth);
    QString AT_CMD;
    if(d->m_BluetoothVoiceMode == BVM_VoicePhone){
        AT_CMD= QString("HFPADTS=2");//CP切换到车机蓝牙
        d->m_BluetoothVoiceMode = BVM_VoiceBluetooth;
    }else {
        AT_CMD= QString("HFPADTS=1");//CN音频切换到手机
        d->m_BluetoothVoiceMode = BVM_VoicePhone;
    }
    Send_AT_CMD(AT_CMD);
    onVoiceChange(d->m_BluetoothVoiceMode);
}

void Bluetooth::redialLastPhone()//重播电话
{
    QString AT_CMD = QString("HFPDIAL");
    Send_AT_CMD(AT_CMD);
}

void Bluetooth::pickupPhone()//接听
{
    QString CMD = "HFPANSW" ;
    Send_AT_CMD(CMD);
}

void Bluetooth::hanupPhone()//挂断
{
    QString CMD = "HFPCHUP" ;
    Send_AT_CMD(CMD);
    g_Audio->releaseAudioSource(AS_BluetoothPhone);
    emit onHangUpPhone();
}

void Bluetooth::asynchronousQueryStatus()
{
    QString AT_CMD = "HFPSTAT" ;
    Send_AT_CMD(AT_CMD);
}

int Bluetooth::getPowerStatus()//蓝牙开关状态
{
    Q_D(Bluetooth);
    return d->m_BluetoothPowerStatus;//蓝牙开关状态
}

void Bluetooth::powerOn()
{
    Q_D(Bluetooth);
    QString CMD = "BTEN=1" ;
    Send_AT_CMD(CMD);
}

void Bluetooth::powerOff()
{
    Q_D(Bluetooth);
    d->cancelSync();
    QString CMD = "BTEN=0" ;
    Send_AT_CMD(CMD);
}

int Bluetooth::getAutoConnectStatus()
{
    Q_D(Bluetooth);
    return d->m_BluetoothAutoConnect;
}

void Bluetooth::autoConnectOn()
{
    Q_D(Bluetooth);
    d->m_HFPCFGBit = d->m_HFPCFGBit | 0x01;
    qDebug()<<"+++++[autoConnectOn:d->m_HFPCFGBit]+++++"<< d->m_HFPCFGBit;
    QString CMD = "HFPCFG=" + QString("%1").arg(d->m_HFPCFGBit);
    Send_AT_CMD(CMD);
}

void Bluetooth::autoConnectOff()
{
    Q_D(Bluetooth);
    d->m_HFPCFGBit = d->m_HFPCFGBit & 0x0E;
    qDebug()<<"+++++[autoConnectOff:d->m_HFPCFGBit]+++++"<<d->m_HFPCFGBit;
    QString CMD = "HFPCFG=" + QString("%1").arg(d->m_HFPCFGBit);
    Send_AT_CMD(CMD);
}

int Bluetooth::getAutoAnswer()
{
    Q_D(Bluetooth);
    return d->m_BluetoothAutoAnswer;
}

void Bluetooth::autoAnswerOn()
{
    Q_D(Bluetooth);
    d->m_HFPCFGBit = d->m_HFPCFGBit | 0x02;
    qDebug()<<"+++++[autoAnswerOn:d->m_HFPCFGBit]+++++"<<d->m_HFPCFGBit;
    QString CMD = "HFPCFG=" + QString("%1").arg(d->m_HFPCFGBit);
    Send_AT_CMD(CMD);
}

void Bluetooth::autoAnswerOff()
{
    Q_D(Bluetooth);
    d->m_HFPCFGBit = d->m_HFPCFGBit & 0x0D;
    qDebug()<<"+++++[autoAnswerOff:d->m_HFPCFGBit]+++++"<<d->m_HFPCFGBit;
    QString CMD = "HFPCFG=" + QString("%1").arg(d->m_HFPCFGBit);
    Send_AT_CMD(CMD);
}



int Bluetooth::getBTMusicStatus()
{
    Q_D(Bluetooth);
    return d->m_BluetoothPlayerStatus;
}



static void* RevBtCmdTask(void *parent)
{
    BluetoothPrivate *pThis = (BluetoothPrivate *)parent;
    std::string readData;

    printf("[Bluetooth] RevBtCmdTask Create\n");
    while (1)
    {
        readData.clear();
        if(std::getline(pThis->readStream, readData))
        {
            if(readData.size() <= 0)
            {
                usleep(50*1000);
            }
            else
            {
                pThis->getBwSerialData(readData);
            }
        }
    }
    return NULL;
}

int Bluetooth::Blutooth_startThread()
{
    Q_D(Bluetooth);
    pthread_t mThread;
    int ret = -1;
    std::string filename("/dev/bw_serial");
    if (access(filename.data(), F_OK) == 0)
    {
        d->readStream.open(filename);
        printf("[Bluetooth] open %s success.\n", filename.c_str());
    }
    ret = pthread_create(&mThread, NULL, RevBtCmdTask, d);
    if(ret != 0)
    {
        printf("[Bluetooth] pthread_create failed.\n");
        ret = -1;
    }
    return ret;
}

void Bluetooth::Send_AT_CMD(QString AT_STR)
{
    qDebug()<<__PRETTY_FUNCTION__ <<"LINE :"<<__LINE__;
    QString AT_CMD;
    AT_CMD = QString("AT+") + AT_STR;
    AT_CMD.append("\r\n");
    QFile gocsdk_dev("/dev/bw_serial");
    qDebug()<<__PRETTY_FUNCTION__ <<__LINE__ <<"ATCMD:"<<AT_CMD.toLocal8Bit();
    if(gocsdk_dev.exists()){
        if(gocsdk_dev.open(QIODevice::ReadWrite)){
            gocsdk_dev.write(AT_CMD.toLocal8Bit());
            gocsdk_dev.close();
        }else {
            qDebug("/dev/bw_serial is open failed");
        }
    }else {
        qDebug("/dev/bw_serial is not exists");
    }
    qDebug()<<__PRETTY_FUNCTION__ <<"LINE :"<<__LINE__;
}


void Bluetooth::timerEvent(QTimerEvent *event)
{
    Q_D(Bluetooth);
    if ((d->m_Unmute)
            && (MI_Unmute == g_Audio->getMute())) {
        d->m_Unmute = false;
        g_Audio->requestMute(MI_Unmute);
    }
    killTimer(event->timerId());
}

void Bluetooth::connectRemoteDevice(const unsigned short index)
{
    Q_D(Bluetooth);
    qDebug()<<"+++++++++connectRemoteDevice+++++++++++="<<d->m_RemoteDeviceInfoList.size();
    if(d->m_RemoteDeviceInfoList.size() > index)
    {
        if(d->m_RemoteDeviceInfoList.at(index).macAddress != d->m_RemoteBtAddress){
            if(d->m_BluetoothConnectStatus >=3)
            {
                disconnectRemoteDevice();
            }
            QString AT_CMD = QString("HFPCONN=") + d->m_RemoteDeviceInfoList.at(index).macAddress;
            Send_AT_CMD(AT_CMD);
        }
    }
    qDebug()<<"+++++++++>m_PairedMap.size()+++++++++++="<<d->m_PairedMap.size();
    if(d->m_PairedMap.size()>0)
    {
        QMap<QString, QString>::iterator itor = d->m_PairedMap.begin();
        bool _CanPaired(true);
        int count = 0;
        QString _RemoteAddr;
        while(itor != d->m_PairedMap.end())
        {
            if(count == index)
            {
                _RemoteAddr = itor.key();
            }
            if(itor.key() == d->m_RemoteBtAddress)
            {
                _CanPaired = false;
                break;
            }
            count++;
            itor++;
        }

        qDebug()<<"+++++++count++++++++"<<count;
        if(_CanPaired == true)
        {
            if(d->m_BluetoothConnectStatus >=3)
            {
                disconnectRemoteDevice();
            }
            QString AT_CMD = QString("HFPCONN=") + _RemoteAddr;
            Send_AT_CMD(AT_CMD);
        }
    }
}

void Bluetooth::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder)
{
    switch (newHolder) {
    case AS_Aux: {
        if (AS_BluetoothMusic == oldHolder) {
            musicPause();
        }
        break;
    }
    case AS_BluetoothMusic: {
        qDebug() << __PRETTY_FUNCTION__ << __LINE__;
        break;
    }
    default:
        break;
    }
}

void Bluetooth::onTimeout()
{
    qDebug() << __PRETTY_FUNCTION__ << __LINE__;
    QTimer* ptr = static_cast<QTimer*>(sender());
    Q_D(Bluetooth);
    if (ptr == d->m_AutoCancelTimer) {
        d->cancelSync();
        qDebug() << __PRETTY_FUNCTION__ << __LINE__;
    }
}

void Bluetooth::onMuteChange(const int mute)
{
    Q_D(Bluetooth);
    d->m_Unmute = false;
}


/**********************************************************
 * source_str: 源字符串
 * n: 默认为2，每间隔两个字符串添加ch
 * ch：默认为空格
 * eg:Get_MAC_addr(out_str, 2 , ":");得到MAC地址
**********************************************************/
QString  Get_MAC_addr(QString source_str, int n=2,const QString &ch=" ")
{
    int size= source_str.size();
    int lenth = size/n;
    QString target_str = source_str;
    int str_lenth = ch.size();
    int i = 0;

    for(i = 0; i < lenth ; i++){
        target_str = target_str.insert((i + 1) * n + (i * str_lenth), ch);
    }
    return target_str.left(target_str.size() -1);
}



void Bluetooth::readHFPCFGValue()
{
    QString CMD = "HFPCFG";
    Send_AT_CMD(CMD);
}

void Bluetooth::ScanNearByRemoteDevice(){
    QString CMD = "SCAN=1";
    Send_AT_CMD(CMD);
}
void Bluetooth::StopScanNearByRemoteDevice(){
    QString CMD = "SCAN=0";
    Send_AT_CMD(CMD);
}
void Bluetooth::QueryPairedList()
{
    qDebug()<<"+++[Bluetooth::QueryPairedList]+++";
    QString CMD = "PLIST";
    Send_AT_CMD(CMD);
}
void Bluetooth::BtMusicPlyaStatus(int status)
{
    QString CMD = "PLAYSTAT=" + QString("%1").arg(status);
    Send_AT_CMD(CMD);
}

void Bluetooth::BtMusicInfo(int param)
{
    QString CMD = "AVRCPCFG=" + QString("%1").arg(param);
    Send_AT_CMD(CMD);
}
void Bluetooth::BtMicMuteChanged(int mute){
    QString CMD = "MICMUTE=" + QString("%1").arg(mute);
    Send_AT_CMD(CMD);
}

void Bluetooth::disableBtMusic()
{
    QString CMD = "A2DPDISC";
    Send_AT_CMD(CMD);
}
void Bluetooth::connectBtMusic()
{
    QString CMD = "A2DPCONN";
    Send_AT_CMD(CMD);
}
int Bluetooth::getBtMicMuteStatus()
{
    Q_D(Bluetooth);
    return d->m_BtMicMuteStatus;
}
void Bluetooth::reConnectLastDevice()
{
    QString CMD = "HFPCONN";
    Send_AT_CMD(CMD);
}
QString Bluetooth::getRemoteBtAddress(){
    Q_D(Bluetooth);
    return d->m_RemoteBtAddress;
}
void Bluetooth::setMusicType(int musicType)
{
    Q_D(Bluetooth);
    d->m_MusicType = musicType;
}
char convertCharToHex(char ch)
{
    if((ch >= '0') && (ch <= '9'))
         return ch-0x30;
     else if((ch >= 'A') && (ch <= 'F'))
         return ch-'A'+10;
     else if((ch >= 'a') && (ch <= 'f'))
         return ch-'a'+10;
    else return (-1);
}

BluetoothPrivate::BluetoothPrivate(Bluetooth *parent)
    : q_ptr(parent)
{
    m_BluetoothConnectStatus = 0;
    m_LastBluetoothConnectStatus = 0;
    m_BluetoothPlayerStatus = -1;
    m_AutoCancelTimer      = NULL;
    m_BluetoothVoiceMode   = BVM_VoiceBluetooth;
    m_BluetoothPairedMode  = BPM_Undefine;
    m_BluetoothPowerStatus = undefined;
    m_BluetoothAutoConnect = BAC_Undefine;
    m_BluetoothAutoAnswer  = BAA_Undefine;
    m_SyncType = BRT_Undefine;
    m_Unmute = false;
    m_index = 0;
    m_HFPCFGBit = 0;
    m_BtMicMuteStatus = 0;
    m_BtMusicTitle = "";
    m_BtMusicArtist= "";
    m_BtMusicAlbum = "";
    m_BtFirstEnterStatus = true;
    m_LastBtStatus = 0;
    m_MusicType = 0;
    connectAllSlots();
}

BluetoothPrivate::~BluetoothPrivate()
{

}

void BluetoothPrivate::connectAllSlots()
{
    Q_Q(Bluetooth);
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onHolderChange(const int, const int)));
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onMuteChange(const int)));
}

void BluetoothPrivate::cancelSync()
{
    qDebug() << __PRETTY_FUNCTION__ << __LINE__;
    if (BRT_Undefine != m_SyncType) {
        Q_Q(Bluetooth);
        QString AT_CMD = QString("PBABORT");//停止下载
        q->Send_AT_CMD(AT_CMD);
        if(m_SyncType = BRT_PhoneBook)
        {
            emit q->onSyncPhoneBook();
        }
        else if(m_SyncType != BRT_PhoneBook && m_SyncType != BRT_Undefine){
            emit q->onSyncAllCallLog();
        }
         m_SyncType = BRT_Undefine;
        if (NULL != m_AutoCancelTimer) {
            m_AutoCancelTimer->stop();
        }
    }
}

void BluetoothPrivate::getAddrMac()
{
    qDebug() << __PRETTY_FUNCTION__;
    Q_Q(Bluetooth);
    QString AT_CMD = QString("ADDR");
    q->Send_AT_CMD(AT_CMD);
}
void BluetoothPrivate::syncPhoneBook()
{
    qDebug() << __PRETTY_FUNCTION__;
    Q_Q(Bluetooth);
    m_SyncType = BRT_PhoneBook;
    QString AT_CMD = QString("PBDOWN=1");//同步电话本
    q->Send_AT_CMD(AT_CMD);
}
void BluetoothPrivate::syncAllCallLog(){
    qDebug() << __PRETTY_FUNCTION__;
    Q_Q(Bluetooth);
    m_SyncType = BRT_AllCall;
    QString AT_CMD = QString("PBDOWN=5");//同步电话本
    q->Send_AT_CMD(AT_CMD);
}
void BluetoothPrivate::syncInComingCallLog(){
    qDebug() << __PRETTY_FUNCTION__;
    Q_Q(Bluetooth);
    m_SyncType = BRT_Incoming;
    QString AT_CMD = QString("PBDOWN=2");//同步电话本
    q->Send_AT_CMD(AT_CMD);
}
void BluetoothPrivate::syncOutComingCallLog(){
    qDebug() << __PRETTY_FUNCTION__;
    Q_Q(Bluetooth);
    cancelSync();
    m_SyncType = BRT_Outgoing;
    QString AT_CMD = QString("PBDOWN=3");//同步电话本
    q->Send_AT_CMD(AT_CMD);
    initializeAutoCancelTimer();
    m_AutoCancelTimer->start();
}
void BluetoothPrivate::syncMissedCallLog(){
    qDebug() << __PRETTY_FUNCTION__;
    Q_Q(Bluetooth);
    cancelSync();
    m_SyncType = BRT_Missed;
    QString AT_CMD = QString("PBDOWN=4");//同步电话本
    q->Send_AT_CMD(AT_CMD);
    initializeAutoCancelTimer();
    m_AutoCancelTimer->start();
}


void BluetoothPrivate::initializeAutoCancelTimer()
{
    if (NULL == m_AutoCancelTimer) {
        Q_Q(Bluetooth);
        m_AutoCancelTimer = new QTimer(q);
        m_AutoCancelTimer->setInterval(10 * 1000);
        m_AutoCancelTimer->setSingleShot(true);
        QObject::connect(m_AutoCancelTimer, ARKSENDER(timeout()),
                         q,                 ARKRECEIVER(onTimeout()));
    }
}

void BluetoothPrivate::getBwSerialData(std::string data)
{
    Q_Q(Bluetooth);
    q->BlutoothHandle(QString::fromStdString(data));
}
QString BluetoothPrivate::getChineseSpell(QString src)
{
    QTextCodec *codec4gbk = QTextCodec::codecForName("GBK"); //获取qt提供的gbk的解码器
    QByteArray buf = codec4gbk->fromUnicode(src); //qt用的unicode，转成gbk
    int size = buf.size();
    quint16 *array = new quint16[size+1];
    QString alphbats;

    for( int i = 0, j = 0; i < buf.size(); i++, j++ )
    {
        if( (quint8)buf[i] < 0x80 ) //gbk的第一个字节都大于0x81，所以小于0x80的都是符号，字母，数字etc
            continue;
        array[j] = (((quint8)buf[i]) << 8) + (quint8)buf[i+1]; //计算gbk编码
        i++;
        alphbats.append( convert( array[j] ) ); //相当于查表，用gbk编码得到首拼音字母
    }
    delete [] array;
    return alphbats;
}
QList<struct RemoteDeviceInfo> Bluetooth::getRemoteDeviceInfoList(){
    Q_D(Bluetooth);
    return d->m_RemoteDeviceInfoList;
}

void Bluetooth::sendHicarDataToblueware(QString atCmd,QString Data)
{
    qDebug()<<__PRETTY_FUNCTION__ <<"LINE :"<<__LINE__;
    QString AT_CMD;
    AT_CMD = QString("AT#") + atCmd + QString("=")+Data;
    AT_CMD.append("\r\n");
    QFile gocsdk_dev("/dev/bw_serial");
    qDebug()<<__PRETTY_FUNCTION__ <<__LINE__ <<"ATCMD:"<<AT_CMD.toLocal8Bit();
    if(gocsdk_dev.exists()){
        if(gocsdk_dev.open(QIODevice::ReadWrite)){
            gocsdk_dev.write(AT_CMD.toLocal8Bit());
            gocsdk_dev.close();
        }else {
            qDebug("/dev/bw_serial is open failed");
        }
    }else {
        qDebug("/dev/bw_serial is not exists");
    }
    qDebug()<<__PRETTY_FUNCTION__ <<"LINE :"<<__LINE__;
}

QString Bluetooth::getLocalMacAddress(){
    Q_D(Bluetooth);
    return d->m_LocalAddress;
}

QString Bluetooth::getHicarBackLinkMac(){
    Q_D(Bluetooth);
    return d->m_HicarBackLinkMac;
}
QString Bluetooth::getHicarBackLinkCmd(){
    Q_D(Bluetooth);
    return d->m_HicarBackLinkCmd;
}
void Bluetooth::BlutoothHandle(QString data)
{
    Q_D(Bluetooth);
    //qDebug()<<__PRETTY_FUNCTION__<<__LINE__<<"readData:"<<data;
    //蓝牙开关状态
    if(data.contains("DEVSTAT"))
    {
        int ofIndex = data.indexOf('=');
        QString powerStatusStr = data.mid(ofIndex+1);
        powerStatusStr = powerStatusStr.left(powerStatusStr.size()-1);
        d->m_BluetoothPowerStatus = powerStatusStr.toInt() & 0x01;
        emit onPowerChange(d->m_BluetoothPowerStatus);
        if(d->m_BluetoothPowerStatus > 0){
            d->getAddrMac();
            setPincode(QString("0000"));
        }
        qDebug()<<"===[Bluetooth::BlutoothHandle:m_BluetoothPowerStatus000]====="<<d->m_BluetoothPowerStatus;
    }
    //自动连接和自动接听状态
    else if(data.contains("HFPCFG"))
    {
        int ofIndex = data.indexOf('=');
        QString HFPCFGStr = data.mid(ofIndex+1);
        HFPCFGStr = HFPCFGStr.left(HFPCFGStr.size()-1);
        d->m_HFPCFGBit = HFPCFGStr.toInt();
        d->m_BluetoothAutoConnect = HFPCFGStr.toInt() & 0x01;
        qDebug()<<"++++++d->m_BluetoothAutoConnect++++++"<<d->m_BluetoothAutoConnect;
        emit onAutoConnectChange(d->m_BluetoothAutoConnect);
        if((HFPCFGStr.toInt() & 0x02) == 0x02)
        {
            d->m_BluetoothAutoAnswer  = 0x01;
        }
        else
        {
            d->m_BluetoothAutoAnswer  = 0;
        }
        qDebug()<<"++++++d->m_BluetoothAutoAnswer++++++"<<d->m_BluetoothAutoAnswer;
        emit onAutoAnswerChange(d->m_BluetoothAutoAnswer);
    }
    //扫描远程设别的信息
    else if(data.contains("SCAN"))
    {
        int ofIndex = data.indexOf('=');
        QString scanStr = data.mid(ofIndex+1);
        //E\r
        QStringList scanStrList;
        scanStrList.clear();
        RemoteDeviceInfo _RemoteDeviceInfo;
        if(scanStr.left(2)!="E\r")
        {
            scanStrList = scanStr.split("�");
            if(QString(scanStrList.at(0)).toInt() == 1)
            {
                d->m_RemoteDeviceInfoList.clear();
            }
            for(int i= 0;i< scanStrList.size();i++)
            {
                switch (i) {
                    case 0:
                         _RemoteDeviceInfo.index = QString(scanStrList.at(0)).toInt();
                        break;
                    case 1:
                         _RemoteDeviceInfo.rssi = QString(scanStrList.at(1)).toInt();
                        break;
                    case 2:
                         _RemoteDeviceInfo.deviceType = QString(scanStrList.at(2)).toInt();
                        break;
                    case 3:
                         _RemoteDeviceInfo.macAddress = QString(scanStrList.at(3));
                        break;
                    case 4:
                         _RemoteDeviceInfo.name = QString(scanStrList.at(4));
                        break;
                    case 5:
                         _RemoteDeviceInfo.deviceClass = QString(scanStrList.at(5)).left(QString(scanStrList.at(5)).size()-1);
                        break;
                    default:
                        break;
                }
            }
            d->m_RemoteDeviceInfoList.append(_RemoteDeviceInfo);
        }
        else{
            d->m_PairedMap.clear();
            d->m_BtFirstEnterStatus = false;
            emit onScanFinish();
        }
    }
    //配对状态
    else if(data.contains("HFPSTAT")){
        int ofIndex = data.indexOf('=');
        QString btConnectStatusStr = data.mid(ofIndex+1);
        d->m_BluetoothConnectStatus= btConnectStatusStr.left(1).toInt();
        QString _onCallingNumber;
        if(d->m_BluetoothConnectStatus == 6)
        {
            QStringList _onCallingList = btConnectStatusStr.split(",");
            if(_onCallingList.size() > 1)
            {
                _onCallingNumber = QString(_onCallingList.at(1)).left(QString(_onCallingList.at(1)).size()-1);
                d->m_LastOnCallingNum = _onCallingNumber;
            }
        }
        qDebug()<<"++++++++++++d->m_BluetoothConnectStatus++++++++++++"<<d->m_BluetoothConnectStatus;
        qDebug()<<"++++++++++++_onCallingNumber++++++++++++"<<_onCallingNumber;
        qDebug()<<"++++++++++++d->m_LastBluetoothConnectStatus++++++++++++"<<d->m_LastBluetoothConnectStatus;
        qDebug()<<"++++++++++++d->m_LastOnCallingNum++++++++++++"<<d->m_LastOnCallingNum;
        qDebug()<<"++++++++++++d->m_LastOnCallingNum.size++++++++++++"<<d->m_LastOnCallingNum.size();
        if(d->m_BluetoothConnectStatus != d->m_LastBluetoothConnectStatus)
        {
            if(_onCallingNumber.size() == 0 || ( _onCallingNumber.size() != 0 && _onCallingNumber != QString("000000")))
            {
                emit onConnectStatusChange(d->m_BluetoothConnectStatus);
            }
            if(d->m_LastBluetoothConnectStatus >= 4 && d->m_BluetoothConnectStatus < 4)
            {
                if(d->m_LastBluetoothConnectStatus != Bluetooth::BCS_Talking || d->m_LastOnCallingNum != QString("000000"))
                {
                    qDebug()<<"++++++0000####xxx++++++";
                    g_Audio->releaseAudioSource(AS_BluetoothPhone);
                }
            }
        }
        d->m_LastBluetoothConnectStatus = d->m_BluetoothConnectStatus;
        if(d->m_LastBluetoothConnectStatus != Bluetooth::BCS_Talking)
        {
            d->m_LastOnCallingNum.clear();
        }
        if(d->m_BluetoothConnectStatus < 3){
            if(d->m_LastBtStatus >= 3)
                d->m_BtFirstEnterStatus = true;
            d->m_RemoteBtAddress.clear();
            d->m_RemoteDeviceName.clear();
        }
        d->m_LastBtStatus = d->m_BluetoothConnectStatus;
        qDebug()<<"+++[Bluetooth::BlutoothHandle:BluetoothConnectStatus]+++"<<d->m_BluetoothConnectStatus;
        //电话接入
        if(d->m_BluetoothConnectStatus == Bluetooth::BCS_Incoming){
            if(g_Link->getLinkConnectStatus() != 0 && g_Link->getLinkType() == Carlife && g_Setting->getCarlifeTelephoneStatus() == 0)
            {
                g_Link->requestKey(KEY_MUSIC_PAUSE);
                g_Link->requestLink(g_Link->getLinkType(), g_Link->getLinkMode(), DBUS_REQUEST_BACKGROUND);
            }
            QStringList _IncomingList = btConnectStatusStr.split(",");
            QString IncomingNumber;
            if(_IncomingList.size() > 1)
            {
                IncomingNumber = QString(_IncomingList.at(1)).left(QString(_IncomingList.at(1)).size()-1);
            }
            emit onDialInfo(d->m_BluetoothConnectStatus,IncomingNumber);
            g_Audio->requestAudioSource(AS_BluetoothPhone);
        }//正在接听中
        else if(d->m_BluetoothConnectStatus == Bluetooth::BCS_Talking && _onCallingNumber != "000000"){
            qDebug()<<"------_onCallingNumber-----"<<_onCallingNumber;
            emit onDialInfo(d->m_BluetoothConnectStatus,_onCallingNumber);
            g_Audio->requestAudioSource(AS_BluetoothPhone);
        }//电话拨出
        else if(d->m_BluetoothConnectStatus == Bluetooth::BCS_Outgoing){
            QStringList _OutComingList = btConnectStatusStr.split(",");
            QString _OutComingNumber;
            if(_OutComingList.size() > 1)
            {
                _OutComingNumber = QString(_OutComingList.at(1)).left(QString(_OutComingList.at(1)).size()-1);
            }
            g_Audio->requestAudioSource(AS_BluetoothPhone);
            emit onDialInfo(d->m_BluetoothConnectStatus,_OutComingNumber);
        }
    }
    //配对过的链表
    else if(data.contains("PLIST")){
        if(!d->m_BtFirstEnterStatus){
            return;
        }
        int ofIndex = data.indexOf('=');
        QString pairedListStr = data.mid(ofIndex+1);
        QStringList pairedList;
        if(pairedListStr.left(2)!="E\r"){
            pairedList = pairedListStr.split("�");
            QString pairedAddr = QString(pairedList.at(2));
            QString pairedName = QString(pairedList.at(3)).left(QString(pairedList.at(3)).size()-1);
            d->m_PairedMap.insert(pairedAddr,pairedName);
        }
        else
        {
            qDebug()<<"=======onGetPairedListFinish======";
            emit onGetPairedListFinish();
        }
    }
    //配对成功的远程设备MAC地址和名称
    else if(data.contains("HFPDEV")){
        int ofIndex = data.indexOf('=');
        QString pairedConnectStr = data.mid(ofIndex+1);
        QStringList pairedConnectList;
        pairedConnectList = pairedConnectStr.split("�");
        d->m_RemoteBtAddress  = QString(pairedConnectList.at(0));
        d->m_RemoteDeviceName = QString(pairedConnectList.at(1)).left(QString(pairedConnectList.at(1)).size()-1);
        qDebug()<<"+++[Bluetooth::BlutoothHandle:m_RemoteBtAddress]+++"<<__LINE__ <<d->m_RemoteBtAddress;
        qDebug()<<"+++[Bluetooth::BlutoothHandle:m_RemoteDeviceName]+++"<<__LINE__ <<d->m_RemoteDeviceName;
        onRemoteDeviceNameChange(d->m_RemoteDeviceName);
    }
    //蓝牙音乐Play状态
    else if(data.contains("PLAYSTAT")){
        int ofIndex = data.indexOf('=');
        int btMusicPlayStatus = QString(data.mid(ofIndex+1)).left(QString(data.mid(ofIndex+1)).size()-1).toInt();
        qDebug()<<"++++++++Bluetooth::BtMusic_Playing+++++++++" << btMusicPlayStatus;
        if(btMusicPlayStatus == Bluetooth::BtMusic_Playing)
        {
            qDebug()<<"++++++++Bluetooth::BtMusic_Playing0000+++++++++";
            g_Audio->requestAudioSource(AS_BluetoothMusic);
            int param = 35;
            BtMusicInfo(param);
        }
        else
        {
            if(d->m_BluetoothPlayerStatus == Bluetooth::BtMusic_Playing)
            {
                if(d->m_MusicType == 1 && btMusicPlayStatus == BtMusic_Stopped)
                {
                    qDebug()<<"+++++++++++holderChange+++++++++++";
                    g_Audio->requestAudioSource(AS_Music);
                }
            }
        }
        d->m_BluetoothPlayerStatus  = btMusicPlayStatus;
        emit onMusicStatusChange(d->m_BtMusicTitle,d->m_BluetoothPlayerStatus);
    }
    else if(data.contains("TRACKINFO")){
        int ofIndex = data.indexOf('=');
        QString btMusicID3InfoStr = data.mid(ofIndex+1);
        QStringList btMusicID3InfoList;
        btMusicID3InfoList = btMusicID3InfoStr.split("�");
        qDebug()<<"++++++++btMusicID3InfoList000++++++"<<btMusicID3InfoList.size();
        if(btMusicID3InfoList.size() < 3){
            return;
        }
        QString _Title_Artist = QString(btMusicID3InfoList.at(1));
        QStringList _Title_ArtistList = _Title_Artist.split("-");
        qDebug()<<"++++++++_Title_Artist000++++++"<<_Title_Artist;
        qDebug()<<"++++++++_Title_ArtistList.size()0000++++++"<<_Title_ArtistList.size();
        if(_Title_ArtistList.size() == 1)
        {
            _Title_ArtistList = _Title_Artist.split(" — ");
        }
        if(_Title_ArtistList.size() >= 2)
        {
            QString _TitleStr  = QString(_Title_ArtistList.at(1));
            QString _ArtistStr = QString(_Title_ArtistList.at(0));
            QString _AlbumStr  = QString(btMusicID3InfoList.at(2)).left(QString(btMusicID3InfoList.at(2)).size()-1);
            d->m_BtMusicTitle = _TitleStr;
            d->m_BtMusicArtist= _ArtistStr;
            d->m_BtMusicAlbum = _AlbumStr;
            qDebug()<<"++++++++_TitleStr111++++++"<<_TitleStr;
            qDebug()<<"++++++++_ArtistStr111++++++"<<_ArtistStr;
            qDebug()<<"++++++++_AlbumStr+++++"<<_AlbumStr;
            emit onBtMusicID3InfoChange(_TitleStr,_ArtistStr,_AlbumStr);
        }
        else
        {
            if(btMusicID3InfoList.size() >= 2)
            {
                QString _TitleStr  = QString(btMusicID3InfoList.at(0));
                QString _ArtistStr = QString(btMusicID3InfoList.at(1));
                QString _AlbumStr  ="";
                qDebug()<<"++++++++_TitleStr222++++++"<<_TitleStr;
                qDebug()<<"++++++++_ArtistStr22++++++"<<_ArtistStr;
                emit onBtMusicID3InfoChange(_TitleStr,_ArtistStr,_AlbumStr);
            }
        }
        qDebug()<<"--------TRACKINFO----END-------";
    }
    else if(data.contains("TRACKSTAT")){
        int ofIndex = data.indexOf('=');
        QString btMusicTimeInfoStr = data.mid(ofIndex+1);
        QStringList btMusicTimeInfoList;
        btMusicTimeInfoList = btMusicTimeInfoStr.split(",");
        int _ElapsedTime = QString(btMusicTimeInfoList.at(1)).toInt();
        int _EndTime = QString(btMusicTimeInfoList.at(2)).toInt();
        qDebug()<<"+++++++++++TRACKSTAT++++++++++";
        onBtMusicElapsedInfo(_ElapsedTime,_EndTime);
    }
    else if(data.contains("PBDATA")){
        int ofIndex = data.indexOf('=');
        QString _RecordStr = data.mid(ofIndex+1);
        if(_RecordStr.left(1) != "E")
        {
            if(_RecordStr.left(1) == "1")
            {
                QStringList _PhoneBookList = _RecordStr.split("�");
                if(_PhoneBookList.size() < 3){
                   return;
                }
                struct PhoneBookInfo _PhoneBookStr;
                _PhoneBookStr.name  = QString(_PhoneBookList.at(1));
                _PhoneBookStr.phone = QString(_PhoneBookList.at(2)).left(QString(_PhoneBookList.at(2)).size()-1);
                if(QString(_PhoneBookStr.name) <= 0)
                {
                    _PhoneBookStr.name = _PhoneBookStr.phone;
                }
                QString character;
                if(IsChinese(_PhoneBookStr.name.left(1))){
                    character = d->getChineseSpell(QString(_PhoneBookStr.name.left(1)));
                }
                else{
                    character = _PhoneBookStr.name.left(1);
                }
                if(character.size() >= 1)
                    _PhoneBookStr.head = character.left(1);
                d->m_PhoneBookList.append(_PhoneBookStr);
            }
            else{
                QStringList _AllCallList = _RecordStr.split("�");
                if(_AllCallList.size() < 4){
                   return;
                }
                struct CallLogInfo _AllCallStr;
                _AllCallStr.callType = QString(_AllCallList.at(0)).toInt();
                _AllCallStr.name     = QString(_AllCallList.at(1));
                _AllCallStr.phoneNumber = QString(_AllCallList.at(2));
                if(_AllCallStr.name.size()==0)
                {
                    _AllCallStr.name = _AllCallStr.phoneNumber;
                }
                QStringList _DataTimeList   = QString(_AllCallList.at(3)).split("T");
                _AllCallStr.data = QString(_DataTimeList.at(0));
                _AllCallStr.time = QString(_DataTimeList.at(1));
                d->m_AllCallList.append(_AllCallStr);
            }
        }
        else{
            if(d->m_AutoCancelTimer->isActive())
            {
                d->m_AutoCancelTimer->stop();
            }
            if(d->m_SyncType == BRT_PhoneBook){
                emit onSyncPhoneBook();
            }
            else if(d->m_SyncType == BRT_Incoming){
                d->syncOutComingCallLog();
            }
            else if(d->m_SyncType == BRT_Outgoing){
                d->syncMissedCallLog();
            }
            else if(d->m_SyncType == BRT_Missed){
                emit onSyncAllCallLog();
            }
        }
    }
    else if(data.contains("MICMUTED")){
        int ofIndex = data.indexOf('=');
        QString _BtMicMuteStatusStr = data.mid(ofIndex+1);
        d->m_BtMicMuteStatus = _BtMicMuteStatusStr.left(1).toInt();
    }
    //Hicar无线连接
    else if(data.contains("ADDR=")){
        int ofIndex = data.indexOf('=');
        QString _AddrStr = data.mid(ofIndex+1);
        _AddrStr = _AddrStr.left(_AddrStr.size()-1);
        d->m_LocalAddress = _AddrStr;
        qDebug()<<"=====_AddrStr=========="<<_AddrStr;
    }
    else if(data.left(2)== QString("SR")){
        qDebug()<<"========Hicar无线连接SR======="<<data.left(data.size()-1);
        g_Link->requestBluetoothCmd(data.left(data.size()-1).toStdString());
    }
    else if(data.left(2)== QString("SI")){
        qDebug()<<"========Hicar无线连接SI======="<<data.left(data.size()-1);
        g_Link->requestBluetoothCmd(data.left(data.size()-1).toStdString());
    }
    else if(data.left(2)== QString("SV")){
        qDebug()<<"========Hicar无线连接SV======="<<data.left(data.size()-1);
        g_Link->requestBluetoothCmd(data.left(data.size()-1).toStdString());
    }
    else if(data.left(2)== QString("SS")){
        qDebug()<<"========Hicar无线连接SS======="<<data.left(data.size()-1);
        g_Link->requestBluetoothCmd(data.left(data.size()-1).toStdString());
    }
    else if(data.left(3)== QString("MX0")){
        qDebug()<<"========Hicar无线连接MX0======="<<data.left(data.size()-1);
        d->m_HicarBackLinkMac = data.left(15).right(12);
        d->m_HicarBackLinkCmd = data.left(data.size()-1);
    }
    else if(data.left(4)== QString("MZ12")){
        qDebug()<<"========Hicar无线连接MZ12======="<<data.left(data.size()-1);
        g_Link->requestBluetoothCmd(data.left(data.size()-1).toStdString());
    }

}
