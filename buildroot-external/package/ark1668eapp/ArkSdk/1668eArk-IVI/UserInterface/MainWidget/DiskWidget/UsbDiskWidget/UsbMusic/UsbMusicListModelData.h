#ifndef USBMUSICLISTMODELDATA_H
#define USBMUSICLISTMODELDATA_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "./BusinessLogic/Multimedia.h"
#include "BusinessLogic/Setting.h"
#include "../UsbMusicListModel/UsbMusicListModel.h"
class UsbMusicListModelDataPrivate;
class UsbMusicListModelData : public QObject
{
    Q_OBJECT
public:
    explicit UsbMusicListModelData(QObject *parent = nullptr);
    Q_INVOKABLE UsbMusicListModel* getObjectModel();
public slots:
    void onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml);
    void onMusicPlayerFileArtist(const DeviceWatcherType type,QStringList fileArtist);
    void onLanguageChanged();
private:
    UsbMusicListModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(UsbMusicListModelData)
};

#endif // USBMUSICLISTMODELDATA_H
