#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include "DeviceWatcher/DeviceWatcher.h"
#include <QObject>
#include <QProcess>
#include <QScopedPointer>

class VideoPlayerPrivate;
class VideoPlayer : public QObject
{
    Q_OBJECT
#ifdef g_VideoPlayer
#undef g_VideoPlayer
#endif
#define g_VideoPlayer (VideoPlayer::instance())
public:
    inline static VideoPlayer* instance() {
        static VideoPlayer* videoPlayer(new VideoPlayer(qApp));
        return videoPlayer;
    }
    void videoPlayerRequestFileNames(const int type);
    void videoPlayerSetPlayModeToggle();
    void videoPlayerSetPlayMode(const int mode);
    void videoPlayerSetPlayStatusToggle();
    void videoPlayerSetPlayStatus(const int status);
    void videoPlayerPlayListViewIndex(const int type, const int index, const int x, const int y, const int width, const int height, const int millesimal);
    void videoPlayerPlayPreviousListViewIndex();
    void videoPlayerPlayNextListViewIndex();
    void videoPlayerSeekToMillesimal(const int millesimal);
    void videoPlayerExit();
    void videoPlayerVisible(const bool flag);
    void playVideoIndex(const DeviceWatcherType type, const int index, const int x, const int y, const int width, const int height, const int millesimal);
    void videoPlayerSetGeometry(int x,int y,int width,int height);
signals:
    void onVideoPlayerPlayMode(const int mode);
    void onVideoPlayerShowStatus(const int status);
    void onVideoPlayerPlayStatus(const int type, const int status);
    void onVideoPlayerFileNames(const int type, const QString &xml);
    //void onVideoPlayerFilePath(const int type, const QString& path);
    void onVideoPlayerInformation(const int type, const int index, const QString &fileName, const int endTime);
    void onVideoPlayerElapsedInformation(const int elapsedTime, const int elapsedMillesimal);
    void onVideoPlayerVisible(const bool flag);
    void onSliderTouchEnable(const bool flag);   //dll add 20190805
    void onVideoPlayerExit();
protected slots:
    void onDeviceWatcherStatus(const int type, const int status);
    void onVideoFilePath(const QString &path, const int type);
private:
    explicit VideoPlayer(QObject* parent = NULL);
    ~VideoPlayer();
    VideoPlayerPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(VideoPlayer)
};

#endif // VIDEOPLAYER_H
