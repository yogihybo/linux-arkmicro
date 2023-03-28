#include "MoreSettingWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include "AutoConnect.h"
#include "MoreSettingDataTimeWidget.h"
#include "MoreSettingPhoneLinkWidget.h"
#include <QQmlProperty>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <QSettings>
#include <QFile>
static const QString PhoneLinkConfig("/data/PhoneLink.ini");
static const QString DataTime("/data/DataTime.ini");
class MoreSettingWidgetPrivate
{
    Q_DISABLE_COPY(MoreSettingWidgetPrivate)
public:
    explicit MoreSettingWidgetPrivate(MoreSettingWidget* parent);
    ~MoreSettingWidgetPrivate();
    void initializeMoreSettingDataTimeWidget();
    void initializeMoreSettingPhoneLinkWidget();
    void initializeObject();
    void initializeTimer();
public:
    QObject* m_MoreSettingWidgetObject;
    QObject* m_DataSetBtnObject;
    QObject* m_PhoneLinkSetBtnObject;
    QObject* m_DataTextObject;
    QObject* m_SystemSwitchBtnObject;
    QObject* m_PhoneLinkTypeObject;
    MoreSettingDataTimeWidget* m_MoreSettingDataTimeWidget;
    MoreSettingPhoneLinkWidget* m_MoreSettingPhoneLinkWidget;
    QTimer* m_Timer;
private:
    Q_DECLARE_PUBLIC(MoreSettingWidget)
    MoreSettingWidget* const q_ptr;
};

MoreSettingWidget::MoreSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new MoreSettingWidgetPrivate(this))
{

}
void MoreSettingWidget::setMoreSettingWidgetObject(QObject* qmlObject)
{
    Q_D(MoreSettingWidget);
    if(d->m_MoreSettingWidgetObject == NULL)
    {
        d->m_MoreSettingWidgetObject = qmlObject;
    }
    d->initializeObject();
    d->initializeMoreSettingDataTimeWidget();
    d->initializeMoreSettingPhoneLinkWidget();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_DataSetBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_PhoneLinkSetBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_MoreSettingWidgetObject, ARKSENDER(visibleChanged()),
                     this,      ARKRECEIVER(onVisibleChanged()),
                     type);
    QObject::connect(d->m_SystemSwitchBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_MoreSettingWidgetObject, ARKSENDER(simpChineseBtnClicked()),
                     this,      ARKRECEIVER(onSimpChineseBtnClicked()),
                     type);
    QObject::connect(d->m_MoreSettingWidgetObject, ARKSENDER(englishBtnBtnClicked()),
                     this,      ARKRECEIVER(onEnglishBtnBtnClicked()),
                     type);

    QObject::connect(d->m_MoreSettingWidgetObject, ARKSENDER(traditionalChineseBtnClicked()),
                     this,      ARKRECEIVER(onTraditionalChineseBtnClicked()),
                     type);

   // connectSignalAndSlotByNamesake(g_Setting, this, ARKRECEIVER(onDataTimeSetting()));

}

