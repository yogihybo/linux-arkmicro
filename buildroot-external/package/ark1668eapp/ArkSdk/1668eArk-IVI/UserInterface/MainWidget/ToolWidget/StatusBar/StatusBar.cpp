#include "StatusBar.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Multimedia.h"
#include "AutoConnect.h"
#include <QDebug>
StatusBar::StatusBar(QObject *parent) : QObject(parent)
{
    m_myModel = NULL;
    m_SdExist   = false;
    m_UsbExist  = false;
    m_BtConnect = false;
    m_WifiConnect = false;
    connectAllSlots();
}

myModel* StatusBar::objectModel()
{
    if(m_myModel == NULL)
    {
        m_myModel = new myModel();
    }
    return m_myModel;
}

void StatusBar::onConnectStatusChange(const int status){
    if(status == Bluetooth::BCS_Connected)
    {
        m_BtConnect = true;
    }
    else{
        m_BtConnect = false;
    }
    setModelData();
}

void StatusBar::setModelData(){

    if(m_myModel != NULL)
    {
        m_myModel->clear();
    }
    if(m_BtConnect)
    {
        QString data1 = QString("qrc:/images/HomeWidget/BtIcon.png");
        myData d1(data1);
        if(m_myModel != NULL)
        {
            m_myModel->Add(d1);
        }
    }
    if(m_SdExist){
        QString data2 = QString("qrc:/images/HomeWidget/SDIcon.png");
        myData d2(data2);
        if(m_myModel != NULL)
        {
            m_myModel->Add(d2);
        }
    }
    if(m_UsbExist){
        QString data3 = QString("qrc:/images/HomeWidget/UsbIcon.png");
        myData  d3(data3);
        if(m_myModel != NULL)
        {
            m_myModel->Add(d3);
        }
    }
    if(m_WifiConnect){
        QString data4 = QString("qrc:/images/HomeWidget/Wifi.png");
        myData  d4(data4);
        if(m_myModel != NULL)
        {
            m_myModel->Add(d4);
        }
    }
}

void StatusBar::onWifiConnectStatusChange(const int status){
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "#### WiFiConnectStatus: " << status;
    if(status == WCS_Success){
        m_WifiConnect = true;
    }
    else{
        m_WifiConnect = false;
    }
    setModelData();

}
void StatusBar::onDeviceWatcherStatus(const int type, const int status){
    qDebug()<<"++++[StatusBar::onDeviceWatcherStatus:type]++++"<<type;
    if (DWT_USBDisk == type) {
        switch (status) {
        case DWS_Empty: {
            break;
        }
        case DWS_Unsupport: {
            break;
        }
        case DWS_Busy: {

            break;
        }
        case DWS_Ready: {
            m_UsbExist = true;
            break;
        }
        case DWS_Remove: {
            m_UsbExist = false;
            break;
        }
        default: {
            break;
        }
        }
    }

    if (DWT_SDDisk == type) {
        switch (status) {
        case DWS_Empty: {
            break;
        }
        case DWS_Unsupport: {
            break;
        }
        case DWS_Busy: {

            break;
        }
        case DWS_Ready: {
            m_SdExist = true;
            break;
        }
        case DWS_Remove: {
            m_SdExist = false;
            break;
        }
        default: {
            break;
        }
        }
    }
    setModelData();
}

void StatusBar::connectAllSlots(){
    connectSignalAndSlotByNamesake(g_Bluetooth, this, ARKRECEIVER(onConnectStatusChange(const int)));
    connectSignalAndSlotByNamesake(g_Multimedia,this, ARKRECEIVER(onDeviceWatcherStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_WiFiManager,this, ARKRECEIVER(onWifiConnectStatusChange(const int)));
}
