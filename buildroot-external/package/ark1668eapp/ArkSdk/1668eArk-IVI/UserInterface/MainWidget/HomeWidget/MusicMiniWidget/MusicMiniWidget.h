#ifndef MUSICMINIWIDGET_H
#define MUSICMINIWIDGET_H

#include <QObject>
#include "BusinessLogic/Audio.h"
class MusicMiniWidgetPrivate;
class MusicMiniWidget : public QObject
{
    Q_OBJECT
public:
    explicit MusicMiniWidget(QObject *parent = nullptr);
    void setMusicMiniWidgetObject(QObject* qmlObject);
protected slots:
    void onMusicPlayerID3TagChange(const int type, const int index, const QString &fileName, const QString& title, const QString& artist, const QString& album, const int endTime);
    void onMusicPlayerPlayStatus(const int type, const int status);
    void onMusicStatusChange(const QString fileName, const int status);
    void onHolderChange(const AudioSource oldHolder, const AudioSource newHolder);
    void onUsbMediaPlayExit();
    void onSdMediaPlayExit();
    void onConnectStatusChange(int status);
    void onBtMusicID3InfoChange(QString titile,QString artist,QString album);
public slots:
    void onToolButtonRelease();
private:
    MusicMiniWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MusicMiniWidget)
};

#endif // MUSICMINIWIDGET_H
