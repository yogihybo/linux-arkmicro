#include "DialerWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Bluetooth.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
class DialerWidgetPrivate
{
    Q_DISABLE_COPY(DialerWidgetPrivate)
public:
    explicit DialerWidgetPrivate(DialerWidget* parent);
    ~DialerWidgetPrivate();
    void initializeObjectWidget();
    void connectAllSlots();
public:
    QObject* m_DialerWidgetObject;
    QObject* m_PhoneNumberObject;
    QObject* m_AnswerBtnObject;
private:
    Q_DECLARE_PUBLIC(DialerWidget)
    DialerWidget* const q_ptr;
};

DialerWidget::DialerWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new DialerWidgetPrivate(this))
{

}

void DialerWidget::setDialerWidgetObject(QObject* qmlObject){
    Q_D(DialerWidget);
    if(d->m_DialerWidgetObject == NULL)
    {
        d->m_DialerWidgetObject = qmlObject;
    }
    d->initializeObjectWidget();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_AnswerBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_DialerWidgetObject, ARKSENDER(listViewItemClicked(QString)),
                     this,      ARKRECEIVER(onListViewItemClicked(QString)),
                     type);
}
void DialerWidget::onListViewItemClicked(QString phoneNumber)
{
    Q_D(DialerWidget);
    QQmlProperty(d->m_DialerWidgetObject,"phoneNumberStr").write(phoneNumber);
    QQmlProperty(d->m_PhoneNumberObject,"text").write(phoneNumber);
}
void DialerWidget::onToolButtonRelease(){
    Q_D(DialerWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_AnswerBtnObject){
        QString _PhoneNumber = d->m_PhoneNumberObject->property("text").toString();
        if(_PhoneNumber.size() > 0){
            g_Bluetooth->dialPhone(_PhoneNumber);
        }
        else{
            g_Bluetooth->redialLastPhone();
        }
    }
}
void DialerWidget::onHangUpPhone()
{
    Q_D(DialerWidget);
    QQmlProperty(d->m_PhoneNumberObject,"text").write(QString(""));
    QQmlProperty(d->m_DialerWidgetObject,"phoneNumberStr").write(QString(""));
}
void DialerWidget::onConnectStatusChange(const int status){
    Q_D(DialerWidget);
    if(status < 3)
    {
        QQmlProperty(d->m_PhoneNumberObject,"text").write(QString(""));
        QQmlProperty(d->m_DialerWidgetObject,"phoneNumberStr").write(QString(""));
    }
}
DialerWidgetPrivate::DialerWidgetPrivate(DialerWidget *parent)
    : q_ptr(parent)
{
    m_DialerWidgetObject = NULL;
    m_PhoneNumberObject  = NULL;
    m_AnswerBtnObject    = NULL;
    connectAllSlots();
}

DialerWidgetPrivate::~DialerWidgetPrivate()
{

}

void DialerWidgetPrivate::initializeObjectWidget(){
    Q_Q(DialerWidget);
    if(m_DialerWidgetObject != NULL)
    {
        if(m_PhoneNumberObject == NULL)
        {
            m_PhoneNumberObject = m_DialerWidgetObject->findChild<QObject*>("phoneNumberObject");
        }
        if(m_AnswerBtnObject == NULL)
        {
            m_AnswerBtnObject = m_DialerWidgetObject->findChild<QObject*>("answerBtnObject");
        }
    }
}

void DialerWidgetPrivate::connectAllSlots()
{
    Q_Q(DialerWidget);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onHangUpPhone()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
}


