#ifndef CARLIFEPLAYER_H
#define CARLIFEPLAYER_H

typedef enum
{
    CARPLAY = 0x00,
    CARLIFE= 0x01,
    ANDROID_MIRROR = 0x02,
    ANDROID_CARLIFE = 0x03,
    IOS_CARLIFE = 0x04,
    AUTO = 0x05,
    ECLINK = 0x06,
    CARPLAY_WIRELESS = 0x07,
    CARLIFE_WIRELESS = 0x08,
}Link_TYPE;
#include "carlife.h"
//#include <QFileSystemWatcher>
//#include <QTimer>
#include "timer.h"
//#include "ArkDbus.h"
class UsbManager;
class CarLife;

#define CARPLAY_PATH "/tmp/carplay"
#define CARLIFE_PATH "/tmp/carlife"

typedef std::function<void (int, int, unsigned char*, int, void*)> FUNCVIDEOINFO;
typedef std::function<void (bool, int, int,void*)> FUNCVIDEOSTART;
typedef std::function<void(bool, int,int,int,int,void*)> FUNCAUDIOSTART;
typedef std::function<void(int, unsigned char*, int,void*)> FUNCAUDIOINFO;
typedef std::function<void(int,void*)> FUNCSTATUS;
typedef std::function<void(string,void*)> FUNCPHONENUMBER;

class CarlifePlayer
{

public:
    explicit CarlifePlayer();
    virtual ~CarlifePlayer();


    bool Initialize();

    void UnInitialize();

    bool SetPlayForeground(bool bForeground);//前后台切换

    bool SetPlayMute(bool bMute);

    void SetLinkType(int type);

    int StartLink();
    //主动退出解码库
    bool ExitLink();

    //失败的处理
    bool HandleFailed(int error);

    //退出进程
    bool ExitProcess();

    //切源后重新解码库
    bool RestartLink();

    bool Authentication();

    void InitCarlife();

    void VideoChangeRate(int fps);

    void VideoPause();

    void VideoStart();

    void VideoDisplay(bool bDisplay);

    void Touch(int x, int y, int screenwidth, int screenheight, ScreenAction act);

    void Key(int key);

   // void SetWireless(QString str);
    void SetVideoStartCallback(FUNCVIDEOSTART funcVideoStart, void* parameter);

    void SetVideoInfoCallback(bool bControlVideo, FUNCVIDEOINFO funcVideoInfo, void* parameter);

    void SetAudioStartCallback(FUNCAUDIOSTART funcAudioStart, void* parameter);

    void SetAudioInfoCallback(FUNCAUDIOINFO funcAudioInfo, void* parameter);

    void SetLinkstatusCallback(FUNCSTATUS funcStatus, void* parameter);

    void SetCmdPhoneNumberCallback(FUNCPHONENUMBER funcPhoneNumber, void *parameter);

    void SetCmdMediaInfoCallback(void (*callback)(string, string, string, void*), void *parameter);

    void SetInsertPath(char *pBuffer);


    void setMicAudioData(unsigned char *data, int length);
    //是否准备好可以连接
    bool GetReadyLink() const {
        return m_bConnectable;
    }

    bool GetInserted() const {
        return m_bInserted;
    }

    bool GetConnected() const {
        return m_bConnected;
    }

    //主要是设置carlife拔出没有状态给出
    void SetInserted(bool bInsert);

    //检测手机端拔出状态
    bool GetRemoved();


    //获取用户设置的手机类型
    PhoneOS GetUserPhoneOS() const {
        return m_PhoneOS;
    }

    //获取插入手机的类型
    PhoneOS GetPhoneOS() const {
        return m_nPassive;
    }
    //用户主动去设置
    void SetUserPhoneOS(PhoneOS os);

    bool IsCarplayLinked();

    //车机端的蓝牙信息
    void SetCarBTInfo(int status);

    //作为盒子端从车机获取屏幕大小
    void GetCarScreenSize(int screenwidth, int screenheight);

    void SendCarBluetooth(const string& name, const string& address, const string& pin);
    //
    void SetBTHFPInComming(char *pPhoneNumber);
    void SetBTHFPActive();
    void SetBTHFPInActive();
    void SetBTHFPOutGoing();

    void SetEapLink(bool bEapLink);
    void SetAoaLink(bool bAoaLink);
    void SetidVendor(int idVendor);
    void SetIPAddress(char *pAddress);
    void SetTrustMode(bool bTrust);
    void SetOpenCarlife(bool bOpen);
    void SetInvokedExit(bool Invoked){
        m_bInvokedExit = Invoked;
    }

    char *GetIPAddress() const {return m_pAddress;}
protected:
    int StartLink(Timer* pTimer);
    //苹果手机切换device时，拔出是无法知道苹果手机时候还连着的.carplay并不知道的，所以创建/tmp/carlife文件来发给carplay
    //carplay点击的时候来判断时候可以进入carplay
    void Exited();

    void Linked();
private:    
    static void usb_callback_func(int online, int type, int vid,void* parameter);
    static void socket_disconnect_callback_func(void* parameter);
    static void mic_disconnect_callback_func(void* parameter);
    static void cmd_status_callback_func(int status, void* parameter);
    static void music_status_callback_func(int status, void* parameter);
    static void cmd_phonenumber_callback_func(string phonenumber, void* parameter);
    static void cmd_mediainfo_callback_func(string song, string artist, string album, void* parameter);
    static void vido_callback_func(int ready , int width, int height, void* parameter);
    static void video_info_callback_func(int width, int height, unsigned char* data, int length, void*parameter);
    static void audio_info_callback_func(int type, unsigned char* data, int length, void*parameter);
    static void audio_start_callback_func(bool start, int type, int rate, int bit, int channel, void*parameter);
    static void* StartLinkThread(void *param);
private:
    bool        m_bExitProcess;
    UsbManager *m_pUsbManger;
    CarLife    *m_pCarlife;
    //被动插入的手机类型
    PhoneOS     m_nPassive;
    //主动发起的手机类型
    PhoneOS     m_PhoneOS;
    //已检测到的
    bool        m_bInserted;
    //初始化完成可以连接的
    bool        m_bConnectable;
    //连接上的状态
    bool        m_bConnected;

    int         m_idVendor;
    //检测手机端拔出状态
    //手机端在启动过程中会切一次OTG，会有一个断开再连接的过程，所以必须检测socket断开和usb未检测到才是拔出状态
    bool        m_bRemoved;

    void*       m_parameter;
    //命令状态
    FUNCSTATUS     mFuncStatus;

    FUNCPHONENUMBER mFuncPhoneNumber;

    void        (*m_cmd_mediainfo_callback)(string, string, string ,void*);


    FUNCVIDEOSTART mFuncVideoStart;
    FUNCVIDEOINFO  mFuncVideoInfo;
    FUNCAUDIOSTART mFuncAudioStart;
    FUNCAUDIOINFO  mFuncAudioInfo;

    bool        m_bCarBack;
    bool        m_bControlVideo;

    sem_t       m_SemInit;
    sem_t       m_SemRelease;

    bool        m_changeOTG;

    char     *m_pAddress;
    bool     m_bTrustMode;
    bool     m_bOpenCarlife;
    bool     m_bEapLink;
    bool     m_bAoaLink;

    bool     m_bMusicPlayState;

    bool     m_bExited;
    Timer*     m_OTGTimer;
    Timer*  m_InvokedExitTimer;
    bool     m_bInvokedExit;
    bool     m_bClickAPKAndStart;

};

#endif // CARLIFEPLAYER_H
