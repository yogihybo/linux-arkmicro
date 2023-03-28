#include "PhoneLinkWidget.h"
#include "CarLifeCarPlayWidget/CarLifeCarPlayWidget.h"
#include "PhoneLinkMsgWidget/PhoneLinkMsgWidget.h"
#include "AutoCarPlayWidget/AutoCarPlayWidget.h"
#include "EcLinkWidget/EcLinkWidget.h"
#include "HicarWidget/HicarWidget.h"
#include "PhoneLinkMsgShowWidget.h"
#include <QDebug>
#include <QSettings>
#include <QQmlProperty>
static const QString PhoneLinkConfig("/data/PhoneLink.ini");
class PhoneLinkWidgetPrivate
{
    Q_DISABLE_COPY(PhoneLinkWidgetPrivate)
public:
    explicit PhoneLinkWidgetPrivate(PhoneLinkWidget* parent);
    ~PhoneLinkWidgetPrivate();
    void initializeCarLifeCarPlayWidget();
    void initializeAutoCarPlayWidget();
    void initializeEcLinkWidget();
    void initializeHicarWidget();
    void initPhoneLinkMsgWidget();
    void initPhoneLinkMsgShowWidget();
public:
    QObject* m_PhoneLinkWidgetObject;
    CarLifeCarPlayWidget* m_CarLifeCarPlayWidget;
    AutoCarPlayWidget* m_AutoCarPlayWidget;
    PhoneLinkMsgWidget* m_PhoneLinkMsgWidget;
    EcLinkWidget* m_EcLinkWidget;
    HicarWidget* m_HicarWidget;
    PhoneLinkMsgShowWidget* m_PhoneLinkMsgShowWidget;
private:
    Q_DECLARE_PUBLIC(PhoneLinkWidget)
    PhoneLinkWidget* const q_ptr;
};

PhoneLinkWidget::PhoneLinkWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new PhoneLinkWidgetPrivate(this))
{

}
void PhoneLinkWidget::setPhoneLinkWidgetObject(QObject* qmlObject){

    Q_D(PhoneLinkWidget);
    if(d->m_PhoneLinkWidgetObject == NULL)
    {
        d->m_PhoneLinkWidgetObject = qmlObject;
    }
    QSettings * PhoneLinkcfgSetFile = new QSettings(PhoneLinkConfig,QSettings::IniFormat);
    int _PhoneLinkType  = PhoneLinkcfgSetFile->value("PhoneLink").toString().toInt();
    delete PhoneLinkcfgSetFile;
    QQmlProperty(d->m_PhoneLinkWidgetObject,"phoneLinkType").write(_PhoneLinkType);
    if(_PhoneLinkType == 0){
        d->initializeCarLifeCarPlayWidget();
    }
    else if(_PhoneLinkType == 1)
    {
        d->initializeAutoCarPlayWidget();
    }
    else if(_PhoneLinkType == 2)
    {
        d->initializeEcLinkWidget();
    }
    else if(_PhoneLinkType == 3)
    {
        d->initializeHicarWidget();
    }
    if(_PhoneLinkType != 3){
        d->initPhoneLinkMsgWidget();
    }
    d->initPhoneLinkMsgShowWidget();
}
PhoneLinkWidgetPrivate::PhoneLinkWidgetPrivate(PhoneLinkWidget *parent)
    : q_ptr(parent)
{
    m_PhoneLinkWidgetObject = NULL;
    m_CarLifeCarPlayWidget  = NULL;
    m_AutoCarPlayWidget     = NULL;
    m_PhoneLinkMsgWidget    = NULL;
    m_EcLinkWidget = NULL;
    m_HicarWidget = NULL;
    m_PhoneLinkMsgShowWidget = NULL;
}

