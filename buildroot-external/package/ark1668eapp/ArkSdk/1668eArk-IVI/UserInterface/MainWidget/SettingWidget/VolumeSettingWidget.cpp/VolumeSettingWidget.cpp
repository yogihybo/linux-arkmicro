#include "VolumeSettingWidget.h"
#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include "VolumeValueSettingWidget.h"
#include "VolumeBalanceSettingWidget.h"
#include <QQmlProperty>
#include <QDebug>

class VolumeSettingWidgetPrivate
{
    Q_DISABLE_COPY(VolumeSettingWidgetPrivate)
public:
    explicit VolumeSettingWidgetPrivate(VolumeSettingWidget* parent);
    ~VolumeSettingWidgetPrivate();
    void initializeVolumeValueSettingWidget();
    void initializeVolumeBalanceSettingWidget();
public:
    QObject* m_VolumeSettingWidgetObject;
    VolumeValueSettingWidget* m_VolumeValueSettingWidget;
    VolumeBalanceSettingWidget* m_VolumeBalanceSettingWidget;
private:
    Q_DECLARE_PUBLIC(VolumeSettingWidget)
    VolumeSettingWidget* const q_ptr;
};

VolumeSettingWidget::VolumeSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new VolumeSettingWidgetPrivate(this))
{

}
void VolumeSettingWidget::setVolumeSettingWidgetObject(QObject* qmlObject){
    Q_D(VolumeSettingWidget);
    if(d->m_VolumeSettingWidgetObject == NULL)
    {
        d->m_VolumeSettingWidgetObject = qmlObject;
    }
    d->initializeVolumeValueSettingWidget();
    d->initializeVolumeBalanceSettingWidget();
}
VolumeSettingWidgetPrivate::VolumeSettingWidgetPrivate(VolumeSettingWidget *parent)
    : q_ptr(parent)
{
    m_VolumeSettingWidgetObject = NULL;
    m_VolumeValueSettingWidget = NULL;
    m_VolumeBalanceSettingWidget = NULL;
}

VolumeSettingWidgetPrivate::~VolumeSettingWidgetPrivate()
{

}
void VolumeSettingWidgetPrivate::initializeVolumeValueSettingWidget(){
    Q_Q(VolumeSettingWidget);
    if(m_VolumeValueSettingWidget == NULL)
    {
        m_VolumeValueSettingWidget = new VolumeValueSettingWidget(q);
        QObject* _VolumeValueSettingWidgetObject = m_VolumeSettingWidgetObject->findChild<QObject*>("soundValueSettingWidgetObject");
        m_VolumeValueSettingWidget->setVolumeValueSettingWidgetObject(_VolumeValueSettingWidgetObject);
    }

}

void VolumeSettingWidgetPrivate::initializeVolumeBalanceSettingWidget(){
    Q_Q(VolumeSettingWidget);
    if(m_VolumeBalanceSettingWidget == NULL)
    {
        m_VolumeBalanceSettingWidget = new VolumeBalanceSettingWidget(q);
        QObject* _VolumeBalanceSettingWidgetObject = m_VolumeSettingWidgetObject->findChild<QObject*>("soundBalanceSettingWidgetObject");
        m_VolumeBalanceSettingWidget->setVolumeBalanceSettingWidgetObjecgt(_VolumeBalanceSettingWidgetObject);
    }

}
