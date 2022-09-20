#ifndef IUSERLINKPLAYER_H
#define IUSERLINKPLAYER_H

#ifdef __cplusplus
#include "VideoDecoder.h"
#include "Thread.h"
#include "LinkBase.h"
#include <functional>
using namespace std;

typedef std::function<void (ConnectedStatus, PhoneType)> FUNCCONNECTCALLBACK;
typedef std::function<void (AppStatusMessage, void*)> FUNCAPPSTATUSCALLBACK;
typedef std::function<void (CallType, char*, char*)> FUNCBLUETOOTHPHONECALLBACK;

class CarlifeLink;
class CarplayLink;
class AutoLink;
class EasyConnectLink;
class MirrorLink;
class HiCarLink;
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
    void Release(); //release link player

    bool Start();   
    bool Stop();    //stop link

    bool StartMirror();
    bool StopMirror();

    bool SetBackground();                                                                           //set link background run
    bool SetForeground();                                                                           //set link foreground run

    void SetInserted(bool inserted, PhoneType phoneType);

    void SendScreenSize(int width, int height);
    void SendTouch(int x, int y, TouchCode touchCode);
    void SendMultiTouch(int x1, int y1, TouchCode touchCode1,int x2, int y2, TouchCode touchCode2);
    void SendKey(KeyCode keyCode);
    void SendWheel(WheelCode wheelCode, bool bFoucs);
    bool SendNightMode(bool night);
    bool SendRightHandDriver(bool right);

    void GetIniConfig(LinkAssist *pLinkAssist);
    void RegisterConnectCallback(FUNCCONNECTCALLBACK funcConnectCallback);
    void RegisterAppStatusCallback(FUNCAPPSTATUSCALLBACK funcAppStatusCallback);
    void RegisterBlueToothPhoneCallback(FUNCBLUETOOTHPHONECALLBACK funcBlueToothPhoneCallback);
    bool RequestStatus(RequestAppStatus requestAppStatus, void *reserved = nullptr);
    void onSdkConnectStatus(ConnectedStatus status, PhoneType type);

    bool OpenPage(AppPage appPage);   
    void SendCarBluetooth(const string& name, const string& address, const string& pin);
    void SendPhoneBluetooth(const string& address);
    bool SendIphoneMacAddress(string address);
    void SendCarWifi(WifiInfo& info);

    void SendLisenceCode(const string& license);
    void SendInputText(const string& text);
    void SendInputSelection(int start, int stop);
    void SendInputAction(int action, int keyCode);

    void SendBlueToothCmd(const string& cmd);
    void SendBroadcast(bool enable);
    void SendDelayRecord(int millisecond);
    void SendWifiStateChanged(WifiStateAction action, WifiState state, const string& phoneIp, const string& carIp);
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
   FUNCBLUETOOTHPHONECALLBACK mFuncBlueToothPhoneCallback;

   VideoFrame       mVideoFrame;
   LinkConfig       mLinkConfig;
   CarplayConfig    mCarplayConfig;
   CarlifeConfig    mCarlifeConfig;
   Mutex            mMutexVideo;
   bool             mConnected;
   bool             mVideoStart;
private:
    IUserLinkPlayer *mpIULPlayer;
    AudioDecoder    *mpMusicDecoder;
    AudioDecoder    *mpTTSDecoder;
    AudioDecoder    *mpVRDecoder;
    AudioDecoder    *mpCallDecoder;

};
#endif
#endif // IUSERLINKPLAYER_H
