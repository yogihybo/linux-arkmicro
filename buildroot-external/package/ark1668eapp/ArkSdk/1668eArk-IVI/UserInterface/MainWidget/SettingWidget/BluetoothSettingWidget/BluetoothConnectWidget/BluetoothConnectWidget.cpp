#include "BluetoothConnectWidget.h"
#include "BusinessLogic/carback.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <QSettings>
#include <QTimer>
namespace SourceString {
static const QString Bluetooth(QObject::tr("蓝牙:"));
}
class BluetoothConnectWidgetPrivate
{
    Q_DISABLE_COPY(BluetoothConnectWidgetPrivate)
public:
    explicit BluetoothConnectWidgetPrivate(BluetoothConnectWidget* parent);
    ~BluetoothConnectWidgetPrivate();
    void initializeObject();
    void initializeRotationTimer();
    void connectAllSlots();
    void initializeTimer();
public:
    QObject* m_BtConnectWidgetObject;
    QObject* m_BtPowerBtnObject;
    QObject* m_BtScanBtnObject;
    QObject* m_BtScanBtnIconRotationAnimationObject;
    QObject* m_RotationObject;
    QObject* m_BtConnectedBtnObject;
    QObject* m_BtConnectedBtnTextObject;
    QObject* m_BtTitleObject;
    QObject* m_MsgTextObject;
    QTimer*  m_RotationTimer;
    QTimer*  m_Timer;
    int      m_CarplayConnectedStatus;
private:
    Q_DECLARE_PUBLIC(BluetoothConnectWidget)
    BluetoothConnectWidget* const q_ptr;
};
BluetoothConnectWidget::BluetoothConnectWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new BluetoothConnectWidgetPrivate(this))
{

}

void BluetoothConnectWidget::setBluetoothConnectWidgetObject(QObject *qmlObject)
{
    Q_D(BluetoothConnectWidget);
    if(d->m_BtConnectWidgetObject == NULL)
    {
        d->m_BtConnectWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_BtPowerBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_BtScanBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_BtConnectedBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_BtConnectWidgetObject, SIGNAL(listviewItemClicked(int)),
                     this,      SLOT(onListviewItemClicked(int)),
                     type);
    QObject::connect(d->m_BtConnectWidgetObject, ARKSENDER(visibleChanged()),
                     this,      ARKRECEIVER(onVisibleChanged()),
                     type);

}

void BluetoothConnectWidget::onVisibleChanged(){
    Q_D(BluetoothConnectWidget);
    if(d->m_BtTitleObject != NULL)
    {
        QString deviceName = g_Bluetooth->localDeviceName();
        deviceName = QString(QObject::tr("蓝牙:")) + deviceName;
        QQmlProperty(d->m_BtTitleObject,"text").write(deviceName);
    }
}
void BluetoothConnectWidget::onListviewItemClicked(int index){
    Q_D(BluetoothConnectWidget);
    g_Bluetooth->connectRemoteDevice(index);
}
void BluetoothConnectWidget::onToolButtonRelease()
{
    Q_D(BluetoothConnectWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_BtPowerBtnObject)
    {
        int powerStatus = d->m_BtPowerBtnObject->property("btPowerStatus").toInt();
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
    }
    if(ptr == d->m_BtScanBtnObject){
        if(d->m_BtScanBtnIconRotationAnimationObject->property("running").toBool() == false)
        {
            g_Bluetooth->ScanNearByRemoteDevice();
            QQmlProperty(d->m_BtScanBtnIconRotationAnimationObject,"running").write(true);
        }
    }
    if(ptr == d->m_BtConnectedBtnObject)
    {
        g_Bluetooth->disconnectRemoteDevice();
    }
}
void BluetoothConnectWidget::onPowerChange(int mode)
{
    qDebug()<<"+++[BluetoothConnectWidget::onPowerChange]+++"<<mode;
    Q_D(BluetoothConnectWidget);
    if(d->m_BtPowerBtnObject != NULL)
    {
        QQmlProperty(d->m_BtPowerBtnObject,"btPowerStatus").write(mode);
    }
}
void BluetoothConnectWidget::onScanFinish()
{
    Q_D(BluetoothConnectWidget);
    qDebug()<<"++++[BluetoothConnectWidget::onScanFinish]++++"<<__LINE__;
    if(d->m_BtScanBtnIconRotationAnimationObject != NULL){
        if(d->m_RotationObject != NULL)
        {
            QQmlProperty(d->m_RotationObject,"scanFinish").write(true);
        }
    }
}
void BluetoothConnectWidget::onConnectStatusChange(const int status){
    Q_D(BluetoothConnectWidget);
    if(d->m_BtConnectWidgetObject != NULL)
    {
        QQmlProperty(d->m_BtConnectWidgetObject,"btConnectStatus").write(status);
    }
}
void BluetoothConnectWidget::onRemoteDeviceNameChange(const QString& name){
    Q_D(BluetoothConnectWidget);
    if(d->m_BtConnectedBtnTextObject != NULL)
    {
        QQmlProperty(d->m_BtConnectedBtnTextObject,"text").write(name);
    }
}
void BluetoothConnectWidget::onTimeout(){
    Q_D(BluetoothConnectWidget);
    QTimer* ptr = static_cast<QTimer*>(sender());
    if(ptr == d->m_RotationTimer)
    {

    }
    if(ptr == d->m_Timer)
    {
        if(d->m_MsgTextObject->property("visible").toBool())
        {
            QQmlProperty(d->m_MsgTextObject,"visible").write(false);
        }
    }
}
void BluetoothConnectWidget::onCarPlayConnected()
{
    Q_D(BluetoothConnectWidget);
    d->m_CarplayConnectedStatus = 1;
}
void BluetoothConnectWidget::onCarPlayExit()
{
    Q_D(BluetoothConnectWidget);
    d->m_CarplayConnectedStatus = 0;
}
BluetoothConnectWidgetPrivate::BluetoothConnectWidgetPrivate(BluetoothConnectWidget *parent)
    : q_ptr(parent)
{
    m_BtConnectWidgetObject = NULL;
    m_BtPowerBtnObject = NULL;
    m_BtScanBtnObject  = NULL;
    m_BtScanBtnIconRotationAnimationObject = NULL;
    m_RotationObject = NULL;
    m_RotationTimer  = NULL;
    m_BtConnectedBtnObject = NULL;
    m_BtConnectedBtnTextObject = NULL;
    m_BtTitleObject = NULL;
    m_MsgTextObject = NULL;
    m_Timer   = NULL;
    m_CarplayConnectedStatus = 0;
    connectAllSlots();
}

