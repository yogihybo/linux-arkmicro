#include "MultiMediaPlayWidget.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Setting.h"
#include <QDomDocument>
#include <QDebug>
#include <unistd.h>
#include <QQmlProperty>
namespace SourceString {
static const QString Unknown(QObject::tr("Unknown"));
}
/*
*功能：保存当前的播放模式到指定文件
*/
void SaveCurrentPlaymodeToFile(int mode)
{
    QString cmd = "echo ";
    cmd+= QString::number(mode,10);
    cmd+= " > /etc/playmode";
    if(access("/etc/playmode",F_OK) == 0)
    {
        g_Setting->executeShellCmd(cmd.toLocal8Bit().constData());
        g_Setting->executeShellCmd("sync");
        qDebug()<<Q_FUNC_INFO<<"exec "<<cmd;
    }else{
        qDebug()<<Q_FUNC_INFO<<cmd<<"file:/etc/playmode is not exist";
    }
}

/*
*功能:获取当前的播放模式
*/
int getCrrentMusicPlayerMode()
{
    QFile file("/etc/playmode");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug()<<Q_FUNC_INFO<<"file :/etc/playmode is open failed";
    }
    QTextStream in(&file);
    QString line = in.readLine();
    qDebug() << __PRETTY_FUNCTION__<<line.toInt();
    file.close();
    return line.toInt();
}

class MultiMediaPlayWidgetPrivate
{
    Q_DISABLE_COPY(MultiMediaPlayWidgetPrivate)
public:
    explicit MultiMediaPlayWidgetPrivate(MultiMediaPlayWidget* parent);
    ~MultiMediaPlayWidgetPrivate();
    QString convertTime(const int time);
    void connectAllSlots();
    void initializeTimer();
    void initializeMusicTypeTimer();
    void initializeMediaTypeTimer();
public:
    QObject* m_MultiMediaPlayWidgetObject;
    QObject* m_UsbMusicListviewWidgetObject;
    QObject* m_UsbMusicListviewObject;
    QObject* m_SdMusicListviewWidgetObject;
    QObject* m_SdMusicListviewObject;
    QObject* m_AlbumCoveImageObject;
    QObject* m_TitleObject;
    QObject* m_ArtistObject;
    QObject* m_AlbumObject;
    QObject* m_SliderObject;
    QObject* m_RemaTimeObject;
    QObject* m_PreBtnObject;
    QObject* m_PlayBtnObject;
    QObject* m_NextBtnObject;
    QObject* m_ModeBtnObject;
    QObject* m_MultiMediaObject;
    QList<QString> m_UsbMusicList;
    QList<QString> m_SdMusicList;
    QString m_TitleText;
    QString m_ArtistText;
    QString m_AlbumText;
    QString m_FilePath;
    int m_UsbElapsed;
    int m_UsbLastIndex;
    int m_SdElapsed;
    int m_SdLastIndex;
    int m_EndTime;
    int m_Playtype;
    int m_PlayStatus;
    bool m_InitMusicPlay;
    int  m_LastDeviceType;
    int  m_LastPlayIndex;
    QTimer* m_Timer;
    int m_Type;
    QTimer* m_MusicTypeTimer;
    QTimer* m_MediaTypeTimer;
private:
    Q_DECLARE_PUBLIC(MultiMediaPlayWidget)
    MultiMediaPlayWidget* const q_ptr;
};

MultiMediaPlayWidget::MultiMediaPlayWidget(QObject *parent)
    : QObject(parent),
      d_ptr(new MultiMediaPlayWidgetPrivate(this))
{

}

