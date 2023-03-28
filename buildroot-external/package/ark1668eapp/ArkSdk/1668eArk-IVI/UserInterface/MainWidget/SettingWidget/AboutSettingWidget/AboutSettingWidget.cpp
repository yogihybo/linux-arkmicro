#include "AboutSettingWidget.h"
#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include "AboutSettingNativeInfoWidget/AboutSettingNativeInfoWidget.h"
#include "AboutSettingResetWidget/AboutSettingResetWidget.h"
#include "AboutSettingRecoveryWidget/AboutSettingRecoveryWidget.h"
#include <QQmlProperty>
#include <QDebug>

class AboutSettingWidgetPrivate
{
    Q_DISABLE_COPY(AboutSettingWidgetPrivate)
public:
    explicit AboutSettingWidgetPrivate(AboutSettingWidget* parent);
    ~AboutSettingWidgetPrivate();
    void initializeAboutSettingNativeInfoWidget();
    void initializeAboutSettingResetWidget();
    void initializeAboutSettingRecoveryWidget();
public:
    QObject* m_AboutSettingWidgetObject;
    AboutSettingNativeInfoWidget* m_AboutSettingNativeInfoWidget;
    AboutSettingResetWidget* m_AboutSettingResetWidget;
    AboutSettingRecoveryWidget* m_AboutSettingRecoveryWidget;
private:
    Q_DECLARE_PUBLIC(AboutSettingWidget)
    AboutSettingWidget* const q_ptr;
};

AboutSettingWidget::AboutSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new AboutSettingWidgetPrivate(this))
{

}
void AboutSettingWidget::setAboutSettingWidgetObject(QObject* qmlObject){
    Q_D(AboutSettingWidget);
    //qDebug()<<"======setAboutSettingWidgetObject====start==========";
    if(d->m_AboutSettingWidgetObject == NULL)
    {
        d->m_AboutSettingWidgetObject = qmlObject;
    }
    d->initializeAboutSettingNativeInfoWidget();
    d->initializeAboutSettingResetWidget();
    d->initializeAboutSettingRecoveryWidget();
   // qDebug()<<"======setAboutSettingWidgetObject====end==========";
}

AboutSettingWidgetPrivate::AboutSettingWidgetPrivate(AboutSettingWidget *parent)
    : q_ptr(parent)
{
    m_AboutSettingWidgetObject = NULL;
    m_AboutSettingNativeInfoWidget = NULL;
    m_AboutSettingResetWidget = NULL;
    m_AboutSettingRecoveryWidget = NULL;
}

AboutSettingWidgetPrivate::~AboutSettingWidgetPrivate()
{

}

void AboutSettingWidgetPrivate::initializeAboutSettingNativeInfoWidget(){
    Q_Q(AboutSettingWidget);
    if(m_AboutSettingNativeInfoWidget == NULL)
    {
        m_AboutSettingNativeInfoWidget = new AboutSettingNativeInfoWidget(q);
        QObject* _AboutSettingNativeInfoWidgetObject = m_AboutSettingWidgetObject->findChild<QObject*>("nativeInfoWidgetObject");
        m_AboutSettingNativeInfoWidget->setAboutSettingNativeInfoWidgetObject(_AboutSettingNativeInfoWidgetObject);
    }
}


void AboutSettingWidgetPrivate::initializeAboutSettingResetWidget(){
    Q_Q(AboutSettingWidget);
    if(m_AboutSettingResetWidget == NULL)
    {
        m_AboutSettingResetWidget = new AboutSettingResetWidget(q);
        QObject* _AboutSettingResetWidgetObject = m_AboutSettingWidgetObject->findChild<QObject*>("resetWidgetObject");
        m_AboutSettingResetWidget->setAboutSettingResetWidgetObject(_AboutSettingResetWidgetObject);
    }
}

void AboutSettingWidgetPrivate::initializeAboutSettingRecoveryWidget(){
    Q_Q(AboutSettingWidget);
    if(m_AboutSettingRecoveryWidget == NULL)
    {
        m_AboutSettingRecoveryWidget = new AboutSettingRecoveryWidget(q);
        QObject* _AboutSettingRecoveryWidgetObject = m_AboutSettingWidgetObject->findChild<QObject*>("recoveryWidgetObject");
        m_AboutSettingRecoveryWidget->setAboutSettingRecoveryWidgetObject(_AboutSettingRecoveryWidgetObject);
    }
}
