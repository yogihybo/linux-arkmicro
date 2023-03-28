#ifndef USBMUSICLISTMODELDATA_H
#define USBMUSICLISTMODELDATA_H

#include <QObject>

class UsbMusicListModelData : public QObject
{
    Q_OBJECT
public:
    explicit UsbMusicListModelData(QObject *parent = nullptr);

signals:

public slots:
};

#endif // USBMUSICLISTMODELDATA_H