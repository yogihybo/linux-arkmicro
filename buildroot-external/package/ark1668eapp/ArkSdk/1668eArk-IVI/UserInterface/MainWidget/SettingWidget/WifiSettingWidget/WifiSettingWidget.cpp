#include "WifiSettingWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/HostApd.h"
#include "BusinessLogic/WiFiManager.h"
#include "BusinessLogic/Setting.h"
#include "BusinessLogic/QmlWidget.h"
#include <QQmlProperty>
#include <unistd.h>
#include <QDebug>
#include <QProcess>
#include <QTimer>
#include <QFile>
#include <QSettings>
class WifiSettingWidgetPrivate
{
    Q_DISABLE_COPY(WifiSettingWidgetPrivate)
public:
    explicit WifiSettingWidgetPrivate(WifiSettingWidget* parent);
    ~WifiSettingWidgetPrivate();
    void initializeObject();
    void initializeGetScanResultTimer();
    void connectAllSlots();
    void wifiPowerOpen();
    void wifiPowerClose();
public:
    QObject* m_WifiSettingWidgetObject;
    QObject* m_WifiPowerBtnObject;
    QObject* m_WifiScanBtnObject;
    QObject* m_RotationObject;
    QObject* m_WifiScanBtnIconRotationAnimationObject;
    QObject* m_PasswordWidgetObject;
    QObject* m_TextInputObject;
    QObject* m_CancleBtnObject;
    QObject* m_ConfirmBtnBtnObject;
    QObject* m_WifiConnectedBtnObject;
    QObject* m_WifiConnectedBtnTextObject;
    QTimer*  m_GetScanResultTimer;
    QString  m_WiFiName;
    QString  m_WiFiPasswd;
    int m_NetId;
private:
    Q_DECLARE_PUBLIC(WifiSettingWidget)
    WifiSettingWidget* const q_ptr;
};
static QString ParsingChineseSSID(QString ssid)
{
    char utf8[28] ={0};
    bool ok = true;
    int index = 0;
    int position[128] = {-1};
    QStringList utfStr;
    QString nameStr;
    bool    indexFlag =false;
    int temp = 0;
    QString chinese;

    QStringList list = ssid.split("\\");
    for(int i=0;i<list.size();i++) {
        //qDebug() << "list.at" << i << ":"<< list.at(i);
        if(i>0) {
            QString qstr = ("0"+list.at(i).left(3));
            if (QString(list.at(i)).size() > 3) {
                //qDebug() << "=== qStr > 3:" << i << ":"<< list.at(i);
                qstr = "0"+ list.at(i).left(3);
                utfStr.append(qstr);
                position[index] = i+index;
                index ++;
                utfStr.append(QString(list.at(i)).mid(3, QString(list.at(i)).size() -3));
            }
            else
            {
                utfStr.append(qstr);
            }
        }
    }

    //qDebug() << "=== utfStr.size():" << utfStr.size();
    //qDebug() << "===0000 utfStr:" << utfStr;
    //qDebug() << "===0000 index:" << index;
    for(int inf= 0;inf < utfStr.size();inf++)
    {
        indexFlag = false;
        for(int i=0;i<index;i++)
        {
            if(position[i]==inf)
            {
                //qDebug()<<"----inf--------"<<inf;
                indexFlag = true;
                nameStr += QString::fromUtf8(utf8);
                //for (int i=0;i< temp;i++) {
                //    printf("utf8[i]:%d 0x%x\n", i, utf8[i]);
                //}
                //qDebug() << "111nameStr:" << nameStr;
                nameStr += utfStr.at(inf);
                memset(utf8,0,sizeof(utf8));
                temp = 0;
                //qDebug() << "222nameStr:" << nameStr;
                break;
            }
        }
        if(!indexFlag)
        {
            utf8[temp++] = utfStr.at(inf).toInt(&ok,16);
        }
    }
    nameStr += QString::fromUtf8(utf8);

    if (nameStr != "") {
        if (list.at(0) == "") { //startwith chinese
            chinese = nameStr;
        } else {
            chinese = list.at(0) + nameStr;
        }
    } else {
        chinese = "";
    }
    return chinese;
}

WifiSettingWidget::WifiSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new WifiSettingWidgetPrivate(this))
{

}

