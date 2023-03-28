#ifndef SDMUSICLISTMODELDDATA_H
#define SDMUSICLISTMODELDDATA_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "./BusinessLogic/Multimedia.h"
#include "UserInterface/MainWidget/DiskWidget/UsbDiskWidget/UsbMusicListModel/UsbMusicListModel.h"
class SdMusicListModelDdataPrivate;
class SdMusicListModelDdata : public QObject
{
    Q_OBJECT
public:
    explicit SdMusicListModelDdata(QObject *parent = nullptr);

    Q_INVOKABLE UsbMusicListModel* getObjectModel();
public slots:
    void onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml);
    void onMusicPlayerFileArtist(const DeviceWatcherType type,QStringList fileArtist);
    void onLanguageChanged();
private:
    SdMusicListModelDdataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(SdMusicListModelDdata)
};

#endif // SDMUSICLISTMODELDDATA_H
