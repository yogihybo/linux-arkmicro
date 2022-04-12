#ifndef CARLIFE_H
#define CARLIFE_H

#include <semaphore.h>
//#include <QTimer>
#include "timer.h"
#include<string>
#include "libusb.h"

using std::string;
class BlueToothControl;
enum PhoneOS
{
    Unknown = 0,
    Android = 1,
    IOS = 2,
    WIRELESS = 3,
};

enum ScreenAction
{
    ScreenPress,
    ScreenRelease,
    ScreenMove,
    ScreenSingleClick,
    ScreenDoubleClick,
    ScreenLongPress,
};

typedef enum
{
    FAILED = -1,
    FAILED_EAP = -2,
    FAILED_UNSTART = -3,
}FAILED_STATUS;

//#define CARLIFE_ADB  1
//#undef CARLIFE_ADB

#define EAP_SIZE 100

class CarLife
{

public:
    explicit CarLife();
    virtual ~CarLife();

public:

    int AdbInitialize();
    int EapInitialize();
    int AOAInitialize(int idVendor);

    int SendHuProtocolVer();
    int SendHuInfo();
    int SendVideoInit();

    void VideoPause();
    void VideoStart();

    void VideoChangeRate(int fps);

    bool Authentication();
    int Install(int idVendor, char* pAddress = NULL);
    bool Connect(Timer* pTimer, char* pAddress = NULL);
    bool ChannelsStart(char* pAddress = NULL);
    //主动退出视频解码并且mute掉声音
    bool UnInitDecode();
    //释放了解码后第二次进入
    bool InitDecode();

    //mute掉所有声音,包括VR,TTS,Media和输入的Micphone
    bool AudioMute(bool bMute);

    void DecodeEntry(bool bEntry);

    void DecoderExit(bool bExit);
    //打开显示层
    bool OpenVideoLayer();
    //关闭显示层
    bool CloseVideoLayer();

    void ShowPlayer(bool visible);

    bool IsVideoLayerVisible();

    bool touchAction(int x, int y, int screenwidth, int screenheight,ScreenAction act);
    bool touchKey(int key);

    void SetPhoneOS(PhoneOS os); //获取是什么手机类型


    void SetBTHFPInComming(char *pPhoneNumber);
    void SetBTHFPActive();
    void SetBTHFPInActive();
    void SetBTHFPOutGoing();

    void SetEapLink(bool bEapLink);
    void SetAoaLink(bool bAoaLink);
    void DisconnectAoa();

    void setMicAudioData(unsigned char *data, int length);


    //注册手机断开的回调函数注册
    void SetSocketDisconnectCallback(void (*callback)(void*), void *parameter);
    //
    void SetMicDisconnectCallback(void (*callback)(void*), void *parameter);
    //cmd状态回调函数注册
    void SetCmdStatusCallback(void (*callback)(int ,void*), void *parameter);

    void SetCmdPhoneNumberCallback(void (*callback)(string, void*), void *parameter);

    void SetCmdMediaInfoCallback(void (*callback)(string, string, string, void*), void *parameter);

    //车机端是否显示问题
    void SetVideoCallback(void (*callback)(int, int ,int ,void*), void *parameter);

    void SetVideoInfoCallback(bool bControlVideo, void (*callback)(int,int,unsigned char*, int, void*), void*parameter);

    void SetAudioInfoCallback(void (*callback)(int,unsigned char*, int, void*), void*parameter);

    void SetAudioStartCallback(void (*callback)(bool,int,int,int,int, void*), void*parameter);
    //手机拔掉
    static void disconnect_callback_func();

    //作为盒子端从车机获取屏幕大小
    void GetCarScreenSize(int screenwidth, int screenheight);

    void SendCarBluetooth(const string& name, const string& address, const string& pin);
    //
    static void disconnect_mic_callback_func();

    //断开失败的处理
    void DisconnectHandler(char* pAddress = NULL);
    //cmd消息回调到这里
    static void cmd_status_callback_func(int status, void* parameter);
    static void music_status_callback_func(int status, void* parameter);
    static void cmd_phonenumber_callback_func(string phonenumber, void* parameter);
    static void cmd_mediainfo_callback_func(string song, string artist, string album, void* parameter);
    //车机端显示回调
    static void video_callback_func(int ready, int width, int height, void* parameter);
    static void video_info_callback_func(int width, int height, unsigned char* data, int length, void*parameter);

    static void music_info_callback_func(unsigned char* data, int length, void* parameter);
    static void music_start_callback_func(bool start, int type, int rate, int bit, int channel, void *parameter);

    static void tts_info_callback_func(unsigned char* data, int length, void* parameter);
    static void tts_start_callback_func(bool start, int type, int rate, int bit, int channel, void *parameter);

    static void vr_info_callback_func(unsigned char* data, int length, void* parameter);
    static void vr_start_callback_func(bool start, int type, int rate, int bit, int channel, void *parameter);


private:
    //手机类型
    PhoneOS  m_PhoneOS;
    int      m_nScreenWidth;
    int      m_nScreenHeight;
    void*    m_parameter;
    void     (*m_socket_disconnect_callback)(void*);
    void     (*m_mic_disconnect_callback)(void*);
    void     (*m_cmd_status_callback)(int ,void*);

    void     (*m_music_status_callback)(int ,void*);
    void     (*m_cmd_phonenumber_callback)(string ,void*);
    void     (*m_cmd_mediainfo_callback)(string, string,string ,void*);
    void     (*m_vido_callback)(int, int, int,void*);
    void     (*m_video_info_callback)(int,int,unsigned char*, int,void*);
    void     (*m_audio_info_callback)(int,unsigned char*, int,void*);
    void     (*m_audio_start_callback)(bool,int,int,int,int,void*);

    bool m_bControlVideo;
    libusb_device *m_device;
    //视频准备好
    bool     m_ready;
    bool     m_bEapLink;
    bool     m_bAoaLink;

};

#endif // CARLIFE_H
