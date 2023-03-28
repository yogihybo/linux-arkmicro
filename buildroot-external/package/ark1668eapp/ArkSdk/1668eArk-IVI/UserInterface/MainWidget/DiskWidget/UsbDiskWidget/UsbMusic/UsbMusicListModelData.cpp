#include "UsbMusicListModelData.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include <QDomDocument>
#include <QDebug>
class UsbMusicListModelDataPrivate
{
    Q_DISABLE_COPY(UsbMusicListModelDataPrivate)
public:
    explicit UsbMusicListModelDataPrivate(UsbMusicListModelData* parent);
    ~UsbMusicListModelDataPrivate();
    void connectAllSlots();
public:
    QList<QString> m_UsbMusicList;
    UsbMusicListModel* m_UsbMusicListModel;
    bool    m_LanguageFlag;
private:
    Q_DECLARE_PUBLIC(UsbMusicListModelData)
    UsbMusicListModelData* const q_ptr;
};

UsbMusicListModelData::UsbMusicListModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new UsbMusicListModelDataPrivate(this))
{
    QObject::connect(g_Setting,SIGNAL(onLanguageChanged()),this,SLOT(onLanguageChanged()));
}
void UsbMusicListModelData::onLanguageChanged()
{
    Q_D(UsbMusicListModelData);
    if(d->m_UsbMusicList.size() == 0){
        if(d->m_UsbMusicListModel != NULL)
        {
            d->m_UsbMusicListModel->clear();
        }
        QString data1 = QString(QObject::tr("USB没有音频文件"));
        QString data2 = "";
        UsbMusicListModeData data(data1,data2);
        if(d->m_UsbMusicListModel != NULL)
        {
            d->m_UsbMusicListModel->Add(data);
        }
    }
}

UsbMusicListModel* UsbMusicListModelData::getObjectModel()
{
    Q_D(UsbMusicListModelData);
    if(d->m_UsbMusicListModel == NULL)
    {
        d->m_UsbMusicListModel = new UsbMusicListModel();
    }
    return d->m_UsbMusicListModel;
}

void UsbMusicListModelData::onMusicPlayerFileArtist(const DeviceWatcherType type, QStringList fileArtist)
{
    Q_D(UsbMusicListModelData);
    qDebug()<<"++++++++onMusicPlayerFileArtist00000++++++++++"<<d->m_UsbMusicList.size();
    if(DWT_USBDisk == type)
    {
        if(d->m_UsbMusicList.size() > 0)
        {
            if(d->m_UsbMusicListModel != NULL)
            {
                d->m_UsbMusicListModel->clear();
            }
            qDebug()<<"+++++++fileArtist.size()++++++"<<fileArtist.size();
            for(int i = 0; i<d->m_UsbMusicList.size();i++)
            {
                if(fileArtist.size() > i)
                {
                    QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0')) + d->m_UsbMusicList.at(i);
                    QString data2 = fileArtist.at(i);
                    if(data2.size() == 0)
                    {
                        data2 = QString("Unknown");
                    }
                    UsbMusicListModeData data(data1,data2);
                    if(d->m_UsbMusicListModel != NULL)
                    {
                        d->m_UsbMusicListModel->Add(data);
                    }
                }
            }
        }
        else{
            if(d->m_UsbMusicListModel != NULL)
            {
                d->m_UsbMusicListModel->clear();
            }
            QString data1 = QString(QObject::tr("USB没有音频文件"));
            QString data2 = "";
            UsbMusicListModeData data(data1,data2);
            if(d->m_UsbMusicListModel != NULL)
            {
                d->m_UsbMusicListModel->Add(data);
            }
        }
    }
}


void UsbMusicListModelData::onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml)
{
    Q_D(UsbMusicListModelData);
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
                        }
                    }
                }
                node = node.nextSibling();
            }
        }
    }
}

UsbMusicListModelDataPrivate::UsbMusicListModelDataPrivate(UsbMusicListModelData *parent)
    : q_ptr(parent)
{
    m_UsbMusicListModel =NULL;
    connectAllSlots();
}
UsbMusicListModelDataPrivate::~UsbMusicListModelDataPrivate()
{

}
void UsbMusicListModelDataPrivate::connectAllSlots()
{
    Q_Q(UsbMusicListModelData);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerFileNames(const int, const QString &)));
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onMusicPlayerFileArtist(int,QStringList)));
}