void WifiSettingWidget::setWifiSettingWidgetObject(QObject* qmlObject)
{
    Q_D(WifiSettingWidget);
    //qDebug()<<"=======WifiSettingWidget===start=====";
    if(d->m_WifiSettingWidgetObject == NULL)
    {
        d->m_WifiSettingWidgetObject = qmlObject;
    }
    d->initializeObject();
    d->connectAllSlots();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_WifiPowerBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_WifiScanBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_TextInputObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_CancleBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_ConfirmBtnBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_WifiSettingWidgetObject, SIGNAL(listviewItemClicked(int)),
                     this,      SLOT(onListviewItemClicked(int)),
                     type);
    QObject::connect(d->m_WifiConnectedBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
   // qDebug()<<"=======WifiSettingWidget===end=====";

}

void WifiSettingWidget::onListviewItemClicked(int index)
{
    Q_D(WifiSettingWidget);
    //qDebug()<<"=========onListviewItemClicked=========";
    QQmlProperty(d->m_PasswordWidgetObject,"visible").write(true);
    d->m_WiFiName = g_Setting->getWifiSsidList().at(index);
}

void WifiSettingWidget::onToolButtonRelease()
{
    Q_D(WifiSettingWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_WifiPowerBtnObject){
        int _WifiPowerStatus = d->m_WifiSettingWidgetObject->property("wifiPowerStatus").toInt();
        qDebug()<<"++++++++_WifiPowerStatus++++++++++"<<_WifiPowerStatus;
        if(_WifiPowerStatus == 0)
        {
            d->wifiPowerOpen();
        }
        else if(_WifiPowerStatus == 1)
        {
            d->wifiPowerClose();
        }
    }
    else if(ptr == d->m_WifiScanBtnObject)
    {
        g_Setting->clearSsidList();
        g_Setting->onWifiSsidListChanged();
        if(d->m_WifiScanBtnIconRotationAnimationObject->property("running").toBool() == false)
        {
            QQmlProperty(d->m_WifiScanBtnIconRotationAnimationObject,"running").write(true);
        }
        g_WiFiManager->wifiScan();
        d->initializeGetScanResultTimer();
        if (d->m_GetScanResultTimer->isActive())
            d->m_GetScanResultTimer->stop();
        d->m_GetScanResultTimer->start();
    }
    else if(ptr == d->m_TextInputObject)
    {
       // qDebug()<<"=======d->m_PasswordWidgetObject=======";
        QmlWidget::instance()->onKeyBoardWidgetVisibel(true);
    }
    else if(ptr == d->m_CancleBtnObject)
    {
        Q_D(WifiSettingWidget);
        QmlWidget::instance()->onKeyBoardWidgetVisibel(false);
        QmlWidget::instance()->clearKeyBoardInputStr();
        QQmlProperty(d->m_PasswordWidgetObject,"visible").write(false);
    }
    else if(ptr == d->m_ConfirmBtnBtnObject)
    {
        Q_D(WifiSettingWidget);
        QmlWidget::instance()->onKeyBoardWidgetVisibel(false);
        QmlWidget::instance()->clearKeyBoardInputStr();
        QQmlProperty(d->m_PasswordWidgetObject,"visible").write(false);
        d->m_WiFiPasswd = d->m_TextInputObject->property("textInputText").toString();
        QQmlProperty(d->m_TextInputObject,"textInputText").write("");
        //qDebug()<<"+++[WifiSettingWidget::onToolButtonRelease:m_WiFiPasswd]+++"<< d->m_WiFiPasswd;
        if (d->m_WiFiPasswd.length() >= WIFIPASSWDMINLENGTH) {
            QByteArray ssid = d->m_WiFiName.toLocal8Bit();
            QByteArray passwd = d->m_WiFiPasswd.toLocal8Bit();
            d->m_NetId = g_WiFiManager->wifiAddNetwork();
            qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "=== netID:" << d->m_NetId;
            qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "=== ssid:" << ssid;
            qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "=== passwd:" << passwd;
            g_WiFiManager->wifiSetSsid(d->m_NetId,ssid.data());
            g_WiFiManager->wifiSetPassword(d->m_NetId, passwd.data());
            g_WiFiManager->wifiEnableNetwork(d->m_NetId);
            g_WiFiManager->onWifiConnectStatusChange(WCS_Connectting);
        } else {
            g_WiFiManager->onWifiConnectStatusChange(WCS_IleaglPasswd);
        }
        d->m_WiFiPasswd.clear();
    }
    else if(ptr == d->m_WifiConnectedBtnObject){
        qDebug()<<"+++++++++d->m_NetId++++++++"<<d->m_NetId;
        g_WiFiManager->disableNetWork(d->m_NetId);
        g_WiFiManager->removeNetWork(d->m_NetId);
    }
}
void WifiSettingWidget::onWifiGetResultFail(){
    g_WiFiManager->wifiScanResult();
}
void WifiSettingWidget::onTimerOut()
{
    Q_D(WifiSettingWidget);
    //qDebug()<<"++++[WifiSettingWidget::onTimerOut]++++"<<__LINE__;
    if(d->m_WifiScanBtnIconRotationAnimationObject != NULL){
        if(d->m_RotationObject != NULL)
        {
            qDebug()<<"++++[WifiSettingWidget::onTimerOut]++++"<<__LINE__;
            QQmlProperty(d->m_RotationObject,"scanFinish").write(true);
        }
    }
    g_WiFiManager->wifiScanResult();
}
void WifiSettingWidget::onWifiSSIDInfoChange(char *ssid)
{
    Q_D(WifiSettingWidget);
    QString m_ssid(ssid);

    if (m_ssid.contains("\\")) {
        m_ssid = ParsingChineseSSID(m_ssid);
    }
    //qDebug()<<"======m_ssid====="<<m_ssid;
    if (m_ssid != "") {
        if (g_Setting->wifiSsidListSize() == 0) {
            g_Setting->appendSsidWifiSsidList(m_ssid);
            g_Setting->onWifiSsidListChanged();

        } else {
            for (int i=0; i< g_Setting->wifiSsidListSize(); i++) {
                if (m_ssid == g_Setting->getWifiSsidList().at(i)) {
                    break;
                } else {
                    if (i == (g_Setting->wifiSsidListSize() -1))
                    {
                        g_Setting->appendSsidWifiSsidList(m_ssid);
                        g_Setting->onWifiSsidListChanged();
                    }
                }
            }
        }
    }
}

