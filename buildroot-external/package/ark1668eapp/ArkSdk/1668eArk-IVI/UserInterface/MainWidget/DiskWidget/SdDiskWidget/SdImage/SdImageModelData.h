#ifndef SDIMAGEMODELDATA_H
#define SDIMAGEMODELDATA_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h"
class SdImageModelDataPrivate;
class SdImageModelData : public QObject
{
    Q_OBJECT
public:
    explicit SdImageModelData(QObject *parent = nullptr);
    Q_INVOKABLE myModel* getObjectModel();
protected slots:
    void onImagePlayerFileNames(const DeviceWatcherType type, const QString &xml);
private:
    SdImageModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(SdImageModelData)
};

#endif // SDIMAGEMODELDATA_H
