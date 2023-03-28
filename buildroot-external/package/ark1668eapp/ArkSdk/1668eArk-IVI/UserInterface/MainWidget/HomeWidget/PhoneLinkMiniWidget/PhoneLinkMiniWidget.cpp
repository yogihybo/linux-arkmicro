#include "PhoneLinkMiniWidget.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/carlink.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>

class PhoneLinkMiniWidgetPrivate
{
    Q_DISABLE_COPY(PhoneLinkMiniWidgetPrivate)
public:
    explicit PhoneLinkMiniWidgetPrivate(PhoneLinkMiniWidget* parent);
    ~PhoneLinkMiniWidgetPrivate();
    void initializeObject();
public:
    QObject* m_PhoneLinkMiniWidgetObject;
    QObject* m_LinkTypeNameObject;
private:
    Q_DECLARE_PUBLIC(PhoneLinkMiniWidget)
    PhoneLinkMiniWidget* const q_ptr;
};

PhoneLinkMiniWidget::PhoneLinkMiniWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new PhoneLinkMiniWidgetPrivate(this))
{

}
void PhoneLinkMiniWidget::setPhoneLinkMiniWidgetObject(QObject* qmlObject){
    Q_D(PhoneLinkMiniWidget);
    if(d->m_PhoneLinkMiniWidgetObject == NULL)
    {
        d->m_PhoneLinkMiniWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_PhoneLinkMiniWidgetObject, SIGNAL(phoneLinkMiniWidgetClicked()),
                     this,      SLOT(onPhoneLinkMiniWidgetClicked()),
                     type);
    QObject::connect(g_Link, SIGNAL(onLinkStatus(int,int,int)),
                     this,   SLOT(onLinkStatus(int,int,int)));
}

void PhoneLinkMiniWidget::onLinkStatus(int type, int mode, int status)
{
    Q_D(PhoneLinkMiniWidget);
   if(status == DBUS_BACKGROUND){
       if(type == Carplay){
           QQmlProperty(d->m_LinkTypeNameObject,"text").write("Carplay");
       }
       else if(type == Carlife){
           QQmlProperty(d->m_LinkTypeNameObject,"text").write("Carlife");
       }
       else if(type == Android_Auto){
           QQmlProperty(d->m_LinkTypeNameObject,"text").write("Android_Auto");
       }
       else if(type == HiCar){
           QQmlProperty(d->m_LinkTypeNameObject,"text").write("HiCar");
       }
    }
}

void PhoneLinkMiniWidget::onPhoneLinkMiniWidgetClicked()
{
    emit g_Widget->onWidgetTypeChange(Widget::T_PhoneLink, Widget::T_Home,QString("show"));
}

PhoneLinkMiniWidgetPrivate::PhoneLinkMiniWidgetPrivate(PhoneLinkMiniWidget *parent)
    : q_ptr(parent)
{
    m_PhoneLinkMiniWidgetObject = NULL;
    m_LinkTypeNameObject = NULL;
}

PhoneLinkMiniWidgetPrivate::~PhoneLinkMiniWidgetPrivate()
{


}
void PhoneLinkMiniWidgetPrivate::initializeObject(){

    if(m_LinkTypeNameObject == NULL)
    {
        m_LinkTypeNameObject = m_PhoneLinkMiniWidgetObject->findChild<QObject*>("linkTypeNameObject");
    }

}
