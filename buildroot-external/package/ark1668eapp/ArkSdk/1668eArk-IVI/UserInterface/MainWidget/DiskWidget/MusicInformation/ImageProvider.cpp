#include "ImageProvider.h"
#include <QDebug>

ImageProvider::ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image)
{

}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
//    qDebug()<<"ImageProvider requestImage"<<id<<size<<requestedSize;
    return img;
}

QPixmap ImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
//    qDebug()<<"ImageProvider requestPixmap";
    return QPixmap::fromImage(img);
}

void ImageProvider::setImageRc(const QImage &image)
{
    img = QImage();
    img = image;
}
