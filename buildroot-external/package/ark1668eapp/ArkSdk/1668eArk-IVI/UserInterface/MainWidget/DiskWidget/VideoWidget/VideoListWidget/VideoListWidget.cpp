#include "VideoListWidget.h"
#include "UserInterface/MainWidget/DiskWidget/VideoWidget/ImageListWidget/ImageListWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Multimedia.h"
#include <QQmlProperty>
class VideoListWidgetPrivate
{
    Q_DISABLE_COPY(VideoListWidgetPrivate)
public:
    explicit VideoListWidgetPrivate(VideoListWidget* parent);
    ~VideoListWidgetPrivate();
    void initializeImageListWidget();
    void connectAllSlots();
public:
    QObject* m_ListWidgetObject;
    QObject* m_SdTypeBtnObject;
    QObject* m_UsbTypeBtnObject;
    QObject* m_VideoBtnObject;
    QObject* m_ImageBtnObject;
    QObject* m_UsbImageListWidgetObject;
    QObject* m_UsbVideoListWidgetObject;
    QObject* m_UsbVideoListviewObject;
    QObject* m_SdVideoListWidgetObject;
    QObject* m_SdVideoListviewObject;
    QObject* m_SdImageListWidgetObject;
    ImageListWidget* m_ImageListWidget;
    DeviceWatcherType m_DeviceWatcherType;
    bool     m_SdExist;
    bool     m_UsbExist;
    int      m_UsbLastIndex;
    int      m_SdLastIndex;
private:
    Q_DECLARE_PUBLIC(VideoListWidget)
    VideoListWidget* const q_ptr;
};

VideoListWidget::VideoListWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new VideoListWidgetPrivate(this))
{

}

void VideoListWidget::setListWidgetObject(QObject* qmlObject){
    Q_D(VideoListWidget);
    if(d->m_ListWidgetObject == NULL)
    {
        d->m_ListWidgetObject = qmlObject;
        if(d->m_ListWidgetObject != NULL)
        {
            if(d->m_SdTypeBtnObject == NULL)
            {
                d->m_SdTypeBtnObject  = d->m_ListWidgetObject->findChild<QObject*>("sdTypeBtnObject");
            }
            if(d->m_UsbTypeBtnObject == NULL)
            {
                d->m_UsbTypeBtnObject = d->m_ListWidgetObject->findChild<QObject*>("usbTypeBtnObject");
            }
            if(d->m_VideoBtnObject == NULL)
            {
                d->m_VideoBtnObject = d->m_ListWidgetObject->findChild<QObject*>("videoBtnObject");
            }
            if(d->m_ImageBtnObject == NULL)
            {
                d->m_ImageBtnObject = d->m_ListWidgetObject->findChild<QObject*>("imageBtnObject");
            }
        }
    }
    if(d->m_ListWidgetObject != NULL)
    {
        if(d->m_UsbVideoListWidgetObject == NULL)
        {
            d->m_UsbVideoListWidgetObject = d->m_ListWidgetObject->findChild<QObject*>("videoListWidgetObject");
        } 
    }

    if(d->m_UsbVideoListWidgetObject != NULL)
    {
        if(d->m_UsbVideoListviewObject == NULL)
        {
            d->m_UsbVideoListviewObject = d->m_UsbVideoListWidgetObject->findChild<QObject*>("listviewObject");
        }
    }
    if(d->m_ListWidgetObject != NULL)
    {
        if(d->m_UsbImageListWidgetObject == NULL)
        {
            d->m_UsbImageListWidgetObject = d->m_ListWidgetObject->findChild<QObject*>("imageListWidgetObject");
        }
    }
    if(d->m_ListWidgetObject != NULL)
    {
        if(d->m_SdImageListWidgetObject == NULL)
        {
            d->m_SdImageListWidgetObject = d->m_ListWidgetObject->findChild<QObject*>("sDimageListWidgetObject");
        }
    }

    if(d->m_ListWidgetObject != NULL)
    {
        if(d->m_SdVideoListWidgetObject == NULL)
        {
            d->m_SdVideoListWidgetObject = d->m_ListWidgetObject->findChild<QObject*>("sdVideoListWidgetObject");
        }
    }

    if(d->m_SdVideoListWidgetObject != NULL)
    {
        if(d->m_SdVideoListviewObject == NULL)
        {
            d->m_SdVideoListviewObject = d->m_SdVideoListWidgetObject->findChild<QObject*>("listviewObject");
        }
    }

    connectSignalAndSlotByNamesake(g_Multimedia, this, ARKRECEIVER(onDeviceWatcherStatus(const int, const int)));
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_UsbVideoListWidgetObject, SIGNAL(videoListviewItemClicked(int)),
                     this,        SLOT(onVideoListviewItemClicked(int)),type);
    QObject::connect(d->m_SdVideoListWidgetObject, SIGNAL(videoListviewItemClicked(int)),
                     this,        SLOT(onVideoListviewItemClicked(int)),type);
    QObject::connect(d->m_UsbImageListWidgetObject, SIGNAL(imageListviewItemClicked(int,int)),
                     this,        SLOT(onImageListviewItemClicked(int,int)),type);
    QObject::connect(d->m_SdImageListWidgetObject, SIGNAL(imageListviewItemClicked(int,int)),
                     this,        SLOT(onImageListviewItemClicked(int,int)),type);
    QObject::connect(d->m_VideoBtnObject, SIGNAL(clicked()),
                     this,        SLOT(onVideoBtnClicked()),type);
    QObject::connect(d->m_ImageBtnObject, SIGNAL(clicked()),
                     this,        SLOT(onImageBtnClicled()),type);

    QObject::connect(d->m_SdTypeBtnObject, SIGNAL(clicked()),
                     this,        SLOT(onSDBtnClicked()),type);
    QObject::connect(d->m_UsbTypeBtnObject, SIGNAL(clicked()),
                     this,        SLOT(onUSBBtnClicled()),type);

}

