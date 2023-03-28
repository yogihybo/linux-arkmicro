#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
class VideoWidgetPrivate;
class VideoWidget : public QObject
{
    Q_OBJECT
public:
    explicit VideoWidget(QObject *parent = nullptr);
    void setVideoObject(QObject* qmlObject);
    void setParentObject(QObject* qmlObject);
    QObject* getImageWidgetObject();
    QObject* getPixmapObject();
    QObject* getAnimatedObject();
signals:
    void startVideoListviewItem(int type,int index);
protected slots:
    void onVideoPlayerFileNames(const DeviceWatcherType type, const QString &xml);
    void onVideoPlayerPlayStatus(const DeviceWatcherType type, const VideoPlayerPlayStatus playStatus);
    void onHolderChange(const int oldHolder, const int newHolder);
    void onVideoPlayerElapsedInformation(const int elapsedTime, const int elapsedMillesimal);
    void onVideoPlayerInformation(const int type, const int index, const QString &fileName, const int endTime);
public slots:
    void onVideoListviewItemClicked(int type,int index);
    void onImageListviewItemClicked(int type,int index);
    void onVideoPlayerPlayError();
    void onTimeout();
    void onVideoBtnClicked();
    void onMousePressed();
    void onVideoListWidgetVideoBtnClicked(int type);
    void onVideoListWidgetImageBtnClicked(int type);
    void onTypeStatusChanged();
    void onVisibleChanged();
    void onVideoPlayerExit();
private:
    VideoWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(VideoWidget)
};

#endif // VIDEOWIDGET_H
