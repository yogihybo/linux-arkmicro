#include "ImageListWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Multimedia.h"
#include <QQmlProperty>
#include <QDomDocument>
#include <QDebug>
class ImageListWidgetPrivate
{
    Q_DISABLE_COPY(ImageListWidgetPrivate)
public:
    explicit ImageListWidgetPrivate(ImageListWidget* parent);
    ~ImageListWidgetPrivate();
    void connectAllSlots();
public:
    QObject* m_UsbImageListWidgetObject;
    QObject* m_UsbImageListviewObject;
    QObject* m_SdImageListWidgetObject;
    QObject* m_SdImageListviewObject;
    int m_UsbLastIndex;
    int m_SdLastIndex;
private:
    Q_DECLARE_PUBLIC(ImageListWidget)
    ImageListWidget* const q_ptr;
};

ImageListWidget::ImageListWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new ImageListWidgetPrivate(this))
{

}
void ImageListWidget::setUsbImageListWidgetObject(QObject *qmlObject){
    Q_D(ImageListWidget);
    if(d->m_UsbImageListWidgetObject == NULL)
    {
        d->m_UsbImageListWidgetObject = qmlObject;
    }
    if(d->m_UsbImageListWidgetObject != NULL)
    {
        if(d->m_UsbImageListviewObject == NULL)
        {
            d->m_UsbImageListviewObject = d->m_UsbImageListWidgetObject->findChild<QObject*>("listviewObject");
        }
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_UsbImageListWidgetObject, SIGNAL(imageListviewItemClicked(int,int)),
                     this,        SLOT(onImageListviewItemClicked(int,int)),type);
}
void ImageListWidget::setSdImageListWidgetObject(QObject* qmlObject){

    Q_D(ImageListWidget);
    if(d->m_SdImageListWidgetObject == NULL)
    {
        d->m_SdImageListWidgetObject = qmlObject;
    }
    if(d->m_SdImageListWidgetObject != NULL)
    {
        if(d->m_SdImageListviewObject == NULL)
        {
            d->m_SdImageListviewObject = d->m_SdImageListWidgetObject->findChild<QObject*>("listviewObject");
        }
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_SdImageListWidgetObject, SIGNAL(imageListviewItemClicked(int,int)),
                     this,        SLOT(onImageListviewItemClicked(int,int)),type);
}
void ImageListWidget::onImageListviewItemClicked(int type,int index){
    Q_D(ImageListWidget);
    if(type == DWT_USBDisk)
    {
        d->m_UsbLastIndex = index;
    }
    else if(type == DWT_SDDisk)
    {
        d->m_SdLastIndex = index;
    }
}
void ImageListWidget::onImagePlayerChange(const DeviceWatcherType type, const QString &filePath, const int index, const int percent, const int rotate)
{
    Q_D(ImageListWidget);
    if(type == DWT_USBDisk){
        if(d->m_UsbLastIndex != index)
        {
            QQmlProperty(d->m_UsbImageListWidgetObject,"itemClicked").write(false);
        }
        if(!d->m_UsbImageListWidgetObject->property("itemClicked").toBool())
        {
            QQmlProperty(d->m_UsbImageListviewObject,"contentY").write(88*index);
            QQmlProperty(d->m_UsbImageListviewObject,"currentIndex").write(index);
            QQmlProperty(d->m_UsbImageListWidgetObject,"listViewCurrentIndex").write(index);
        }
        d->m_UsbLastIndex = index;
    }
    else if(type == DWT_SDDisk){
        if(d->m_SdLastIndex != index)
        {
            QQmlProperty(d->m_SdImageListWidgetObject,"itemClicked").write(false);
        }
        if(!d->m_SdImageListWidgetObject->property("itemClicked").toBool())
        {
            QQmlProperty(d->m_SdImageListviewObject,"contentY").write(88*index);
            QQmlProperty(d->m_SdImageListviewObject,"currentIndex").write(index);
            QQmlProperty(d->m_SdImageListWidgetObject,"listViewCurrentIndex").write(index);
        }
        d->m_SdLastIndex = index;
    }

}

ImageListWidgetPrivate::ImageListWidgetPrivate(ImageListWidget *parent)
    : q_ptr(parent)
{
    m_UsbImageListWidgetObject = NULL;
    m_UsbImageListviewObject   = NULL;
    m_SdImageListWidgetObject   = NULL;
    m_SdImageListviewObject    = NULL;
    m_UsbLastIndex = -1;
    m_SdLastIndex  = -1;
    connectAllSlots();
}
ImageListWidgetPrivate::~ImageListWidgetPrivate()
{

}


void ImageListWidgetPrivate::connectAllSlots()
{
    Q_Q(ImageListWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onImagePlayerChange(const int, const QString &, const int, const int, const int)));
}

