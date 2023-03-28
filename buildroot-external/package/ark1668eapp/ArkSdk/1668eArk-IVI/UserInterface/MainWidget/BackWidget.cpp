#include "BackWidget.h"
#include "BusinessLogic/Widget.h"
#include <QTimer>
#include <QQmlProperty>
#include <QDebug>
class BackWidgetPrivate
{
    Q_DISABLE_COPY(BackWidgetPrivate)
public:
    explicit BackWidgetPrivate(BackWidget* parent);
    ~BackWidgetPrivate();
    void initializeTimer();
public:
    QObject* m_BackWidgetObject;
    QObject* m_BackBtnObject;
    QObject* m_BackLoaderObject;
    int  m_WidgetType;
    QTimer* m_Timer;
private:
    Q_DECLARE_PUBLIC(BackWidget)
    BackWidget* const q_ptr;
};

BackWidget::BackWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new BackWidgetPrivate(this))
{

}

void BackWidget::setBackWidgetObject(QObject* qmlObject)
{
    Q_D(BackWidget);
    if(d->m_BackWidgetObject == NULL)
    {
        d->m_BackWidgetObject = qmlObject;
    }
    if(d->m_BackBtnObject == NULL)
    {
        d->m_BackBtnObject = d->m_BackWidgetObject->findChild<QObject*>("backBtnObject");
    }
    QObject::connect(d->m_BackBtnObject, SIGNAL(clicked()),
                     this,               SLOT(onClicked()));
    QObject::connect(d->m_BackLoaderObject, SIGNAL(visibleChanged()),
                     this,               SLOT(onVisibleChanged()));
}

void BackWidget::setBackLoaderObject(QObject *qmlObject)
{
    Q_D(BackWidget);
    if(d->m_BackLoaderObject == NULL)
    {
        d->m_BackLoaderObject = qmlObject;
    }

}
void BackWidget::setWidgetType(int widgetType)
{
    Q_D(BackWidget);
    d->m_WidgetType = widgetType;
}
void BackWidget::onClicked()
{
    Q_D(BackWidget);
    QObject* ptr = (QObject*)(sender());
    if(ptr == d->m_BackBtnObject)
    {
        if(d->m_WidgetType == Widget::T_Aux)
        {
            emit g_Widget->onWidgetTypeChange(Widget::T_Home, Widget::T_Aux,QString("show"));
        }
    }
}
void BackWidget::onVisibleChanged()
{
    Q_D(BackWidget);
    if(d->m_BackLoaderObject->property("visible").toBool())
    {
        d->initializeTimer();
        //d->m_Timer->start();
    }
    else{
        d->initializeTimer();
        if(d->m_Timer->isActive())
        {
            d->m_Timer->stop();
        }
    }
}
void BackWidget::onTimeout()
{
    Q_D(BackWidget);
    qDebug()<<"++++++++onTimeout+mmmm++++++++";
    if(d->m_BackLoaderObject->property("visible").toBool())
    {
        QQmlProperty(d->m_BackLoaderObject,"visible").write(false);
    }
}
BackWidgetPrivate::BackWidgetPrivate(BackWidget *parent)
    : q_ptr(parent)
{
    m_BackWidgetObject = NULL;
    m_BackBtnObject    = NULL;
    m_BackLoaderObject = NULL;
    m_Timer = NULL;
    m_WidgetType       = 0;
}

BackWidgetPrivate::~BackWidgetPrivate()
{

}
void BackWidgetPrivate::initializeTimer(){
    Q_Q(BackWidget);
    if(m_Timer == NULL)
    {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(2000);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer,SIGNAL(timeout()),q,SLOT(onTimeout()));
    }
}
