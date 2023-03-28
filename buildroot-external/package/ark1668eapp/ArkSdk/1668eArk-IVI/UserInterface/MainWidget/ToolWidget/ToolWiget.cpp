#include "ToolWiget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Setting.h"
#include <QDateTime>
#include <QQmlProperty>
#include <QDebug>
#include <QTimer>
class ToolWigetPrivate
{
    Q_DISABLE_COPY(ToolWigetPrivate)
public:
    explicit ToolWigetPrivate(ToolWiget* parent);
    ~ToolWigetPrivate();
    void showDataTime();
    void connectAllSlots();
    void initializeSycTimer();
public:
    QObject* m_ToolWigetObject;
    QObject* m_TiemObject;
    QTimer* m_SycTimer;
private:
    Q_DECLARE_PUBLIC(ToolWiget)
    ToolWiget* const q_ptr;
};

ToolWiget::ToolWiget(QObject *parent)
    : QObject(parent),
      d_ptr(new ToolWigetPrivate(this))
{

}
void ToolWiget::setToolWigetObject(QObject* qmlObject){
    Q_D(ToolWiget);
    if(d->m_ToolWigetObject == NULL)
    {
        d->m_ToolWigetObject = qmlObject;
    }
    if(d->m_TiemObject == NULL)
    {
        d->m_TiemObject = d->m_ToolWigetObject->findChild<QObject*>("tiemObject");
    }
    d->showDataTime();
    d->initializeSycTimer();
    if(d->m_SycTimer->isActive())
    {
        d->m_SycTimer->stop();
    }
    d->m_SycTimer->start();
}
QObject* ToolWiget::getToolWigetObject(){
    Q_D(ToolWiget);
    if(d->m_ToolWigetObject != NULL)
    {
        return d->m_ToolWigetObject;
    }
}

void ToolWiget::onDataTimeSetting()
{
    Q_D(ToolWiget);
    d->showDataTime();
    d->initializeSycTimer();
    if(d->m_SycTimer->isActive())
    {
        d->m_SycTimer->stop();
    }
    d->m_SycTimer->start();

}
void ToolWiget::onTimeout(){
    Q_D(ToolWiget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_SycTimer)
    {
        d->showDataTime();
    }
}
ToolWigetPrivate::ToolWigetPrivate(ToolWiget *parent)
    : q_ptr(parent)
{
    m_ToolWigetObject = NULL;
    m_TiemObject = NULL;
    m_SycTimer = NULL;
    connectAllSlots();
}
ToolWigetPrivate::~ToolWigetPrivate()
{

}
void ToolWigetPrivate::showDataTime(){
    QDateTime begin_time = QDateTime::currentDateTime();//获取系统现在的时间
    QString begin =begin_time .toString("yyyy.MM.dd hh:mm");
    QString _Time = begin_time.toString("hh:mm");
    QQmlProperty(m_TiemObject,"text").write(_Time);
}

void ToolWigetPrivate::connectAllSlots()
{
    Q_Q(ToolWiget);
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onDataTimeSetting()));
}

void ToolWigetPrivate::initializeSycTimer(){
    Q_Q(ToolWiget);
    if(m_SycTimer == NULL)
    {
        m_SycTimer = new QTimer(q);
        m_SycTimer->setInterval(1000);
        QObject::connect(m_SycTimer, ARKSENDER(timeout()),
                         q,                 ARKRECEIVER(onTimeout()));
    }
}

