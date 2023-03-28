#include "HomeWidget.h"
#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include "BtTelMiniWidget/BtTelMiniWidget.h"
#include "MusicMiniWidget/MusicMiniWidget.h"
#include "PhoneLinkMiniWidget/PhoneLinkMiniWidget.h"
#include "AuxMiniWidget/AuxMiniWidget.h"
#include <QQmlProperty>
#include <QDebug>

class HomeWidgetPrivate
{
    Q_DISABLE_COPY(HomeWidgetPrivate)
public:
    explicit HomeWidgetPrivate(HomeWidget* parent);
    ~HomeWidgetPrivate();
    void initializeObject();
    void initializeBtTelMiniWidget();
    void initializeMusicMiniWidget();
    void initializePhoneLinkMiniWidget();
    void initializeAuxMiniWidget();
public:
    QObject* m_HomeWidgetObject;
    QObject* m_SwipeViewObject;
    QObject* m_FirstHomeWidgetObject;
    BtTelMiniWidget* m_BtTelMiniWidget;
    MusicMiniWidget* m_MusicMiniWidget;
    PhoneLinkMiniWidget* m_PhoneLinkMiniWidget;
    AuxMiniWidget* m_AuxMiniWidget;
private:
    Q_DECLARE_PUBLIC(HomeWidget)
    HomeWidget* const q_ptr;
};

HomeWidget::HomeWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new HomeWidgetPrivate(this))
{

}

void HomeWidget::setHomeWidgetObject(QObject* qmlObject){
    Q_D(HomeWidget);
    if(d->m_HomeWidgetObject == NULL)
    {
        d->m_HomeWidgetObject = qmlObject;
    }
    d->initializeObject();
    d->initializeBtTelMiniWidget();
    d->initializeMusicMiniWidget();
    d->initializePhoneLinkMiniWidget();
    d->initializeAuxMiniWidget();
}
HomeWidgetPrivate::HomeWidgetPrivate(HomeWidget *parent)
    : q_ptr(parent)
{
    m_HomeWidgetObject = NULL;
    m_SwipeViewObject  = NULL;
    m_FirstHomeWidgetObject = NULL;
    m_BtTelMiniWidget  = NULL;
    m_MusicMiniWidget  = NULL;
    m_PhoneLinkMiniWidget = NULL;
    m_AuxMiniWidget = NULL;
}

HomeWidgetPrivate::~HomeWidgetPrivate()
{

}
void HomeWidgetPrivate::initializeObject()
{
    if(m_SwipeViewObject == NULL)
    {
        m_SwipeViewObject = m_HomeWidgetObject->findChild<QObject*>("swipeViewObject");
    }

    if(m_FirstHomeWidgetObject == NULL)
    {
        m_FirstHomeWidgetObject = m_SwipeViewObject->findChild<QObject*>("firstHomeWidgetObject");
    }
}

void HomeWidgetPrivate::initializeBtTelMiniWidget()
{
    Q_Q(HomeWidget);
    if(m_BtTelMiniWidget == NULL)
    {
        m_BtTelMiniWidget = new BtTelMiniWidget(q);
        QObject* _BtTelMiniWidgetObject = m_FirstHomeWidgetObject->findChild<QObject*>("btTelMiniWidgetObject");
        m_BtTelMiniWidget->setBtTelMiniWidgetObject(_BtTelMiniWidgetObject);
    }
}

void HomeWidgetPrivate::initializeMusicMiniWidget()
{
    Q_Q(HomeWidget);
    if(m_MusicMiniWidget == NULL)
    {
        m_MusicMiniWidget = new MusicMiniWidget(q);
        QObject* _MusicMiniWidgetObject = m_FirstHomeWidgetObject->findChild<QObject*>("musicMiniWidgetObject");
        m_MusicMiniWidget->setMusicMiniWidgetObject(_MusicMiniWidgetObject);
    }
}


void HomeWidgetPrivate::initializePhoneLinkMiniWidget()
{
    Q_Q(HomeWidget);
    if(m_PhoneLinkMiniWidget == NULL)
    {
        m_PhoneLinkMiniWidget = new PhoneLinkMiniWidget(q);
        QObject* _PhoneLinkMiniWidgetObject = m_FirstHomeWidgetObject->findChild<QObject*>("phoneLinkMiniWidgetObject");
        m_PhoneLinkMiniWidget->setPhoneLinkMiniWidgetObject(_PhoneLinkMiniWidgetObject);
    }

}
void HomeWidgetPrivate::initializeAuxMiniWidget(){
    Q_Q(HomeWidget);
    if(m_AuxMiniWidget == NULL)
    {
        m_AuxMiniWidget = new AuxMiniWidget(q);
        QObject* _AuxMiniWidgetObject = m_FirstHomeWidgetObject->findChild<QObject*>("auxMiniWidgetObject");
        m_AuxMiniWidget->setAuxMiniWidgetObject(_AuxMiniWidgetObject);
    }

}
