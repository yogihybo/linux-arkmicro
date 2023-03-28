#ifndef BLUETOOTHSETTINGWIDGET_H
#define BLUETOOTHSETTINGWIDGET_H

#include <QObject>
#include "BluetoothSwitchSettingWidget/BluetoothSwitchSettingWidget.h"
#include "BluetoothConnectWidget/BluetoothConnectWidget.h"
class BluetoothSettingWidgetPrivate;
class BluetoothSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothSettingWidget(QObject *parent = nullptr);
    void setBtSettingWidgetObject(QObject* qmlObject);
private:
    BluetoothSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(BluetoothSettingWidget)
};

#endif // BLUETOOTHSETTINGWIDGET_H
