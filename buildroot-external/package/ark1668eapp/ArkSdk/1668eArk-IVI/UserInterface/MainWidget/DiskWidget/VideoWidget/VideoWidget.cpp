#include "VideoWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Multimedia.h"
#include "VideoListWidget/VideoListWidget.h"
#include "VideoToolBarWidget/VideoToolBarWidget.h"
#include "ImageToolBarWidget/ImageToolBarWidget.h"
#include "ImageWidget/ImageWidget.h"
#include <QDomDocument>
#include <QDebug>
#include <QQmlProperty>
namespace SourceString {
static const QString Unsupport(QObject::tr("Unsupport..."));
static const QString UnDragAndDrop(QObject::tr("can't drag and drop..."));
static const QString PlayError(QObject::tr("can't play,go to next..."));
}
class VideoWidgetPrivate
{
    Q_DISABLE_COPY(VideoWidgetPrivate)
public:
    explicit VideoWidgetPrivate(VideoWidget* parent);
    ~VideoWidgetPrivate();
    void setMsgTextObject();
    void initializeVideoListWidget();
    void initializeVideoToolBarWidget();
    void initializeImageWidget();
    void initializePlayErrorTimer();
    void initializeImageToolBarWidget();
    void initializeMsgTimer();
    void initializeTimer();
    void initializeStartPlayTimer();
    void connectAllSlots();
public:
    QObject* m_ParentObject;
    QObject* m_VideoObject;
    QObject* m_MsgTextObject;
    QObject* m_VideoBtnObject;
    QObject* m_ImageWidgetObject;
    QObject* m_ImageToolBarWidgetObject;
    QObject* m_VideoToolBarWidgetObject;
    QList<QString> m_UsbVideoList;
    QList<QString> m_SDVideoList;
    VideoListWidget* m_VideoListWidget;
    VideoToolBarWidget* m_VideoToolBarWidget;
    ImageToolBarWidget* m_ImageToolBarWidget;
    ImageWidget* m_ImageWidget;
    QTimer* m_PlayErrorTimer;
    QTimer* m_MsgTimer;
    QTimer* m_Timer;
    QTimer* m_StartPlayTimer;
    int m_UsbElapsed;
    int m_SdElapsed;
    int m_UsbVideoLastIndex;
    int m_SdVideoLastIndex;
    int  m_PlayMode;
    bool m_InitVideoPlay;
    int m_LastDeviceType;
    int m_LastPlayIndex;
private:
    Q_DECLARE_PUBLIC(VideoWidget)
    VideoWidget* const q_ptr;
};


VideoWidget::VideoWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new VideoWidgetPrivate(this))
{

}

