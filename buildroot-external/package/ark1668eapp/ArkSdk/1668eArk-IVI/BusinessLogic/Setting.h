#ifndef SETTING_H
#define SETTING_H

#include <QObject>
#include <QGuiApplication>
#include <QTime>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <QSettings>
#include <QFile>
#define LOADERCOMPLETED 8
enum VolumeType{
    V_MediaVolume,
    V_NavigationVolume,
    V_TelPhoneVolume
};
enum LanguageType {
    LT_English = 0,
    LT_Chinese = 1,
    LT_TChinese = 2,
    LT_Japaness = 3,
    LT_Korean = 4,
    LT_Spanish = 5,
    LT_Portuguese = 6,
    LT_Russian = 7,
    LT_German = 8,
    LT_French = 9,
};
#define LanguageType int
class SettingPrivate;
class Setting : public QObject
{
    Q_OBJECT
#ifdef g_Setting
#undef g_Setting
#endif
#define g_Setting (Setting::instance())
public:
    inline static Setting* instance() {
        static Setting *setting(new Setting(qApp));
        return setting;
    }
    void settingDataTime();
    void setMediaVolume(int value);
    void setNavigationVolume(int value);
    void setTelPhoneVolume(int value);
    void setDisplayMode(int mode);
    int  getMediaVolume();
    int  getNavigationVolume();
    int  getTelPhoneVolume();
    void setVolumeType(int type);
    int  getVolumeType();
    void clearSsidList();//wifi名称列表
    int  wifiSsidListSize();//wifi名称列表数量
    void appendSsidWifiSsidList(QString ssid);//添加ssid到wifi Ssid列表
    QList<QString> getWifiSsidList();
    void setLanguage(const LanguageType language);
    void mySleep(int msec);
    void tmpDirHostapdIsExist();
    QString getHostapdSsid();
    int  executeShellCmd(const char* cmd);
    int  backstageExecuteShellCmd(const char* cmd);
    void setWifiOpenStatus(int status);
    int  getwifiOpenStatus();
    void setWlanApStatus(int status);
    int  getWlanApStatus();
    void setWifiConfig(int value);
    void setCarlifeTelephoneStatus(int status);
    int  getCarlifeTelephoneStatus();
    void setLoaderInterfaceCount();
    void loaderInterfaceCompleted();
signals:
    void onDataTimeSetting();
    void onWifiSsidListChanged();
    void onLanguageChanged();
    void onCarPlayConnected();
    void onCarPlayExit();
    void onWlanClose();
    void onWlanOpen();
    void onPhoneLinkTelePhone(int callType);
    void onLoaderCompleted();
private:
    explicit Setting(QObject *parent = nullptr);
    ~Setting();
    SettingPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(Setting)
};

#endif // SETTING_H
