#ifndef ALLCALLLOGMODELDATA_H
#define ALLCALLLOGMODELDATA_H

#include <QObject>
#include "AllCallLogModel.h"
#include "BusinessLogic/Bluetooth.h"
class AllCallLogModelDataPrivate;
class AllCallLogModelData : public QObject
{
    Q_OBJECT
public:
    explicit AllCallLogModelData(QObject *parent = nullptr);
    Q_INVOKABLE AllCallLogModel* getObjectModel();
protected slots:
    void onSyncAllCallLog();
    void onConnectStatusChange(const int status);
private:
    AllCallLogModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AllCallLogModelData)
};

#endif // ALLCALLLOGMODELDATA_H
