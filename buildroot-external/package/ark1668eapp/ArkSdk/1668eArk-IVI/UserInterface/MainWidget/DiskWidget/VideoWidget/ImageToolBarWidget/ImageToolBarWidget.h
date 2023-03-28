#ifndef IMAGETOOLBARWIDGET_H
#define IMAGETOOLBARWIDGET_H

#include <QObject>
#include "./BusinessLogic/Multimedia.h"
class ImageToolBarWidgetPrivate;
class ImageToolBarWidget : public QObject
{
    Q_OBJECT
public:
    explicit ImageToolBarWidget(QObject *parent = nullptr);
    void setImageToolBarWidgetObject(QObject* qmlObject);
    void getImageWidgetObject();
public slots:
    void onToolButtonRelease();
protected slots:
    void onImagePlayerChange(const DeviceWatcherType type, const QString &filePath, const int index, const int percent, const int rotate);
    void onUsbMediaPlayExit();
    void onSdMediaPlayExit();
private:
    ImageToolBarWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(ImageToolBarWidget)
};

#endif // IMAGETOOLBARWIDGET_H
