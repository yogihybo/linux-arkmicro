#ifndef VIDEOLISTWIDGET_H
#define VIDEOLISTWIDGET_H

#include <QObject>
class VideoListWidgetPrivate;
class VideoListWidget : public QObject
{
    Q_OBJECT
public:
    explicit VideoListWidget(QObject *parent = nullptr);
    void setListWidgetObject(QObject* qmlObject);
    int  getDeviceWatcherType();
signals:
    void videoListviewItemClicked(int type,int index);
    void imageListviewItemClicked(int type,int index);
    void videoBtnClicked(int type);
    void imageBtnClicked(int type);
public slots:
    void onVideoListviewItemClicked(int index);
    void onImageListviewItemClicked(int type,int index);
    void onSDBtnClicked();
    void onUSBBtnClicled();
protected slots:
    void onVideoPlayerInformation(const int type, const int index, const QString &fileName, const int endTime);
    void onDeviceWatcherStatus(const int type, const int status);
    void onVideoBtnClicked();
    void onImageBtnClicled();

private:
    VideoListWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(VideoListWidget)
};

#endif // VIDEOLISTWIDGET_H
