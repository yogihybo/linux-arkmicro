#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QObject>
#include "myModel/myModel.h"
#include "BusinessLogic/WiFiManager.h"
class StatusBar : public QObject
{
    Q_OBJECT
public:
    explicit StatusBar(QObject *parent = nullptr);
    Q_INVOKABLE myModel* objectModel();
public slots:
    void onConnectStatusChange(const int status);
    void onDeviceWatcherStatus(const int type, const int status);
    void onWifiConnectStatusChange(const int status);
private:
    void connectAllSlots();
    void setModelData();
private:
    myModel* m_myModel;
    bool m_SdExist;
    bool m_UsbExist;
    bool m_BtConnect;
    bool m_WifiConnect;
};

#endif // STATUSBAR_H
