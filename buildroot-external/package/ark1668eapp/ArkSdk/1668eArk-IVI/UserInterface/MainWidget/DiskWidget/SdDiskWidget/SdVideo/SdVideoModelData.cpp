#include "SdVideoModelData.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include "BusinessLogic/Setting.h"
#include <QDomDocument>
#include <QDebug>
class SdVideoModelDataPrivate
{
    Q_DISABLE_COPY(SdVideoModelDataPrivate)
public:
    explicit SdVideoModelDataPrivate(SdVideoModelData* parent);
    ~SdVideoModelDataPrivate();
    void connectAllSlots();
public:
    myModel* m_myModel;
    QList<QString> m_SdVideoList;
private:
    Q_DECLARE_PUBLIC(SdVideoModelData)
    SdVideoModelData* const q_ptr;
};

SdVideoModelData::SdVideoModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new SdVideoModelDataPrivate(this))
{
    QObject::connect(g_Setting,SIGNAL(onLanguageChanged()),this,SLOT(onLanguageChanged()));
}

void SdVideoModelData::onLanguageChanged()
{
    Q_D(SdVideoModelData);
    if(d->m_SdVideoList.size() == 0)
    {
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }
        QString data1 = QString(QObject::tr("SD卡没有视频文件"));
        myData data(data1);
        if(d->m_myModel != NULL)
        {
            d->m_myModel->Add(data);
        }
    }
}

myModel* SdVideoModelData::getObjectModel(){

    Q_D(SdVideoModelData);
    if(d->m_myModel == NULL)
    {
        d->m_myModel = new myModel();
    }
    return d->m_myModel;
}

void SdVideoModelData::onVideoPlayerFileNames(const int type, const QString& xml)
{
    Q_D(SdVideoModelData);
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
                            d->m_SdVideoList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            d->m_SdVideoList.append(node.toElement().text());
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
    if(d->m_SdVideoList.size() > 0)
    {
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }
        for(int i=0;i<d->m_SdVideoList.size();i++)
        {
             QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0'))+ d->m_SdVideoList.at(i);
             myData data(data1);
             if(d->m_myModel != NULL)
             {
                 d->m_myModel->Add(data);
             }
        }
    }
    else
    {
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }
        QString data1 = QString(QObject::tr("SD卡没有视频文件"));
        myData data(data1);
        if(d->m_myModel != NULL)
        {
            d->m_myModel->Add(data);
        }
    }
}

SdVideoModelDataPrivate::SdVideoModelDataPrivate(SdVideoModelData *parent)
    : q_ptr(parent)
{
    m_myModel = NULL;
    connectAllSlots();
}
SdVideoModelDataPrivate::~SdVideoModelDataPrivate()
{

}
void SdVideoModelDataPrivate::connectAllSlots()
{
    Q_Q(SdVideoModelData);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerFileNames(const int, const QString&)));

}

