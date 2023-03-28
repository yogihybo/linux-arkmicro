#include "BluetoothSettingWidget.h"
#include <QDebug>
class BluetoothSettingWidgetPrivate
{
    Q_DISABLE_COPY(BluetoothSettingWidgetPrivate)
public:
    explicit BluetoothSettingWidgetPrivate(BluetoothSettingWidget* parent);
    ~BluetoothSettingWidgetPrivate();
    void initializeWidget();
    void initializeBtSettingSwitchSettingWidget();
    void initialzeBluetoothConnectWidget();
public:
    QObject* m_BtSettingWidgetObject;
    BluetoothSwitchSettingWidget* m_BtSettingSwitchSettingWidget;
    BluetoothConnectWidget* m_BtConnectWidget;
private:
    Q_DECLARE_PUBLIC(BluetoothSettingWidget)
    BluetoothSettingWidget* const q_ptr;
};
BluetoothSettingWidget::BluetoothSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new BluetoothSettingWidgetPrivate(this))
{

}

void BluetoothSettingWidget::setBtSettingWidgetObject(QObject *qmlObject)
{
    Q_D(BluetoothSettingWidget);
    if(d->m_BtSettingWidgetObject == NULL)
    {
        d->m_BtSettingWidgetObject = qmlObject;
    }
    d->initializeWidget();
}
BluetoothSettingWidgetPrivate::BluetoothSettingWidgetPrivate(BluetoothSettingWidget *parent)
    : q_ptr(parent)
{
    m_BtSettingWidgetObject = NULL;
    m_BtSettingSwitchSettingWidget = NULL;
    m_BtConnectWidget = NULL;
}

BluetoothSettingWidgetPrivate::~BluetoothSettingWidgetPrivate()
{

}
void BluetoothSettingWidgetPrivate::initializeWidget()
{
    initializeBtSettingSwitchSettingWidget();
    initialzeBluetoothConnectWidget();
}

void BluetoothSettingWidgetPrivate::initializeBtSettingSwitchSettingWidget()
{
    Q_Q(BluetoothSettingWidget);
    if(m_BtSettingSwitchSettingWidget == NULL)
    {
        m_BtSettingSwitchSettingWidget = new BluetoothSwitchSettingWidget(q);
        QObject* btSettingSwitchSettingWidgetObject = m_BtSettingWidgetObject->findChild<QObject*>("btSwitchSettingObject");
        m_BtSettingSwitchSettingWidget->setBtSwitchSettingWidgetObject(btSettingSwitchSettingWidgetObject);
    }
}

void BluetoothSettingWidgetPrivate::initialzeBluetoothConnectWidget()
{
    Q_Q(BluetoothSettingWidget);
    if(m_BtConnectWidget == NULL)
    {
        m_BtConnectWidget = new BluetoothConnectWidget(q);
        QObject* btConnectWidgetObject = m_BtSettingWidgetObject->findChild<QObject*>("btConnectWidgetObject");
        m_BtConnectWidget->setBluetoothConnectWidgetObject(btConnectWidgetObject);
    }
}
