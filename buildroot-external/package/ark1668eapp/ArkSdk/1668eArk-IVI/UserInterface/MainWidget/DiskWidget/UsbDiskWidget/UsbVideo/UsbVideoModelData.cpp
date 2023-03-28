#include "UsbVideoModelData.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include "./BusinessLogic/Setting.h"
#include <QDomDocument>
#include <QDebug>
class UsbVideoModelDataPrivate
{
    Q_DISABLE_COPY(UsbVideoModelDataPrivate)
public:
    explicit UsbVideoModelDataPrivate(UsbVideoModelData* parent);
    ~UsbVideoModelDataPrivate();
    void connectAllSlots();
public:
    myModel* m_myModel;
    QList<QString> m_UsbVideoList;
private:
    Q_DECLARE_PUBLIC(UsbVideoModelData)
    UsbVideoModelData* const q_ptr;
};
UsbVideoModelData::UsbVideoModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new UsbVideoModelDataPrivate(this))
{
    QObject::connect(g_Setting,SIGNAL(onLanguageChanged()),this,SLOT(onLanguageChanged()));
}

void UsbVideoModelData::onLanguageChanged()
{
    Q_D(UsbVideoModelData);
    if(d->m_UsbVideoList.size() == 0)
    {
        if(d->m_myModel != NULL)
        {
           d->m_myModel->clear();
        }
        QString data1 = QString(QObject::tr("USB没有视频文件"));
        myData data(data1);
        if(d->m_myModel != NULL)
        {
           d->m_myModel->Add(data);
        }
    }
}

myModel* UsbVideoModelData::getObjectModel(){

    Q_D(UsbVideoModelData);
    if(d->m_myModel == NULL)
    {
        d->m_myModel = new myModel();
    }
    return d->m_myModel;
}
void UsbVideoModelData::onVideoPlayerFileNames(const int type, const QString& xml)
{
    Q_D(UsbVideoModelData);
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
                        }
                    }
                }
                node = node.nextSibling();
            }
        }
    }
    if(d->m_UsbVideoList.size() > 0)
    {
        if(d->m_myModel != NULL)
        {
           d->m_myModel->clear();
        }
        for(int i=0;i<d->m_UsbVideoList.size();i++)
        {
             QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0'))+ d->m_UsbVideoList.at(i);
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
        QString data1 = QString(QObject::tr("USB没有视频文件"));
        myData data(data1);
        if(d->m_myModel != NULL)
        {
            d->m_myModel->Add(data);
        }
    }
}

UsbVideoModelDataPrivate::UsbVideoModelDataPrivate(UsbVideoModelData *parent)
    : q_ptr(parent)
{
    m_myModel = NULL;
    connectAllSlots();
}
UsbVideoModelDataPrivate::~UsbVideoModelDataPrivate()
{

}
void UsbVideoModelDataPrivate::connectAllSlots()
{
    Q_Q(UsbVideoModelData);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onVideoPlayerFileNames(const int, const QString&)));

}
