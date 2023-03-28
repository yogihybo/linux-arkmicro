#include "ImageToolBarWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Multimedia.h"
#include "BusinessLogic/Widget.h"
#include "../VideoWidget.h"
#include <QQmlProperty>
#include <QDomDocument>
#include <QDebug>
class ImageToolBarWidgetPrivate
{
    Q_DISABLE_COPY(ImageToolBarWidgetPrivate)
public:
    explicit ImageToolBarWidgetPrivate(ImageToolBarWidget* parent);
    ~ImageToolBarWidgetPrivate();
    void connectAllSlots();
public:
    QObject* m_ImageToolBarWidgetObject;
    QObject* m_ZoomOutBtnObject;
    QObject* m_ZoomInBtnObject;
    QObject* m_PrevBtnObject;
    QObject* m_ToggleBtnObject;
    QObject* m_NextBtnObject;
    QObject* m_RotateBtnObject;
    QObject* m_FullScreenBtnObject;
    QObject* m_ImageWidgetObject;
    QObject* m_PixmapObject;
    QObject* m_AnimatedObject;
    float m_Scale;
    float scale_step;
    bool  m_Gif;
    int   m_Rotate;
    int   m_DeviceType;
    unsigned short int m_OriginalWidth;
    unsigned short int m_OriginalHeight;
    unsigned short int m_ImageWidgetRootWidth;
    unsigned short int m_ImageWidgetRootHeight;
private:
    Q_DECLARE_PUBLIC(ImageToolBarWidget)
    ImageToolBarWidget* const q_ptr;
};

