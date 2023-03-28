#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
class ImageWidgetPrivate;
class ImageWidget : public QObject
{
    Q_OBJECT
public:
    explicit ImageWidget(QObject *parent = nullptr);
    void setImageWidgetObject(QObject* qmlObject);
    QObject* getImageWidgetObject();
    QObject* getPixmapObject();
    QObject* getAnimatedObject();
protected slots:
    void onImagePlayerChange(const DeviceWatcherType type, const QString &filePath, const int index, const int percent, const int rotate);
    void onUsbMediaPlayExit();
    void onSdMediaPlayExit();
private:
    ImageWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(ImageWidget)
};

#endif // IMAGEWIDGET_H
