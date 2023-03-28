#include "ImageWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Multimedia.h"
#include "BusinessLogic/Widget.h"
#include <QQmlProperty>
#include <QDomDocument>
#include <QDebug>

class ImageWidgetPrivate
{
    Q_DISABLE_COPY(ImageWidgetPrivate)
public:
    explicit ImageWidgetPrivate(ImageWidget* parent);
    ~ImageWidgetPrivate();
    void connectAllSlots();
public:
    QObject* m_ImageWidgetObject;
    QObject* m_PixmapObject;
    QObject* m_AnimatedObject;
    QList<QString> m_UsbImageList;
    bool m_Gif;
    int  m_DeviceType;
private:
    Q_DECLARE_PUBLIC(ImageWidget)
    ImageWidget* const q_ptr;
};

ImageWidget::ImageWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new ImageWidgetPrivate(this))
{

}
void ImageWidget::setImageWidgetObject(QObject* qmlObject){
    Q_D(ImageWidget);
    if(d->m_ImageWidgetObject == NULL)
    {
        d->m_ImageWidgetObject = qmlObject;
    }
    if(d->m_ImageWidgetObject != NULL)
    {
        if(d->m_PixmapObject == NULL)
        {
            d->m_PixmapObject = d->m_ImageWidgetObject->findChild<QObject*>("pixmapObject");
        }
        if(d->m_AnimatedObject == NULL)
        {
            d->m_AnimatedObject = d->m_ImageWidgetObject->findChild<QObject*>("animatedObject");
        }
    }
}

QObject* ImageWidget::getImageWidgetObject(){
    Q_D(ImageWidget);
    return d->m_ImageWidgetObject;
}
QObject* ImageWidget::getPixmapObject(){
    Q_D(ImageWidget);
    return d->m_PixmapObject;
}
QObject* ImageWidget::getAnimatedObject(){
    Q_D(ImageWidget);
    return d->m_AnimatedObject;
}

void ImageWidget::onImagePlayerChange(const DeviceWatcherType type, const QString &filePath, const int index, const int percent, const int rotate)
{
    Q_D(ImageWidget);
    qDebug()<<"+++++++filePath++++++++++"<<filePath;
    d->m_DeviceType = type;
    if(filePath.size()>3)
    {
        if(filePath.right(3) == "gif")
        {
            d->m_Gif = true;
        }
        else
        {
            d->m_Gif = false;
        }
    }

    if(d->m_Gif == true)
    {
        QString pixmapPath = QString("file://") + filePath;
        QQmlProperty(d->m_AnimatedObject,"source").write(pixmapPath);
        QQmlProperty(d->m_AnimatedObject,"visible").write(true);
        QQmlProperty(d->m_PixmapObject,"visible").write(false);
    }
    else
    {
        QString pixmapPath = QString("file://") + filePath;
        QQmlProperty(d->m_PixmapObject,"source").write(pixmapPath);
        QQmlProperty(d->m_PixmapObject,"visible").write(true);
        QQmlProperty(d->m_AnimatedObject,"visible").write(false);
    }
}
void ImageWidget::onUsbMediaPlayExit(){
    Q_D(ImageWidget);
    if(d->m_DeviceType == DWT_USBDisk){
       QQmlProperty(d->m_AnimatedObject,"source").write("");
       QQmlProperty(d->m_PixmapObject,"source").write("");
       QQmlProperty(d->m_PixmapObject,"visible").write(false);
       QQmlProperty(d->m_AnimatedObject,"visible").write(false);
       d->m_DeviceType = DWT_Undefine;
    }
}
void ImageWidget::onSdMediaPlayExit(){
    Q_D(ImageWidget);
    if(d->m_DeviceType == DWT_SDDisk){
        QQmlProperty(d->m_AnimatedObject,"source").write("");
        QQmlProperty(d->m_PixmapObject,"source").write("");
        QQmlProperty(d->m_PixmapObject,"visible").write(false);
        QQmlProperty(d->m_AnimatedObject,"visible").write(false);
        d->m_DeviceType = DWT_Undefine;
    }
}

ImageWidgetPrivate::ImageWidgetPrivate(ImageWidget *parent)
    : q_ptr(parent)
{
    m_Gif = false;
    m_ImageWidgetObject = NULL;
    m_PixmapObject = NULL;
    m_AnimatedObject = NULL;
    m_DeviceType = -1;
    connectAllSlots();
}
ImageWidgetPrivate::~ImageWidgetPrivate()
{

}

void ImageWidgetPrivate::connectAllSlots()
{
    Q_Q(ImageWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onImagePlayerChange(const int, const QString &, const int, const int, const int)));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onUsbMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onSdMediaPlayExit()));
}