void VideoWidget::setVideoObject(QObject* qmlObject){
    Q_D(VideoWidget);
    if(d->m_VideoObject == NULL)
    {
        d->m_VideoObject = qmlObject;
    }
    if(d->m_VideoObject != NULL)
    {
       if(d->m_VideoBtnObject == NULL)
       {
           d->m_VideoBtnObject = d->m_VideoObject->findChild<QObject*>("videoBtnObject");
       }
       if(d->m_ImageWidgetObject == NULL){

           d->m_ImageWidgetObject = d->m_VideoObject->findChild<QObject*>("imageWidgetObject");
       }
       if(d->m_ImageToolBarWidgetObject == NULL){

           d->m_ImageToolBarWidgetObject = d->m_VideoObject->findChild<QObject*>("imageToolWidgetObject");
       }
    }
    d->initializeVideoListWidget();
    d->initializeVideoToolBarWidget();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_VideoListWidget,SIGNAL(videoListviewItemClicked(int,int)),this,
                     SLOT(onVideoListviewItemClicked(int,int)),type);
    QObject::connect(this,SIGNAL(startVideoListviewItem(int,int)),this,
                     SLOT(onVideoListviewItemClicked(int,int)),type);
    QObject::connect(d->m_VideoListWidget,SIGNAL(imageListviewItemClicked(int,int)),this,
                     SLOT(onImageListviewItemClicked(int,int)),type);
    QObject::connect(d->m_VideoListWidget,SIGNAL(videoBtnClicked(int)),this,
                     SLOT(onVideoListWidgetVideoBtnClicked(int)),type);
    QObject::connect(d->m_VideoListWidget,SIGNAL(imageBtnClicked(int)),this,
                     SLOT(onVideoListWidgetImageBtnClicked(int)),type);
    QObject::connect(d->m_VideoBtnObject,SIGNAL(clicked()),this,
                     SLOT(onVideoBtnClicked()));
    QObject::connect(d->m_VideoToolBarWidget,SIGNAL(mousePressed()),this,
                     SLOT(onMousePressed()),type);

    QObject::connect(d->m_VideoObject,SIGNAL(visibleChanged()),this,
                     SLOT(onVisibleChanged()),type);
}
void VideoWidget::onVisibleChanged()
{
    Q_D(VideoWidget);
//    static bool _FirstEnter(true);
//    if(d->m_VideoObject->property("visible").toBool())
//    {
//        if(_FirstEnter){
//            qDebug()<<"+++++m_StartPlayTimer start++++++++++";
//            d->initializeStartPlayTimer();
//            d->m_StartPlayTimer->start();
//            _FirstEnter = false;
//        }
//    }
}
void VideoWidget::setParentObject(QObject* qmlObject){
    Q_D(VideoWidget);
    if(d->m_ParentObject == NULL)
    {
        d->m_ParentObject = qmlObject;
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_ParentObject,SIGNAL(typeStatusChanged()),this,
                     SLOT(onTypeStatusChanged()),type);
}
void VideoWidget::onTypeStatusChanged(){
    Q_D(VideoWidget);
    qDebug()<<"+++++++d->m_PlayMode000++++++++"<<d->m_PlayMode;
    qDebug()<<"+++++++++####0000+++++++++++"<<d->m_ParentObject->property("typeStatus").toInt();
    static bool _CurrentWidget(false);
    if(d->m_ParentObject->property("typeStatus").toInt() == 4)
    {
        _CurrentWidget = true;
    }
    else{
        _CurrentWidget = false;
    }
    if((_CurrentWidget) && d->m_PlayMode == VPPS_Pause)
    {
        g_Multimedia->videoPlayerSetPlayStatusToggle();
    }
    else if(!_CurrentWidget && d->m_PlayMode != VPPS_Pause)
    {
        g_Multimedia->videoPlayerSetPlayStatusToggle();
    }
}
void VideoWidget::onVideoPlayerInformation(const int type, const int index,
                                           const QString &fileName, const int endTime){
    Q_D(VideoWidget);
    if(type == DWT_USBDisk){
        d->m_UsbVideoLastIndex = index;
    }
    else if(type == DWT_SDDisk){
        d->m_SdVideoLastIndex = index;
    }
}
void VideoWidget::onVideoPlayerElapsedInformation(const int elapsedTime, const int elapsedMillesimal){
    Q_D(VideoWidget);
    if(d->m_VideoListWidget != NULL)
    {
        if(d->m_VideoListWidget->getDeviceWatcherType() == DWT_USBDisk){
            d->m_UsbElapsed = elapsedTime;
        }
        else if(d->m_VideoListWidget->getDeviceWatcherType() == DWT_SDDisk){
            d->m_SdElapsed = elapsedTime;
        }
    }

}
void VideoWidget::onVideoListWidgetVideoBtnClicked(int type)
{
    Q_D(VideoWidget);
    QQmlProperty(d->m_ImageWidgetObject,"visible").write(false);
    QQmlProperty(d->m_ImageToolBarWidgetObject,"visible").write(false);
    QQmlProperty(d->m_VideoBtnObject,"visible").write(true);

}
void VideoWidget::onVideoListWidgetImageBtnClicked(int type){
    Q_D(VideoWidget);
    d->initializeImageWidget();
    d->initializeImageToolBarWidget();
    QQmlProperty(d->m_ImageWidgetObject,"visible").write(true);
    QQmlProperty(d->m_ImageToolBarWidgetObject,"visible").write(true);
    QQmlProperty(d->m_VideoToolBarWidgetObject,"visible").write(false);
    QQmlProperty(d->m_VideoBtnObject,"visible").write(false);
    if(d->m_MsgTextObject != NULL)
    {
        QQmlProperty(d->m_VideoBtnObject,"m_MsgTextObject").write(false);
    }
}