void WifiSettingWidget::onAddKeyBoardInputStr(QString str)
{
    Q_D(WifiSettingWidget);
    if(d->m_TextInputObject != NULL)
        QQmlProperty(d->m_TextInputObject,"textInputText").write(str);
}
void WifiSettingWidget::onSubKeyBoardInputStr(QString str){
    Q_D(WifiSettingWidget);
    if(d->m_TextInputObject != NULL)
        QQmlProperty(d->m_TextInputObject,"textInputText").write(str);
}
void WifiSettingWidget::onWifiConnectStatusChange(const int status){
    Q_D(WifiSettingWidget);
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "#### WiFiConnectStatus: " << status;
    switch (status) {
    case WCS_Success: {
        QQmlProperty(d->m_WifiSettingWidgetObject,"wifiConnectStatus").write(1);
        QQmlProperty(d->m_WifiConnectedBtnTextObject,"text").write(d->m_WiFiName);
        break;
    }
    case WCS_Fail: {
        QQmlProperty(d->m_WifiSettingWidgetObject,"wifiConnectStatus").write(2);
        QQmlProperty(d->m_WifiConnectedBtnTextObject,"text").write("");
        break;
    }
    case WCS_DisConnect: {
        QQmlProperty(d->m_WifiSettingWidgetObject,"wifiConnectStatus").write(3);
        QQmlProperty(d->m_WifiConnectedBtnTextObject,"text").write("");
        break;
    }
    case WCS_Connectting: {
        QQmlProperty(d->m_WifiSettingWidgetObject,"wifiConnectStatus").write(0);
        break;
    }
    case WCS_IleaglPasswd: {
        QQmlProperty(d->m_WifiSettingWidgetObject,"wifiConnectStatus").write(5);
        QQmlProperty(d->m_WifiConnectedBtnTextObject,"text").write("");
        break;
    }
    default:
        break;
    }
}

