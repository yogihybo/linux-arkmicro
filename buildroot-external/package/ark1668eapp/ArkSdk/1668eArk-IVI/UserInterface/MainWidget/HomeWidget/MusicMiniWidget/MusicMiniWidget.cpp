#include "MusicMiniWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Multimedia.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Widget.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>

class MusicMiniWidgetPrivate
{
    Q_DISABLE_COPY(MusicMiniWidgetPrivate)
public:
    explicit MusicMiniWidgetPrivate(MusicMiniWidget* parent);
    ~MusicMiniWidgetPrivate();
    void initializeObject();
    void connectAllSlots();
public:
    QObject* m_MusicMiniWidgetObject;
    QObject* m_MusicNameObject;
    QObject* m_PrevBtnObject;
    QObject* m_NextBtnObject;
    QObject* m_PlayBtnObject;
    int      m_MusicPlayType;
private:
    Q_DECLARE_PUBLIC(MusicMiniWidget)
    MusicMiniWidget* const q_ptr;
};

MusicMiniWidget::MusicMiniWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new MusicMiniWidgetPrivate(this))
{

}
void MusicMiniWidget::setMusicMiniWidgetObject(QObject* qmlObject){
    Q_D(MusicMiniWidget);
    if(d->m_MusicMiniWidgetObject == NULL)
    {
        d->m_MusicMiniWidgetObject = qmlObject;
    }
    d->initializeObject();
    d->connectAllSlots();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_PrevBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_NextBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_PlayBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_MusicMiniWidgetObject, ARKSENDER(musicMiniWidgetClicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
}
void MusicMiniWidget::onToolButtonRelease(){
    Q_D(MusicMiniWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_PrevBtnObject){
        if(g_Audio->getAudioSource() == AS_BluetoothMusic){
            g_Bluetooth->musicPrevious();
        }
        else
        {
            g_Multimedia->musicPlayerPlayPreviousListViewIndex();
        }
    }
    else if(ptr == d->m_PlayBtnObject){
        if(g_Audio ->getAudioSource() == AS_BluetoothMusic)
        {
            g_Bluetooth->musicToggle();
        }
        else if(g_Audio ->getAudioSource() == AS_Music)
        {
            g_Multimedia->musicPlayerSetPlayStatusToggle();
        }
        else{
            if(g_Bluetooth->connectStatus()== Bluetooth::BCS_Connected)
            {
                g_Bluetooth->musicPlay();
            }
        }
    }
    else if(ptr == d->m_NextBtnObject){
        if(g_Audio->getAudioSource() == AS_BluetoothMusic)
        {
            g_Bluetooth->musicNext();
        }
        else{
            g_Multimedia->musicPlayerPlayNextListViewIndex();
        }
    }
    else if(ptr ==d->m_MusicMiniWidgetObject){

        emit g_Widget->onWidgetTypeChange(Widget::T_MusicPlay, Widget::T_Home,QString("show"));
    }

}
void MusicMiniWidget::onMusicPlayerID3TagChange(const int type, const int index, const QString &fileName,
                                                const QString& title, const QString& artist,
                                                const QString& album, const int endTime)
{

    Q_D(MusicMiniWidget);
    QStringList fileNameDirList = fileName.split("/");
    QString _FileName = fileNameDirList.at(fileNameDirList.size()-1);
    _FileName = _FileName.left(_FileName.size()-4);
    QQmlProperty(d->m_MusicNameObject,"text").write(_FileName);

}

void MusicMiniWidget::onMusicPlayerPlayStatus(const int type, const int status)
{
    Q_D(MusicMiniWidget);
    if(g_Audio->getAudioSource() != AS_Music)
    {
        return;
    }
    QQmlProperty(d->m_PrevBtnObject, "enabled").write(MPPS_Start != status);
    QQmlProperty(d->m_PlayBtnObject, "enabled").write(MPPS_Start != status);
    QQmlProperty(d->m_NextBtnObject, "enabled").write(MPPS_Start != status);
    switch (status) {
        case MPPS_Play: {
            if(type == 0)
            {
                d->m_MusicPlayType = M_SdMusic;
            }
            else if(type == 1)
            {
                d->m_MusicPlayType = M_UsbMusic;
            }
            QQmlProperty(d->m_PlayBtnObject, "playstatus").write(true);
            break;
        }
        case MPPS_Start:
        case MPPS_Unsupport:
        case MPPS_Pause:
        case MPPS_Stop: {

            QQmlProperty(d->m_PlayBtnObject, "playstatus").write(false);
            break;
        }
        default : {
            break;
        }
    }
}
void MusicMiniWidget::onBtMusicID3InfoChange(QString titile,QString artist,QString album){
    Q_D(MusicMiniWidget);
    if(g_Audio->getAudioSource() == AS_BluetoothMusic)
    {
        if(d->m_MusicNameObject == NULL)
        {
            return;
        }
        QQmlProperty(d->m_MusicNameObject,"text").write(titile+QString("-")+artist);
    }
}
void MusicMiniWidget::onMusicStatusChange(const QString fileName, const int status){
    Q_D(MusicMiniWidget);
    if(g_Audio->getAudioSource() != AS_BluetoothMusic)
    {
        return;
    }
    if(status == Bluetooth::BtMusic_Playing){
        if(d->m_PlayBtnObject != NULL)
        {
            d->m_MusicPlayType = M_BtMusic;
            QQmlProperty(d->m_PrevBtnObject, "enabled").write(true);
            QQmlProperty(d->m_PlayBtnObject, "enabled").write(true);
            QQmlProperty(d->m_NextBtnObject, "enabled").write(true);
            QQmlProperty(d->m_PlayBtnObject, "playstatus").write(true);
        }
    }
    else {
        if(d->m_PlayBtnObject!= NULL){
            if(d->m_MusicPlayType == M_BtMusic)
            {
                QQmlProperty(d->m_PlayBtnObject, "playstatus").write(false);
            }
        }
    }
}

void MusicMiniWidget::onConnectStatusChange(int status){
     Q_D(MusicMiniWidget);
    if(status < 3)
    {
        if(d->m_MusicPlayType == M_BtMusic){
            if(d->m_PlayBtnObject != NULL)
            {
                QQmlProperty(d->m_PrevBtnObject, "enabled").write(false);
                QQmlProperty(d->m_PlayBtnObject, "enabled").write(false);
                QQmlProperty(d->m_NextBtnObject, "enabled").write(false);
                QQmlProperty(d->m_MusicNameObject, "text").write("");
                QQmlProperty(d->m_PlayBtnObject, "playstatus").write(false);
            }
        }
    }
}

void MusicMiniWidget::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder){
   Q_D(MusicMiniWidget);
   if(oldHolder == AS_Music)
   {
       if(newHolder != AS_BluetoothMusic)
       {
            QQmlProperty(d->m_PlayBtnObject, "playstatus").write(false);
       }
   }
   else if(oldHolder == AS_BluetoothMusic){
       if(newHolder != AS_Music)
       {
            QQmlProperty(d->m_PlayBtnObject, "playstatus").write(false);
       }
   }
}

