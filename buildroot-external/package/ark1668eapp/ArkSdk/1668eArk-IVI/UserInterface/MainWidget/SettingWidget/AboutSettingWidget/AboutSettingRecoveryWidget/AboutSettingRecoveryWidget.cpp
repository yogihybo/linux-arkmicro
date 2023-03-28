#include "AboutSettingRecoveryWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
class AboutSettingRecoveryWidgetPrivate
{
    Q_DISABLE_COPY(AboutSettingRecoveryWidgetPrivate)
public:
    explicit AboutSettingRecoveryWidgetPrivate(AboutSettingRecoveryWidget* parent);
    ~AboutSettingRecoveryWidgetPrivate();
public:
    QObject* m_AboutSettingRecoveryWidgetObject;
    QObject* m_ConfirmBtnObject;
private:
    Q_DECLARE_PUBLIC(AboutSettingRecoveryWidget)
    AboutSettingRecoveryWidget* const q_ptr;
};

AboutSettingRecoveryWidget::AboutSettingRecoveryWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new AboutSettingRecoveryWidgetPrivate(this))
{

}

void AboutSettingRecoveryWidget::setAboutSettingRecoveryWidgetObject(QObject *qmlObject){
    Q_D(AboutSettingRecoveryWidget);
    if(d->m_AboutSettingRecoveryWidgetObject == NULL)
    {
        d->m_AboutSettingRecoveryWidgetObject = qmlObject;
    }
    if(d->m_ConfirmBtnObject == NULL)
    {
        d->m_ConfirmBtnObject = d->m_AboutSettingRecoveryWidgetObject->findChild<QObject*>("confirmBtnObject");
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_ConfirmBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void AboutSettingRecoveryWidget::onToolButtonRelease(){
    Q_D(AboutSettingRecoveryWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_ConfirmBtnObject)
    {
        g_Setting->executeShellCmd("rm -rf /data/*");
        g_Setting->executeShellCmd("sync");
        reboot(LINUX_REBOOT_CMD_RESTART);
    }

}
AboutSettingRecoveryWidgetPrivate::AboutSettingRecoveryWidgetPrivate(AboutSettingRecoveryWidget *parent)
    : q_ptr(parent)
{
    m_AboutSettingRecoveryWidgetObject = NULL;
    m_ConfirmBtnObject = NULL;
}

AboutSettingRecoveryWidgetPrivate::~AboutSettingRecoveryWidgetPrivate()
{

}
