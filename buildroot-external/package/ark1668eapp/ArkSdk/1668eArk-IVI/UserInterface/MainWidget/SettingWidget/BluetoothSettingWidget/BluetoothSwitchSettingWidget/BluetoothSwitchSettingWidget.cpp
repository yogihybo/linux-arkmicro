#include "BluetoothSwitchSettingWidget.h"
#include "BusinessLogic/carback.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <QSettings>
#include <QTimer>
namespace SourceString {
static const QString DeviceName = QString(QObject::tr("设备名称:"));
static const QString PinCode =    QString(QObject::tr("Pin码:"));
}
class BluetoothSwitchSettingWidgetPrivate
{
    Q_DISABLE_COPY(BluetoothSwitchSettingWidgetPrivate)
public:
    explicit BluetoothSwitchSettingWidgetPrivate(BluetoothSwitchSettingWidget* parent);
    ~BluetoothSwitchSettingWidgetPrivate();
    void initializeObject();
    void initializeWidget();
    void setBtInfomation();
    void connectAllSlots();
    void initializeTimer();
public:
    QObject* m_BtSwitchSettingWidgetObject;
    QObject* m_DeviceNameObject;
    QObject* m_PinCodeObject;
    QObject* m_PowerBtnObject;
    QObject* m_AutoConnectObject;
    QObject* m_AutoAnswerObject;
    QObject* m_MsgTextObject;
    QTimer* m_Timer;
    int m_CarplayConnectedStatus;
private:
    Q_DECLARE_PUBLIC(BluetoothSwitchSettingWidget)
    BluetoothSwitchSettingWidget* const q_ptr;
};

BluetoothSwitchSettingWidget::BluetoothSwitchSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new BluetoothSwitchSettingWidgetPrivate(this))
{

}

void BluetoothSwitchSettingWidget::setBtSwitchSettingWidgetObject(QObject* qmlObject){
    Q_D(BluetoothSwitchSettingWidget);
    if(d->m_BtSwitchSettingWidgetObject == NULL)
    {
        //qDebug()<<"===BluetoothSwitchSettingWidget::setBtSwitchSettingWidgetObject===";
        d->m_BtSwitchSettingWidgetObject = qmlObject;
    }
    d->initializeObject();
    //d->initializeWidget();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_PowerBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_AutoConnectObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_AutoAnswerObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_BtSwitchSettingWidgetObject, ARKSENDER(visibleChanged()),
                     this,      ARKRECEIVER(onVisibleChanged()),
                     type);


}
void BluetoothSwitchSettingWidget::onToolButtonRelease()
{
    Q_D(BluetoothSwitchSettingWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_PowerBtnObject){
        int powerStatus = d->m_PowerBtnObject->property("powerStatus").toInt();
        if(powerStatus == 0 && d->m_CarplayConnectedStatus == 0)
        {
            g_Bluetooth->powerOn();
        }
        else if(powerStatus == 0 && d->m_CarplayConnectedStatus == 1)
        {
            if(d->m_MsgTextObject->property("visible").toBool() == false)
            {
                QQmlProperty(d->m_MsgTextObject,"visible").write(true);
                d->initializeTimer();
                if(d->m_Timer->isActive())
                {
                    d->m_Timer->stop();
                }
                d->m_Timer->start();
            }
        }
        else if(powerStatus == 1){
            g_Bluetooth->powerOff();
        }
    }else if(ptr == d->m_AutoConnectObject){
        int autoConnectStatus = d->m_AutoConnectObject->property("autoConnectStatus").toInt();
        if(autoConnectStatus == 0)
        {
            g_Bluetooth->autoConnectOn();
            g_Bluetooth->readHFPCFGValue();
        }
        else if(autoConnectStatus == 1){
            g_Bluetooth->autoConnectOff();
            g_Bluetooth->readHFPCFGValue();
        }

    }else if(ptr == d->m_AutoAnswerObject){
        int autoAnswerStatus = d->m_AutoAnswerObject->property("autoAnswerStatus").toInt();
        if(autoAnswerStatus == 0)
        {
            g_Bluetooth->autoAnswerOn();
            g_Bluetooth->readHFPCFGValue();
        }
        else if(autoAnswerStatus == 1){
            g_Bluetooth->autoAnswerOff();
            g_Bluetooth->readHFPCFGValue();
        }
    }
}
void BluetoothSwitchSettingWidget::onVisibleChanged()
{
    Q_D(BluetoothSwitchSettingWidget);
    static bool _FirstShow(true);
    if(d->m_BtSwitchSettingWidgetObject != NULL)
    {
        if(d->m_BtSwitchSettingWidgetObject->property("visible").toBool())
        {
            //qDebug()<<"+++++++onVisibleChanged++++++++" ;
            g_Bluetooth->readHFPCFGValue();
            d->setBtInfomation();
            if(_FirstShow)
            {
                g_Bluetooth->QueryPairedList();
                _FirstShow = false;
            }
        }
    }
}
void BluetoothSwitchSettingWidget::onPowerChange(int mode)
{
    qDebug()<<"+++[BluetoothSwitchSettingWidget::onPowerChange]+++"<<mode;
    Q_D(BluetoothSwitchSettingWidget);
    if(d->m_PowerBtnObject != NULL)
    {
        QQmlProperty(d->m_PowerBtnObject,"powerStatus").write(mode);
    }
}

