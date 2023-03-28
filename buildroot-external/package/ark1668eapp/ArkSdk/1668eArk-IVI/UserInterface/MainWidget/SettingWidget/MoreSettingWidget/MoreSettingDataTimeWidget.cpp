#include "MoreSettingDataTimeWidget.h"
#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include "Utility.h"
#include "BusinessLogic/Setting.h"
#include <QQmlProperty>
#include <QDebug>
#include <QDateTime>
static const QString DataTime("/data/DataTime.ini");
class MoreSettingDataTimeWidgetPrivate
{
    Q_DISABLE_COPY(MoreSettingDataTimeWidgetPrivate)
public:
    explicit MoreSettingDataTimeWidgetPrivate(MoreSettingDataTimeWidget* parent);
    ~MoreSettingDataTimeWidgetPrivate();
    void initializeObject();
    void initializeDataTime();
public:
    QObject* m_MoreSettingDataTimeWidgetObject;
//    QObject* m_YearListViewObject;
//    QObject* m_MonthListViewObject;
//    QObject* m_DayListViewObject;
//    QObject* m_HourListViewObject;
//    QObject* m_MinListViewObject;
    QObject* m_CancleBtnObject;
   // QObject* m_ConfirmBtnObject;
private:
    Q_DECLARE_PUBLIC(MoreSettingDataTimeWidget)
    MoreSettingDataTimeWidget* const q_ptr;
};

MoreSettingDataTimeWidget::MoreSettingDataTimeWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new MoreSettingDataTimeWidgetPrivate(this))
{

}

void MoreSettingDataTimeWidget::setMoreSettingDataTimeWidgetObject(QObject *qmlObject){
    Q_D(MoreSettingDataTimeWidget);
    if(d->m_MoreSettingDataTimeWidgetObject == NULL)
    {
        d->m_MoreSettingDataTimeWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_MoreSettingDataTimeWidgetObject, ARKSENDER(confirmBtnClicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_CancleBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void MoreSettingDataTimeWidget::onToolButtonRelease(){
    Q_D(MoreSettingDataTimeWidget);
    int year  = d->m_MoreSettingDataTimeWidgetObject->property("yearStr").toString().toInt();
    int month = d->m_MoreSettingDataTimeWidgetObject->property("monthStr").toString().toInt();
    int day = d->m_MoreSettingDataTimeWidgetObject->property("dayStr").toString().toInt();
    int hour = d->m_MoreSettingDataTimeWidgetObject->property("hourStr").toString().toInt();
    int min = d->m_MoreSettingDataTimeWidgetObject->property("minStr").toString().toInt();
    int second = 0;
    setDateTime(year,
                month - 1,
                day,
                hour,
                min,
                second);
//    QDateTime begin_time = QDateTime::currentDateTime();//获取系统现在的时间
//    QString begin =begin_time .toString("yyyy.MM.dd hh:mm");
//    qDebug()<<"---[ToolWigetPrivate::showDataTime:]---"<<begin;
    g_Setting->settingDataTime();

}

MoreSettingDataTimeWidgetPrivate::MoreSettingDataTimeWidgetPrivate(MoreSettingDataTimeWidget *parent)
    : q_ptr(parent)
{
    m_MoreSettingDataTimeWidgetObject = NULL;
//    m_YearListViewObject = NULL;
//    m_MonthListViewObject= NULL;
//    m_DayListViewObject  = NULL;
//    m_HourListViewObject = NULL;
//    m_MinListViewObject  = NULL;
    m_CancleBtnObject = NULL;
    initializeDataTime();
}

MoreSettingDataTimeWidgetPrivate::~MoreSettingDataTimeWidgetPrivate()
{

}
void MoreSettingDataTimeWidgetPrivate::initializeObject()
{
//    if(m_YearListViewObject == NULL)
//    {
//        m_YearListViewObject = m_MoreSettingDataTimeWidgetObject->findChild<QObject*>("yearListViewObject");
//    }
//    if(m_MonthListViewObject == NULL)
//    {
//        m_MonthListViewObject = m_MoreSettingDataTimeWidgetObject->findChild<QObject*>("monthListViewObject");
//    }

//    if(m_DayListViewObject == NULL)
//    {
//        m_DayListViewObject = m_MoreSettingDataTimeWidgetObject->findChild<QObject*>("dayListViewObject");
//    }

//    if(m_HourListViewObject == NULL)
//    {
//        m_HourListViewObject = m_MoreSettingDataTimeWidgetObject->findChild<QObject*>("hourListViewObject");
//    }

//    if(m_MinListViewObject == NULL)
//    {
//        m_MinListViewObject = m_MoreSettingDataTimeWidgetObject->findChild<QObject*>("minListViewObject");
//    }
    if(m_CancleBtnObject == NULL)
    {
        m_CancleBtnObject = m_MoreSettingDataTimeWidgetObject->findChild<QObject*>("cancleBtnObject");
    }

//    if(m_ConfirmBtnObject == NULL)
//    {
//        m_ConfirmBtnObject = m_MoreSettingDataTimeWidgetObject->findChild<QObject*>("confirmBtnObject");
//    }
}

void MoreSettingDataTimeWidgetPrivate::initializeDataTime()
{
    QFile DataTimecfgFile(DataTime);
    if(DataTimecfgFile.exists())
    {
        QSettings *DataTimecfgsetFile = new QSettings(DataTime,QSettings::IniFormat);
        QString _DataTimeStr  = DataTimecfgsetFile->value("DataTime").toString();
        delete DataTimecfgsetFile;
        QStringList DataTimeList = _DataTimeStr.split(" ");
        //qDebug()<<"+++++++++DataTimeList.at(0)++++++++++" << DataTimeList.at(0);
        //qDebug()<<"+++++++++DataTimeList.at(1)++++++++++" << DataTimeList.at(1);
        QStringList DataList;
        QStringList TimeList;
        if(DataTimeList.size() >= 2)
        {
            DataList = QString(DataTimeList.at(0)).split("-");
            TimeList = QString(DataTimeList.at(1)).split(":");
        }
        if(DataList.size() < 3 || TimeList.size() < 2)
        {
            return;
        }
        int year   = QString(DataList.at(0)).toInt();
        int month  = QString(DataList.at(1)).toInt();
        int day    = QString(DataList.at(2)).toInt();
        int hour   = QString(TimeList.at(0)).toInt();
        int min    = QString(TimeList.at(1)).toInt();
        int second = 0;
        setDateTime(year,
                    month - 1,
                    day,
                    hour,
                    min,
                    second);
    //    QDateTime begin_time = QDateTime::currentDateTime();//获取系统现在的时间
    //    QString begin =begin_time .toString("yyyy.MM.dd hh:mm");
    //    qDebug()<<"---[ToolWigetPrivate::showDataTime:]---"<<begin;
        g_Setting->settingDataTime();
    }
}
