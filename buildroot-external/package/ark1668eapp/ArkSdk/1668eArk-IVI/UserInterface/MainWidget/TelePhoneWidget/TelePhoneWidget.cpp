#include "TelePhoneWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Bluetooth.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include "TelePhoneInComingWidget/InComingWidget.h"
#include "TelephoneOnCallWidget/OnCallWidget.h"
#include "TelephoneDialerWidget/DialerWidget.h"
class TelePhoneWidgetPrivate
{
    Q_DISABLE_COPY(TelePhoneWidgetPrivate)
public:
    explicit TelePhoneWidgetPrivate(TelePhoneWidget* parent);
    ~TelePhoneWidgetPrivate();
    void initializeObjectWidget();
    void initializeWidget();
    void initializeInComingWidget();
    void initializeOnCallWidget();
    void initializeDialerWidget();
    void connectAllSlots();
public:
    QObject* m_TelePhoneWidgetObject;
    QObject* m_CallLogBtnObject;
    QObject* m_CallLogSyncBtnObject;
    QObject* m_CallLogSyncBtnIconRotationObject;
    QObject* m_CallLogSyncBtnIconAnimationObject;
    QObject* m_PhoneBookBtnObject;
    QObject* m_PhoneBookSyncBtnObject;
    QObject* m_PhoneBookSyncBtnIconRotationObject;
    QObject* m_PhoneBookSyncBtnIconAnimationObject;
    InComingWidget* m_InComingWidget;
    OnCallWidget*   m_OnCallWidget;
    DialerWidget*   m_DialerWidget;
private:
    Q_DECLARE_PUBLIC(TelePhoneWidget)
    TelePhoneWidget* const q_ptr;
};