void MultiMediaPlayWidget::setMultiMediaPlayWidgetObject(QObject* qmlObject)
{
    Q_D(MultiMediaPlayWidget);
    if(d->m_MultiMediaPlayWidgetObject == NULL)
    {
        d->m_MultiMediaPlayWidgetObject = qmlObject;
    }
    if(d->m_MultiMediaPlayWidgetObject != NULL)
    {
        if(d->m_AlbumCoveImageObject == NULL)
        {
            d->m_AlbumCoveImageObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("albumCoveImageObject");
        }

        if(d->m_TitleObject == NULL)
        {
            d->m_TitleObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("titleObject");
        }

        if(d->m_ArtistObject == NULL)
        {
            d->m_ArtistObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("artistObject");
        }

        if(d->m_AlbumObject == NULL)
        {
            d->m_AlbumObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("albumObject");
        }

        if(d->m_SliderObject == NULL)
        {
            d->m_SliderObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("sliderObject");
        }

        if(d->m_PreBtnObject == NULL)
        {
            d->m_PreBtnObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("preBtnObject");
        }

        if(d->m_PlayBtnObject == NULL)
        {
            d->m_PlayBtnObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("playBtnObject");
        }

        if(d->m_NextBtnObject == NULL)
        {
            d->m_NextBtnObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("nextBtnObject");
        }

        if(d->m_ModeBtnObject == NULL)
        {
            d->m_ModeBtnObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("modeBtnObject");
        }
        if(d->m_RemaTimeObject == NULL)
        {
            d->m_RemaTimeObject = d->m_MultiMediaPlayWidgetObject->findChild<QObject*>("remaTimeObject");
        }
    }
    d->connectAllSlots();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_PreBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_PlayBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_NextBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_ModeBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_SliderObject, ARKSENDER(sliderReleased(int)),
                     this,        ARKRECEIVER(onSliderReleased(int)),
                     type);
   QObject::connect(this, SIGNAL(startMusicPlay(int,int)),
                     this,        SLOT(onMusicListViewItemClicked(int,int)),type);

   QObject::connect(d->m_MultiMediaPlayWidgetObject, ARKSENDER(musicTypeChanged()),
                    this,        ARKRECEIVER(onMusicTypeChanged()),
                    type);

}
void MultiMediaPlayWidget::setMultiMediaObject(QObject* qmlObject)
{
    Q_D(MultiMediaPlayWidget);
    d->m_MultiMediaObject = qmlObject;
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_MultiMediaObject, ARKSENDER(visibleChanged()),
                     this,      ARKRECEIVER(onVisibleChanged()),
                     type);
}
void MultiMediaPlayWidget::onVisibleChanged(){
    Q_D(MultiMediaPlayWidget);
    qDebug()<<"++++MultiMediaPlayWidget::onVisibleChanged()++++"<<d->m_Playtype;
    if(d->m_MultiMediaObject->property("visible").toBool())
    {
        QQmlProperty(d->m_MultiMediaObject,"mutilMediaIndex").write(d->m_Playtype);
    }
    else{
        QQmlProperty(d->m_MultiMediaObject,"mutilMediaIndex").write(-1);
    }
}
void MultiMediaPlayWidget::onToolButtonRelease()
{
    Q_D(MultiMediaPlayWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if (ptr == d->m_PreBtnObject) {
        if(g_Audio->getAudioSource() == AS_BluetoothMusic){
            g_Bluetooth->musicPrevious();
        }
        else
        {
            QQmlProperty(d->m_UsbMusicListviewWidgetObject,"itemClicked").write(false);
            g_Multimedia->musicPlayerPlayPreviousListViewIndex();
        }
    } else if (ptr == d->m_PlayBtnObject) {
        qDebug() <<"++++++++g_Audio->getAudioSource()223++++++"<<g_Audio->getAudioSource();
        if(g_Audio->getAudioSource() == AS_BluetoothMusic)
        {
            g_Bluetooth->musicToggle();
        }
        else if(g_Audio->getAudioSource() == AS_Music)
        {
            g_Multimedia->musicPlayerSetPlayStatusToggle();
        }
        else{
            if(g_Bluetooth->connectStatus()== Bluetooth::BCS_Connected)
            {
                g_Bluetooth->musicPlay();
            }
        }
    } else if (ptr == d->m_NextBtnObject) {
        if(g_Audio->getAudioSource() == AS_BluetoothMusic)
        {
            g_Bluetooth->musicNext();
        }
        else{
            QQmlProperty(d->m_UsbMusicListviewWidgetObject,"itemClicked").write(false);
            g_Multimedia->musicPlayerPlayNextListViewIndex();
        }
    } else if (ptr == d->m_ModeBtnObject) {
        g_Multimedia->musicPlayerSetPlayModeToggle();//音乐播放模式
    }
}
void MultiMediaPlayWidget::onMusicTypeChanged(){
    Q_D(MultiMediaPlayWidget);
    d->initializeMusicTypeTimer();
    d->m_MusicTypeTimer->start();
}
void MultiMediaPlayWidget::startMusicTypeTimer()
{
    Q_D(MultiMediaPlayWidget);
    d->initializeMusicTypeTimer();
    d->m_MusicTypeTimer->start();
}
void MultiMediaPlayWidget::setUsbMusicListviewObject(QObject* qmlObject){
    Q_D(MultiMediaPlayWidget);
    if(d->m_UsbMusicListviewWidgetObject == NULL)
    {
        d->m_UsbMusicListviewWidgetObject = qmlObject;
    }
    if(d->m_UsbMusicListviewWidgetObject != NULL)
    {
        if(d->m_UsbMusicListviewObject == NULL)
        {
            d->m_UsbMusicListviewObject = d->m_UsbMusicListviewWidgetObject->findChild<QObject*>("listViewObject");
        }
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_UsbMusicListviewWidgetObject, SIGNAL(usbMusicListviewItemClicked(int,int)),
                     this,        SLOT(onMusicListViewItemClicked(int,int)),type);
}

void MultiMediaPlayWidget::setSdMusicListviewObject(QObject* qmlObject){
    Q_D(MultiMediaPlayWidget);
    if(d->m_SdMusicListviewWidgetObject == NULL)
    {
        d->m_SdMusicListviewWidgetObject = qmlObject;
    }
    if(d->m_SdMusicListviewWidgetObject != NULL)
    {
        if(d->m_SdMusicListviewObject == NULL)
        {
            d->m_SdMusicListviewObject = d->m_SdMusicListviewWidgetObject->findChild<QObject*>("listViewObject");
        }
    }
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_SdMusicListviewWidgetObject, SIGNAL(sdMusicListviewItemClicked(int,int)),
                     this,        SLOT(onMusicListViewItemClicked(int,int)),type);
}

