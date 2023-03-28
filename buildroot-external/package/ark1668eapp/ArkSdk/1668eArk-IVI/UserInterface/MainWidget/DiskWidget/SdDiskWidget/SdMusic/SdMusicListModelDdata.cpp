#include "SdMusicListModelDdata.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include "./BusinessLogic/Setting.h"
#include <QDomDocument>
#include <QDebug>
class SdMusicListModelDdataPrivate
{
    Q_DISABLE_COPY(SdMusicListModelDdataPrivate)
public:
    explicit SdMusicListModelDdataPrivate(SdMusicListModelDdata* parent);
    ~SdMusicListModelDdataPrivate();
    void connectAllSlots();
public:
    QList<QString> m_SdMusicList;
    UsbMusicListModel* m_SdMusicListModel;
private:
    Q_DECLARE_PUBLIC(SdMusicListModelDdata)
    SdMusicListModelDdata* const q_ptr;
};

SdMusicListModelDdata::SdMusicListModelDdata(QObject *parent) :
    QObject(parent),
    d_ptr(new SdMusicListModelDdataPrivate(this))
{
    QObject::connect(g_Setting,SIGNAL(onLanguageChanged()),this,SLOT(onLanguageChanged()));
}

void SdMusicListModelDdata::onLanguageChanged()
{
    Q_D(SdMusicListModelDdata);
    if(d->m_SdMusicList.size() == 0){
        if(d->m_SdMusicListModel != NULL)
        {
            d->m_SdMusicListModel->clear();
        }
        QString data1 = QString(QObject::tr("SD卡没有音频文件"));
        QString data2 = "";
        UsbMusicListModeData data(data1,data2);
        if(d->m_SdMusicListModel != NULL)
        {
            d->m_SdMusicListModel->Add(data);
        }
    }
}

UsbMusicListModel* SdMusicListModelDdata::getObjectModel()
{
    Q_D(SdMusicListModelDdata);
    if(d->m_SdMusicListModel == NULL)
    {
        d->m_SdMusicListModel = new UsbMusicListModel();
    }
    return d->m_SdMusicListModel;
}

void SdMusicListModelDdata::onMusicPlayerFileArtist(const DeviceWatcherType type, QStringList fileArtist)
{
    Q_D(SdMusicListModelDdata);
    qDebug()<<"======[SdMusicListModelDdata::onMusicPlayerFileArtist]====="<<d->m_SdMusicList.size();
    if(DWT_SDDisk == type)
    {
        if(d->m_SdMusicList.size()>0)
        {
            if(d->m_SdMusicListModel != NULL)
            {
                d->m_SdMusicListModel->clear();
            }
            qDebug()<<"======[SdMusicListModelDdata::onMusicPlayerFileArtist:fileArtist.size()]====="<<fileArtist.size();
            for(int i = 0; i<d->m_SdMusicList.size();i++)
            {
                if(fileArtist.size() > i)
                {
                    QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0')) + d->m_SdMusicList.at(i);
                    QString data2 = fileArtist.at(i);
                    if(data2.size() == 0)
                    {
                        data2 = QString("Unknown");
                    }
                    UsbMusicListModeData data(data1,data2);
                    if(d->m_SdMusicListModel != NULL)
                    {
                        d->m_SdMusicListModel->Add(data);
                    }
                }
            }
            qDebug()<<"++++++m_SdMusicListModel->size++++++++"<<d->m_SdMusicListModel->rowCount();
        }
        else{
            if(d->m_SdMusicListModel != NULL)
            {
                d->m_SdMusicListModel->clear();
            }
            QString data1 = QString(QObject::tr("SD卡没有音频文件"));
            QString data2 = "";
            UsbMusicListModeData data(data1,data2);
            if(d->m_SdMusicListModel != NULL)
            {
                d->m_SdMusicListModel->Add(data);
            }
        }
    }

}


void SdMusicListModelDdata::onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml)
{
    qDebug()<<"======[SdMusicListModelDdata::onMusicPlayerFileNames]====="<<type;
    Q_D(SdMusicListModelDdata);
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

                        }
                    }
                }
                node = node.nextSibling();
            }
        }
    }
    qDebug()<<"======[SdMusicListModelDdata::onMusicPlayerFileNames0000:]====="<<d->m_SdMusicList.size();
}

SdMusicListModelDdataPrivate::SdMusicListModelDdataPrivate(SdMusicListModelDdata *parent)
    : q_ptr(parent)
{
    m_SdMusicListModel =NULL;
    connectAllSlots();
}
SdMusicListModelDdataPrivate::~SdMusicListModelDdataPrivate()
{

}
void SdMusicListModelDdataPrivate::connectAllSlots()
{
    Q_Q(SdMusicListModelDdata);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerFileNames(const int, const QString &)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerFileArtist(int,QStringList)));
}
