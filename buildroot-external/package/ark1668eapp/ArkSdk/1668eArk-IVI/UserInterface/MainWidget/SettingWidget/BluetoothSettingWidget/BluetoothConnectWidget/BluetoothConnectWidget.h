#ifndef BLUETOOTHCONNECTWIDGET_H
#define BLUETOOTHCONNECTWIDGET_H

#include <QObject>
#include "BusinessLogic/Bluetooth.h"
class BluetoothConnectWidgetPrivate;
class BluetoothConnectWidget : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothConnectWidget(QObject *parent = nullptr);
    void setBluetoothConnectWidgetObject(QObject *qmlObject);
protected slots:
    void onPowerChange(int mode);
    void onScanFinish();
    void onConnectStatusChange(const int status);
    void onRemoteDeviceNameChange(const QString& name);
    void onCarPlayConnected();
    void onCarPlayExit();
public slots:
    void onToolButtonRelease();
    void onTimeout();
    void onListviewItemClicked(int index);
    void onVisibleChanged();
private:
    BluetoothConnectWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(BluetoothConnectWidget)
};

#endif // BLUETOOTHCONNECTWIDGET_H