void BluetoothSwitchSettingWidget::onAutoConnectChange(const int mode){
    Q_D(BluetoothSwitchSettingWidget);
    qDebug()<<"+++[BluetoothSwitchSettingWidget::onAutoConnectChange]+++"<<mode;
    if(d->m_AutoConnectObject != NULL)
    {
        QQmlProperty(d->m_AutoConnectObject,"autoConnectStatus").write(mode);
    }
}
void BluetoothSwitchSettingWidget::onAutoAnswerChange(const  int mode){
    Q_D(BluetoothSwitchSettingWidget);
    qDebug()<<"+++[BluetoothSwitchSettingWidget::onAutoAnswerChange]+++"<<mode;
    if(d->m_AutoAnswerObject != NULL)
    {
        QQmlProperty(d->m_AutoAnswerObject,"autoAnswerStatus").write(mode);
    }
}
void BluetoothSwitchSettingWidget::onTimeout()
{
    Q_D(BluetoothSwitchSettingWidget);
    if(d->m_MsgTextObject->property("visible").toBool())
    {
        QQmlProperty(d->m_MsgTextObject,"visible").write(false);
    }
}
void BluetoothSwitchSettingWidget::onCarPlayConnected()
{
    Q_D(BluetoothSwitchSettingWidget);
    d->m_CarplayConnectedStatus = 1;
}
void BluetoothSwitchSettingWidget::onCarPlayExit()
{
    Q_D(BluetoothSwitchSettingWidget);
    d->m_CarplayConnectedStatus = 0;
}
BluetoothSwitchSettingWidgetPrivate::BluetoothSwitchSettingWidgetPrivate(BluetoothSwitchSettingWidget *parent)
    : q_ptr(parent)
{
    m_BtSwitchSettingWidgetObject = NULL;
    m_DeviceNameObject = NULL;
    m_PinCodeObject    = NULL;
    m_PowerBtnObject   = NULL;
    m_AutoConnectObject= NULL;
    m_AutoAnswerObject = NULL;
    m_MsgTextObject    = NULL;
    m_Timer            = NULL;
    m_CarplayConnectedStatus = 0;
    connectAllSlots();
}

BluetoothSwitchSettingWidgetPrivate::~BluetoothSwitchSettingWidgetPrivate()
{

}
void BluetoothSwitchSettingWidgetPrivate::initializeWidget()
{
    setBtInfomation();
}

void BluetoothSwitchSettingWidgetPrivate::setBtInfomation()
{
    Q_Q(BluetoothSwitchSettingWidget);
    QString deviceName = g_Bluetooth->localDeviceName();
    deviceName = QString(QObject::tr("设备名称:")) + deviceName;
    QQmlProperty(m_DeviceNameObject,"text").write(deviceName);
    QString pinCode = QString(QObject::tr("Pin码:")) + g_Bluetooth->pinCode();
    QQmlProperty(m_PinCodeObject,"text").write(pinCode);
}
void BluetoothSwitchSettingWidgetPrivate::initializeObject()
{
    Q_Q(BluetoothSwitchSettingWidget);
    if(m_BtSwitchSettingWidgetObject != NULL)
    {
        if(m_DeviceNameObject == NULL)
        {
            m_DeviceNameObject = m_BtSwitchSettingWidgetObject->findChild<QObject*>("btDeviceNameObject");
        }
        if(m_PinCodeObject == NULL)
        {
            m_PinCodeObject = m_BtSwitchSettingWidgetObject->findChild<QObject*>("btPinCodeObject");
        }
        if(m_PowerBtnObject == NULL)
        {
            m_PowerBtnObject = m_BtSwitchSettingWidgetObject->findChild<QObject*>("powerBtnObject");
        }
        if(m_AutoConnectObject == NULL)
        {
            m_AutoConnectObject = m_BtSwitchSettingWidgetObject->findChild<QObject*>("autoConnectBtnObject");
        }
        if(m_AutoAnswerObject == NULL)
        {
            m_AutoAnswerObject = m_BtSwitchSettingWidgetObject->findChild<QObject*>("autoAnswerBtnObject");
        }
        if(m_MsgTextObject == NULL)
        {
            m_MsgTextObject = m_BtSwitchSettingWidgetObject->findChild<QObject*>("msgTextObject");
        }
    }
}

void BluetoothSwitchSettingWidgetPrivate::initializeTimer(){
    Q_Q(BluetoothSwitchSettingWidget);
    if(m_Timer == NULL)
    {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(1000);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer, ARKSENDER(timeout()),
                         q,               ARKRECEIVER(onTimeout()));
    }
}
void BluetoothSwitchSettingWidgetPrivate::connectAllSlots()
{
    Q_Q(BluetoothSwitchSettingWidget);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onPowerChange(int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onAutoConnectChange(const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onAutoAnswerChange(const int)));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onCarPlayConnected()));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onCarPlayExit()));
}
