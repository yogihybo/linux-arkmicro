#include "AuxMiniWidget.h"
#include "BusinessLogic/Widget.h"
#include <QQmlProperty>
#include <QDebug>

class AuxMiniWidgetPrivate
{
    Q_DISABLE_COPY(AuxMiniWidgetPrivate)
public:
    explicit AuxMiniWidgetPrivate(AuxMiniWidget* parent);
    ~AuxMiniWidgetPrivate();
public:
    QObject* m_AuxMiniWidgetObject;
private:
    Q_DECLARE_PUBLIC(AuxMiniWidget)
    AuxMiniWidget* const q_ptr;
};

AuxMiniWidget::AuxMiniWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new AuxMiniWidgetPrivate(this))
{

}
void AuxMiniWidget::setAuxMiniWidgetObject(QObject* qmlObject){
    Q_D(AuxMiniWidget);
    if(d->m_AuxMiniWidgetObject == NULL)
    {
        d->m_AuxMiniWidgetObject  = qmlObject;
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_AuxMiniWidgetObject, SIGNAL(auxWidgetClicked()),
                     this,      SLOT(onAuxWidgetClicked()),
                     type);
}

void AuxMiniWidget::onAuxWidgetClicked(){
    Q_D(AuxMiniWidget);
    emit g_Widget->onWidgetTypeChange(Widget::T_Aux, Widget::T_Home,QString("show"));
}
AuxMiniWidgetPrivate::AuxMiniWidgetPrivate(AuxMiniWidget *parent)
    : q_ptr(parent)
{
    m_AuxMiniWidgetObject = NULL;
}

AuxMiniWidgetPrivate::~AuxMiniWidgetPrivate()
{

}

