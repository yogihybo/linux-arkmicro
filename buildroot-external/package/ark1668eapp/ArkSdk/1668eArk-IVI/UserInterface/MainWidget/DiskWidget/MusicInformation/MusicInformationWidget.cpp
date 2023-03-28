#include "MusicInformationWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Multimedia.h"
#include "BusinessLogic/Widget.h"
#include "RunnableThread.h"
#include <QMutex>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QDebug>
#include <QFile>
#include <tag.h>
#include <fileref.h>
#include <id3v2tag.h>
#include <attachedpictureframe.h>
#include <mpegfile.h>
#include <mp4file.h>
#include <flac/flacfile.h>
namespace SourceString {
static const QString Unknown(QObject::tr("Unknown"));
}

class MusicInformationWidgetPrivate
{
    Q_DISABLE_COPY(MusicInformationWidgetPrivate)
public:
    explicit MusicInformationWidgetPrivate(MusicInformationWidget* parent);
    ~MusicInformationWidgetPrivate();
    void connectAllSlots();
    void initializeImgProvider();
    void callQmlRefreshImg();
    void setImageSlot(const QImage &image);
public:
    QString m_TitleText;
    QString m_ArtistText;
    QString m_AlbumText;
    QString m_FilePath;
    CustomThread* m_CustomThread;
    QImage m_ConverImage;
    QMutex m_Mutex;
    bool   m_Flag;
    ImageProvider *m_pImgProvider;
    int    m_PlayType;
private:
    Q_DECLARE_PUBLIC(MusicInformationWidget)
    MusicInformationWidget* const q_ptr;
};

static void readThread(void *paramater)
{
    MusicInformationWidgetPrivate* ptr = static_cast<MusicInformationWidgetPrivate*>(paramater);
    //ptr->m_ConverImage = QImage("");
    if (ptr != NULL) {
        if (!ptr->m_Flag) {
            return;
        }
        QFileInfo fileInfo(ptr->m_FilePath);
        QString fileName = ptr->m_FilePath;
        if (fileInfo.suffix().contains(QString("mp3"), Qt::CaseInsensitive)) {
            if (!ptr->m_Flag) {
                return;
            }
            qDebug()<< "++++++++++MusicInformationWidgetPrivate:+++++++";
            TagLib::MPEG::File mpegFile(QFile::encodeName(fileName).constData(), false);
            if (mpegFile.isValid()) {
                if (!ptr->m_Flag) {
                    return;
                }
                TagLib::ID3v2::Tag* tag = mpegFile.ID3v2Tag();
                if (NULL != tag) {
                    if (!ptr->m_Flag) {
                        return;
                    }
                    TagLib::ID3v2::FrameList framelist = tag->frameListMap()["APIC"];
                    if (!framelist.isEmpty()) {
                        if (!ptr->m_Flag) {
                            return;
                        }
                        TagLib::ID3v2::Frame* frame = framelist.front();
                        if (NULL != frame) {
                            if (!ptr->m_Flag) {
                                return;
                            }
                            TagLib::ID3v2::AttachedPictureFrame* attachedpictureframe = reinterpret_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);
                            if ((NULL != attachedpictureframe)
                                    && (0 != frame->toString().size())
                                    && !(attachedpictureframe->picture().isNull())) {
                                if (!ptr->m_Flag) {
                                    return;
                                }
                                //这个地方提取封面有问题，暂不显示MP3封面
                                //return;
                                QImage image;
                                qDebug()<< "++++++++++picture size: "<< attachedpictureframe->picture().size();
                                if(!attachedpictureframe->picture().data())
                                {
                                    qDebug() << "attachedpictureframe picture data NULL!";
                                    return;
                                }

                                if (image.loadFromData(QByteArray::fromRawData(attachedpictureframe->picture().data(), attachedpictureframe->picture().size()))) {
                                    if (!ptr->m_Flag) {
                                        return;
                                    }
                                    if(image.format() == QImage::Format_Indexed8)
                                    {
                                        image = image.convertToFormat(QImage::Format_RGB32);
                                    }
                                    QImage tmp(416, 416, image.format());
                                    QPainter painter(&tmp);
                                    painter.drawImage(QRect(0, 0, 416, 416), image);
                                    if (!ptr->m_Flag) {
                                        return;
                                    }
                                    ptr->m_Mutex.lock();
                                    ptr->m_ConverImage = tmp;
                                    ptr->m_Mutex.unlock();
                                    if (!ptr->m_Flag) {
                                        return;
                                    }
                                    ptr->setImageSlot(tmp);
                                }
                            }
                        }
                    }
                }
            }
        } else if (fileInfo.suffix().contains(QString("flac"), Qt::CaseInsensitive)) {
            if (!ptr->m_Flag) {
                return;
            }
            TagLib::FLAC::File flacFile(fileName.toLocal8Bit().constData());
            if (flacFile.isValid()) {
                if (!ptr->m_Flag) {
                    return;
                }
                TagLib::List<TagLib::FLAC::Picture*> list = flacFile.pictureList();
                if (!list.isEmpty()) {
                    if (!ptr->m_Flag) {
                        return;
                    }
                    TagLib::FLAC::Picture* picture = list.front();
                    if ((NULL != picture)
                            && (!picture->data().isEmpty())) {
                        if (!ptr->m_Flag) {
                            return;
                        }
                        QImage image;
                        if (image.loadFromData(QByteArray::fromRawData(picture->data().data(), picture->data().size()))) {
                            if (!ptr->m_Flag) {
                                return;
                            }
                            if(image.format() == QImage::Format_Indexed8)
                            {
                                image = image.convertToFormat(QImage::Format_RGB32);
                            }
                            QImage tmp(416 , 416, image.format());
                            QPainter painter(&tmp);
                            painter.drawImage(QRect(0, 0, 416, 416), image);
                            if (!ptr->m_Flag) {
                                return;
                            }
                            ptr->m_Mutex.lock();
                            ptr->m_ConverImage = tmp;
                            ptr->m_Mutex.unlock();
                            if (!ptr->m_Flag) {
                                return;
                            }
                            ptr->setImageSlot(tmp);
                        }
                    }
                }
            }
        }
    }
    if(ptr->m_ConverImage.isNull())
    {
        ptr->callQmlRefreshImg();
    }
    qDebug() << "readThread B";
}