void WifiSettingWidget::onAutoConncetSsid(QString ssid)
{
    Q_D(WifiSettingWidget);
    if(ssid.contains("\\")) {  //contain chinese
        ssid = ParsingChineseSSID(ssid);
    }
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "=== Auto connect ssid: " << ssid;
    if (-1 == g_WiFiManager->checkSsidExist(FILEPATH, ssid)) {
        qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "========= save ssid config";
        //g_WiFiManager->wifiSaveConfig();
    }
    d->m_WiFiName = ssid;
    if (g_Setting->wifiSsidListSize() != 0) {
        QList<QString> ssidList;
        int i;
        ssidList.append(ssid);
        //qDebug()<<"+++++++++++g_Setting->wifiSsidListSize()000++++++++++"<<g_Setting->wifiSsidListSize();
        for (i=0;i<g_Setting->wifiSsidListSize();i++) {
            if (g_Setting->getWifiSsidList().at(i) != ssid)
                ssidList.append(g_Setting->getWifiSsidList().at(i));
        }
        //qDebug()<<"+++++++++++ssidList.size()000++++++++++"<<ssidList.size();
        g_Setting->clearSsidList();
        for(int i= 0; i<ssidList.size();i++)
        {
            g_Setting->appendSsidWifiSsidList(ssidList.at(i));
            g_Setting->onWifiSsidListChanged();
        }
    } else {
        g_Setting->appendSsidWifiSsidList(ssid);
        g_Setting->onWifiSsidListChanged();
    }
}

void WifiSettingWidget::onWlanClose()
{
    Q_D(WifiSettingWidget);
    QQmlProperty(d->m_WifiSettingWidgetObject,"wifiPowerStatus").write(0);
}
void WifiSettingWidget::onWlanOpen()
{
    Q_D(WifiSettingWidget);
    qDebug()<<"+++onWlanOpen0000++++";
    d->wifiPowerOpen();
}
void WifiSettingWidget::onReInputPassword()
{
    Q_D(WifiSettingWidget);
    if(d->m_PasswordWidgetObject->property("visible").toBool() == false)
    {
        QQmlProperty(d->m_PasswordWidgetObject,"visible").write(true);
        QmlWidget::instance()->onKeyBoardWidgetVisibel(true);
    }
}
WifiSettingWidgetPrivate::WifiSettingWidgetPrivate(WifiSettingWidget *parent)
    : q_ptr(parent)
{
    m_WifiSettingWidgetObject = NULL;
    m_WifiPowerBtnObject = NULL;
    m_WifiScanBtnObject = NULL;
    m_RotationObject = NULL;
    m_WifiScanBtnIconRotationAnimationObject = NULL;
    m_GetScanResultTimer = NULL;
    m_PasswordWidgetObject = NULL;
    m_CancleBtnObject = NULL;
    m_TextInputObject = NULL;
    m_ConfirmBtnBtnObject = NULL;
    m_WifiConnectedBtnObject = NULL;
    m_WifiConnectedBtnTextObject = NULL;
    m_NetId = 0;
}

WifiSettingWidgetPrivate::~WifiSettingWidgetPrivate()
{

}
void WifiSettingWidgetPrivate::initializeObject(){
    if(m_WifiPowerBtnObject == NULL)
    {
        m_WifiPowerBtnObject = m_WifiSettingWidgetObject->findChild<QObject*>("wifiPowerBtnObject");
    }
    if(m_WifiScanBtnObject == NULL)
    {
        m_WifiScanBtnObject = m_WifiSettingWidgetObject->findChild<QObject*>("wifiScanBtnObject");
    }
    if(m_RotationObject == NULL)
    {
        m_RotationObject  = m_WifiScanBtnObject->findChild<QObject*>("rotationObject");
    }
    if(m_WifiScanBtnIconRotationAnimationObject == NULL)
    {
        m_WifiScanBtnIconRotationAnimationObject  = m_RotationObject->findChild<QObject*>("wifiScanBtnIconRotationAnimationObject");
    }

    if(m_PasswordWidgetObject == NULL)
    {
        m_PasswordWidgetObject = m_WifiSettingWidgetObject->findChild<QObject*>("passwordWidgetObject");
    }
    if(m_TextInputObject == NULL)
    {
        m_TextInputObject = m_PasswordWidgetObject->findChild<QObject*>("textInputObject");
    }
    if(m_CancleBtnObject == NULL)
    {
        m_CancleBtnObject = m_PasswordWidgetObject->findChild<QObject*>("cancleBtnObject");
    }
    if(m_ConfirmBtnBtnObject == NULL)
    {
        m_ConfirmBtnBtnObject = m_PasswordWidgetObject->findChild<QObject*>("confirmBtnBtnObject");
    }

    if(m_WifiConnectedBtnObject == NULL)
    {
        m_WifiConnectedBtnObject = m_WifiSettingWidgetObject->findChild<QObject*>("wifiConnectedBtnObject");
    }
    if(m_WifiConnectedBtnTextObject == NULL)
    {
        m_WifiConnectedBtnTextObject = m_WifiConnectedBtnObject->findChild<QObject*>("wifiConnectedBtnTextObject");
    }
}