QObject* VideoWidget::getImageWidgetObject(){
    Q_D(VideoWidget);
    return d->m_ImageWidget->getImageWidgetObject();
}
QObject* VideoWidget::getPixmapObject(){
    Q_D(VideoWidget);
    return d->m_ImageWidget->getPixmapObject();
}
QObject* VideoWidget::getAnimatedObject(){
    Q_D(VideoWidget);
    return d->m_ImageWidget->getAnimatedObject();
}

void VideoWidget::onVideoPlayerPlayStatus(const DeviceWatcherType type, const VideoPlayerPlayStatus playStatus)
{
    Q_D(VideoWidget);
    QQmlProperty(d->m_VideoBtnObject,"enabled").write(VPPS_Start != playStatus);
    d->m_PlayMode = playStatus;
    switch (playStatus) {
    case VPPS_Start: {
        d->setMsgTextObject();
        QQmlProperty(d->m_MsgTextObject,"visible").write(false);
        break;
    }
    case VPPS_Play: {
        if(!d->m_Timer->isActive())
        {
            d->m_Timer->start();
        }
        break;
    }
    case VPPS_Pause: {
        d->m_Timer->stop();
        break;
    }
    case VPPS_Exit: {   
        d->m_Timer->stop();
        break;
    }
    case VPPS_Unsupport: {
        d->setMsgTextObject();
        QQmlProperty(d->m_MsgTextObject,"text").write(SourceString::Unsupport);
        QQmlProperty(d->m_MsgTextObject,"visible").write(true);
        break;
    }
    case VPPS_UndragAndDrop:{
        d->setMsgTextObject();
        QQmlProperty(d->m_MsgTextObject,"text").write(SourceString::UnDragAndDrop);
        d->m_MsgTimer->start();
        QQmlProperty(d->m_MsgTextObject,"visible").write(true);
        break;
    }
    case VPPS_PlayError:{
        d->initializePlayErrorTimer();
        d->setMsgTextObject();
        d->m_PlayErrorTimer->start();
        QQmlProperty(d->m_MsgTextObject,"text").write(SourceString::PlayError);
        d->m_MsgTimer->start();
        QQmlProperty(d->m_MsgTextObject,"visible").write(true);
        break;
    }
    default : {
        break;
    }
    }
}
void VideoWidget::onVideoListviewItemClicked(int type,int index){
    Q_D(VideoWidget);
    if((d->m_LastDeviceType == type && d->m_LastPlayIndex == index && d->m_InitVideoPlay == false) || (index < 0))
    {
        return;
    }
    if(type == DWT_USBDisk){
        if (index == d->m_UsbVideoLastIndex) {
            if(d->m_InitVideoPlay)
            {
                g_Multimedia->videoPlayerPlayListViewIndex(DWT_USBDisk, index, 680, 0,1240, 720, d->m_UsbElapsed);
                d->m_InitVideoPlay = false;
            }
            else
            {
                 d->m_UsbElapsed = 0;
                 g_Multimedia->videoPlayerPlayListViewIndex(DWT_USBDisk, index, 680, 0, 1240,720 , d->m_UsbElapsed);
            }

        } else {
            d->m_UsbElapsed = 0;
            d->m_UsbVideoLastIndex = index;
            g_Multimedia->videoPlayerPlayListViewIndex(DWT_USBDisk, index, 680, 0, 1240,720 , d->m_UsbElapsed);
            d->m_InitVideoPlay = false;
        }
    }
    else if(type == DWT_SDDisk){
        if (index == d->m_SdVideoLastIndex) {
            if(d->m_InitVideoPlay)
            {
                g_Multimedia->videoPlayerPlayListViewIndex(DWT_SDDisk, index, 680, 0,1240, 720, d->m_SdElapsed);
                d->m_InitVideoPlay = false;
            }
            else
            {
                d->m_SdElapsed = 0;
                g_Multimedia->videoPlayerPlayListViewIndex(DWT_SDDisk, index, 680, 0, 1240,720 , d->m_SdElapsed);

            }
        } else {
            d->m_SdElapsed = 0;
            d->m_SdVideoLastIndex = index;
            g_Multimedia->videoPlayerPlayListViewIndex(DWT_SDDisk, index, 680, 0, 1240,720 , d->m_SdElapsed);
            d->m_InitVideoPlay = false;
        }
    }
    d->m_LastDeviceType = type;
    d->m_LastPlayIndex  = index;
}

