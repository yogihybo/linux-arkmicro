#include "PhoneLinkMsgShowWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/QmlWidget.h"
#include <QQmlProperty>
#include <QDebug>
class PhoneLinkMsgShowWidgetPrivate
{
    Q_DISABLE_COPY(PhoneLinkMsgShowWidgetPrivate)
public:
    explicit PhoneLinkMsgShowWidgetPrivate(PhoneLinkMsgShowWidget* parent);
    ~PhoneLinkMsgShowWidgetPrivate();
public:
    QObject* m_PhoneLinkMsgShowWidgetObject;
    QObject* m_LinkMsgObject;
private:
    Q_DECLARE_PUBLIC(PhoneLinkMsgShowWidget)
    PhoneLinkMsgShowWidget* const q_ptr;
};

PhoneLinkMsgShowWidget::PhoneLinkMsgShowWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new PhoneLinkMsgShowWidgetPrivate(this))
{

}
void PhoneLinkMsgShowWidget::setPhoneLinkMsgShowWidgetObject(QObject* qmlObject)
{
    Q_D(PhoneLinkMsgShowWidget);
    if(d->m_PhoneLinkMsgShowWidgetObject == NULL)
    {
        d->m_PhoneLinkMsgShowWidgetObject = qmlObject;
    }
    if(d->m_LinkMsgObject == NULL)
    {
        d->m_LinkMsgObject = d->m_PhoneLinkMsgShowWidgetObject->findChild<QObject*>("linkMsgObject");
    }
//    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
//    QObject::connect(d->m_WireBtnObject, ARKSENDER(clicked()),
//                     this,      ARKRECEIVER(onToolButtonRelease()),
//                     type);
//    QObject::connect(d->m_WirelessBtnObject, ARKSENDER(clicked()),
//                     this,      ARKRECEIVER(onToolButtonRelease()),
//                     type);
    connectSignalAndSlotByNamesake(QmlWidget::instance(), this, ARKRECEIVER(onPhoneLinkMsgShowWidgetShow(QString )));
}

void PhoneLinkMsgShowWidget::onPhoneLinkMsgShowWidgetShow(QString msg)
{
    Q_D(PhoneLinkMsgShowWidget);
    qDebug()<<"========onPhoneLinkMsgShowWidgetShow:============"<<msg;
    bool _Visible = d->m_PhoneLinkMsgShowWidgetObject->property("visible").toBool();
    QQmlProperty(d->m_LinkMsgObject,"text").write(msg);
    if(_Visible == false)
    {
        QQmlProperty(d->m_PhoneLinkMsgShowWidgetObject,"visible").write(true);
    }
}

PhoneLinkMsgShowWidgetPrivate::PhoneLinkMsgShowWidgetPrivate(PhoneLinkMsgShowWidget *parent)
    : q_ptr(parent)
{
    m_PhoneLinkMsgShowWidgetObject = NULL;
    m_LinkMsgObject = NULL;

}
PhoneLinkMsgShowWidgetPrivate::~PhoneLinkMsgShowWidgetPrivate()
{

}

