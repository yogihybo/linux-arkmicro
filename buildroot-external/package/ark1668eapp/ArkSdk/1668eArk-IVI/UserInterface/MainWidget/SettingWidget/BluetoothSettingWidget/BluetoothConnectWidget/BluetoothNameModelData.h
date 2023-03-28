#ifndef BLUETOOTHNAMEMODELDATA_H
#define BLUETOOTHNAMEMODELDATA_H

#include <QObject>
#include "./BusinessLogic/Bluetooth.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h"
class BluetoothNameModelDataPrivate;
class BluetoothNameModelData : public QObject
{
    Q_OBJECT
public:
    explicit BluetoothNameModelData(QObject *parent = nullptr);
    Q_INVOKABLE myModel* getObjectModel();
protected slots:
    void onScanFinish();
    void onPowerChange(int mode);
    void onGetPairedListFinish();
private:
    BluetoothNameModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(BluetoothNameModelData)
};

#endif // BLUETOOTHNAMEMODELDATA_H
