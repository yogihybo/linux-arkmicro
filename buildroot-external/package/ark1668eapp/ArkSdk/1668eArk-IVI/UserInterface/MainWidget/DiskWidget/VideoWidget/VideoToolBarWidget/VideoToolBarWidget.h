#ifndef VIDEOTOOLBARWIDGET_H
#define VIDEOTOOLBARWIDGET_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
class VideoToolBarWidgetPrivate;
class VideoToolBarWidget : public QObject
{
    Q_OBJECT
public:
    explicit VideoToolBarWidget(QObject *parent = nullptr);
    void setVideoToolBarWidgetObject(QObject* qmlObject);
    void setVideoToolBarWidgetVisible(bool visble);
signals:
    void mousePressed();
protected slots:
    void onVideoPlayerPlayStatus(const DeviceWatcherType type, const VideoPlayerPlayStatus status);
    void onSliderTouchEnable(const bool flag);
    void onVideoPlayerInformation(const int type, const int index, const QString &fileName, const int endTime);
    void onVideoPlayerElapsedInformation(const int elapsedTime, const int elapsedMillesimal);
    void onHolderChange(const int oldHolder, const int newHolder);
    void onUsbMediaPlayExit();
    void onSdMediaPlayExit();
public slots:
    void onToolButtonRelease();
    void onMousePressed();
    void onSliderReleased(int value);

private:
    VideoToolBarWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(VideoToolBarWidget)
};

#endif // VIDEOTOOLBARWIDGET_H
