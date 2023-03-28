#include "UsbListModelData.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include <QDomDocument>
#include <QDebug>

UsbListModelData::UsbListModelData(QObject *parent) : QObject(parent)
{
    m_Elapsed = 0;
    m_CurrentIndex = 0;
    m_LastIndex = 0;
    m_LastUsbMusicList.clear();
    m_UsbMusicListModel = NULL;
    connectSignalAndSlotByNamesake(g_Multimedia, this, ARKRECEIVER(onMusicPlayerFileNames(const int, const QString &)));
    connectSignalAndSlotByNamesake(g_Multimedia, this, ARKRECEIVER(onMusicPlayerFileArtist(QStringList)));
}
UsbMusicListModel* UsbListModelData::getObjectModel()
{
    if(m_UsbMusicListModel == NULL)
    {
        m_UsbMusicListModel = new UsbMusicListModel();
        if(m_UsbMusicList.size() != m_LastUsbMusicList.size())
        {
            m_UsbMusicListModel->clear();
            for(int i = 0; i<m_UsbMusicList.size();i++)
            {
                if(m_UsbMusicFileArtistList.size() > i)
                {
                    QByteArray array = m_UsbMusicList.at(i).toLocal8Bit();
                    QString qStr = QString::fromLocal8Bit(array.data());
                    QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0'))+ qStr ;
                    QString data2 = m_UsbMusicFileArtistList.at(i);
                    if(data2.size() == 0)
                    {
                        data2 = QString("Unknown");
                    }
                    UsbMusicListModeData d(data1,data2);
                    m_UsbMusicListModel->Add(d);
                }

            }
            m_LastUsbMusicList = m_UsbMusicList;
        }
    }
    return m_UsbMusicListModel;
}

void UsbListModelData::onMusicPlayerFileArtist(QStringList fileArtist)
{
    m_UsbMusicFileArtistList.clear();
    m_UsbMusicFileArtistList = fileArtist;
    if(m_UsbMusicList.size() != m_LastUsbMusicList.size())
    {
        m_UsbMusicListModel->clear();
        for(int i = 0; i<m_UsbMusicList.size();i++)
        {
            if(m_UsbMusicFileArtistList.size() > i)
            {
                QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0')) + m_UsbMusicList.at(i);
                QString data2 = m_UsbMusicFileArtistList.at(i);
                if(data2.size() == 0)
                {
                    data2 = QString("Unknown");
                }
                UsbMusicListModeData d(data1,data2);
                m_UsbMusicListModel->Add(d);
            }
        }
        m_LastUsbMusicList = m_UsbMusicList;
    }
}
void UsbListModelData::onMusicPlayerFileNames(const DeviceWatcherType type, const QString &xml)
{
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
                            m_UsbMusicList.clear();
                            for (int i = 0; i < nodeList.size(); ++i) {
                                QDomNode node = nodeList.at(i);
                                if (node.isElement()) {
                                    if (!node.toElement().isNull()) {
                                        if (node.isElement()) {
                                            QFileInfo fileInfo(node.toElement().text());
                                            m_UsbMusicList.append(fileInfo.fileName());
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
                                            m_Elapsed = QString(node.toElement().text().split(QChar('-')).at(1)).toInt();
                                            if((m_UsbMusicList.size()-1) > index)
                                            {
                                                m_CurrentIndex = index;
                                                m_LastIndex = index;
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
