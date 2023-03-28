#ifndef USBVIDEOMODELDATA_H
#define USBVIDEOMODELDATA_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h"
class UsbVideoModelDataPrivate;
class UsbVideoModelData : public QObject
{
    Q_OBJECT
public:
    explicit UsbVideoModelData(QObject *parent = nullptr);
    Q_INVOKABLE myModel* getObjectModel();
protected slots:
    void onVideoPlayerFileNames(const DeviceWatcherType type, const QString &xml);
public slots:
    void onLanguageChanged();
private:
    UsbVideoModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(UsbVideoModelData)
};

#endif // USBVIDEOMODELDATA_H
