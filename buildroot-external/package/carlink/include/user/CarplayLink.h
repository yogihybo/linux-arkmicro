#ifndef CARPLAYLINK_H
#define CARPLAYLINK_H

#include "IUserLinkPlayer.h"
#include "carplayWrapper.h"
#include "carplayVideoWrapper.h"
#include "carplayAudioWrapper.h"
#include <list>
#include <mutex>
#ifdef USE_CARPLAY
class CarplayWrapper;
class CarplayAudioCtx;
class ICarplayVideoCallbacksImpl : public ICarplayVideoCallbacks
{
public:
    ICarplayVideoCallbacksImpl(CarplayLink* handle) : mHandle(handle) {}

    int carplayVideoStartCB();
    void carplayVideoStopCB();
    int carplayVideoDataProcCB(const char *buf, int len);
private:
    CarplayLink* mHandle;
};

class ICarplayAudioCallbacksImpl : public ICarplayAudioCallbacks
{
public:
    ICarplayAudioCallbacksImpl(CarplayLink* handle) : mHandle(handle) {}

    void carplayAudioStartCB(int handle, AudioStreamType type, int rate, int bits, int channels);
    void carplayAudioStopCB(int handle, AudioStreamType type);
private:
    CarplayLink*        mHandle;
  //  CarplayAudioCtx*    mAudioHandle;
    Mutex               mLock;
    std::list<CarplayAudioCtx*> mAudioHandlList;
};

#endif

typedef enum
{
    MEDIA_NONE = 0,
    MEDIA_PLAY,
    MEDIA_PAUSE,
    MEDIA_PLAY_PAUSE,
    MEDIA_NEXT,
    MEDIA_PREVIOUS
}MEDIA;

class CarplayLink : public IUserLinkPlayer
{
public:
    CarplayLink();
    virtual ~CarplayLink();
#ifdef USE_CARPLAY
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
    virtual void send_multi_touch(int x1, int y1, TouchCode touchCode1, int x2, int y2, TouchCode touchCode2){}
    virtual bool send_key(KeyCode keyCode);
    virtual bool send_wheel(WheelCode wheel, bool foucs);
    virtual bool open_page(AppPage appPage);
    virtual void request_status(RequestAppStatus requestAppStatus, void *reserved = nullptr);
    virtual void send_license(const string& license){}

protected:
    LinkConfig getLinkConfig() const {return mLinkConfig;}
    PhoneType getPhoneType() const {return mPhoneType;}

    bool sendPlayData(int handle, char type, const char* buf, int len, int frames, long long time_stamp);
    bool receiveRecordData(int handle, int frames);

    void setLocalTime(long long local);

friend class CarplayAudioPlayCtx;
friend class CarplayAudioRecordCtx;
friend class ICarplayVideoCallbacksImpl;
friend class ICarplayAudioCallbacksImpl;
friend class CarplayLinkCbsImpl;

    std::mutex                              mMutex;
    string                                  mRecData;
    CarplayWrapper*                         mHandle;
    ICarplayCallbacks*                      mCbs;
    ICarplayVideoCallbacks*                 mVideoCbs;
    ICarplayAudioCallbacks*                 mAudioCbs;
    PhoneType                               mPhoneType;
    long long                               mLocalTime;
    LinkMode                                mLinkMode;
    bool                                    mDefaultWifi;
    bool                                    mDdefaultPhonebtMac;
#endif
};

#endif // CARPLAYLINK_H
