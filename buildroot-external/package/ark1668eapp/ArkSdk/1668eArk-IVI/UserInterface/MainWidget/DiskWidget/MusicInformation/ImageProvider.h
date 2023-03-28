#ifndef IMAGEPROVIDER_H
#define IMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QImage>
class ImageProvider : public QQuickImageProvider
{
public:
    ImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize& requestedSize);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize& requestedSize);
    void setImageRc(const QImage &image);
public:
    QImage img;
};

#endif // IMAGEPROVIDER_H