TelePhoneWidget::TelePhoneWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new TelePhoneWidgetPrivate(this))
{

}
void TelePhoneWidget::setTelePhoneWidgetObject(QObject* qmlObject){
    Q_D(TelePhoneWidget);
    if(d->m_TelePhoneWidgetObject == NULL)
    {
        d->m_TelePhoneWidgetObject = qmlObject;
    }
    d->initializeObjectWidget();
    d->initializeWidget();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_CallLogBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_PhoneBookBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_CallLogSyncBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_PhoneBookSyncBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void TelePhoneWidget::onToolButtonRelease()
{
    Q_D(TelePhoneWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_CallLogBtnObject)
    {
        QQmlProperty(d->m_TelePhoneWidgetObject,"telTypeIndex").write(1);
    }
    else if(ptr == d->m_PhoneBookBtnObject)
    {
        QQmlProperty(d->m_TelePhoneWidgetObject,"telTypeIndex").write(2);
    }
    else if(ptr == d->m_CallLogSyncBtnObject){
        if(d->m_CallLogSyncBtnIconAnimationObject->property("running").toBool() == false)
        {
            g_Bluetooth->synchronizeAllCallLog();
            QQmlProperty(d->m_CallLogSyncBtnIconAnimationObject,"running").write(true);
        }
        g_Bluetooth->synchronizeAllCallLog();
    }
    else if(ptr == d->m_PhoneBookSyncBtnObject){
        if(d->m_PhoneBookSyncBtnIconAnimationObject->property("running").toBool() == false)
        {
            g_Bluetooth->synchronizePhoneBook();
            QQmlProperty(d->m_PhoneBookSyncBtnIconAnimationObject,"running").write(true);
        }
    }
}
void TelePhoneWidget::onSyncPhoneBook(){
    Q_D(TelePhoneWidget);
    if(d->m_PhoneBookSyncBtnIconAnimationObject != NULL){
        if(d->m_PhoneBookSyncBtnIconRotationObject != NULL)
        {
            QQmlProperty(d->m_PhoneBookSyncBtnIconRotationObject,"scanFinish").write(true);
        }
    }
}
void TelePhoneWidget::onSyncAllCallLog(){
    Q_D(TelePhoneWidget);
    if(d->m_CallLogSyncBtnIconAnimationObject != NULL){
        if(d->m_CallLogSyncBtnIconRotationObject != NULL)
        {
            QQmlProperty(d->m_CallLogSyncBtnIconRotationObject,"scanFinish").write(true);
        }
    }
}
void TelePhoneWidget::onConnectStatusChange(const int status){
    Q_D(TelePhoneWidget);
    if(d->m_TelePhoneWidgetObject != NULL)
    {
        if(status >= 3)
        {
            QQmlProperty(d->m_CallLogBtnObject,"enabled").write(true);
            QQmlProperty(d->m_PhoneBookBtnObject,"enabled").write(true);
            QQmlProperty(d->m_CallLogSyncBtnObject,"enabled").write(true);
            QQmlProperty(d->m_PhoneBookSyncBtnObject,"enabled").write(true);
            if(d->m_TelePhoneWidgetObject->property("telTypeIndex").toInt() == 0)
            {
                QQmlProperty(d->m_TelePhoneWidgetObject,"telTypeIndex").write(1);
            }
            if(status == Bluetooth::BCS_Incoming){
                QQmlProperty(d->m_TelePhoneWidgetObject,"btConnectStatus").write(5);
            }
            else if(status == Bluetooth::BCS_Talking){
                //qDebug()<<"--------------xxxx11111------------";
                QQmlProperty(d->m_TelePhoneWidgetObject,"btConnectStatus").write(6);
            }
            else if(status == Bluetooth::BCS_Outgoing)
            {
                //qDebug()<<"--------------xxxx0000------------";
                QQmlProperty(d->m_TelePhoneWidgetObject,"btConnectStatus").write(6);
            }
            else if(status == Bluetooth::BCS_Connected)
            {
                //qDebug()<<"--------------xxxx22222------------";
                QQmlProperty(d->m_TelePhoneWidgetObject,"btConnectStatus").write(0);
            }
        }
        else{
            QQmlProperty(d->m_CallLogBtnObject,"enabled").write(false);
            QQmlProperty(d->m_PhoneBookBtnObject,"enabled").write(false);
            QQmlProperty(d->m_CallLogSyncBtnObject,"enabled").write(false);
            QQmlProperty(d->m_PhoneBookSyncBtnObject,"enabled").write(false);
            QQmlProperty(d->m_TelePhoneWidgetObject,"telTypeIndex").write(0);
            QQmlProperty(d->m_TelePhoneWidgetObject,"btConnectStatus").write(0);
        }
    }
}

TelePhoneWidgetPrivate::TelePhoneWidgetPrivate(TelePhoneWidget *parent)
    : q_ptr(parent)
{
    m_TelePhoneWidgetObject = NULL;
    m_CallLogBtnObject = NULL;
    m_CallLogSyncBtnObject =NULL;
    m_CallLogSyncBtnIconRotationObject = NULL;
    m_CallLogSyncBtnIconAnimationObject = NULL;
    m_PhoneBookBtnObject = NULL;
    m_PhoneBookSyncBtnObject = NULL;
    m_PhoneBookSyncBtnIconRotationObject  = NULL;
    m_PhoneBookSyncBtnIconAnimationObject = NULL;
    m_InComingWidget = NULL;
    m_OnCallWidget   = NULL;
    m_DialerWidget   = NULL;
    connectAllSlots();
}

TelePhoneWidgetPrivate::~TelePhoneWidgetPrivate()
{

}

void TelePhoneWidgetPrivate::initializeObjectWidget(){
    Q_Q(TelePhoneWidget);
    if(m_TelePhoneWidgetObject != NULL)
    {
        if(m_CallLogBtnObject == NULL)
        {
            m_CallLogBtnObject = m_TelePhoneWidgetObject->findChild<QObject*>("callLogBtnObject");
        }
        if(m_CallLogSyncBtnObject == NULL)
        {
            m_CallLogSyncBtnObject = m_TelePhoneWidgetObject->findChild<QObject*>("callLogRefBtnObject");
            if(m_CallLogSyncBtnObject != NULL)
            {
                if(m_CallLogSyncBtnIconRotationObject == NULL)
                {

                    m_CallLogSyncBtnIconRotationObject = m_CallLogSyncBtnObject->findChild<QObject*>("rotationObject");
                }
                if(m_CallLogSyncBtnIconAnimationObject == NULL)
                {

                    m_CallLogSyncBtnIconAnimationObject = m_CallLogSyncBtnIconRotationObject->
                            findChild<QObject*>("callLogRefBtnIconAnimationObject");
                }
            }
        }
        if(m_PhoneBookBtnObject == NULL)
        {
            m_PhoneBookBtnObject = m_TelePhoneWidgetObject->findChild<QObject*>("phoneBookBtnObject");
        }
        if(m_PhoneBookSyncBtnObject == NULL)
        {
            m_PhoneBookSyncBtnObject = m_TelePhoneWidgetObject->findChild<QObject*>("phoneBookRefBtnObject");
            if(m_PhoneBookSyncBtnObject != NULL)
            {
                if(m_PhoneBookSyncBtnIconRotationObject == NULL)
                {

                    m_PhoneBookSyncBtnIconRotationObject = m_PhoneBookSyncBtnObject->findChild<QObject*>("rotationObject");
                }
                if(m_PhoneBookSyncBtnIconAnimationObject == NULL)
                {

                    m_PhoneBookSyncBtnIconAnimationObject = m_PhoneBookSyncBtnIconRotationObject->
                            findChild<QObject*>("phoneBookRefBtnIconAnimationObject");
                }
            }
        }
    }
}
void TelePhoneWidgetPrivate::initializeWidget()
{
    initializeInComingWidget();
    initializeOnCallWidget();
    initializeDialerWidget();
}
void TelePhoneWidgetPrivate::initializeInComingWidget()
{
    Q_Q(TelePhoneWidget);
    if(m_InComingWidget == NULL)
    {
        m_InComingWidget = new InComingWidget(q);
        QObject* _InComingWidgetObject = m_TelePhoneWidgetObject->findChild<QObject*>("inCommingWidgetObject");
        m_InComingWidget->setInComingWidgetObject(_InComingWidgetObject);
    }
}
void TelePhoneWidgetPrivate::initializeOnCallWidget(){
    Q_Q(TelePhoneWidget);
    if(m_OnCallWidget == NULL)
    {
        m_OnCallWidget = new OnCallWidget (q);
        QObject* _OnCallWidgetObject = m_TelePhoneWidgetObject->findChild<QObject*>("onCallWidgetObject");
        m_OnCallWidget->setOnCallWidgetObject(_OnCallWidgetObject);
    }
}
void TelePhoneWidgetPrivate::initializeDialerWidget(){
    Q_Q(TelePhoneWidget);
    if(m_DialerWidget == NULL)
    {
        m_DialerWidget = new DialerWidget (q);
        QObject* _DialerWidgetObject = m_TelePhoneWidgetObject->findChild<QObject*>("dialerWidgetObject");
        m_DialerWidget->setDialerWidgetObject(_DialerWidgetObject);
    }
}
void TelePhoneWidgetPrivate::connectAllSlots()
{
    Q_Q(TelePhoneWidget);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onSyncPhoneBook()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onSyncAllCallLog()));
}
