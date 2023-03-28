#ifndef MULTIMEDIAPLAYWIDGET_H
#define MULTIMEDIAPLAYWIDGET_H

#include <QObject>
#include <QProcess>
#include "BusinessLogic/Multimedia.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Audio.h"


class MultiMediaPlayWidgetPrivate;
class MultiMediaPlayWidget : public QObject
{
    Q_OBJECT
public:
    explicit MultiMediaPlayWidget(QObject *parent = nullptr);
    void setMultiMediaPlayWidgetObject(QObject* qmlObject);
    void setUsbMusicListviewObject(QObject* qmlObject);
    void setSdMusicListviewObject(QObject* qmlObject);
    void setMultiMediaObject(QObject* qmlObject);
    void startMusicTypeTimer();
signals:
    void startMusicPlay(int type,int index);
protected slots:
    void onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml);
    void onMusicPlayerPlayStatus(const int type, const int status);
    void onMusicPlayerPlayMode(const MusicPlayerPlayMode mode);
    void onMusicPlayerElapsedInformation(const int elapsedTime, const int elapsedMillesimal);
    void onHolderChange(const int oldHolder, const int newHolder);
    void onMusicPlayerID3TagChange(const int type, const int index, const QString &fileName, const QString& title, const QString& artist, const QString& album, const int endTime);
    void onMusicStatusChange(const QString& musicName, const int status);
    void onBtMusicID3InfoChange(QString titile,QString artist,QString album);
    void onBtMusicElapsedInfo(int elapsed,int EndTime);
    void onConnectStatusChange(int status);
    void onUsbMediaPlayExit();
    void onSdMediaPlayExit();
    void onMusicTypeChanged();
    void onVisibleChanged();
    void onMusicPlayerExit();
public slots:
    void onMusicListViewItemClicked(int type,int index);
    void onToolButtonRelease();
    void onSliderReleased(int value);
    void onTimeout();
    void onDeviceWatcherStatus(const int type, const int status);
private:
    MultiMediaPlayWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MultiMediaPlayWidget)
};

#endif // MULTIMEDIAPLAYWIDGET_H
