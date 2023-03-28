#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <QObject>
#include <QScopedPointer>
#include <QGuiApplication>
#include <QMap>
#include <QProcess>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
struct RemoteDeviceInfo {
    int index;
    int rssi;
    int deviceType;//(0)BR/EDR address(1)LE public address(2)LE random address(3)iOS device with Carplay support
    QString macAddress;
    QString name;
    QString deviceClass;
};
struct CallLogInfo {
    int callType;
    QString name;
    QString phoneNumber;
    QString data;
    QString time;
};
struct PhoneBookInfo {
    QString head;
    QString name;
    QString phone;
};
enum BluetoothRecordType {
    BRT_Undefine = -1,
    BRT_PhoneBook,
    BRT_Outgoing,
    BRT_Incoming,
    BRT_Missed,
    BRT_AllCall,
};
enum BluetoothVoiceMode {
    BVM_Undefine = -1,
    BVM_VoicePhone,
    BVM_VoiceBluetooth,
};

enum BluetoothPowerStatus {
    undefined = -1,
    BPS_PowerOff,
    BPS_PowerOn,
};
enum BluetoothPairedMode {
    BPM_Undefine = -1,
    BPM_PairedMode,
    BPM_CancelPairedMode,
};

enum BluetoothAutoConnect {
    BAC_Undefine = -1,
    BAC_AutoConnect,
    BAC_CancelAutoConnect,
};

enum BluetoothAutoAnswer {
    BAA_Undefine = -1,
    BAA_AutoAnswer,
    BAA_CancelAutoAnswer,
};

class BluetoothPrivate;
class Bluetooth
        : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Bluetooth)
#ifdef g_Bluetooth
#undef g_Bluetooth
#endif
#define g_Bluetooth (Bluetooth::instance())
public:
    inline static Bluetooth* instance() {
        static Bluetooth* bluetooth(new Bluetooth(qApp));
        return bluetooth;
    }
    enum PlayStatus{
        BtMusic_Stopped,
        BtMusic_Playing,
        BtMusic_Paused,
        BtMusic_FastForwarding,
        BtMusic_FastRewinding,
    };
    enum BluetoothConnectStatus {
        BCS_Undefine = -1,
        BCS_Unsupported,
        BCS_Idle,
        BCS_Connecting,
        BCS_Connected,
        BCS_Outgoing,
        BCS_Incoming,
        BCS_Talking,
        BCS_ActiveHeld,
    };
    QString pinCode();
    QString localDeviceName();
    QString remoteDeviceName();
    QString remoteDeviceAddress();
    QList<struct RemoteDeviceInfo> getRemoteDeviceInfoList();
    void    readHFPCFGValue();
    int     connectStatus();
    const QList<struct PhoneBookInfo>& getRecordList();
    QList<struct CallLogInfo>& getCallLogInfo();
    const QMap<QString, QString>& getPairedList();
    void musicToggle();
    void musicPause();
    void musicStop();
    void musicPlay();
    void musicPrevious();
    void musicNext();
    void connectRemoteDevice(const unsigned short index);
    void reConnectLastDevice();
    void disconnectRemoteDevice();
    void deleteRemoteDevice(const unsigned short index);
    int  getSynchronizeType();
    void cancelSynchronize(BluetoothRecordType type);
    void synchronizePhoneBook();
    void synchronizeAllCallLog();
    void setPincode(const QString& pincode);
    void setLocalDeviceName(const QString& devicename);
    void dialPhone(const QString& phone);
    void dialNumber(const QString& number);
    BluetoothVoiceMode getVoiceMode();
    void voiceToggleSwitch();
    void redialLastPhone();
    void pickupPhone();
    void hanupPhone();
    void getCallingNumber();
    void asynchronousQueryStatus();
    int  getPowerStatus();
    void powerOn();
    void powerOff();
    int  getAutoConnectStatus();
    void autoConnectOn();
    void autoConnectOff();
    int  getAutoAnswer();
    void autoAnswerOn();
    void autoAnswerOff();
    int  getBTMusicStatus();
    int  Blutooth_startThread();
    void Send_AT_CMD(QString AT_STR);
    void ScanNearByRemoteDevice();
    void StopScanNearByRemoteDevice();
    void QueryPairedList();
    void BtMusicPlyaStatus(int status);
    void BtMusicInfo(int param);
    void BtMicMuteChanged(int mute);
    int  getBtMicMuteStatus();
    QString getRemoteBtAddress();
    void    sendHicarDataToblueware(QString atCmd,QString Data);
    QString getLocalMacAddress();
    QString getHicarBackLinkMac();
    QString getHicarBackLinkCmd();
    void    setMusicType(int musicType);
    void    disableBtMusic();
    void    connectBtMusic();
protected:
    void timerEvent(QTimerEvent* event);
signals:
    void onPinCodeChange(const QString& name);
    void onLocalDeviceNameChange(const QString& name);
    void onRemoteDeviceNameChange(const QString& name);
    void onCurrentRemoteAddrChange(const QString& addr);
    void onConnectStatusChange(const int status);
    void onMusicStatusChange(const QString& musicName, const int status);
    void onRecordCountChange(const BluetoothRecordType type, const unsigned count);
    void onDialInfo(const int type,const QString& phone);
    void onVoiceChange(const BluetoothVoiceMode mode);
    void onPowerChange(int mode);
    void onAutoConnectChange(const int mode);
    void onAutoAnswerChange(const  int mode);
    void onScanFinish();
    void onGetPairedListFinish();
    void onBtMusicID3InfoChange(QString titile,QString artist,QString album);
    void onBtMusicElapsedInfo(int elapsed,int EndTime);
    void onSyncPhoneBook();
    void onSyncAllCallLog();
    void onHangUpPhone();
private slots:
    void onHolderChange(const int oldHolder, const int newHolder);
    void onTimeout();
    void onMuteChange(const int mute);
    void BlutoothHandle(QString data);
private:
    explicit Bluetooth(QObject *parent = NULL);
    ~Bluetooth();
    void initializePrivate();
    BluetoothPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(Bluetooth)
};

#endif // BLUETOOTH_H