void MoreSettingWidget::onSimpChineseBtnClicked()
{
   //qDebug()<<"+++++onSimpChineseBtnClicked++++++++";
   g_Setting->setLanguage(LT_Chinese);
}
void MoreSettingWidget::onEnglishBtnBtnClicked()
{
    //qDebug()<<"+++++onEnglishBtnBtnClicked++++++++";
    g_Setting->setLanguage(LT_English);

}
void MoreSettingWidget::onTraditionalChineseBtnClicked()
{
    //qDebug()<<"+++++onTraditionalChineseBtnClicked++++++++";
    g_Setting->setLanguage(LT_TChinese);
}
//void MoreSettingWidget::onDataTimeSetting()
//{
//    Q_D(MoreSettingWidget);
//    onTimeout();
//}
void MoreSettingWidget::onVisibleChanged()
{
    Q_D(MoreSettingWidget);
    bool _Visible = d->m_MoreSettingWidgetObject->property("visible").toBool();
    if(_Visible)
    {
        d->initializeTimer();
        onTimeout();
        if(d->m_Timer->isActive())
        {
            d->m_Timer->stop();
        }
        d->m_Timer->start();

        static const QString LanguageConfig("/data/Language.ini");
        QFile _LanguageConfigFile(LanguageConfig);
        int _LanguageType = 1;
        if(_LanguageConfigFile.exists()){
            QSettings *LanguagecfgsetFile = new QSettings(LanguageConfig,QSettings::IniFormat);
            _LanguageType = LanguagecfgsetFile->value("Language").toInt();
            delete LanguagecfgsetFile;
        }
        QQmlProperty(d->m_MoreSettingWidgetObject,"languageType").write(_LanguageType);
    }
    else{
        d->initializeTimer();
        if(d->m_Timer->isActive())
        {
            d->m_Timer->stop();
        }
    }
}
void MoreSettingWidget::onTimeout()
{
    Q_D(MoreSettingWidget);
    QDateTime begin_time = QDateTime::currentDateTime();//获取系统现在的时间
    QString begin = begin_time .toString("yyyy.MM.dd hh:mm");
    QStringList list = begin.split(".");
    QString year;
    QString month;
    QString day;
    if(list.size() >= 3)
    {
        year = list.at(0);
        month = list.at(1);
        day = list.at(2);
    } 
    QString _DatTimeStr = year+QString("-")+month+QString("-")+day.left(2)+QString("-")+QString(" ")+day.right(5);
    QQmlProperty(d->m_DataTextObject,"text").write(_DatTimeStr);
    QFile DataTimecfgFile(DataTime);
    QSettings *DataTimecfgsetFile = new QSettings(DataTime,QSettings::IniFormat);
    if(!DataTimecfgFile.exists())
    {
        qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"DataTimecfgFile is not exist, creating...";
        g_Setting->executeShellCmd(QString(QString("touch ")+ DataTime).toLocal8Bit().constData());
        g_Setting->executeShellCmd("sync");
    }
    DataTimecfgsetFile->setValue("DataTime", _DatTimeStr);
    DataTimecfgsetFile->sync();
    g_Setting->executeShellCmd("sync");;
    delete DataTimecfgsetFile;
    QFile _PhoneLinkcfgFile(PhoneLinkConfig);
    int _PhoneLinkType = 0;
    if(_PhoneLinkcfgFile.exists())
    {
        QSettings* PhoneLinkcfgSetFile = new QSettings(PhoneLinkConfig,QSettings::IniFormat);
        _PhoneLinkType  = PhoneLinkcfgSetFile->value("PhoneLink").toString().toInt();
        delete PhoneLinkcfgSetFile;
    }
    if(_PhoneLinkType == 0)
    {
        QQmlProperty(d->m_PhoneLinkTypeObject,"text").write("CarLife+Carplay");
    }
    else if(_PhoneLinkType == 1)
    {
        QQmlProperty(d->m_PhoneLinkTypeObject,"text").write("Auto+Carplay");
    }
    else if(_PhoneLinkType == 2)
    {
        QQmlProperty(d->m_PhoneLinkTypeObject,"text").write("亿连手机互联");
    }
    else if(_PhoneLinkType == 3)
    {
        QQmlProperty(d->m_PhoneLinkTypeObject,"text").write("HiCar");
    }
}
void MoreSettingWidget::onToolButtonRelease()
{
    Q_D(MoreSettingWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_DataSetBtnObject)
    {
        QQmlProperty(d->m_MoreSettingWidgetObject,"showType").write(1);
    }
    else if(ptr == d->m_PhoneLinkSetBtnObject)
    {
        QQmlProperty(d->m_MoreSettingWidgetObject,"showType").write(2);
    }
    else if(ptr == d->m_SystemSwitchBtnObject)
    {
        //qDebug()<<"++++SystemSwitchBtn clicked+++++";
        g_Setting->executeShellCmd("echo a > /data/processType");
        g_Setting->executeShellCmd("sync");
        reboot(LINUX_REBOOT_CMD_RESTART);
    }

}
MoreSettingWidgetPrivate::MoreSettingWidgetPrivate(MoreSettingWidget *parent)
    : q_ptr(parent)
{
    m_MoreSettingWidgetObject = NULL;
    m_MoreSettingDataTimeWidget = NULL;
    m_MoreSettingPhoneLinkWidget = NULL;
    m_DataSetBtnObject = NULL;
    m_PhoneLinkSetBtnObject = NULL;
    m_DataTextObject = NULL;
    m_SystemSwitchBtnObject = NULL;
    m_PhoneLinkTypeObject = NULL;
    m_Timer = NULL;
}

MoreSettingWidgetPrivate::~MoreSettingWidgetPrivate()
{

}
void MoreSettingWidgetPrivate::initializeMoreSettingDataTimeWidget(){
    Q_Q(MoreSettingWidget);
    if(m_MoreSettingDataTimeWidget == NULL)
    {
        m_MoreSettingDataTimeWidget = new MoreSettingDataTimeWidget(q);
        QObject* _MoreSettingDataTimeWidgetObject = m_MoreSettingWidgetObject->findChild<QObject*>("moreSettingDataTimeWidgetObject");
        m_MoreSettingDataTimeWidget->setMoreSettingDataTimeWidgetObject(_MoreSettingDataTimeWidgetObject);
    }
}

void MoreSettingWidgetPrivate::initializeMoreSettingPhoneLinkWidget(){
    Q_Q(MoreSettingWidget);
    if(m_MoreSettingPhoneLinkWidget == NULL)
    {
        m_MoreSettingPhoneLinkWidget = new MoreSettingPhoneLinkWidget(q);
        QObject* _MoreSettingPhoneLinkWidgetObject = m_MoreSettingWidgetObject->findChild<QObject*>("phoneLinkWidgetObject");
        m_MoreSettingPhoneLinkWidget->setMoreSettingPhoneLinkWidgetObject(_MoreSettingPhoneLinkWidgetObject);
    }
}

void MoreSettingWidgetPrivate::initializeTimer()
{
    Q_Q(MoreSettingWidget);
    if(m_Timer == NULL)
    {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(1000);
        QObject::connect(m_Timer, ARKSENDER(timeout()),
                         q,      ARKRECEIVER(onTimeout()));
    }
}
void MoreSettingWidgetPrivate::initializeObject(){
    Q_Q(MoreSettingWidget);
    if(m_DataSetBtnObject == NULL)
    {
        m_DataSetBtnObject = m_MoreSettingWidgetObject->findChild<QObject*>("dataSetBtnObject");
    }

    if(m_PhoneLinkSetBtnObject == NULL)
    {
        m_PhoneLinkSetBtnObject = m_MoreSettingWidgetObject->findChild<QObject*>("phoneLinkSetBtnObject");
    }
    if(m_DataTextObject == NULL)
    {
        m_DataTextObject = m_MoreSettingWidgetObject->findChild<QObject*>("dataTextObject");
    }
    if(m_SystemSwitchBtnObject == NULL)
    {
        m_SystemSwitchBtnObject = m_MoreSettingWidgetObject->findChild<QObject*>("systemSwitchBtnObject");
    }
    if(m_PhoneLinkTypeObject == NULL)
    {
        m_PhoneLinkTypeObject = m_MoreSettingWidgetObject->findChild<QObject*>("phoneLinkTypeObject");
    }
}