BluetoothConnectWidgetPrivate::~BluetoothConnectWidgetPrivate()
{

}
void BluetoothConnectWidgetPrivate::initializeObject()
{
    Q_Q(BluetoothConnectWidget);
    if(m_BtConnectWidgetObject != NULL)
    {
        if(m_BtPowerBtnObject == NULL)
        {
            m_BtPowerBtnObject = m_BtConnectWidgetObject->findChild<QObject*>("btPowerBtnObject");
        }

        if(m_BtScanBtnObject == NULL)
        {
            m_BtScanBtnObject = m_BtConnectWidgetObject->findChild<QObject*>("btScanBtnObject");
            if(m_BtScanBtnObject != NULL){
                if(m_BtScanBtnIconRotationAnimationObject == NULL){
                    m_BtScanBtnIconRotationAnimationObject = m_BtScanBtnObject->findChild<QObject*>("btScanBtnIconRotationAnimationObject");
                }
                if(m_RotationObject == NULL)
                {
                    m_RotationObject = m_BtScanBtnObject->findChild<QObject*>("rotationObject");
                }
            }
        }
        if(m_BtConnectedBtnObject == NULL)
        {
            m_BtConnectedBtnObject = m_BtConnectWidgetObject->findChild<QObject*>("btConnectedBtnObject");
            if(m_BtConnectedBtnTextObject == NULL)
            {
                m_BtConnectedBtnTextObject = m_BtConnectedBtnObject->findChild<QObject*>("btConnectedBtnTextObject");
            }
        }
        if(m_BtTitleObject == NULL)
        {
            m_BtTitleObject = m_BtConnectWidgetObject->findChild<QObject*>("btTitleObject");
            QString deviceName = g_Bluetooth->localDeviceName();
            deviceName = QString(QObject::tr("蓝牙:")) + deviceName;
            QQmlProperty(m_BtTitleObject,"text").write(deviceName);
        }
        if(m_MsgTextObject == NULL)
        {
            m_MsgTextObject = m_BtConnectWidgetObject->findChild<QObject*>("msgTextObject");
        }
    }
}
void BluetoothConnectWidgetPrivate::initializeRotationTimer(){
    Q_Q(BluetoothConnectWidget);
    if(m_RotationTimer == NULL)
    {
        m_RotationTimer = new QTimer(q);
        m_RotationTimer->setInterval(0);
        m_RotationTimer->setSingleShot(true);
        QObject::connect(m_RotationTimer, ARKSENDER(timeout()),
                         q,               ARKRECEIVER(onTimeout()));
    }

}
void BluetoothConnectWidgetPrivate::initializeTimer()
{
    Q_Q(BluetoothConnectWidget);
    if(m_Timer == NULL)
    {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(1000);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer, ARKSENDER(timeout()),
                         q,               ARKRECEIVER(onTimeout()));
    }
}
void BluetoothConnectWidgetPrivate::connectAllSlots()
{
    Q_Q(BluetoothConnectWidget);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onPowerChange(int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onScanFinish()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onRemoteDeviceNameChange(const QString)));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onCarPlayConnected()));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onCarPlayExit()));
}
