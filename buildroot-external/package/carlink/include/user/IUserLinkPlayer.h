#ifndef IUSERLINKPLAYER_H
#define IUSERLINKPLAYER_H

#include <string>
#include "VideoDecoder.h"
#include "Thread.h"
#include "LinkBase.h"
#include <functional>
using namespace std;

typedef std::function<void (ConnectedStatus, PhoneType)> FUNCCONNECTCALLBACK;
typedef std::function<void (AppStatusMessage, void*)> FUNCAPPSTATUSCALLBACK;

class CarlifeLink;
class CarplayLink;
class AutoLink;
class EasyConnectLink;
class MirrorLink;
class UsbHostService;
class AudioDecoder;
class LinkAssist;

class IUserLinkPlayer : public LinkBase
{
public:
    IUserLinkPlayer();
    virtual ~IUserLinkPlayer();

    static IUserLinkPlayer *getInstance();
public:
    //init link player
    bool Initialize(LinkMode linkMode, PhoneType phoneType);

    bool Start();
    //release link player
    //stop link
    bool Stop();

    void Release();

    bool StartMirror();

    bool StopMirror();

    //set link background run
    bool SetBackground();
    //set link foreground run
    bool SetForeground();

    void SendScreenSize(int width, int height);

    void SendTouch(int x, int y, TouchCode touchCode);

    void SendMultiTouch(int x1, int y1, TouchCode touchCode1,int x2, int y2, TouchCode touchCode2);

    void SendKey(KeyCode keyCode);

    void SendWheel(WheelCode wheelCode, bool bFoucs);

    bool RequestStatus(RequestAppStatus requestAppStatus, void *reserved = nullptr);

    void onSdkConnectStatus(ConnectedStatus status, PhoneType type);

    bool OpenPage(AppPage appPage);

    void GetIniConfig(LinkAssist *pLinkAssist);

    void RegisterConnectCallback(FUNCCONNECTCALLBACK funcConnectCallback);

    void RegisterAppStatusCallback(FUNCAPPSTATUSCALLBACK funcAppStatusCallback);

    void SendCarBluetooth(const string& name, const string& address, const string& pin);

    void SendPhoneBluetooth(const string& address);

    bool SendIphoneMacAddress(string address);

    void SendCarWifi(WifiInfo& info);

    void SendLisenceCode(const string& license);


protected:

    virtual void set_mac(string mac){}

    void video_play(const void *data, int32_t len);

    void video_start(int offset_x, int offset_y, int width, int height);

    void video_stop();

    void audio_play(AudioType audioType, const void* data, uint32_t len);

    bool audio_start(AudioType audioType, int rate, int bit, int channel);

    bool audio_stop(AudioType audioType);

    //init recorder devices
    void record_start(AudioInfo & audioInfo);

    //uninit recorder devices
    void record_stop();

    //pause or resume record
    void record_pause(bool pause);

    bool app_status(AppStatusMessage appStatusMessage, void *reserved = nullptr);

    bool bt_call_action(CallType callType, const char *name, const char* number);


protected:

   FUNCCONNECTCALLBACK  mFuncConnectCallback;
   FUNCAPPSTATUSCALLBACK mFuncAppStatusCallback;

   VideoFrame       mVideoFrame;
   LinkConfig       mLinkConfig;
   Semaphore        mSemaphore;
   bool             mConnected;

private:
    IUserLinkPlayer *mpIULPlayer;
    AudioDecoder    *mpMusicDecoder;
    AudioDecoder    *mpTTSDecoder;
    AudioDecoder    *mpVRDecoder;
    AudioDecoder    *mpCallDecoder;

};

#endif // IUSERLINKPLAYER_H