ImageToolBarWidget::ImageToolBarWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new ImageToolBarWidgetPrivate(this))
{

}
void ImageToolBarWidget::getImageWidgetObject()
{
    Q_D(ImageToolBarWidget);
    VideoWidget* ptr = (VideoWidget*)parent();
    d->m_ImageWidgetObject = ptr->getImageWidgetObject();
    d->m_PixmapObject      = ptr->getPixmapObject();
    d->m_AnimatedObject    = ptr->getAnimatedObject();
    d->m_ImageWidgetRootWidth = d->m_ImageWidgetObject->property("width").toInt();
    d->m_ImageWidgetRootHeight= d->m_ImageWidgetObject->property("height").toInt();
}
void ImageToolBarWidget::setImageToolBarWidgetObject(QObject* qmlObject)
{
    Q_D(ImageToolBarWidget);
    if(d->m_ImageToolBarWidgetObject == NULL){

        d->m_ImageToolBarWidgetObject = qmlObject;
    }
    if(d->m_ImageToolBarWidgetObject != NULL)
    {
        if(d->m_ZoomOutBtnObject == NULL)
        {
            d->m_ZoomOutBtnObject = d->m_ImageToolBarWidgetObject->findChild<QObject*>("zoomOutBtnObject");
        }
        if(d->m_ZoomInBtnObject == NULL)
        {
            d->m_ZoomInBtnObject = d->m_ImageToolBarWidgetObject->findChild<QObject*>("zoomInBtnObject");
        }
        if(d->m_PrevBtnObject == NULL)
        {
            d->m_PrevBtnObject = d->m_ImageToolBarWidgetObject->findChild<QObject*>("prevBtnObject");
        }

        if(d->m_ToggleBtnObject == NULL)
        {
            d->m_ToggleBtnObject = d->m_ImageToolBarWidgetObject->findChild<QObject*>("toggleBtnObject");
        }

        if(d->m_NextBtnObject == NULL)
        {
            d->m_NextBtnObject = d->m_ImageToolBarWidgetObject->findChild<QObject*>("nextBtnObject");
        }

        if(d->m_RotateBtnObject == NULL)
        {
            d->m_RotateBtnObject = d->m_ImageToolBarWidgetObject->findChild<QObject*>("rotateBtnObject");
        }

        if(d->m_FullScreenBtnObject == NULL)
        {
            d->m_FullScreenBtnObject = d->m_ImageToolBarWidgetObject->findChild<QObject*>("fullScreenBtnObject");
        }
    }
    getImageWidgetObject();
    QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
    QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
    QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(false);
    QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(false);
    QQmlProperty(d->m_RotateBtnObject,"enabled").write(false);
    QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(false);
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_ZoomOutBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_ZoomInBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

    QObject::connect(d->m_PrevBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_ToggleBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_NextBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_RotateBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_FullScreenBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);

}
void ImageToolBarWidget::onUsbMediaPlayExit(){
    Q_D(ImageToolBarWidget);
    if(d->m_DeviceType == DWT_USBDisk){
        QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(false);
        QQmlProperty(d->m_RotateBtnObject,"enabled").write(false);
        QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        if(d->m_ImageToolBarWidgetObject->property("isImageFullScreen").toBool() == true)
        {
            QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(true);
            QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(true);
            QQmlProperty(d->m_ImageToolBarWidgetObject,"isImageFullScreen").write(false);
        }
    }
}
void ImageToolBarWidget::onSdMediaPlayExit(){
    Q_D(ImageToolBarWidget);
    if(d->m_DeviceType == DWT_SDDisk){
        QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(false);
        QQmlProperty(d->m_RotateBtnObject,"enabled").write(false);
        QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        if(d->m_ImageToolBarWidgetObject->property("isImageFullScreen").toBool() == true)
        {
            QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(true);
            QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(true);
            QQmlProperty(d->m_ImageToolBarWidgetObject,"isImageFullScreen").write(false);
        }
    }
}
void ImageToolBarWidget::onToolButtonRelease()
{
    Q_D(ImageToolBarWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_ZoomOutBtnObject)
    {
        float tmpscale = d->m_Scale + d->scale_step;
        unsigned short int w = d->m_OriginalWidth  * tmpscale;
        unsigned short int h = d->m_OriginalHeight * tmpscale;
        if(d->m_Gif == true)
        {
            if(w <= d->m_ImageWidgetRootWidth && h <= d->m_ImageWidgetRootWidth){
                QQmlProperty(d->m_AnimatedObject,"width").write(w);
                QQmlProperty(d->m_AnimatedObject,"height").write(h);
                 d->m_Scale += d->scale_step;
            }
        }
        else{
            if(w <= d->m_ImageWidgetRootWidth && h <= d->m_ImageWidgetRootWidth){
                QQmlProperty(d->m_PixmapObject,"width").write(w);
                QQmlProperty(d->m_PixmapObject,"height").write(h);
                 d->m_Scale += d->scale_step;
            }
        }
    }
    else if(ptr == d->m_ZoomInBtnObject)
    {
        float tmpscale = d->m_Scale - d->scale_step;
        if(tmpscale <= 0.5)
        {
            tmpscale = 0.5;
        }
        unsigned short int w = d->m_OriginalWidth  * tmpscale;
        unsigned short int h = d->m_OriginalHeight * tmpscale;
        if(d->m_Gif == true)
        {
            if(tmpscale > 0.5){
                QQmlProperty(d->m_AnimatedObject,"width").write(w);
                QQmlProperty(d->m_AnimatedObject,"height").write(h);
                d->m_Scale -= d->scale_step;
            }
            else{
                QQmlProperty(d->m_AnimatedObject,"width").write(w);
                QQmlProperty(d->m_AnimatedObject,"height").write(h);
                d->m_Scale = 0.5;
            }
        }
        else{
            if(tmpscale > 0.5){
                QQmlProperty(d->m_PixmapObject,"width").write(w);
                QQmlProperty(d->m_PixmapObject,"height").write(h);
                d->m_Scale -= d->scale_step;
            }
            else{
                QQmlProperty(d->m_PixmapObject,"width").write(w);
                QQmlProperty(d->m_PixmapObject,"height").write(h);
                d->m_Scale = 0.5;
            }
        }
    }
    else if(ptr == d->m_PrevBtnObject)
    {
        QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
        g_Multimedia->imagePlayerPlayPreviousListViewIndex();
    }
    else if(ptr == d->m_ToggleBtnObject)
    {
        bool playStatus = d->m_ToggleBtnObject->property("playStatus").toBool();
        if(playStatus)
        {
            QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
            QQmlProperty(d->m_AnimatedObject,"paused").write(true);
            QQmlProperty(d->m_AnimatedObject,"play").write(false);
        }
        else
        {
            QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(true);
            QQmlProperty(d->m_AnimatedObject,"paused").write(false);
            QQmlProperty(d->m_AnimatedObject,"play").write(true);
        }
    }
    else if(ptr == d->m_NextBtnObject)
    {
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        g_Multimedia->imagePlayerPlayNextListViewIndex();
    }
    else if(ptr == d->m_RotateBtnObject)
    {
        d->m_Rotate++;
        d->m_Rotate %=4;
        if(d->m_Gif == true){
            QQmlProperty(d->m_AnimatedObject,"animatedRotation").write(d->m_Rotate*90);
            QQmlProperty(d->m_AnimatedObject,"rotation").write(d->m_Rotate*90);
        }
        else{
            QQmlProperty(d->m_PixmapObject,"pixmapRotation").write(d->m_Rotate*90);
            QQmlProperty(d->m_PixmapObject,"rotation").write(d->m_Rotate*90);
        }
    }
    else if(ptr == d->m_FullScreenBtnObject)
    {
        if(d->m_ImageToolBarWidgetObject->property("isImageFullScreen").toBool() == false)
        {
            QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(false);
            QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(false);
            QQmlProperty(d->m_ImageToolBarWidgetObject,"isImageFullScreen").write(true);
        }
        else{
            QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(true);
            QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(true);
            QQmlProperty(d->m_ImageToolBarWidgetObject,"isImageFullScreen").write(false);
        }
    }
}
void ImageToolBarWidget::onImagePlayerChange(const DeviceWatcherType type, const QString &filePath, const int index, const int percent, const int rotate)
{
    qDebug()<<"++++percent++++"<<percent;
    qDebug()<<"+++++++rotate++++++++"<<rotate;
    qDebug()<<"+++++++index++++++++"<<index;
    Q_D(ImageToolBarWidget);
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
    if(d->m_Gif ==true)
    {
        QQmlProperty(d->m_AnimatedObject,"index").write(index);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(true);
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(true);
        QQmlProperty(d->m_AnimatedObject,"paused").write(false);
        QQmlProperty(d->m_AnimatedObject,"play").write(true);
        d->m_OriginalWidth  = d->m_AnimatedObject->property("width").toInt();
        d->m_OriginalHeight = d->m_AnimatedObject->property("height").toInt();
        d->m_Scale = 1.0;
        d->m_Rotate = 0;
    }
    else{
        QQmlProperty(d->m_PixmapObject,"index").write(index);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        d->m_OriginalWidth  = d->m_PixmapObject->property("width").toInt();
        d->m_OriginalHeight = d->m_PixmapObject->property("height").toInt();
        d->m_Scale = 1.0;
        d->m_Rotate = 0;
    }
    QQmlProperty(d->m_PrevBtnObject,"enabled").write(true);
    QQmlProperty(d->m_NextBtnObject,"enabled").write(true);
    QQmlProperty(d->m_ZoomOutBtnObject,"enabled").write(true);
    QQmlProperty(d->m_ZoomInBtnObject,"enabled").write(true);
    QQmlProperty(d->m_RotateBtnObject,"enabled").write(true);
    QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(true);
}

ImageToolBarWidgetPrivate::ImageToolBarWidgetPrivate(ImageToolBarWidget *parent)
    : q_ptr(parent)
{
    m_ImageToolBarWidgetObject = NULL;
    m_ZoomOutBtnObject = NULL;
    m_ZoomInBtnObject  = NULL;
    m_PrevBtnObject    = NULL;
    m_ToggleBtnObject  = NULL;
    m_NextBtnObject    = NULL;
    m_RotateBtnObject  = NULL;
    m_FullScreenBtnObject= NULL;
    m_Scale    = 1.0f;
    scale_step = 0.5f;
    m_Gif = false;
    m_OriginalWidth  = 0;
    m_OriginalHeight = 0;
    m_ImageWidgetRootWidth = 0;
    m_ImageWidgetRootHeight= 0;
    m_Rotate = 0;
    m_DeviceType = -1;
    connectAllSlots();
}
ImageToolBarWidgetPrivate::~ImageToolBarWidgetPrivate()
{

}

void ImageToolBarWidgetPrivate::connectAllSlots()
{
    Q_Q(ImageToolBarWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onImagePlayerChange(const int, const QString &, const int, const int, const int)));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onUsbMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onSdMediaPlayExit()));
}