void VideoWidget::onImageListviewItemClicked(int type,int index)
{
    Q_D(VideoWidget);
    if(type == DWT_USBDisk){
        g_Multimedia->imagePlayerPlayListViewIndex(DWT_USBDisk, index);
    }
    else if(type == DWT_SDDisk)
    {
        g_Multimedia->imagePlayerPlayListViewIndex(DWT_SDDisk, index);
    }
}

void VideoWidget::onVideoPlayerPlayError()
{
    Q_D(VideoWidget);
    qDebug()<<__PRETTY_FUNCTION__<<__LINE__;
    d->m_PlayErrorTimer->stop();
    g_Multimedia->videoPlayerPlayNextListViewIndex();
}
void VideoWidget::onTimeout()
{
    Q_D(VideoWidget);
    QTimer* ptr = static_cast<QTimer*>(sender());
    if(ptr == d->m_MsgTimer)
    {
        d->m_MsgTimer->stop();
        d->setMsgTextObject();
        QQmlProperty(d->m_MsgTextObject,"visible").write(false);
    }
    else if(ptr == d->m_Timer){
        d->m_Timer->stop();
        d->m_VideoToolBarWidget->setVideoToolBarWidgetVisible(false);
        QQmlProperty(d->m_VideoBtnObject,"height").write(720);
    }
    else if(ptr == d->m_StartPlayTimer){
        if(d->m_ParentObject->property("typeStatus").toInt() == 4){
            if(d->m_VideoListWidget->getDeviceWatcherType() == DWT_USBDisk)
            {
                qDebug()<<"=========m_UsbVideoLastIndex000========="<<d->m_UsbVideoLastIndex;
                emit startVideoListviewItem(DWT_USBDisk,d->m_UsbVideoLastIndex);
            }
            else if(d->m_VideoListWidget->getDeviceWatcherType() == DWT_SDDisk){
                emit startVideoListviewItem(DWT_SDDisk,d->m_SdVideoLastIndex);
            }
        }
    }
}

void VideoWidget::onVideoBtnClicked()
{
    Q_D(VideoWidget);
    d->m_Timer->stop();
    d->m_Timer->start();
    d->m_VideoToolBarWidget->setVideoToolBarWidgetVisible(true);
    QQmlProperty(d->m_VideoBtnObject,"height").write(560);

}
void VideoWidget::onMousePressed()
{
    Q_D(VideoWidget);
    d->m_Timer->stop();
    d->m_Timer->start();
    d->m_VideoToolBarWidget->setVideoToolBarWidgetVisible(true);
    QQmlProperty(d->m_VideoBtnObject,"height").write(560);
}