ImageProvider* MusicInformationWidget::getImageProvider(){
    Q_D(MusicInformationWidget);
    return d->m_pImgProvider;
}

MusicInformationWidget::MusicInformationWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new MusicInformationWidgetPrivate(this))
{

}

void MusicInformationWidget::onMusicPlayerID3TagChange(const int type, const int index, const QString &fileName, const QString& title, const QString& artist, const QString& album, const int endTime)
{
    Q_D(MusicInformationWidget);
   // qDebug()<<"+++++++++MusicInformationWidget::onMusicPlayerID3TagChange+++++++"<<type;
    if(type == 0)
    {
        d->m_PlayType = M_SdMusic;
    }
    else if(type == 1)
    {
        d->m_PlayType = M_UsbMusic;
    }
    if (d->m_FilePath != fileName) {
        d->m_FilePath = fileName;
        if (title.isEmpty()) {
            d->m_TitleText = QObject::tr(SourceString::Unknown.toLocal8Bit().constData());
        } else {
            d->m_TitleText = title;
            qDebug()<<"+++++[MusicInformationWidget::onMusicPlayerID3TagChange:]++++++++"<<d->m_TitleText;
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
        d->m_Flag = true;
        d->m_Mutex.lock();
        d->m_ConverImage = QImage();
        d->m_Mutex.unlock();
        if (NULL == d->m_CustomThread) {
            d->m_CustomThread = new CustomThread(this);
            d->m_CustomThread->setCallbackFunction(readThread, static_cast<void*>(d));
        }
        if (!d->m_ConverImage.isNull()) {
            d->setImageSlot(d->m_ConverImage);
        }
        if (d->m_CustomThread->isRunning()) {
            d->m_Flag = false;
            d->m_CustomThread->wait();
            d->m_Flag = true;
        }
        qDebug() << "MusicInformationWidget::onMusicPlayerID3TagChange  F"<<d->m_Flag;
        d->m_CustomThread->start(QThread::LowestPriority);
    }
   // d->setImageSlot(d->m_ConverImage);
}
bool MusicInformationWidget::isNullConverImage()
{
    Q_D(MusicInformationWidget);
    if(d->m_ConverImage.isNull()){
        return true;
    }
    else{
        return false;
    }
}

void MusicInformationWidget::onUsbMediaPlayExit()
{
    Q_D(MusicInformationWidget);
    if(d->m_PlayType == M_UsbMusic)
    {
        d->m_ConverImage = QImage();
        d->setImageSlot(d->m_ConverImage);
        d->m_PlayType = M_Undefile;
        d->m_FilePath.clear();
    }
}
void MusicInformationWidget::onSdMediaPlayExit()
{
    Q_D(MusicInformationWidget);
    if(d->m_PlayType == M_SdMusic)
    {
        d->m_ConverImage = QImage();
        d->setImageSlot(d->m_ConverImage);
        d->m_PlayType = M_Undefile;
        d->m_FilePath.clear();
    }
}


MusicInformationWidgetPrivate::MusicInformationWidgetPrivate(MusicInformationWidget *parent)
    : q_ptr(parent)
{
    m_CustomThread = NULL;
    m_pImgProvider = NULL;
    m_Flag = false;
    m_PlayType = M_Undefile;
    initializeImgProvider();
    connectAllSlots();
}
MusicInformationWidgetPrivate::~MusicInformationWidgetPrivate()
{

}
void MusicInformationWidgetPrivate::setImageSlot(const QImage &image)
{
    Q_Q(MusicInformationWidget);
    m_pImgProvider->setImageRc(image);
    emit q->callQmlRefreshImg();
}
void MusicInformationWidgetPrivate::callQmlRefreshImg()
{
    Q_Q(MusicInformationWidget);
    emit q->callQmlRefreshImg();
}
void MusicInformationWidgetPrivate::initializeImgProvider()
{
    Q_Q(MusicInformationWidget);
    if(m_pImgProvider == NULL)
    {
        m_pImgProvider = new ImageProvider();
    }
}

void MusicInformationWidgetPrivate::connectAllSlots()
{
    Q_Q(MusicInformationWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerID3TagChange(const int, const int, const QString &, const QString&, const QString&, const QString&, const int)));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onUsbMediaPlayExit()));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onSdMediaPlayExit()));
}






