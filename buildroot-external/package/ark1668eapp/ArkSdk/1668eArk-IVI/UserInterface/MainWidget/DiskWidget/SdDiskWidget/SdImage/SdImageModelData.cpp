#include "SdImageModelData.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include <QDomDocument>
#include <QDebug>
class SdImageModelDataPrivate
{
    Q_DISABLE_COPY(SdImageModelDataPrivate)
public:
    explicit SdImageModelDataPrivate(SdImageModelData* parent);
    ~SdImageModelDataPrivate();
    void connectAllSlots();
public:
    myModel* m_myModel;
    QList<QString> m_SdImageList;
private:
    Q_DECLARE_PUBLIC(SdImageModelData)
    SdImageModelData* const q_ptr;
};
SdImageModelData::SdImageModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new SdImageModelDataPrivate(this))
{

}
myModel* SdImageModelData::getObjectModel(){

    Q_D(SdImageModelData);
    if(d->m_myModel == NULL)
    {
        d->m_myModel = new myModel();
    }
    return d->m_myModel;
}
void SdImageModelData::onImagePlayerFileNames(const DeviceWatcherType type, const QString &xml)
{
    Q_D(SdImageModelData);
    if (DWT_SDDisk == type) {
        QDomDocument document;
        document.setContent(xml);
        QDomElement root = document.documentElement();
        if ((!root.isNull())
                && (root.isElement())
                && (QString("ImagePlayer") == root.toElement().tagName())
                && (root.hasChildNodes())) {
            QDomNode node = root.firstChild();
            while (!node.isNull()) {
                if (node.isElement()) {
                    QDomElement element = node.toElement();
                    if (!element.isNull()) {
                        if (QString("SDFileNames") == element.tagName()) {
                            QDomNodeList nodeList = element.childNodes();
                            d->m_SdImageList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            d->m_SdImageList.append(node.toElement().text());
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
    if(d->m_SdImageList.size() > 0)
    {
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }
        for(int i=0;i<d->m_SdImageList.size();i++)
        {
             QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0'))+ d->m_SdImageList.at(i);
             myData data(data1);
             if(d->m_myModel != NULL)
             {
                 d->m_myModel->Add(data);
             }
        }
    }
}

SdImageModelDataPrivate::SdImageModelDataPrivate(SdImageModelData *parent)
    : q_ptr(parent)
{
    m_myModel = NULL;
    connectAllSlots();
}
SdImageModelDataPrivate::~SdImageModelDataPrivate()
{

}
void SdImageModelDataPrivate::connectAllSlots()
{
    Q_Q(SdImageModelData);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onImagePlayerFileNames(const int, const QString&)));
}