PhoneLinkWidgetPrivate::~PhoneLinkWidgetPrivate()
{

}
void PhoneLinkWidgetPrivate::initializeCarLifeCarPlayWidget()
{
    Q_Q(PhoneLinkWidget);
    if(m_CarLifeCarPlayWidget == NULL)
    {
        m_CarLifeCarPlayWidget = new CarLifeCarPlayWidget(q);
        QObject* _CarLifeCarPlayWidgetObject = m_PhoneLinkWidgetObject->findChild<QObject*>("CarLifeCarPlayWidgetObject");
        m_CarLifeCarPlayWidget->setCarLifeCarPlayWidgetObject(_CarLifeCarPlayWidgetObject);
        m_CarLifeCarPlayWidget->setCarLifeCarPlayWidgetParendObject(m_PhoneLinkWidgetObject);
    }
}

void PhoneLinkWidgetPrivate::initializeAutoCarPlayWidget(){
    Q_Q(PhoneLinkWidget);
    //qDebug()<<"=========initializeAutoCarPlayWidget=start======";
    if(m_AutoCarPlayWidget == NULL)
    {
        m_AutoCarPlayWidget = new AutoCarPlayWidget(q);
        QObject* _AutoCarPlayWidgetObject = m_PhoneLinkWidgetObject->findChild<QObject*>("autoCarplayWidgetObject");
        m_AutoCarPlayWidget->setAutoCarPlayWidgetObject(_AutoCarPlayWidgetObject);
        m_AutoCarPlayWidget->setAutoCarPlayWidgetParendObject(m_PhoneLinkWidgetObject);
    }
   // qDebug()<<"=========initializeAutoCarPlayWidget====end===";
}

void PhoneLinkWidgetPrivate::initializeEcLinkWidget(){
    Q_Q(PhoneLinkWidget);
   // qDebug()<<"=========initializeEcLinkWidget=start======";
    if(m_EcLinkWidget == NULL)
    {
        m_EcLinkWidget = new EcLinkWidget(q);
        QObject* _EcLinkWidgetObject = m_PhoneLinkWidgetObject->findChild<QObject*>("ecLinkWidgetObject");
        m_EcLinkWidget->setEcLinkWidgetObject(_EcLinkWidgetObject);
        m_EcLinkWidget->setEcLinkWidgetParentObject(m_PhoneLinkWidgetObject);
    }
    //qDebug()<<"=========initializeEcLinkWidget====end===";
}
void PhoneLinkWidgetPrivate::initializeHicarWidget(){
    Q_Q(PhoneLinkWidget);
    //qDebug()<<"=========initializeHicarWidget=start======";
    if(m_HicarWidget == NULL)
    {
        m_HicarWidget = new HicarWidget(q);
        QObject* _HicarWidgetObject = m_PhoneLinkWidgetObject->findChild<QObject*>("hiCarWidgetObject");
        m_HicarWidget->setHicarWidgetObject(_HicarWidgetObject);
        m_HicarWidget->setHicarWidgetParentObject(m_PhoneLinkWidgetObject);
    }
    //qDebug()<<"=========initializeHicarWidget====end===";
}

void PhoneLinkWidgetPrivate::initPhoneLinkMsgWidget()
{
    Q_Q(PhoneLinkWidget);
    if(m_PhoneLinkMsgWidget == NULL)
    {
        m_PhoneLinkMsgWidget = new PhoneLinkMsgWidget(q);
        QObject* _PhoneLinkMsgWidget = m_PhoneLinkWidgetObject->findChild<QObject*>("PhoneLinkMsgWidgetObject");
        m_PhoneLinkMsgWidget->setPhoneLinkMsgWidgetObject(_PhoneLinkMsgWidget);
    }
}
void PhoneLinkWidgetPrivate::initPhoneLinkMsgShowWidget()
{
    Q_Q(PhoneLinkWidget);
    if(m_PhoneLinkMsgShowWidget == NULL)
    {
        m_PhoneLinkMsgShowWidget = new PhoneLinkMsgShowWidget(q);
        QObject* _PhoneLinkMsgShowWidget = m_PhoneLinkWidgetObject->findChild<QObject*>("phoneLinkMsgShowWidgetObject");
        m_PhoneLinkMsgShowWidget->setPhoneLinkMsgShowWidgetObject(_PhoneLinkMsgShowWidget);
    }
}
