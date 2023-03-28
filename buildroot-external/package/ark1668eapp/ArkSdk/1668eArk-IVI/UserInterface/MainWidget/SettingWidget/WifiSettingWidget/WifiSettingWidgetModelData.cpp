#include "WifiSettingWidgetModelData.h"
#include "AutoConnect.h"
#include <QDebug>
WifiSettingWidgetModelData::WifiSettingWidgetModelData(QObject *parent) : QObject(parent)
{
    m_myModel  = NULL;
    connectSignalAndSlotByNamesake(g_Setting, this, ARKRECEIVER(onWifiSsidListChanged()));
}

myModel* WifiSettingWidgetModelData::objectModel()
{
    if(m_myModel == NULL)
    {
        m_myModel = new myModel();
    }
    return m_myModel;
}


void WifiSettingWidgetModelData::onWifiSsidListChanged(){
    m_myModel->clear();
    QList<QString> _WifiSsidList = g_Setting->getWifiSsidList();
    if(_WifiSsidList.size() != 0)
    {
        for(int i= 0;i<_WifiSsidList.size();i++)
        {
            QString data = _WifiSsidList.at(i);
            myData d(data);
            m_myModel->Add(d);
        }
    }
}
