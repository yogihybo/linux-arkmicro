#include "MoreSettingPhoneLinkWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
static const QString PhoneLinkConfig("/data/PhoneLink.ini");
class MoreSettingPhoneLinkWidgetPrivate
{
    Q_DISABLE_COPY(MoreSettingPhoneLinkWidgetPrivate)
public:
    explicit MoreSettingPhoneLinkWidgetPrivate(MoreSettingPhoneLinkWidget* parent);
    ~MoreSettingPhoneLinkWidgetPrivate();
    void initializeObject();
public:
    QObject* m_MoreSettingPhoneLinkWidgetObject;
    QObject*  m_CancleBtnObject;
    QObject* m_ConfirmBtnObject;
    int m_LastIndex;
private:
    Q_DECLARE_PUBLIC(MoreSettingPhoneLinkWidget)
    MoreSettingPhoneLinkWidget* const q_ptr;
};

MoreSettingPhoneLinkWidget::MoreSettingPhoneLinkWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new MoreSettingPhoneLinkWidgetPrivate(this))
{

}
void MoreSettingPhoneLinkWidget::setMoreSettingPhoneLinkWidgetObject(QObject *qmlObject)
{
    Q_D(MoreSettingPhoneLinkWidget);
    if(d->m_MoreSettingPhoneLinkWidgetObject == NULL)
    {
        d->m_MoreSettingPhoneLinkWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_MoreSettingPhoneLinkWidgetObject, ARKSENDER(visibleChanged()),
                     this,      ARKRECEIVER(onVisibleChanged()),
                     type);
    QObject::connect(d->m_CancleBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_ConfirmBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}

void MoreSettingPhoneLinkWidget::onVisibleChanged(){
    Q_D(MoreSettingPhoneLinkWidget);
    bool _Visible  = d->m_MoreSettingPhoneLinkWidgetObject->property("visible").toBool();
    if(_Visible){
        QFile _PhoneLinkcfgFile(PhoneLinkConfig);
        int _PhoneLinkType = 0;
        if(_PhoneLinkcfgFile.exists())
        {
            QSettings * PhoneLinkcfgSetFile = new QSettings(PhoneLinkConfig,QSettings::IniFormat);
            _PhoneLinkType  = PhoneLinkcfgSetFile->value("PhoneLink").toString().toInt();
            delete PhoneLinkcfgSetFile;
        }
        d->m_LastIndex = _PhoneLinkType;
        QQmlProperty(d->m_MoreSettingPhoneLinkWidgetObject,"currentIndex").write(_PhoneLinkType);
    }

}

void MoreSettingPhoneLinkWidget::onToolButtonRelease(){
    Q_D(MoreSettingPhoneLinkWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_CancleBtnObject){

        QQmlProperty(d->m_MoreSettingPhoneLinkWidgetObject,"currentIndex").write(d->m_LastIndex);
    }
    else if(ptr == d->m_ConfirmBtnObject){
        int _PhoneLinkType  = d->m_MoreSettingPhoneLinkWidgetObject->property("currentIndex").toInt();
        //qDebug()<<"++++++++++++_PhoneLinkType++++++++++++"<<_PhoneLinkType;
        QFile _PhoneLinkcfgFile(PhoneLinkConfig);
        if(!_PhoneLinkcfgFile.exists())
        {
            g_Setting->executeShellCmd(QString(QString("touch ")+ PhoneLinkConfig).toLocal8Bit().constData());
            g_Setting->executeShellCmd("sync");
        }
        QSettings * PhoneLinkcfgSetFile = new QSettings(PhoneLinkConfig,QSettings::IniFormat);
        PhoneLinkcfgSetFile->setValue("PhoneLink", QString("%1").arg(_PhoneLinkType));
        PhoneLinkcfgSetFile->sync();
        g_Setting->executeShellCmd("sync");
        delete PhoneLinkcfgSetFile;
        reboot(LINUX_REBOOT_CMD_RESTART);
    }
}

MoreSettingPhoneLinkWidgetPrivate::MoreSettingPhoneLinkWidgetPrivate(MoreSettingPhoneLinkWidget *parent)
    : q_ptr(parent)
{
    m_MoreSettingPhoneLinkWidgetObject = NULL;
    m_CancleBtnObject = NULL;
    m_ConfirmBtnObject = NULL;
    m_LastIndex = 0;
}

MoreSettingPhoneLinkWidgetPrivate::~MoreSettingPhoneLinkWidgetPrivate()
{

}
void MoreSettingPhoneLinkWidgetPrivate::initializeObject(){
    if(m_CancleBtnObject == NULL)
    {
        m_CancleBtnObject = m_MoreSettingPhoneLinkWidgetObject->findChild<QObject*>("cancleBtnObject");
    }
    if(m_ConfirmBtnObject == NULL)
    {
        m_ConfirmBtnObject = m_MoreSettingPhoneLinkWidgetObject->findChild<QObject*>("confirmBtnObject");
    }

}
