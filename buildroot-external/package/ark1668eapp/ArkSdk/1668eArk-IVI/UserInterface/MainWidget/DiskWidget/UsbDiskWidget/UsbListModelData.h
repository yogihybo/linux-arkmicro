#ifndef USBLISTMODELDATA_H
#define USBLISTMODELDATA_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "./BusinessLogic/Multimedia.h"
#include "UsbMusicListModel/UsbMusicListModel.h"
class UsbListModelData : public QObject
{
    Q_OBJECT
public:
    explicit UsbListModelData(QObject *parent = nullptr);
    Q_INVOKABLE UsbMusicListModel* getObjectModel();
public slots:
    void onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml);
    void onMusicPlayerFileArtist(QStringList fileArtist);
public:
    QList<QString> m_UsbMusicList;
    QStringList m_LastUsbMusicList;
    QStringList m_UsbMusicFileArtistList;
    int m_Elapsed;
    int m_CurrentIndex;
    int m_LastIndex;
    UsbMusicListModel* m_UsbMusicListModel;
};

#endif // USBLISTMODELDATA_H
