#ifndef WIFISETTINGWIDGET_H
#define WIFISETTINGWIDGET_H

#include <QObject>
#include "BusinessLogic/WiFiManager.h"
class WifiSettingWidgetPrivate;
class WifiSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit WifiSettingWidget(QObject *parent = nullptr);
    void setWifiSettingWidgetObject(QObject* qmlObject);
protected slots:
    void onWifiGetResultFail();
    void onWifiSSIDInfoChange(char* ssid);
    void onAddKeyBoardInputStr(QString str);
    void onSubKeyBoardInputStr(QString str);
    void onWifiConnectStatusChange(const int status);
    void onAutoConncetSsid(QString ssid);
    void onWlanClose();
    void onWlanOpen();
    void onReInputPassword();
public slots:
    void onToolButtonRelease();
    void onTimerOut();
    void onListviewItemClicked(int index);
private:
    WifiSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(WifiSettingWidget)
};

#endif // WIFISETTINGWIDGET_H