void MultiMediaPlayWidget::onTimeout(){
    Q_D(MultiMediaPlayWidget);
    QTimer* ptr = static_cast<QTimer*>(sender());
    if(ptr == d->m_Timer)
    {
        if(d->m_Type == DWT_USBDisk){
            if(d->m_Playtype == M_Undefile)
            {
                emit startMusicPlay(d->m_Type,d->m_UsbLastIndex);
            }
        }
        else if(d->m_Type == DWT_SDDisk)
        {
            if(d->m_Playtype == M_Undefile)
            {
                emit startMusicPlay(d->m_Type,d->m_SdLastIndex);
            }
        }
    }
    else if(ptr == d->m_MusicTypeTimer)
    {
        qDebug()<<"+++++++onTimeout0000++++++++";
        if(d->m_MultiMediaObject != NULL)
            QQmlProperty(d->m_MultiMediaObject,"mutilMediaIndex").write(d->m_Playtype);
    }
    else if(ptr == d->m_MediaTypeTimer){
        d->m_FilePath.clear();
        qDebug()<<"=======d->m_Type=========="<<d->m_Type;
        qDebug()<<"-------d->m_PlayStatus------"<< d->m_PlayStatus;
        if(d->m_Type == DWT_SDDisk && d->m_PlayStatus == MPPS_Play)
        {
            d->m_Playtype = M_SdMusic;
            qDebug()<<"+++++++++++oldHolder+++++++++++"<<d->m_Playtype;
            g_Multimedia->musicPlayerPlayListViewIndex(DWT_SDDisk, d->m_SdLastIndex, d->m_SdElapsed);
            if(d->m_MultiMediaObject != NULL)
            {
                QQmlProperty(d->m_MultiMediaObject,"mutilMediaIndex").write(d->m_Playtype);
            }
        }
        else if(d->m_Type == DWT_USBDisk && d->m_PlayStatus == MPPS_Play)
        {
            d->m_Playtype = M_UsbMusic;
            g_Multimedia->musicPlayerPlayListViewIndex(DWT_USBDisk, d->m_UsbLastIndex, d->m_UsbElapsed);
            if(d->m_MultiMediaObject != NULL)
            {
                QQmlProperty(d->m_MultiMediaObject,"mutilMediaIndex").write(d->m_Playtype);
            }
        }
    }
}
void MultiMediaPlayWidget::onMusicPlayerExit()
{
    Q_D(MultiMediaPlayWidget);
    qDebug()<<"+++++++++++onMusicPlayerExit0000+++++++++";
    d->m_InitMusicPlay = true;
}
void MultiMediaPlayWidget::onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml)
{
    Q_D(MultiMediaPlayWidget);
    if (DWT_USBDisk == type) {
        QDomDocument document;
        document.setContent(xml);
        QDomElement root = document.documentElement();
        if ((!root.isNull())
                && (root.isElement())
                && (QString("MusicPlayer") == root.toElement().tagName())
                && (root.hasChildNodes())) {
            QDomNode node = root.firstChild();
            while (!node.isNull()) {
                if (node.isElement()) {
                    QDomElement element = node.toElement();
                    if (!element.isNull()) {
                        if (QString("USBFileNames") == element.tagName()) {
                            QDomNodeList nodeList = element.childNodes();
                            d->m_UsbMusicList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            QFileInfo fileInfo(node.toElement().text());
                                            d->m_UsbMusicList.append(fileInfo.fileName());
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
                                            int index = QString(node.toElement().text().split(QChar('-')).at(0)).toInt();
                                            d->m_UsbElapsed = QString(node.toElement().text().split(QChar('-')).at(1)).toInt();
                                            if((d->m_UsbMusicList.size()-1) > index)
                                            {
                                                d->m_FilePath.clear();
                                                d->m_UsbLastIndex = index;
                                                d->m_Type = type;
                                                d->initializeTimer();
                                                if(g_Link->getLinkConnectStatus()== 0 || (g_Link->getLinkConnectStatus()!= 0 && g_Link->getDbusConnectStatus() == DBUS_BACKGROUND))
                                                {
                                                    d->m_Timer->start();
                                                }
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
                && (QString("MusicPlayer") == root.toElement().tagName())
                && (root.hasChildNodes())) {
            QDomNode node = root.firstChild();
            while (!node.isNull()) {
                if (node.isElement()) {
                    QDomElement element = node.toElement();
                    if (!element.isNull()) {
                        if (QString("SDFileNames") == element.tagName()) {
                            QDomNodeList nodeList = element.childNodes();
                            d->m_SdMusicList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            QFileInfo fileInfo(node.toElement().text());
                                            d->m_SdMusicList.append(fileInfo.fileName());
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
                                            if((d->m_SdMusicList.size()-1) > index)
                                            {
                                                d->m_FilePath.clear();
                                                d->m_SdLastIndex = index;
                                                d->m_Type = type;
                                                d->initializeTimer();
                                                if(g_Link->getLinkConnectStatus()== 0 || (g_Link->getLinkConnectStatus()!= 0 && g_Link->getDbusConnectStatus() == DBUS_BACKGROUND))
                                                {
                                                    d->m_Timer->start();
                                                }
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

void MultiMediaPlayWidget::onMusicListViewItemClicked(int type,int index)
{
    Q_D(MultiMediaPlayWidget);
    static bool _InitClicked(true);
    if(_InitClicked){
        _InitClicked = false;
        g_Multimedia->musicPlayerSetPlayMode(getCrrentMusicPlayerMode());//初始化(记忆文件中)播放模式
    }
    qDebug()<<"+++++++++d->m_LastDeviceType+++++++++"<<d->m_LastDeviceType;
    qDebug()<<"+++++++++d->m_LastPlayIndex+++++++++"<<d->m_LastPlayIndex;
    qDebug()<<"+++++++++d->m_InitMusicPlay+++++++++"<<d->m_InitMusicPlay;
    qDebug()<<"+++++++++type+++++++++"<<type;
    qDebug()<<"+++++++++index+++++++++"<<index;
    if((d->m_LastDeviceType == type && d->m_LastPlayIndex == index && d->m_InitMusicPlay == false) || (index < 0))
    {
        return;
    }
    d->m_FilePath.clear();
    if(type == DWT_USBDisk){
        if (index == d->m_UsbLastIndex) {
            if(d->m_InitMusicPlay)
            {
                d->m_InitMusicPlay = false;
                g_Multimedia->musicPlayerPlayListViewIndex(DWT_USBDisk, index, d->m_UsbElapsed);

            }
            else
            {
                d->m_UsbElapsed   = 0;
                g_Multimedia->musicPlayerPlayListViewIndex(DWT_USBDisk, index, d->m_UsbElapsed);
            }
        } else {
            d->m_InitMusicPlay = false;
            d->m_UsbElapsed   = 0;
            d->m_UsbLastIndex = index;
            g_Multimedia->musicPlayerPlayListViewIndex(DWT_USBDisk, index, d->m_UsbElapsed);

        }
    }
    if(type == DWT_SDDisk){
        if (index == d->m_SdLastIndex) {
            if(d->m_InitMusicPlay)
            {
                d->m_InitMusicPlay = false;
                g_Multimedia->musicPlayerPlayListViewIndex(DWT_SDDisk, index, d->m_SdElapsed);

            }
            else
            {
                d->m_SdElapsed = 0;
                d->m_SdLastIndex = index;
                g_Multimedia->musicPlayerPlayListViewIndex(DWT_SDDisk, index, d->m_SdElapsed);
            }
        }else {
            d->m_InitMusicPlay = false;
            d->m_SdElapsed = 0;
            d->m_SdLastIndex = index;
            g_Multimedia->musicPlayerPlayListViewIndex(DWT_SDDisk, index, d->m_SdElapsed);

        }
    }
    d->m_LastDeviceType = type;
    d->m_LastPlayIndex  = index;
}

void MultiMediaPlayWidget::onMusicPlayerPlayStatus(const DeviceWatcherType type, const MusicPlayerPlayStatus status)
{
    Q_D(MultiMediaPlayWidget);
    qDebug()<<"++++++++++MultiMediaPlayWidget::onMusicPlayerPlayStatus+++++++++++++++"<<status;
    d->m_Type = type;
    QQmlProperty(d->m_PreBtnObject, "enabled").write(MPPS_Start != status);
    QQmlProperty(d->m_PlayBtnObject, "enabled").write(MPPS_Start != status);
    QQmlProperty(d->m_NextBtnObject, "enabled").write(MPPS_Start != status);
    switch (status) {
    case MPPS_Play: {
        if(type == DWT_SDDisk){
            d->m_Playtype = M_SdMusic;
        }
        else if(type == DWT_USBDisk){
            d->m_Playtype = M_UsbMusic;
        }
        d->m_PlayStatus = status;
        QQmlProperty(d->m_MultiMediaObject,"mutilMediaIndex").write(d->m_Playtype);
        QQmlProperty(d->m_PlayBtnObject, "playStatus").write(true);
        break;
    }
    case MPPS_Start:
    case MPPS_Unsupport:
    case MPPS_Pause:
    case MPPS_Stop: {
        d->m_PlayStatus = status;
        QQmlProperty(d->m_PlayBtnObject, "playStatus").write(false);
        break;
    }
    case MPPS_Exit:{
        if(d->m_Playtype != M_BtMusic)
        {
            QQmlProperty(d->m_PlayBtnObject, "playStatus").write(false);
            QQmlProperty(d->m_PreBtnObject, "enabled").write(false);
            QQmlProperty(d->m_PlayBtnObject, "enabled").write(false);
            QQmlProperty(d->m_NextBtnObject, "enabled").write(false);
            QQmlProperty(d->m_SliderObject,"currentValue").write(0);
            QQmlProperty(d->m_TitleObject,"text").write("");
            QQmlProperty(d->m_ArtistObject,"text").write("");
            QQmlProperty(d->m_AlbumObject,"text").write("");
            QQmlProperty(d->m_RemaTimeObject,"text").write("00:01");
        }
        break;
    }
    default : {
        break;
    }
    }
    switch (status) {
        case MPPS_Stop:
        case MPPS_Pause:
        case MPPS_Play:
        case MPPS_SeekFinish: {
            if (0 != d->m_SliderObject->property("toValue").toInt()) {
                QQmlProperty(d->m_SliderObject,"enabled").write(true);
            }
            break;
        }
        default: {
            QQmlProperty(d->m_SliderObject,"enabled").write(false);
            break;
        }
    }
}

void MultiMediaPlayWidget::onMusicPlayerPlayMode(const MusicPlayerPlayMode mode)
{
    Q_D(MultiMediaPlayWidget);
    SaveCurrentPlaymodeToFile(mode);//保存当前播放模式到指定文件
    switch (mode) {
    case MPPM_RepeatOnce: {
        QQmlProperty(d->m_ModeBtnObject,"playModeType").write(2);
        break;
    }
    case MPPM_Shuffle: {
        QQmlProperty(d->m_ModeBtnObject,"playModeType").write(1);
        break;
    }
    case MPPM_AllRepeat:
    default : {
        QQmlProperty(d->m_ModeBtnObject,"playModeType").write(0);
        break;
    }
    }
}

void MultiMediaPlayWidget::onMusicPlayerElapsedInformation(const int elapsedTime, const int elapsedMillesimal)
{
    Q_D(MultiMediaPlayWidget);
    if(d->m_SliderObject->property("enabled").toBool() ||
            d->m_SliderObject->property("toValue").toInt() == 0)
    {
        QString remaTime =QString("-") + d->convertTime(d->m_EndTime - elapsedTime);
        QQmlProperty(d->m_RemaTimeObject,"text").write(remaTime);
    }
    if(d->m_SliderObject->property("enabled").toBool())
    {
        if(!d->m_SliderObject->property("clickStatus").toBool())
        {
           QQmlProperty(d->m_SliderObject,"currentValue").write(elapsedTime);
        }
        if(d->m_Type == DWT_SDDisk)
        {
            d->m_SdElapsed = elapsedTime;
        }
        else if(d->m_Type == DWT_USBDisk)
        {
            d->m_UsbElapsed = elapsedTime;
        }
    }
}

void MultiMediaPlayWidget::onMusicPlayerID3TagChange(const int type, const int index, const QString &fileName, const QString& title, const QString& artist, const QString& album, const int endTime)
{
    Q_D(MultiMediaPlayWidget);
    d->m_LastDeviceType = type;
    d->m_LastPlayIndex  = index;
    qDebug()<<"++++++***index***++++++"<<index;
    if(type == DWT_USBDisk)
    {
        QQmlProperty(d->m_RemaTimeObject,"text").write(d->convertTime(endTime));
        QQmlProperty(d->m_SliderObject,"enabled").write(0 != endTime);
        QQmlProperty(d->m_SliderObject,"fromValue").write(0);
        QQmlProperty(d->m_SliderObject,"toValue").write(endTime);
        d->m_EndTime = endTime;
        if(d->m_UsbLastIndex != index)
        {
            QQmlProperty(d->m_UsbMusicListviewWidgetObject,"itemClicked").write(false);
        }
        if(!d->m_UsbMusicListviewWidgetObject->property("itemClicked").toBool())
        {
            qDebug()<<"++++++xxxxx0000+++++++++"<<d->m_UsbMusicListviewWidgetObject->property("listViewCurrentIndex").toInt();
            QQmlProperty(d->m_UsbMusicListviewObject,"contentY").write(88*index);
            QQmlProperty(d->m_UsbMusicListviewObject,"currentIndex").write(index);
            QQmlProperty(d->m_UsbMusicListviewWidgetObject,"listViewCurrentIndex").write(index);
        }
        if (d->m_FilePath != fileName) {
            d->m_FilePath = fileName;
            if (title.isEmpty()) {
                d->m_TitleText = d->m_UsbMusicList.at(index).left(d->m_UsbMusicList.at(index).size()-4);
            } else {
                d->m_TitleText = title;
                qDebug()<<"+++++[MultiMediaPlayWidget::onMusicPlayerID3TagChange:]++++++++"<<d->m_TitleText;
            }
            if (artist.isEmpty()) {
                d->m_ArtistText = QObject::tr(SourceString::Unknown.toLocal8Bit().constData());
            } else {
                d->m_ArtistText = artist;
            }
            if (album.isEmpty()) {
                d->m_AlbumText = QObject::tr(SourceString::Unknown.toLocal8Bit().constData());
            } else {
                d->m_AlbumText = album;
            }
            QQmlProperty(d->m_TitleObject,"text").write(d->m_TitleText);
            QQmlProperty(d->m_ArtistObject,"text").write(d->m_ArtistText);
            QQmlProperty(d->m_AlbumObject,"text").write(d->m_AlbumText);
        }
        d->m_UsbLastIndex = index;
    }
    else if(type == DWT_SDDisk){
        QQmlProperty(d->m_RemaTimeObject,"text").write(d->convertTime(endTime));
        QQmlProperty(d->m_SliderObject,"enabled").write(0 != endTime);
        QQmlProperty(d->m_SliderObject,"fromValue").write(0);
        QQmlProperty(d->m_SliderObject,"toValue").write(endTime);
        d->m_EndTime     = endTime;
        qDebug()<<"++++++fileName++++++"<<fileName;
        if(d->m_SdLastIndex != index)
        {
            QQmlProperty(d->m_SdMusicListviewWidgetObject,"itemClicked").write(false);
        }
        if(!d->m_SdMusicListviewWidgetObject->property("itemClicked").toBool())
        {
            QQmlProperty(d->m_SdMusicListviewObject,"contentY").write(88*index);
            QQmlProperty(d->m_SdMusicListviewObject,"currentIndex").write(index);
            QQmlProperty(d->m_SdMusicListviewWidgetObject,"listViewCurrentIndex").write(index);
        }
        if (d->m_FilePath != fileName) {
            d->m_FilePath = fileName;
            if (title.isEmpty()) {
                d->m_TitleText = d->m_SdMusicList.at(index).left(d->m_SdMusicList.at(index).size()-4);
            } else {
                d->m_TitleText = title;
            }
            if (artist.isEmpty()) {
                d->m_ArtistText = QObject::tr(SourceString::Unknown.toLocal8Bit().constData());
            } else {
                d->m_ArtistText = artist;
            }
            if (album.isEmpty()) {
                d->m_AlbumText = QObject::tr(SourceString::Unknown.toLocal8Bit().constData());
            } else {
                d->m_AlbumText = album;
            }
            QQmlProperty(d->m_TitleObject,"text").write(d->m_TitleText);
            QQmlProperty(d->m_ArtistObject,"text").write(d->m_ArtistText);
            QQmlProperty(d->m_AlbumObject,"text").write(d->m_AlbumText);
        }
        d->m_SdLastIndex = index;
    }
}
void MultiMediaPlayWidget::onSliderReleased(int value)
{
    Q_D(MultiMediaPlayWidget);
    if (0 != d->m_SliderObject->property("toValue").toInt()) {
        if(g_Audio->getAudioSource() == AS_BluetoothMusic){
            QQmlProperty(d->m_SliderObject,"enabled").write(false);
            //int millesimal = 1000 * (static_cast<float>(value) / d->m_SliderObject->property("toValue").toInt());
        }
        else {
            QQmlProperty(d->m_SliderObject,"enabled").write(false);
            int millesimal = 1000 * (static_cast<float>(value) / d->m_SliderObject->property("toValue").toInt());
            g_Multimedia->musicPlayerSeekToMillesimal(millesimal);
        }
    }
}

void MultiMediaPlayWidget::onConnectStatusChange(int status){
    Q_D(MultiMediaPlayWidget);
    qDebug()<<"+++++++++MultiMediaPlayWidget::onConnectStatusChange+++++"<<status;
    if(status < 3){
        if(d->m_Playtype == M_BtMusic){
            QQmlProperty(d->m_PreBtnObject, "enabled").write(false);
            QQmlProperty(d->m_PlayBtnObject, "enabled").write(false);
            QQmlProperty(d->m_NextBtnObject, "enabled").write(false);
            QQmlProperty(d->m_PlayBtnObject, "playStatus").write(false);
            QQmlProperty(d->m_TitleObject,"text").write("");
            QQmlProperty(d->m_ArtistObject,"text").write("");
            QQmlProperty(d->m_AlbumObject,"text").write("");
        }
    }
    qDebug()<<"+++++++++MultiMediaPlayWidget::onConnectStatusChange::END+++++";
}
void MultiMediaPlayWidget::onUsbMediaPlayExit(){
    Q_D(MultiMediaPlayWidget);
    if(d->m_Playtype == M_UsbMusic)
    {
        QQmlProperty(d->m_PreBtnObject, "enabled").write(false);
        QQmlProperty(d->m_PlayBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_PlayBtnObject,"playStatus").write(false);
        QQmlProperty(d->m_TitleObject,"text").write("");
        QQmlProperty(d->m_ArtistObject,"text").write("");
        QQmlProperty(d->m_AlbumObject,"text").write("");
        QQmlProperty(d->m_SliderObject,"currentValue").write(0);
        QQmlProperty(d->m_SliderObject,"enabled").write(false);
        QQmlProperty(d->m_SliderObject,"toValue").write(0.1);
        d->m_Playtype = M_Undefile;
        g_Bluetooth->setMusicType(0);
        if(d->m_LastDeviceType == DWT_SDDisk)
        {
            d->m_InitMusicPlay  = true;
        }
    }
}
void MultiMediaPlayWidget::onSdMediaPlayExit(){
    Q_D(MultiMediaPlayWidget);
    if(d->m_Playtype == M_SdMusic)
    {
        QQmlProperty(d->m_PreBtnObject, "enabled").write(false);
        QQmlProperty(d->m_PlayBtnObject,"enabled").write(false);
        QQmlProperty(d->m_NextBtnObject,"enabled").write(false);
        QQmlProperty(d->m_PlayBtnObject,"playStatus").write(false);
        QQmlProperty(d->m_TitleObject,"text").write("");
        QQmlProperty(d->m_ArtistObject,"text").write("");
        QQmlProperty(d->m_AlbumObject,"text").write("");
        QQmlProperty(d->m_SliderObject,"currentValue").write(0);
        QQmlProperty(d->m_SliderObject,"enabled").write(false);
        QQmlProperty(d->m_SliderObject,"toValue").write(0.1);
        d->m_Playtype = M_Undefile;
        g_Bluetooth->setMusicType(0);
        if(d->m_LastDeviceType == M_SdMusic)
        {
            d->m_InitMusicPlay  = true;
        }
    }
}
void MultiMediaPlayWidget::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder)
{
    Q_D(MultiMediaPlayWidget);
    qDebug()<<"+++++++++MultiMediaPlayWidget::onHolderChange+++++++++++"<< oldHolder <<" "<< newHolder;
    int mode = getCrrentMusicPlayerMode();//保存当前播放模式到指定文件
    if(oldHolder == AS_Music)
    {
        if(newHolder != AS_BluetoothMusic)
        {
             QQmlProperty(d->m_PlayBtnObject, "playStatus").write(false);
        }
    }
    else if(oldHolder == AS_BluetoothMusic)
    {
        if(newHolder != AS_Music)
        {
             QQmlProperty(d->m_PlayBtnObject, "playStatus").write(false);
        }
    }
    switch (newHolder){
        case AS_Music:
            switch (mode) {
                case MPPM_RepeatOnce: {
                    QQmlProperty(d->m_ModeBtnObject,"playModeType").write(2);
                    break;
                }
                case MPPM_Shuffle: {
                    QQmlProperty(d->m_ModeBtnObject,"playModeType").write(1);
                    break;
                }
                case MPPM_AllRepeat:
                default : {
                    QQmlProperty(d->m_ModeBtnObject,"playModeType").write(0);
                    break;
                }
            }
            g_Bluetooth->setMusicType(1);
            if(oldHolder == AS_BluetoothMusic)
            {
                d->initializeMediaTypeTimer();
                if(d->m_InitMusicPlay)
                {
                    d->m_MediaTypeTimer->start();
                }
            }
            break;
        case AS_BluetoothMusic:
            d->initializeMediaTypeTimer();
            if(d->m_MediaTypeTimer->isActive())
            {
                d->m_MediaTypeTimer->stop();
            }
            QQmlProperty(d->m_ModeBtnObject,"playModeType").write(0);
            break;
        default:
            d->m_Playtype = M_Undefile;
            g_Bluetooth->setMusicType(0);
            break;
    }
    switch (oldHolder) {
        case AS_BluetoothMusic:
            g_Bluetooth->musicPause();
            break;
        case AS_Music:
            if(newHolder == AS_BluetoothMusic)
            {
                g_Multimedia->musicPlayerExit();
            }
            break;
        case AS_Video:
            //g_Multimedia->videoPlayerExit();
        default:
            break;
    }
}

void MultiMediaPlayWidget::onMusicStatusChange(const QString& musicName, const int status){
    Q_D(MultiMediaPlayWidget);
    qDebug()<<"+++++++onMusicStatusChange0000++++++++++"<<status;
    if(g_Audio ->getAudioSource() == AS_BluetoothMusic){
        if(d->m_PlayBtnObject != NULL){
            QQmlProperty(d->m_PlayBtnObject, "enabled").write(true);
            QQmlProperty(d->m_PreBtnObject,  "enabled").write(true);
            QQmlProperty(d->m_NextBtnObject, "enabled").write(true);
        }
        if(status == Bluetooth::BtMusic_Playing){
            if(d->m_PlayBtnObject != NULL)
            {
                QQmlProperty(d->m_PlayBtnObject, "playStatus").write(true);
                d->m_Playtype      = M_BtMusic;
                d->m_InitMusicPlay = true;
                if(d->m_MultiMediaObject != NULL)
                    QQmlProperty(d->m_MultiMediaObject,"mutilMediaIndex").write(d->m_Playtype);
            }
        }
        else{
            qDebug()<<"+++++++onMusicStatusChange1111++++++++++"<<d->m_Playtype;
            if(d->m_PlayBtnObject != NULL)
            {
                if(d->m_Playtype == M_BtMusic)
                    QQmlProperty(d->m_PlayBtnObject, "playStatus").write(false);
            }
        }
    }
}

void MultiMediaPlayWidget::onBtMusicID3InfoChange(QString titile,QString artist,QString album){
    Q_D(MultiMediaPlayWidget);
    qDebug()<<"++++++++++g_Audio->getAudioSource()++++++++++"<<g_Audio->getAudioSource();
    if(g_Audio->getAudioSource() == AS_BluetoothMusic)
    {
        if(d->m_TitleObject == NULL || d->m_TitleObject == NULL
                || d->m_TitleObject == NULL)
        {
            return;
        }
        QQmlProperty(d->m_TitleObject,"text").write(titile);
        QQmlProperty(d->m_ArtistObject,"text").write(artist);
        QQmlProperty(d->m_AlbumObject,"text").write(album);
    }
}
void MultiMediaPlayWidget::onBtMusicElapsedInfo(int elapsed,int EndTime){
    Q_D(MultiMediaPlayWidget);
    if(g_Audio->getAudioSource() == AS_BluetoothMusic){
        if(d->m_SliderObject == NULL)
        {
            return;
        }
        QQmlProperty(d->m_SliderObject,"enabled").write(true);
        QQmlProperty(d->m_SliderObject,"fromValue").write(0);
        QQmlProperty(d->m_SliderObject,"toValue").write(EndTime);
        if(d->m_RemaTimeObject != NULL)
        {
            if(d->m_SliderObject->property("enabled").toBool())
            {
                QString remaTime =QString("-") + d->convertTime(EndTime - elapsed);
                QQmlProperty(d->m_RemaTimeObject,"text").write(remaTime);
            }
            if(d->m_SliderObject->property("enabled").toBool())
            {
                if(!d->m_SliderObject->property("clickStatus").toBool())
                {
                   QQmlProperty(d->m_SliderObject,"currentValue").write(elapsed);
                }
            }
        }
    }
}

void MultiMediaPlayWidget::onDeviceWatcherStatus(const int type, const int status){
    Q_D(MultiMediaPlayWidget);
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
            if(d->m_LastDeviceType == DWT_USBDisk)
            {
                d->m_InitMusicPlay  = true;
            }
            break;
        }
        case DWS_Remove: {
            QQmlProperty(d->m_UsbMusicListviewObject,"contentY").write(0);
            QQmlProperty(d->m_UsbMusicListviewObject,"currentIndex").write(0);
            QQmlProperty(d->m_UsbMusicListviewWidgetObject,"listViewCurrentIndex").write(-1);
            QQmlProperty(d->m_UsbMusicListviewWidgetObject,"lastIndex").write(-1);
            QQmlProperty(d->m_UsbMusicListviewWidgetObject,"itemClicked").write(false);
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
                if(d->m_LastDeviceType == DWT_SDDisk)
                {
                    d->m_InitMusicPlay  = true;
                }
                break;
            }
            case DWS_Remove: {
                QQmlProperty(d->m_SdMusicListviewObject,"contentY").write(0);
                QQmlProperty(d->m_SdMusicListviewObject,"currentIndex").write(0);
                QQmlProperty(d->m_SdMusicListviewWidgetObject,"listViewCurrentIndex").write(-1);
                QQmlProperty(d->m_SdMusicListviewWidgetObject,"lastIndex").write(-1);
                QQmlProperty(d->m_SdMusicListviewWidgetObject,"itemClicked").write(false);
                d->m_InitMusicPlay  = true;
                d->m_LastDeviceType = -1;
                d->m_LastPlayIndex  = -1;
                break;
            }
            default: {
                break;
            }
        }
    }
}

MultiMediaPlayWidgetPrivate::MultiMediaPlayWidgetPrivate(MultiMediaPlayWidget *parent)
    : q_ptr(parent)
{
    m_MultiMediaPlayWidgetObject = NULL;
    m_UsbMusicListviewWidgetObject  = NULL;
    m_UsbMusicListviewObject= NULL;
    m_SdMusicListviewWidgetObject = NULL;
    m_SdMusicListviewObject = NULL;
    m_AlbumCoveImageObject= NULL;
    m_TitleObject  = NULL;
    m_ArtistObject = NULL;
    m_AlbumObject  = NULL;
    m_SliderObject = NULL;
    m_PreBtnObject = NULL;
    m_PlayBtnObject= NULL;
    m_NextBtnObject= NULL;
    m_ModeBtnObject= NULL;
    m_RemaTimeObject = NULL;
    m_Timer = NULL;
    m_MusicTypeTimer   = NULL;
    m_MediaTypeTimer   = NULL;
    m_MultiMediaObject = NULL;
    m_UsbElapsed   = 0;
    m_UsbLastIndex = -1;
    m_SdLastIndex    = -1;
    m_SdElapsed      = 0;
    m_EndTime        = 0;
    m_Playtype       = -1;
    m_Type           = -1;
    m_PlayStatus     = -1;
    m_InitMusicPlay  = true;
    m_LastDeviceType = -1;
    m_LastPlayIndex  = -1;
}
MultiMediaPlayWidgetPrivate::~MultiMediaPlayWidgetPrivate()
{

}
void MultiMediaPlayWidgetPrivate::initializeTimer(){
    Q_Q(MultiMediaPlayWidget);
    if(m_Timer == NULL){
        m_Timer = new QTimer(q);
        m_Timer->setInterval(1500);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer,ARKSENDER(timeout()),q,ARKRECEIVER(onTimeout()));
    }

}

void MultiMediaPlayWidgetPrivate::initializeMusicTypeTimer()
{
    Q_Q(MultiMediaPlayWidget);
    if(m_MusicTypeTimer == NULL)
    {
        m_MusicTypeTimer = new QTimer(q);
        m_MusicTypeTimer->setInterval(5000);
        m_MusicTypeTimer->setSingleShot(true);
        QObject::connect(m_MusicTypeTimer,ARKSENDER(timeout()),q,ARKRECEIVER(onTimeout()));
    }
}

void MultiMediaPlayWidgetPrivate::initializeMediaTypeTimer(){
    Q_Q(MultiMediaPlayWidget);
    if(m_MediaTypeTimer == NULL)
    {
        m_MediaTypeTimer = new QTimer(q);
        m_MediaTypeTimer->setInterval(1000);
        m_MediaTypeTimer->setSingleShot(true);
        QObject::connect(m_MediaTypeTimer,ARKSENDER(timeout()),q,ARKRECEIVER(onTimeout()));
    }
}
QString MultiMediaPlayWidgetPrivate::convertTime(const int time)
{
    QString hour("%1");
    QString minute("%1");
    QString second("%1");
    return hour.arg((time / 60) / 60, 2, 10, QChar('0'))
            + QString(":") + minute.arg((time / 60) % 60, 2, 10, QChar('0'))
            + QString(":") + second.arg(time % 60, 2, 10, QChar('0'));
}
void MultiMediaPlayWidgetPrivate::connectAllSlots()
{
    Q_Q(MultiMediaPlayWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerFileNames(const int, const QString &)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerPlayStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerPlayMode(const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerElapsedInformation(const int, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerID3TagChange(const int, const int, const QString &, const QString&, const QString&, const QString&, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerExit()));
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onHolderChange(const int, const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onMusicStatusChange(const QString, const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onBtMusicID3InfoChange(QString, QString,QString)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onBtMusicElapsedInfo(int,int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(int)));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onUsbMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onSdMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerExit()));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onDeviceWatcherStatus(const int, const int)));
}

