#ifndef HICARLINK_H
#define HICARLINK_H
#include <mutex>

#include "IUserLinkPlayer.h"

#ifdef USE_HICAR

#define PLAYLOAD 640

enum HiCarAudioStreamType {
  HiCar_AUDIO_STREAM_TELEPHONY = 0,
  HiCar_AUDIO_STREAM_VR = 2,
  HiCar_AUDIO_STREAM_MEDIA = 3,
  HiCar_AUDIO_STREAM_Alt = 4,
};



class HiCarLink;
class BufferQueue;
class HiCarLinkImpl
{
public:
    HiCarLinkImpl(HiCarLink *handle);
    ~HiCarLinkImpl();

    //int HiCarGetTypeByStreamType(int streamType);
    AudioType ChangeAudioType(HiCarAudioStreamType type);

    static int HiCarSendVHiCarEventCallBack(int type, void *data, int len, void *parameters);
    static int HiCarSendVideoDataCallBack(int type, void *data, int dataLen, void *parameters);
    static int HiCarGetAudioDataCallBack(int streamType, void *data, int dataLen, void *parameters);
    static int HiCarGetAudioMicDataCallBack(int streamType, void *data, int dataLen, void *parameters);
    static int HiCarSendVHiCarBTCallBack(int type, void *data, int dataLen, void *parameters);
    static int HiCarAudioFoucsCallBack(int type, void *streamType, int focus, void* parameters);

private:
    HiCarLink *mHandle;
    int        mHiCarState;
};
#endif

class HiCarLink : public IUserLinkPlayer
{

public:
    HiCarLink();
    virtual ~HiCarLink();

#ifdef USE_HICAR
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
    virtual void send_car_wifi(WifiInfo& info);
    virtual void send_touch(int x, int y, TouchCode touchCode);
    virtual void send_multi_touch(int x1, int y1, TouchCode touchCode1, int x2, int y2, TouchCode touchCode2);
    virtual bool send_key(KeyCode keyCode);
    virtual bool send_wheel(WheelCode wheel, bool foucs);
    virtual bool send_night_mode(bool night);
    virtual bool send_right_hand_driver(bool right);
    virtual bool open_page(AppPage appPage);
    virtual void request_status(RequestAppStatus requestAppStatus, void *reserved = nullptr);
    virtual void send_license(const string& license){}
    virtual void send_input_text(const string& text) {}
    virtual void send_input_selection(const int start, const int stop){}
    virtual void send_input_action(const int acionId, const int keyCode){}
    virtual void send_bluetooth_cmd(const string& cmd);
    virtual void send_broadcast(bool enable);
    virtual void send_delay_record(int millisecond);
    virtual void send_wifi_state_changed(WifiStateAction action, WifiState state, const string& phoneIp, const string& carIp){}

    friend class HiCarLinkImpl;

protected:
    PhoneType getPhoneType() {return mPhoneType;}
private:
     LinkMode                             mLinkMode;
     HiCarLinkImpl*                       mHandle;
     PhoneType                            mPhoneType;
     std::mutex                           mMutex;
     BufferQueue*                         mRecQueue;
     int                                  mRecPos;
     uint8_t                              mRecBuf[PLAYLOAD];
     int                                  mLinkType;
     int                                  mMillisecond;
#endif

};

#endif // HICARLINK_H