void VideoListWidget::onSDBtnClicked(){
    Q_D(VideoListWidget);
    d->m_DeviceWatcherType = DWT_SDDisk;
    int deviceType = d->m_SdTypeBtnObject->property("sdSelect").toBool();
    if(deviceType == false && d->m_SdExist)
    {
        QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(false);
        QQmlProperty(d->m_SdTypeBtnObject,"sdSelect").write(true);
        QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(true);
        QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(true);
        QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
        QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
    }
}
void VideoListWidget::onUSBBtnClicled(){
    Q_D(VideoListWidget);
    d->m_DeviceWatcherType = DWT_USBDisk;
    int deviceType = d->m_SdTypeBtnObject->property("usbSelect").toBool();
    if(deviceType == false && d->m_UsbExist)
    {
        QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(true);
        QQmlProperty(d->m_SdTypeBtnObject,"sdSelect").write(false);
        QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(true);
        QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(true);
        QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
        QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
    }
}

void VideoListWidget::onVideoBtnClicked()
{
    Q_D(VideoListWidget);
    qDebug()<<"+++[VideoListWidget::onVideoBtnClicked:m_DeviceWatcherType]+++"<<d->m_DeviceWatcherType;
    QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(true);
    QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
    if(d->m_DeviceWatcherType == DWT_USBDisk)
    {
        QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(true);
        QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);

    }
    else if(d->m_DeviceWatcherType == DWT_SDDisk){
        QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(true);

    }
    emit videoBtnClicked(d->m_DeviceWatcherType);
}

void VideoListWidget::onImageBtnClicled()
{
    Q_D(VideoListWidget);
    d->initializeImageListWidget();
    QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(false);
    QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(true);
    g_Multimedia->videoPlayerExit();
    qDebug()<<"+++[VideoListWidget::onImageBtnClicled:m_DeviceWatcherType]+++"<<d->m_DeviceWatcherType;
    if(d->m_DeviceWatcherType == DWT_USBDisk)
    {
        QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(true);
        QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);
    }
    else if(d->m_DeviceWatcherType == DWT_SDDisk){
        QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
        QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(true);
        QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);
    }
    emit imageBtnClicked(d->m_DeviceWatcherType);
}

