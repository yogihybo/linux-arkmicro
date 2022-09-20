#ifndef ECLINK_H
#define ECLINK_H

#include "IUserLinkPlayer.h"


#ifdef USE_EASYCONNECT
#include "AudioPlayer.h"
#include "AudioRecoder.h"
#include "VideoPlayer.h"
#include "ECSDKFramework.h"
#include "SDKListener.h"
#include "MirrorListener.h"
#include "ECSDKTouchKeyManager.h"
#include "APPListener.h"
#include "OTAListener.h"
#include "ToolKitListener.h"
#include "Thread.h"
#endif


class EasyConnectLink : public IUserLinkPlayer
{
public:
    EasyConnectLink();
    virtual ~EasyConnectLink();
#ifdef USE_EASYCONNECT
protected:
    virtual bool init(LinkMode linkMode);
    virtual bool release();
    virtual bool start();
    virtual bool stop();
    virtual bool start_mirror();
    virtual bool stop_mirror();
    virtual bool set_background();
    virtual bool set_foreground();
    virtual bool get_audio_focus();
    virtual bool release_audio_focus();
    virtual void set_inserted(bool inserted, PhoneType phoneType);
    virtual void send_screen_size(int width, int height);
    virtual void record_audio_callback(unsigned char *data, int len);
    virtual void send_car_bluetooth(const string& name, const string& address, const string& pin);
    virtual void send_phone_bluetooth(const string& address);
    virtual void send_car_wifi(WifiInfo& info){}
    virtual void send_touch(int x, int y, TouchCode touchCode);
    virtual void send_multi_touch(int x1, int y1, TouchCode touchCode1, int x2, int y2, TouchCode touchCode2){}
    virtual bool send_key(KeyCode keyCode);
    virtual bool send_wheel(WheelCode wheel, bool foucs);
    virtual bool send_night_mode(bool night);
    virtual bool send_right_hand_driver(bool right){}
    virtual bool open_page(AppPage appPage);
    virtual void request_status(RequestAppStatus requestAppStatus, void *reserved = nullptr);
    virtual void send_license(const string& license);
    virtual void send_input_text(const string& text);
    virtual void send_input_selection(const int start, const int stop);
    virtual void send_input_action(const int actionId, const int keyCode);
    virtual void send_bluetooth_cmd(const string& cmd ){}
    virtual void send_broadcast(bool enable){}
    virtual void send_delay_record(int millisecond){}
    virtual void send_wifi_state_changed(WifiStateAction action, WifiState state, const string& phoneIp, const string& carIp);
private:
    //EClinkPlayer *m_pEClinkPlayer;
    static void mirror_direction_callback(int direction, void* parameter);
    static void mirror_status_callback(int status, void* parameter);
    static void qr_callback_func(string qr, void* parameter);
    static void license_auth_success_func(int code, string msg, void* parameter);
    static void license_auth_failed_func(int code, string msg, void* parameter);

    void start_video_callback_func(bool start, int width, int height);
    void video_data_callback_func(unsigned char* data, int length);
    void start_audio_callback_func(bool start, int type, int rate, int bit, int channel);
    void audio_data_callback_func(int type, unsigned char* data, int length);
    void start_rec_callback_func(bool start, int rate, int bit, int channel);
    void rec_data_callback_func(string& recData);
    void status_callback_func(int status, int type);

    void appstatus_callback_func(ECStatusMessage status);
    void phonecall_callback_func(ECCallType type, const string name, const string number);
    void inputstart_callback_func(int inputType, int  imeOptions, string rawText, int minLines, int maxLines,  int maxLength);
    void inputselection_callback_func(int start, int stop);
    void phoneinfo_callback_func(const string& info);
    void vr_textinfo_callback_func(int vrtype, int sequence, string plainText, string htmlText);
    void carcmd_callback_func(int cmd);



private:

    MirrorListener   mirrorListener;
    VideoPlayer      videoplayer;
    AudioPlayer      audioPlayer;
    AudioRecoder     audioRecoder;
    APPListener      appListener;
    OTAListener      otaListener;
    SDKListener      sdkListener;
    ToolKitListener  toolkitListener;
    std::mutex       mMutex;
    string           mQRCode;
    string           mRecData;
    BlueToothInfo    mBlueToothInfo;
    string           mLicense;
    bool             mHasInitRec;
    AudioInfo        mAudioInfo;
#endif

};

#endif // ECLINK_H
