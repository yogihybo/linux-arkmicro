#ifndef BLUETOOTHSWITCHSETTINGWIDGET_H
#define BLUETOOTHSWITCHSETTINGWIDGET_H

#include <QObject>
#include "BusinessLogic/Bluetooth.h"
class BluetoothSwitchSettingWidgetPrivate;
class BluetoothSwitchSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothSwitchSettingWidget(QObject *parent = nullptr);
    void setBtSwitchSettingWidgetObject(QObject* qmlObject);
protected slots:
    void onPowerChange(int mode);
    void onAutoConnectChange(const int mode);
    void onAutoAnswerChange(const  int mode);
    void onCarPlayConnected();
    void onCarPlayExit();
public slots:
    void onToolButtonRelease();
    void onVisibleChanged();
    void onTimeout();

private:
    BluetoothSwitchSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(BluetoothSwitchSettingWidget)
};

#endif // BLUETOOTHSWITCHSETTINGWIDGET_H
