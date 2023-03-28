#ifndef IMAGELISTWIDGET_H
#define IMAGELISTWIDGET_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
class ImageListWidgetPrivate;
class ImageListWidget : public QObject
{
    Q_OBJECT
public:
    explicit ImageListWidget(QObject *parent = nullptr);
    void setUsbImageListWidgetObject(QObject* qmlObject);
    void setSdImageListWidgetObject(QObject* qmlObject);
protected slots:
    void onImagePlayerChange(const DeviceWatcherType type, const QString &filePath, const int index, const int percent, const int rotate);
public slots:
    void onImageListviewItemClicked(int type,int index);
private:
    ImageListWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(ImageListWidget)
};

#endif // IMAGELISTWIDGET_H