void WifiSettingWidgetPrivate::initializeGetScanResultTimer()
{
    Q_Q(WifiSettingWidget);
    if (NULL == m_GetScanResultTimer) {
        m_GetScanResultTimer = new QTimer(q);
        m_GetScanResultTimer->setInterval(2*1000);
        m_GetScanResultTimer->setSingleShot(true);
        QObject::connect(m_GetScanResultTimer, SIGNAL(timeout()), q, SLOT(onTimerOut()));
    }
}

void WifiSettingWidgetPrivate::wifiPowerOpen()
{
    if(g_Setting->getWlanApStatus() == 1)
    {
        g_Setting->executeShellCmd("killall udhcpd");
        g_Setting->executeShellCmd("killall hostapd");
        g_Setting->executeShellCmd("killall udhcpc");
        g_Setting->executeShellCmd("killall wpa_supplicant");
        g_Setting->executeShellCmd("ifconfig wlan0 down");
        g_HostApd->setInitHostapdStatus(1);
        g_Setting->setWlanApStatus(0);
    }
    int _InitHostapdStatus = g_HostApd->getInitHostapdStatus();
    QQmlProperty(m_WifiSettingWidgetObject,"wifiPowerStatus").write(1);
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
            //qDebug()<<"--------_InitHostapdStatus---------"<<i;
            g_Setting->executeShellCmd("ifconfig wlan0 up");
            g_WiFiManager->startSTAThread();
            g_Setting->setWifiOpenStatus(1);
            g_Setting->setWifiConfig(1);
            break;
        }
    }
}
void WifiSettingWidgetPrivate::wifiPowerClose()
{
    g_Setting->executeShellCmd("killall udhcpd");
    g_Setting->executeShellCmd("killall hostapd");
    g_Setting->executeShellCmd("killall udhcpc");
    g_Setting->executeShellCmd("killall wpa_supplicant");
    g_Setting->executeShellCmd("ifconfig wlan0 down");
    g_Setting->setWifiConfig(0);
    g_Setting->clearSsidList();
    g_Setting->onWifiSsidListChanged();
    g_WiFiManager->exitPthread();
    g_HostApd->setInitHostapdStatus(1);
    g_Setting->setWifiOpenStatus(0);
    QQmlProperty(m_WifiSettingWidgetObject,"wifiPowerStatus").write(0);
}
void WifiSettingWidgetPrivate::connectAllSlots(){
    Q_Q(WifiSettingWidget);
    connectSignalAndSlotByNamesake(g_WiFiManager, q, ARKRECEIVER(onWifiSSIDInfoChange(char *)));
    QObject::connect(g_WiFiManager, SIGNAL(onWifiGetResultFail()), q, SLOT(onWifiGetResultFail()));
    connectSignalAndSlotByNamesake(QmlWidget::instance(), q, ARKRECEIVER(onAddKeyBoardInputStr(QString )));
    connectSignalAndSlotByNamesake(QmlWidget::instance(), q, ARKRECEIVER(onSubKeyBoardInputStr(QString )));
    connectSignalAndSlotByNamesake(g_WiFiManager, q, ARKRECEIVER(onWifiConnectStatusChange(const int)));
    connectSignalAndSlotByNamesake(g_WiFiManager, q, ARKRECEIVER(onAutoConncetSsid(QString)));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onWlanClose()));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onWlanOpen()));
    connectSignalAndSlotByNamesake(g_WiFiManager, q, ARKRECEIVER(onReInputPassword()));

}
