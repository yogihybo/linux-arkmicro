#include "AboutSettingResetWidget.h"
#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <unistd.h>
#include<linux/reboot.h>
#include<sys/reboot.h>
class AboutSettingResetWidgetPrivate
{
    Q_DISABLE_COPY(AboutSettingResetWidgetPrivate)
public:
    explicit AboutSettingResetWidgetPrivate(AboutSettingResetWidget* parent);
    ~AboutSettingResetWidgetPrivate();
    void initializeObject();
public:
    QObject* m_AboutSettingResetWidgetObject;
    QObject* m_CancelBtnObject;
    QObject* m_ConfirmBtnObject;
private:
    Q_DECLARE_PUBLIC(AboutSettingResetWidget)
    AboutSettingResetWidget* const q_ptr;
};

AboutSettingResetWidget::AboutSettingResetWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new AboutSettingResetWidgetPrivate(this))
{

}

void AboutSettingResetWidget::setAboutSettingResetWidgetObject(QObject* qmlObject){
    Q_D(AboutSettingResetWidget);
    if(d->m_AboutSettingResetWidgetObject == NULL)
    {
        d->m_AboutSettingResetWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_CancelBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_ConfirmBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

}
void AboutSettingResetWidget::onToolButtonRelease(){
    Q_D(AboutSettingResetWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_CancelBtnObject){

    }
    else if(ptr == d->m_ConfirmBtnObject){
        reboot(LINUX_REBOOT_CMD_RESTART);
    }
}
AboutSettingResetWidgetPrivate::AboutSettingResetWidgetPrivate(AboutSettingResetWidget *parent)
    : q_ptr(parent)
{
    m_AboutSettingResetWidgetObject = NULL;
    m_CancelBtnObject = NULL;
    m_ConfirmBtnObject = NULL;
}

AboutSettingResetWidgetPrivate::~AboutSettingResetWidgetPrivate()
{

}

void AboutSettingResetWidgetPrivate::initializeObject(){
    if(m_CancelBtnObject == NULL)
    {
        m_CancelBtnObject = m_AboutSettingResetWidgetObject->findChild<QObject*>("cancelBtnObject");
    }
    if(m_ConfirmBtnObject == NULL)
    {
        m_ConfirmBtnObject = m_AboutSettingResetWidgetObject->findChild<QObject*>("confirmBtnObject");
    }
}