void VideoWidget::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder)
{
    Q_D(VideoWidget);
    if(oldHolder == AS_Video)
    {
        if(d->m_Timer->isActive())
        {
            d->m_Timer->stop();
        }
        d->m_VideoToolBarWidget->setVideoToolBarWidgetVisible(false);
        QQmlProperty(d->m_VideoBtnObject,"height").write(720);
    }
}
void VideoWidget::onVideoPlayerExit()
{
    Q_D(VideoWidget);
    qDebug()<<"++++++++onVideoPlayerExit0000+++++++++++++";
    d->m_InitVideoPlay = true;
}
void VideoWidget::onVideoPlayerFileNames(const int type, const QString& xml)
{
    Q_D(VideoWidget);
    if (DWT_USBDisk == type) {
        QDomDocument document;
        document.setContent(xml);
        QDomElement root = document.documentElement();
        if ((!root.isNull())
                && (root.isElement())
                && (QString("VideoPlayer") == root.toElement().tagName())
                && (root.hasChildNodes())) {
            QDomNode node = root.firstChild();
            while (!node.isNull()) {
                if (node.isElement()) {
                    QDomElement element = node.toElement();
                    if (!element.isNull()) {
                        if (QString("USBFileNames") == element.tagName()) {
                            QDomNodeList nodeList = element.childNodes();
                            d->m_UsbVideoList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            d->m_UsbVideoList.append(node.toElement().text());
                                        }
                                    }
                                }
                            }

                        } else if (QString("USBPersistant") == element.tagName()) {
                            QDomElement node = element.toElement();
                            if (node.isElement()) {
                                if (!node.toElement().isNull()) {
                                    if (node.isElement()) {
                                        if (!node.toElement().text().isEmpty()) {
                                            int index    =  QString(node.toElement().text().split(QChar('-')).at(0)).toInt();
                                            d->m_UsbElapsed =  QString(node.toElement().text().split(QChar('-')).at(1)).toInt();
                                            if (d->m_UsbVideoList.size() > index) {
                                                d->m_UsbVideoLastIndex = index;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                node = node.nextSibling();
            }
        }
    }

    if (DWT_SDDisk == type) {
        QDomDocument document;
        document.setContent(xml);
        QDomElement root = document.documentElement();
        if ((!root.isNull())
                && (root.isElement())
                && (QString("VideoPlayer") == root.toElement().tagName())
                && (root.hasChildNodes())) {
            QDomNode node = root.firstChild();
            while (!node.isNull()) {
                if (node.isElement()) {
                    QDomElement element = node.toElement();
                    if (!element.isNull()) {
                        if (QString("SDFileNames") == element.tagName()) {
                            QDomNodeList nodeList = element.childNodes();
                            d->m_SDVideoList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            d->m_SDVideoList.append(node.toElement().text());
                                        }
                                    }
                                }
                            }
                        } else if (QString("SDPersistant") == element.tagName()) {
                            QDomElement node = element.toElement();
                            if (node.isElement()) {
                                if (!node.toElement().isNull()) {
                                    if (node.isElement()) {
                                        if (!node.toElement().text().isEmpty()) {
                                            int index = QString(node.toElement().text().split(QChar('-')).at(0)).toInt();
                                            d->m_SdElapsed = QString(node.toElement().text().split(QChar('-')).at(1)).toInt();
                                            if (d->m_SDVideoList.size() > index) {
                                                d->m_SdVideoLastIndex = index;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                node = node.nextSibling();
            }
        }
    }
}



VideoWidgetPrivate::VideoWidgetPrivate(VideoWidget *parent)
    : q_ptr(parent)
{

    m_VideoListWidget = NULL;
    m_PlayErrorTimer = NULL;
    m_VideoToolBarWidget = NULL;
    m_VideoObject = NULL;
    m_MsgTextObject = NULL;
    m_VideoBtnObject = NULL;
    m_ImageWidgetObject = NULL;
    m_ImageWidget = NULL;
    m_Timer = NULL;
    m_MsgTimer = NULL;
    m_ImageToolBarWidget = NULL;
    m_ImageToolBarWidgetObject = NULL;
    m_VideoToolBarWidgetObject = NULL;
    m_ParentObject  = NULL;
    m_StartPlayTimer = NULL;
    m_UsbElapsed = 0;
    m_SdElapsed  = 0;
    m_UsbVideoLastIndex = 0;
    m_SdVideoLastIndex  = 0;
    m_PlayMode = VPPS_Undefine;
    m_InitVideoPlay = true;
    m_LastDeviceType = -1;
    m_LastPlayIndex  = -1;
    initializeTimer();
    initializeMsgTimer();
    connectAllSlots();
}
VideoWidgetPrivate::~VideoWidgetPrivate()
{

}

void VideoWidgetPrivate::setMsgTextObject()
{
    if(m_MsgTextObject == NULL)
    {
        if(m_VideoObject != NULL)
        {
            m_MsgTextObject = m_VideoObject->findChild<QObject*>("msgTextObject");
        }
    }
}
void VideoWidgetPrivate::initializePlayErrorTimer()
{
    Q_Q(VideoWidget);
    if(NULL == m_PlayErrorTimer)
    {
        m_PlayErrorTimer = new QTimer(q);
        m_PlayErrorTimer->setInterval(3000);
        m_PlayErrorTimer->setSingleShot(true);
        QObject::connect(m_PlayErrorTimer,ARKSENDER(timeout()),
                         q, ARKRECEIVER(onVideoPlayerPlayError()));
    }
}

void VideoWidgetPrivate::initializeMsgTimer()
{
    Q_Q(VideoWidget);
    if(NULL == m_MsgTimer)
    {
        m_MsgTimer = new QTimer(q);
        m_MsgTimer->setInterval(3000);
        m_MsgTimer->setSingleShot(true);
        QObject::connect(m_MsgTimer,ARKSENDER(timeout()),
                         q, ARKRECEIVER(onTimeout()));
    }
}

void VideoWidgetPrivate::initializeVideoListWidget()
{
    Q_Q(VideoWidget);
    if(m_VideoObject != NULL)
    {
        if(m_VideoListWidget == NULL)
        {
            m_VideoListWidget = new VideoListWidget(q);
            QObject* videoListWidgetObject = m_VideoObject->findChild<QObject*>("listWidgetObject");
            m_VideoListWidget ->setListWidgetObject(videoListWidgetObject);
        }
    }
}

void VideoWidgetPrivate::initializeVideoToolBarWidget()
{
    Q_Q(VideoWidget);
    if(m_VideoObject != NULL)
    {
        if(m_VideoToolBarWidget == NULL)
        {
            m_VideoToolBarWidget = new VideoToolBarWidget(q);
            QObject* videoToolBarWidgetObject  = m_VideoObject->findChild<QObject*>("videoToolBarWidgetObject");
            m_VideoToolBarWidget ->setVideoToolBarWidgetObject(videoToolBarWidgetObject);
            m_VideoToolBarWidgetObject = videoToolBarWidgetObject;
        }
    }
}
void VideoWidgetPrivate::initializeImageWidget()
{
    Q_Q(VideoWidget);
    if(m_ImageWidget == NULL)
    {
        m_ImageWidget = new ImageWidget(q);
        m_ImageWidget->setImageWidgetObject(m_ImageWidgetObject);
    }
}

void VideoWidgetPrivate::initializeImageToolBarWidget()
{
    Q_Q(VideoWidget);
    if(m_VideoObject != NULL)
    {
        if(m_ImageToolBarWidget == NULL)
        {
            m_ImageToolBarWidget = new ImageToolBarWidget(q);
            m_ImageToolBarWidget->setImageToolBarWidgetObject(m_ImageToolBarWidgetObject);
        }

    }

}
void VideoWidgetPrivate::initializeTimer()
{
    Q_Q(VideoWidget);
    if (NULL == m_Timer) {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(5000);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer,  ARKSENDER(timeout()),
                         q, ARKRECEIVER(onTimeout()));
    }
}

void VideoWidgetPrivate::initializeStartPlayTimer()
{
    Q_Q(VideoWidget);
    if (NULL == m_StartPlayTimer) {
        m_StartPlayTimer = new QTimer(q);
        m_StartPlayTimer->setInterval(1000);
        m_StartPlayTimer->setSingleShot(true);
        QObject::connect(m_StartPlayTimer,  ARKSENDER(timeout()),
                         q, ARKRECEIVER(onTimeout()));
    }
}

void VideoWidgetPrivate::connectAllSlots()
{
    Q_Q(VideoWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerFileNames(const int, const QString&)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerExit()));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerPlayStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onHolderChange(const int, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerElapsedInformation(const int, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerInformation(const int, const int, const QString &, const int)));
}

