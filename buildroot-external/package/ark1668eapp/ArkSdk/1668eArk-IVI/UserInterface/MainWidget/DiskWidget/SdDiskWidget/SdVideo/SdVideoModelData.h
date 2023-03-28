#ifndef SDVIDEOMODELDATA_H
#define SDVIDEOMODELDATA_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h"
class SdVideoModelDataPrivate;
class SdVideoModelData : public QObject
{
    Q_OBJECT
public:
    explicit SdVideoModelData(QObject *parent = nullptr);

    Q_INVOKABLE myModel* getObjectModel();
protected slots:
    void onVideoPlayerFileNames(const DeviceWatcherType type, const QString &xml);
public slots:
    void onLanguageChanged();
private:
    SdVideoModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(SdVideoModelData)
};

#endif // SDVIDEOMODELDATA_H