void MusicMiniWidget::onUsbMediaPlayExit(){
    Q_D(MusicMiniWidget);
    if(d->m_MusicPlayType == M_UsbMusic){
        if(d->m_PlayBtnObject != NULL)
        {
            qDebug()<<"++++++++++onUsbMediaPlayExit+++++++++";
            QQmlProperty(d->m_PrevBtnObject, "enabled").write(false);
            QQmlProperty(d->m_PlayBtnObject, "enabled").write(false);
            QQmlProperty(d->m_NextBtnObject, "enabled").write(false);
            QQmlProperty(d->m_MusicNameObject, "text").write("");
            QQmlProperty(d->m_PlayBtnObject, "playstatus").write(false);
        }
    }

}

void MusicMiniWidget::onSdMediaPlayExit(){
    Q_D(MusicMiniWidget);
    qDebug()<<"++++++++++d->m_MusicPlayType+++++++++"<<d->m_MusicPlayType;
    if(d->m_MusicPlayType == M_SdMusic){
        if(d->m_PlayBtnObject != NULL)
        {
            qDebug()<<"++++++++++onSdMediaPlayExit+++++++++";
            QQmlProperty(d->m_PrevBtnObject, "enabled").write(false);
            QQmlProperty(d->m_PlayBtnObject, "enabled").write(false);
            QQmlProperty(d->m_NextBtnObject, "enabled").write(false);
            QQmlProperty(d->m_MusicNameObject, "text").write("");
            QQmlProperty(d->m_PlayBtnObject, "playstatus").write(false);
        }
    }
}

MusicMiniWidgetPrivate::MusicMiniWidgetPrivate(MusicMiniWidget *parent)
    : q_ptr(parent)
{
    m_MusicMiniWidgetObject = NULL;
    m_PrevBtnObject = NULL;
    m_MusicNameObject = NULL;
    m_NextBtnObject = NULL;
    m_PlayBtnObject = NULL;
    m_MusicPlayType = M_Undefile;
}

MusicMiniWidgetPrivate::~MusicMiniWidgetPrivate()
{

}
void MusicMiniWidgetPrivate::initializeObject(){
    if(m_MusicNameObject == NULL)
    {
        m_MusicNameObject = m_MusicMiniWidgetObject->findChild<QObject*>("musicNameObject");
    }
    if(m_PrevBtnObject == NULL)
    {
        m_PrevBtnObject = m_MusicMiniWidgetObject->findChild<QObject*>("prevBtnObject");
    }
    if(m_NextBtnObject == NULL)
    {
        m_NextBtnObject = m_MusicMiniWidgetObject->findChild<QObject*>("nextBtnObject");
    }

    if(m_PlayBtnObject == NULL)
    {
        m_PlayBtnObject = m_MusicMiniWidgetObject->findChild<QObject*>("playbtnObject");
    }
}
void MusicMiniWidgetPrivate::connectAllSlots(){
    Q_Q(MusicMiniWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerID3TagChange(const int, const int, const QString &, const QString&, const QString&, const QString&, const int)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerPlayStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_Audio, q, ARKRECEIVER(onHolderChange(const int, const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onMusicStatusChange(const QString, const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(int)));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onUsbMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onSdMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onBtMusicID3InfoChange(QString, QString,QString)));
}