void VideoListWidget::onDeviceWatcherStatus(const int type, const int status){
    Q_D(VideoListWidget);
    if (DWT_USBDisk == type) {
        switch (status) {
            case DWS_Empty: {
                break;
            }
            case DWS_Unsupport: {
                break;
            }
            case DWS_Busy: {

                break;
            }
            case DWS_Ready: {
                d->m_UsbExist = true;
                if(d->m_SdExist == false)
                {
                    d->m_DeviceWatcherType = type;
                    QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(true);
                    QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(true);
                    QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(true);
                    QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
                }
                QQmlProperty(d->m_UsbTypeBtnObject,"enabled").write(true);
                break;
            }
            case DWS_Remove: {
                d->m_UsbExist = false;
                if(d->m_SdExist == false)
                {
                    d->m_DeviceWatcherType = -1;
                    QQmlProperty(d->m_SdTypeBtnObject,"sdSelect").write(false);
                    QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(false);
                    QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_UsbTypeBtnObject,"enabled").write(false);
                    QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
                    QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(false);
                    QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
                }
                else{
                    d->m_DeviceWatcherType = 0;
                    QQmlProperty(d->m_SdTypeBtnObject,"sdSelect").write(true);
                    QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(false);
                    QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(true);
                    QQmlProperty(d->m_UsbTypeBtnObject,"enabled").write(false);
                    QQmlProperty(d->m_SdTypeBtnObject,"enabled").write(true);
                    QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
                    QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(true);
                    QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
                }
                QQmlProperty(d->m_UsbVideoListviewObject,"contentY").write(0);
                QQmlProperty(d->m_UsbVideoListviewObject,"currentIndex").write(0);
                QQmlProperty(d->m_UsbVideoListWidgetObject,"listViewCurrentIndex").write(-1);
                QQmlProperty(d->m_UsbVideoListWidgetObject,"lastIndex").write(-1);
                QQmlProperty(d->m_UsbVideoListWidgetObject,"itemClicked").write(false);
                break;
            }
            default: {
                break;
            }
        }
    }

    if (DWT_SDDisk == type) {
        switch (status) {
            case DWS_Empty: {
                break;
            }
            case DWS_Unsupport: {
                break;
            }
            case DWS_Busy: {

                break;
            }
            case DWS_Ready: {
                d->m_DeviceWatcherType = type;
                d->m_SdExist = true;
                QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(false);
                QQmlProperty(d->m_SdTypeBtnObject,"sdSelect").write(true);
                QQmlProperty(d->m_SdTypeBtnObject,"enabled").write(true);
                QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(true);
                QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
                QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
                QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(true);
                QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
                QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);
                break;
            }
            case DWS_Remove: {
                d->m_SdExist = false;
                if(d->m_UsbExist == false)
                {
                    d->m_DeviceWatcherType = -1;
                    QQmlProperty(d->m_SdTypeBtnObject,"sdSelect").write(false);
                    QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(false);
                    QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_UsbTypeBtnObject,"enabled").write(false);
                    QQmlProperty(d->m_SdTypeBtnObject,"enabled").write(false);
                    QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
                    QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(false);
                    QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);

                }
                else
                {
                    d->m_DeviceWatcherType =1;
                    QQmlProperty(d->m_SdTypeBtnObject,"sdSelect").write(false);
                    QQmlProperty(d->m_UsbTypeBtnObject,"usbSelect").write(true);
                    QQmlProperty(d->m_UsbVideoListWidgetObject,"visible").write(true);
                    QQmlProperty(d->m_SdVideoListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_UsbTypeBtnObject,"enabled").write(true);
                    QQmlProperty(d->m_SdTypeBtnObject,"enabled").write(false);
                    QQmlProperty(d->m_ImageBtnObject,"pixmapSelect").write(false);
                    QQmlProperty(d->m_VideoBtnObject,"videoSelect").write(true);
                    QQmlProperty(d->m_UsbImageListWidgetObject,"visible").write(false);
                    QQmlProperty(d->m_SdImageListWidgetObject,"visible").write(false);

                }
                QQmlProperty(d->m_SdVideoListviewObject,"contentY").write(0);
                QQmlProperty(d->m_SdVideoListviewObject,"currentIndex").write(0);
                QQmlProperty(d->m_SdVideoListWidgetObject,"listViewCurrentIndex").write(-1);
                QQmlProperty(d->m_SdVideoListWidgetObject,"lastIndex").write(-1);
                QQmlProperty(d->m_SdVideoListWidgetObject,"itemClicked").write(false);
                break;
            }
            default: {
                break;
            }
        }
    }
}
int VideoListWidget::getDeviceWatcherType()
{
    Q_D(VideoListWidget);
    return d->m_DeviceWatcherType;
}

