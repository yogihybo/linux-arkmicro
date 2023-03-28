#include "UsbImageModelData.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include <QDomDocument>
#include <QDebug>
class UsbImageModelDataPrivate
{
    Q_DISABLE_COPY(UsbImageModelDataPrivate)
public:
    explicit UsbImageModelDataPrivate(UsbImageModelData* parent);
    ~UsbImageModelDataPrivate();
    void connectAllSlots();
public:
    myModel* m_myModel;
    QList<QString> m_UsbImageList;
private:
    Q_DECLARE_PUBLIC(UsbImageModelData)
    UsbImageModelData* const q_ptr;
};
UsbImageModelData::UsbImageModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new UsbImageModelDataPrivate(this))
{

}
myModel* UsbImageModelData::getObjectModel(){

    Q_D(UsbImageModelData);
    if(d->m_myModel == NULL)
    {
        d->m_myModel = new myModel();
    }
    return d->m_myModel;
}
void UsbImageModelData::onImagePlayerFileNames(const DeviceWatcherType type, const QString &xml)
{
    Q_D(UsbImageModelData);
    if (DWT_USBDisk == type) {
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
                        if (QString("USBFileNames") == element.tagName()) {
                            QDomNodeList nodeList = element.childNodes();
                            d->m_UsbImageList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            d->m_UsbImageList.append(node.toElement().text());
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
    if(d->m_UsbImageList.size() > 0)
    {
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }
        for(int i=0;i<d->m_UsbImageList.size();i++)
        {
             QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0'))+ d->m_UsbImageList.at(i);
             myData data(data1);
             if(d->m_myModel != NULL)
             {
                 d->m_myModel->Add(data);
             }
        }
    }
}

UsbImageModelDataPrivate::UsbImageModelDataPrivate(UsbImageModelData *parent)
    : q_ptr(parent)
{
    m_myModel = NULL;
    connectAllSlots();
}
UsbImageModelDataPrivate::~UsbImageModelDataPrivate()
{

}
void UsbImageModelDataPrivate::connectAllSlots()
{
    Q_Q(UsbImageModelData);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onImagePlayerFileNames(const int, const QString&)));
}
