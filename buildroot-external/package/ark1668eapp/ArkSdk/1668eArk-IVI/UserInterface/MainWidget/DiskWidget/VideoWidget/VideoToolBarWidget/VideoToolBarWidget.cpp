#include "VideoToolBarWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Multimedia.h"
#include "BusinessLogic/Widget.h"
#include <QDebug>
#include <QQmlProperty>
class VideoToolBarWidgetPrivate
{
    Q_DISABLE_COPY(VideoToolBarWidgetPrivate)
public:
    explicit VideoToolBarWidgetPrivate(VideoToolBarWidget* parent);
    ~VideoToolBarWidgetPrivate();
    void connectAllSlots();
    QString convertTime(const int time);
public:
    QObject* m_VideoToolBarWidgetObject;
    QObject* m_EndTimeTextObject;
    QObject* m_RemaTimeObject;
    QObject* m_StopBtnObject;
    QObject* m_PrevBtnObject;
    QObject* m_ToggleBtnObject;
    QObject* m_NextBtnObject;
    QObject* m_FullScreenBtnObject;
    QObject* m_SliderObject;
    int      m_EndTime;
    bool     m_TouchStatus;
    int      m_Index;
    int      m_ElapsedTime;
    int      m_DeviceType; 
private:
    Q_DECLARE_PUBLIC(VideoToolBarWidget)
    VideoToolBarWidget* const q_ptr;
};

