#ifndef USBIMAGEMODELDATA_H
#define USBIMAGEMODELDATA_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h"
class UsbImageModelDataPrivate;
class UsbImageModelData : public QObject
{
    Q_OBJECT
public:
    explicit UsbImageModelData(QObject *parent = nullptr);
    Q_INVOKABLE myModel* getObjectModel();
protected slots:
    void onImagePlayerFileNames(const DeviceWatcherType type, const QString &xml);
private:
    UsbImageModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(UsbImageModelData)
};

#endif // USBIMAGEMODELDATA_H
