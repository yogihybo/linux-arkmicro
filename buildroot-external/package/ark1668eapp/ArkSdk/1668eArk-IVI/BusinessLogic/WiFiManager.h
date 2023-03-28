#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QGuiApplication>
#include <QObject>
#include <QRect>
#include <QScopedPointer>
#define WIFIPASSWDMINLENGTH 8
#define FILEPATH "/etc/wpa_supplicant/wpa_supplicant.conf"
enum WiFiConnectStatus {
    WCS_Undefine = -1,
    WCS_Connectting,
    WCS_Success,
    WCS_Fail,
    WCS_DisConnect,
    WCS_IleaglPasswd,
};

enum HotSpotStauts {
    HSS_Undefine = -1,
    HSS_Off,
    HSS_On,
};

enum STAStauts {
    SS_Undefine = -1,
    SS_Off,
    SS_On,
};

struct Data
{
    int count;
    char ssid_buf[30][128];
};

class WiFiManagerPrivate;
class WiFiManager : public QObject
{
    Q_OBJECT
    //Q_DISABLE_COPY(WiFiManager)
#ifdef g_WiFiManager
#undef g_WiFiManager
#endif
#define g_WiFiManager (WiFiManager::instance())
public:
    inline static WiFiManager* instance() {
        static WiFiManager* wifi(new WiFiManager(qApp));
        return wifi;
    }
    //STA Mode
    void startSTAThread();
    void exitPthread();
    int  wifiScan(void);
    void wifiScanResult();
    int  wifiAddNetwork(void);
    int  wifiSetSsid(int id, char *sid_name);
    int  wifiSetPassword(int id, char *password);
    int  wifiEnableNetwork(int id);
    int  wifiSaveConfig(void);
    int  wifiGetGateway(char *gateway);
    int  checkSsidExist(QString filePath, QString ssid);
    int  disableNetWork(int id);
    int  removeNetWork(int id);//忽略网络，先断开然后删除
    int  getWifiStatus();
signals:
    void onWifiSSIDInfoChange(char *ssid_buf);
    void onWifiPskIlegal();
    void onWifiConnectStatusChange(const int status);
    void onSTAStatusChange();
    void onHotSpotStatusChange();
    void onAutoConncetSsid(QString ssid);
    void onWifiGetResultFail();
    void onWifiGetResultSuccess();
    void onWifiGetResultNoWifi();
    void onReInputPassword();
private slots:
   // void standardOutputWifiConnectSta();
private:
    explicit WiFiManager(QObject *parent = NULL);
    ~WiFiManager();
    void initializePrivate();
//    friend class WiFiManagerPrivate;
//    QScopedPointer<WiFiManagerPrivate> m_Private;
    WiFiManagerPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(WiFiManager)
};

#endif // WIFIMANAGER_H