void VideoListWidget::onVideoListviewItemClicked(int index){
    Q_D(VideoListWidget);
    if(d->m_DeviceWatcherType == DWT_USBDisk)
    {
        d->m_UsbLastIndex = index;
        emit videoListviewItemClicked(d->m_DeviceWatcherType,index);
    }
    else if(d->m_DeviceWatcherType == DWT_SDDisk)
    {
        d->m_SdLastIndex = index;
        emit videoListviewItemClicked(d->m_DeviceWatcherType,index);
    }
}

void VideoListWidget::onImageListviewItemClicked(int type,int index)
{
    Q_D(VideoListWidget);
    if(type == DWT_USBDisk)
    {
        emit imageListviewItemClicked(d->m_DeviceWatcherType,index);
    }
    else if(type == DWT_SDDisk)
    {
        emit imageListviewItemClicked(d->m_DeviceWatcherType,index);
    }
}

void VideoListWidget::onVideoPlayerInformation(const int type, const int index, const QString &fileName, const int endTime)
{
    Q_D(VideoListWidget);
    if(type == DWT_USBDisk)
    {
        if(d->m_UsbLastIndex != index)
        {
            QQmlProperty(d->m_UsbVideoListWidgetObject,"itemClicked").write(false);
        }
        if(!d->m_UsbVideoListWidgetObject->property("itemClicked").toBool())
        {
            QQmlProperty(d->m_UsbVideoListviewObject,"contentY").write(88*index);
            QQmlProperty(d->m_UsbVideoListviewObject,"currentIndex").write(index);
            QQmlProperty(d->m_UsbVideoListWidgetObject,"listViewCurrentIndex").write(index);
        }
        d->m_UsbLastIndex = index;
    }
    else if(type == DWT_SDDisk){
        if(d->m_SdLastIndex != index)
        {
            QQmlProperty(d->m_SdVideoListWidgetObject,"itemClicked").write(false);
        }
        if(!d->m_SdVideoListWidgetObject->property("itemClicked").toBool())
        {
            QQmlProperty(d->m_SdVideoListviewObject,"contentY").write(88*index);
            QQmlProperty(d->m_SdVideoListviewObject,"currentIndex").write(index);
            QQmlProperty(d->m_SdVideoListWidgetObject,"listViewCurrentIndex").write(index);
        }
        d->m_SdLastIndex = index;
    }
}

VideoListWidgetPrivate::VideoListWidgetPrivate(VideoListWidget *parent)
    : q_ptr(parent)
{
    m_ListWidgetObject = NULL;
    m_UsbVideoListWidgetObject = NULL;
    m_SdVideoListWidgetObject  = NULL;
    m_SdVideoListviewObject    = NULL;
    m_UsbVideoListviewObject   = NULL;
    m_SdTypeBtnObject = NULL;
    m_UsbTypeBtnObject= NULL;
    m_VideoBtnObject  = NULL;
    m_ImageBtnObject  = NULL;
    m_UsbImageListWidgetObject = NULL;
    m_SdImageListWidgetObject = NULL;
    m_ImageListWidget = NULL;
    m_DeviceWatcherType = -1;
    m_UsbLastIndex = -1;
    m_SdLastIndex  = -1;
    m_SdExist  = false;
    m_UsbExist = false;
    connectAllSlots();
}
VideoListWidgetPrivate::~VideoListWidgetPrivate()
{

}
void VideoListWidgetPrivate::initializeImageListWidget()
{
    Q_Q(VideoListWidget);
    if(m_ImageListWidget == NULL)
    {
        m_ImageListWidget = new ImageListWidget(q);
        m_ImageListWidget->setUsbImageListWidgetObject(m_UsbImageListWidgetObject);
        m_ImageListWidget->setSdImageListWidgetObject(m_SdImageListWidgetObject);
    }
}
void VideoListWidgetPrivate::connectAllSlots()
{
    Q_Q(VideoListWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerInformation(const int, const int, const QString &, const int)));
}
