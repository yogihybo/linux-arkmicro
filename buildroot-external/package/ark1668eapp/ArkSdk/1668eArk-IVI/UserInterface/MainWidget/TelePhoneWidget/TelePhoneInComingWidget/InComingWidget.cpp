#include "InComingWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Bluetooth.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <QTimer>
class InComingWidgetPrivate
{
    Q_DISABLE_COPY(InComingWidgetPrivate)
public:
    explicit InComingWidgetPrivate(InComingWidget* parent);
    ~InComingWidgetPrivate();
    void initializeObjectWidget();
    void initializeTimer();
    void connectAllSlots();
public:
    QObject* m_InComingWidgetObject;
    QObject* m_PhoneNumberObject;
    QObject* m_InConingTextObject;
    QObject* m_AnswerBtnObject;
    QObject* m_HungUpBtnObject;
    QTimer*  m_Timer;
    int      m_RecordIndex;
private:
    Q_DECLARE_PUBLIC(InComingWidget)
    InComingWidget* const q_ptr;
};

InComingWidget::InComingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new InComingWidgetPrivate(this))
{

}
void InComingWidget::setInComingWidgetObject(QObject* qmlObject)
{
    Q_D(InComingWidget);
    if(d->m_InComingWidgetObject == NULL)
    {
        d->m_InComingWidgetObject = qmlObject;
    }
    d->initializeObjectWidget();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_AnswerBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_HungUpBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void InComingWidget::onToolButtonRelease(){
    Q_D(InComingWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_AnswerBtnObject){
        g_Bluetooth->pickupPhone();
    }
    if(ptr == d->m_HungUpBtnObject){
        g_Bluetooth->hanupPhone();
    }
}
void InComingWidget::onConnectStatusChange(const int status){
    Q_D(InComingWidget);
    if(status == Bluetooth::BCS_Talking){
        d->initializeTimer();
        if(d->m_Timer->isActive())
            d->m_Timer->stop();
        d->m_RecordIndex = 0;
    }
    else if(status <= 3)
    {
        d->initializeTimer();
        if(d->m_Timer->isActive())
            d->m_Timer->stop();
        d->m_RecordIndex = 0;
    }
}
void InComingWidget::onDialInfo(const int type,const QString& phone){
    Q_D(InComingWidget);
    if(type == Bluetooth::BCS_Incoming){
        if(d->m_PhoneNumberObject != NULL)
        {
            QQmlProperty(d->m_PhoneNumberObject,"text").write(phone);
        }
        d->initializeTimer();
        d->m_Timer->start();
    }
}
void InComingWidget::onTimeout(){
    Q_D(InComingWidget);
    QTimer* ptr = static_cast<QTimer*>(sender());
    if(ptr == d->m_Timer){
        if(d->m_RecordIndex == 0)
        {
           QQmlProperty(d->m_InConingTextObject,"text").write(QString("正在接入..."));
           d->m_RecordIndex++;
        }
        else if(d->m_RecordIndex == 1)
        {
           QQmlProperty(d->m_InConingTextObject,"text").write(QString("正在接入.. "));
           d->m_RecordIndex++;
        }
        else if(d->m_RecordIndex == 2){
            QQmlProperty(d->m_InConingTextObject,"text").write(QString("正在接入.  "));
            d->m_RecordIndex++;
        }
        else if(d->m_RecordIndex == 3){
            QQmlProperty(d->m_InConingTextObject,"text").write(QString("正在接入   "));
            d->m_RecordIndex = 0;
        }
    }

}
InComingWidgetPrivate::InComingWidgetPrivate(InComingWidget *parent)
    : q_ptr(parent)
{
    m_InComingWidgetObject = NULL;
    m_PhoneNumberObject    = NULL;
    m_InConingTextObject   = NULL;
    m_AnswerBtnObject      = NULL;
    m_HungUpBtnObject      = NULL;
    m_Timer                = NULL;
    m_RecordIndex          = 0;
    connectAllSlots();
}

InComingWidgetPrivate::~InComingWidgetPrivate()
{

}

void InComingWidgetPrivate::initializeObjectWidget(){
    Q_Q(InComingWidget);
    if(m_InComingWidgetObject != NULL)
    {
        if(m_PhoneNumberObject == NULL)
        {
            m_PhoneNumberObject = m_InComingWidgetObject->findChild<QObject*>("phoneNumberObject");
        }
        if(m_InConingTextObject == NULL)
        {
            m_InConingTextObject = m_InComingWidgetObject->findChild<QObject*>("InConingTextObject");
        }
        if(m_AnswerBtnObject == NULL)
        {
            m_AnswerBtnObject = m_InComingWidgetObject->findChild<QObject*>("answerBtnObject");
        }
        if(m_HungUpBtnObject == NULL)
        {
            m_HungUpBtnObject = m_InComingWidgetObject->findChild<QObject*>("hungUpBtnObject");
        }
    }
}
void InComingWidgetPrivate::initializeTimer(){
    Q_Q(InComingWidget);
    if(m_Timer == NULL){
        m_Timer = new QTimer(q);
        m_Timer->setInterval(300);
        //m_Timer->setSingleShot(false);
        QObject::connect(m_Timer, ARKSENDER(timeout()),
                         q,                 ARKRECEIVER(onTimeout()));
    }
}
void InComingWidgetPrivate::connectAllSlots()
{
    Q_Q(InComingWidget);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onDialInfo(const int,const QString&)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
}
