#include "BtTelMiniWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Widget.h"
#include "./../../MainWidget.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>

class BtTelMiniWidgetPrivate
{
    Q_DISABLE_COPY(BtTelMiniWidgetPrivate)
public:
    explicit BtTelMiniWidgetPrivate(BtTelMiniWidget* parent);
    ~BtTelMiniWidgetPrivate();
    void initializeObject();
    void connectAllSlots();
public:
    QObject* m_BtTelMiniWidgetObject;
    QObject* m_AnswerBtnObject;
    QObject* m_HangUpBtnObject;
    QObject* m_CallerNameObject;
    QObject* m_CallTypeObject;
private:
    Q_DECLARE_PUBLIC(BtTelMiniWidget)
    BtTelMiniWidget* const q_ptr;
};

BtTelMiniWidget::BtTelMiniWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new BtTelMiniWidgetPrivate(this))
{

}
void BtTelMiniWidget::setBtTelMiniWidgetObject(QObject* qmlObject){
    Q_D(BtTelMiniWidget);
    if(d->m_BtTelMiniWidgetObject == NULL)
    {
        d->m_BtTelMiniWidgetObject = qmlObject;
    }
    d->initializeObject();
    d->connectAllSlots();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_AnswerBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_HangUpBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_BtTelMiniWidgetObject, ARKSENDER(btTelMiniWidgetClicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

}

void BtTelMiniWidget::onToolButtonRelease()
{
    Q_D(BtTelMiniWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_AnswerBtnObject){
        g_Bluetooth->pickupPhone();
    }
    if(ptr == d->m_HangUpBtnObject){
        g_Bluetooth->hanupPhone();
    }
    else if(ptr == d->m_BtTelMiniWidgetObject)
    {
        emit g_Widget->onWidgetTypeChange(Widget::T_BluetoothTel, Widget::T_Home,QString("show"));
    }
}

void BtTelMiniWidget::onDialInfo(const int type,const QString& phone){
    Q_D(BtTelMiniWidget);
    if(type == Bluetooth::BCS_Incoming){
        QQmlProperty(d->m_AnswerBtnObject,"enabled").write(true);
        QQmlProperty(d->m_HangUpBtnObject,"enabled").write(true);
        QQmlProperty(d->m_CallerNameObject,"text").write(phone);
        QQmlProperty(d->m_CallTypeObject,"text").write("Incoming...");
    }
    else if(type == Bluetooth::BCS_Talking){
        QQmlProperty(d->m_AnswerBtnObject,"enabled").write(true);
        QQmlProperty(d->m_HangUpBtnObject,"enabled").write(true);
        QQmlProperty(d->m_CallerNameObject,"text").write(phone);
        QQmlProperty(d->m_CallTypeObject,"text").write("Talking...");
    }
    else if(type == Bluetooth::BCS_Outgoing){
        QQmlProperty(d->m_AnswerBtnObject,"enabled").write(false);
        QQmlProperty(d->m_HangUpBtnObject,"enabled").write(true);
        QQmlProperty(d->m_CallerNameObject,"text").write(phone);
        QQmlProperty(d->m_CallTypeObject,"text").write("Outgoing...");
    }
}
void BtTelMiniWidget::onConnectStatusChange(const int status){
    Q_D(BtTelMiniWidget);
    if(status <= 3)
    {
        QQmlProperty(d->m_AnswerBtnObject,"enabled").write(false);
        QQmlProperty(d->m_HangUpBtnObject,"enabled").write(false);
        QQmlProperty(d->m_CallerNameObject,"text").write("");
        QQmlProperty(d->m_CallTypeObject,"text").write("Idle Call");
    }
}
BtTelMiniWidgetPrivate::BtTelMiniWidgetPrivate(BtTelMiniWidget *parent)
    : q_ptr(parent)
{
    m_BtTelMiniWidgetObject = NULL;
    m_AnswerBtnObject = NULL;
    m_HangUpBtnObject = NULL;
    m_CallerNameObject= NULL;
    m_CallTypeObject  = NULL;
}

BtTelMiniWidgetPrivate::~BtTelMiniWidgetPrivate()
{

}
void BtTelMiniWidgetPrivate::initializeObject(){
    if(m_AnswerBtnObject == NULL)
    {
        m_AnswerBtnObject = m_BtTelMiniWidgetObject->findChild<QObject*>("answerBtnObject");
    }
    if(m_HangUpBtnObject == NULL)
    {
        m_HangUpBtnObject = m_BtTelMiniWidgetObject->findChild<QObject*>("hangUpBtnObject");
    }
    if(m_CallerNameObject == NULL)
    {
        m_CallerNameObject = m_BtTelMiniWidgetObject->findChild<QObject*>("callerNameObject");
    }
    if(m_CallTypeObject == NULL)
    {
        m_CallTypeObject = m_BtTelMiniWidgetObject->findChild<QObject*>("callTypeObject");
    }
}

void BtTelMiniWidgetPrivate::connectAllSlots()
{
    Q_Q(BtTelMiniWidget);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onDialInfo(const int,const QString&)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
}