VideoToolBarWidget::VideoToolBarWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new VideoToolBarWidgetPrivate(this))
{

}
void VideoToolBarWidget::setVideoToolBarWidgetObject(QObject* qmlObject){
    Q_D(VideoToolBarWidget);
    if(d->m_VideoToolBarWidgetObject == NULL)
    {
        d->m_VideoToolBarWidgetObject = qmlObject;
    }
    if(d->m_VideoToolBarWidgetObject != NULL)
    {
        if(d->m_EndTimeTextObject == NULL)
        {
            d->m_EndTimeTextObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("endTimeObject");
        }
        if(d->m_RemaTimeObject == NULL)
        {
            d->m_RemaTimeObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("remaTimeObject");
        }
        if(d->m_StopBtnObject == NULL)
        {
            d->m_StopBtnObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("stopBtnObject");
        }
        if(d->m_PrevBtnObject == NULL)
        {
            d->m_PrevBtnObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("prevBtnObject");
        }
        if(d->m_ToggleBtnObject == NULL)
        {
            d->m_ToggleBtnObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("toggleBtnObject");
        }
        if(d->m_NextBtnObject == NULL)
        {
            d->m_NextBtnObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("nextBtnObject");
        }
        if(d->m_FullScreenBtnObject == NULL)
        {
            d->m_FullScreenBtnObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("fullScreenBtnObject");
        }
        if(d->m_SliderObject == NULL)
        {
            d->m_SliderObject = d->m_VideoToolBarWidgetObject->findChild<QObject*>("sliderObject");
        }

    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_StopBtnObject, ARKSENDER(clicked()),
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
    QObject::connect(d->m_FullScreenBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_VideoToolBarWidgetObject, ARKSENDER(mousePressed()),
                     this,      ARKRECEIVER(onMousePressed()),
                     type);

    QObject::connect(d->m_SliderObject, ARKSENDER(sliderReleased(int)),
                     this,        ARKRECEIVER(onSliderReleased(int)),
                     type);
    QObject::connect(d->m_FullScreenBtnObject, ARKSENDER(clicked()),
                     this,        ARKRECEIVER(onToolButtonRelease()),
                     type);

}
void VideoToolBarWidget::setVideoToolBarWidgetVisible(bool visible)
{
    Q_D(VideoToolBarWidget);
    if(d->m_VideoToolBarWidgetObject != NULL)
    {
        QQmlProperty(d->m_VideoToolBarWidgetObject,"visible").write(visible);
    }
}
void VideoToolBarWidget::onToolButtonRelease()
{
    Q_D(VideoToolBarWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if (ptr == d->m_PrevBtnObject) {
        g_Multimedia->videoPlayerPlayPreviousListViewIndex();
    } else if (ptr == d->m_ToggleBtnObject) {
        g_Multimedia->videoPlayerSetPlayStatusToggle();
    } else if (ptr == d->m_NextBtnObject) {
        g_Multimedia->videoPlayerPlayNextListViewIndex();
    } else if (ptr == d->m_StopBtnObject) {
    } else if (ptr == d->m_FullScreenBtnObject){
        if(d->m_FullScreenBtnObject->property("fullScreenStatus").toBool() == false)
        {
            QQmlProperty(d->m_FullScreenBtnObject,"fullScreenStatus").write(true);
            QQmlProperty(d->m_VideoToolBarWidgetObject,"fullScreenType").write(1);
            if(d->m_DeviceType == DWT_USBDisk){
                g_Multimedia->videoPlayerPlayListViewIndex(DWT_USBDisk, d->m_Index, 0, 0,1920, 720, d->m_ElapsedTime);
            }
            else if(d->m_DeviceType == DWT_SDDisk){
                g_Multimedia->videoPlayerPlayListViewIndex(DWT_SDDisk, d->m_Index, 0, 0,1920, 720, d->m_ElapsedTime);
            }
        }
        else
        {
            QQmlProperty(d->m_FullScreenBtnObject,"fullScreenStatus").write(false);
            QQmlProperty(d->m_VideoToolBarWidgetObject,"fullScreenType").write(0);
            if(d->m_DeviceType == DWT_USBDisk){
                g_Multimedia->videoPlayerPlayListViewIndex(DWT_USBDisk, d->m_Index, 680, 0,1240, 720, d->m_ElapsedTime);
            }
            else if(d->m_DeviceType == DWT_SDDisk){
                g_Multimedia->videoPlayerPlayListViewIndex(DWT_SDDisk, d->m_Index, 680, 0,1240, 720, d->m_ElapsedTime);
            }
        }
    }
}
void VideoToolBarWidget::onMousePressed()
{
    emit mousePressed();
}

void VideoToolBarWidget::onSliderReleased(const int value)
{
    Q_D(VideoToolBarWidget);
    if(0 != d->m_SliderObject->property("toValue").toInt()) {
        QQmlProperty(d->m_SliderObject,"enabled").write(false);
        int millesimal = 1000 * (static_cast<float>(value) / d->m_SliderObject->property("toValue").toInt());
        g_Multimedia->videoPlayerSeekToMillesimal(millesimal);
    }
}
void VideoToolBarWidget::onVideoPlayerPlayStatus(const DeviceWatcherType type, const VideoPlayerPlayStatus status)
{
    Q_D(VideoToolBarWidget);
    d->m_DeviceType = type;
    qDebug()<<"+++++++++++onVideoPlayerPlayStatus:status+++++++++++++"<<status;
    QQmlProperty(d->m_StopBtnObject,"enabled").write(VPPS_Start != status);
    QQmlProperty(d->m_PrevBtnObject,"enabled").write(VPPS_Start != status);
    QQmlProperty(d->m_ToggleBtnObject,"enabled").write(VPPS_Start != status);
    QQmlProperty(d->m_NextBtnObject,"enabled").write(VPPS_Start != status);
    QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(VPPS_Start != status);
    switch (status) {
    case VPPS_Play: {
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(true);
        break;
    }
    case VPPS_Start:
    case VPPS_Unsupport:
    case VPPS_Stop:
    case VPPS_Pause: {
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        break;
    }
    default : {
        break;
    }
    }

    switch (status) {
    case VPPS_Stop:
    case VPPS_Pause:
    case VPPS_Play:
    case VPPS_SeekFinish: {
        if (0 != d->m_SliderObject->property("toValue").toInt()) {
            if(d->m_TouchStatus == true)
            {
                QQmlProperty(d->m_SliderObject,"enabled").write(true);
            }
        }
        break;
    }
    case VPPS_Exit:
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        QQmlProperty(d->m_StopBtnObject,"enabled").write(false);
        QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(false);
        QQmlProperty(d->m_SliderObject,"currentValue").write(0);
        QQmlProperty(d->m_RemaTimeObject,"text").write("00:01");
        if(d->m_FullScreenBtnObject->property("fullScreenStatus").toBool() == true){
            QQmlProperty(d->m_FullScreenBtnObject,"fullScreenStatus").write(false);
            QQmlProperty(d->m_VideoToolBarWidgetObject,"fullScreenType").write(0);
        }
        break;
    default: {
        QQmlProperty(d->m_SliderObject,"enabled").write(false);
        break;
    }
    }
}
void VideoToolBarWidget::onSliderTouchEnable(const bool flag)
{
    Q_D(VideoToolBarWidget);
    qDebug()<<__PRETTY_FUNCTION__<<__LINE__<<flag;
    d->m_TouchStatus = flag;
    QQmlProperty(d->m_SliderObject,"enabled").write(flag);
}

void VideoToolBarWidget::onVideoPlayerInformation(const int type, const int index, const QString &fileName, const int endTime)
{
    Q_D(VideoToolBarWidget);
    if(d->m_EndTimeTextObject != NULL)
    {
        QQmlProperty(d->m_EndTimeTextObject,"text").write(d->convertTime(endTime));
    }
    if(d->m_TouchStatus == true)
    {
        if(d->m_EndTimeTextObject != NULL)
        {
            QQmlProperty(d->m_SliderObject,"enabled").write(0 != endTime);
        }
    }
    QQmlProperty(d->m_SliderObject,"fromValue").write(0);
    QQmlProperty(d->m_SliderObject,"toValue").write(endTime);
    d->m_EndTime = endTime;
    d->m_Index   = index;
}

void VideoToolBarWidget::onVideoPlayerElapsedInformation(const int elapsedTime, const int elapsedMillesimal)
{
    Q_D(VideoToolBarWidget);
    d->m_ElapsedTime = elapsedTime;
    QString remaTime =QString("-") + d->convertTime(d->m_EndTime - elapsedTime);
    QQmlProperty(d->m_RemaTimeObject,"text").write(remaTime);
    if (!d->m_SliderObject->property("clickStatus").toBool()) {
        QQmlProperty(d->m_SliderObject,"currentValue").write(elapsedTime);
    }
}
void VideoToolBarWidget::onUsbMediaPlayExit(){
    Q_D(VideoToolBarWidget);
    if(d->m_DeviceType == DWT_USBDisk){
        QQmlProperty(d->m_StopBtnObject,"enabled").write(false);
        QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(false);
        QQmlProperty(d->m_SliderObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        QQmlProperty(d->m_RemaTimeObject,"text").write("");
        QQmlProperty(d->m_EndTimeTextObject,"text").write("");
        QQmlProperty(d->m_SliderObject,"currentValue").write(0);
        d->m_DeviceType = DWT_Undefine;
    }
}
void VideoToolBarWidget::onSdMediaPlayExit(){
    Q_D(VideoToolBarWidget);
    if(d->m_DeviceType == DWT_SDDisk){
        QQmlProperty(d->m_StopBtnObject,"enabled").write(false);
        QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(false);
        QQmlProperty(d->m_SliderObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        QQmlProperty(d->m_RemaTimeObject,"text").write("");
        QQmlProperty(d->m_EndTimeTextObject,"text").write("");
        QQmlProperty(d->m_SliderObject,"currentValue").write(0);
        d->m_DeviceType = DWT_Undefine;
    }
}
void VideoToolBarWidget::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder)
{
    Q_D(VideoToolBarWidget);
    if(oldHolder == AS_Video)
    {
        QQmlProperty(d->m_StopBtnObject,"enabled").write(false);
        QQmlProperty(d->m_PrevBtnObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_FullScreenBtnObject,"enabled").write(false);
        QQmlProperty(d->m_SliderObject,"enabled").write(false);
        QQmlProperty(d->m_ToggleBtnObject,"playStatus").write(false);
        QQmlProperty(d->m_RemaTimeObject,"text").write("");
        QQmlProperty(d->m_EndTimeTextObject,"text").write("");
        QQmlProperty(d->m_SliderObject,"currentValue").write(0);
        d->m_DeviceType = DWT_Undefine;
    }
}
VideoToolBarWidgetPrivate::VideoToolBarWidgetPrivate(VideoToolBarWidget *parent)
    : q_ptr(parent)
{
    m_VideoToolBarWidgetObject = NULL;
    m_EndTimeTextObject= NULL;
    m_RemaTimeObject= NULL;
    m_StopBtnObject= NULL;
    m_PrevBtnObject= NULL;
    m_ToggleBtnObject= NULL;
    m_NextBtnObject= NULL;
    m_FullScreenBtnObject= NULL;
    m_SliderObject = NULL;
    m_TouchStatus = true;
    m_Index = -1;
    m_ElapsedTime = -1;
    m_DeviceType = DWT_Undefine;
    connectAllSlots();
}
VideoToolBarWidgetPrivate::~VideoToolBarWidgetPrivate()
{

}
QString VideoToolBarWidgetPrivate::convertTime(const int time)
{
    QString hour("%1");
    QString minute("%1");
    QString second("%1");
    return hour.arg((time / 60) / 60, 2, 10, QChar('0'))
            + QString(":") + minute.arg((time / 60) % 60, 2, 10, QChar('0'))
            + QString(":") + second.arg(time % 60, 2, 10, QChar('0'));
}

void VideoToolBarWidgetPrivate::connectAllSlots()
{
    Q_Q(VideoToolBarWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerPlayStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerInformation(const int, const int, const QString &, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerElapsedInformation(const int, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onSliderTouchEnable(const bool)));
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onHolderChange(const int, const int)));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onUsbMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onSdMediaPlayExit()));
}
