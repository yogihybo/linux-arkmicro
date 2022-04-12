#ifndef AUTOLINK_H
#define AUTOLINK_H
#include <mutex>

#include "IUserLinkPlayer.h"

#ifdef USE_AUTO
#include "AndroidAuto.h"
#include "IUserAutoCbs.h"

enum AutoAudioStreamType {
  AUTO_AUDIO_STREAM_GUIDANCE = 1,
  AUTO_AUDIO_STREAM_SYSTEM_AUDIO = 2,
  AUTO_AUDIO_STREAM_MEDIA = 3,
  AUTO_AUDIO_STREAM_TELEPHONY = 4
};

class AutoLink;
class IUserAutoImpl : public IUserAutoCbs
{
public:
    IUserAutoImpl(AutoLink* handle);
    ~IUserAutoImpl();

    void videoStart(int width, int height, int offsetX, int offsetY);
    void videoStop();
    void videoPlay(char *buf, int len);

    void audioStart(int type, int rate, int channels, int bits);
    void audioStop(int type);
    void audioPlay(int type, char *buf, int len);

    void recordStart(int rate, int channels, int bits);
    void recordStop();
    void recordProc(char *buf, int len);

    void notifyStatus(int state);
    void notifyPhoneBtInfo(const char *phoneBTAddr, int pairMethod);
    void getLocalBtAddr(char* mac);

    AudioType ChangeAudioType(AutoAudioStreamType type);
private:
    AutoLink*        mHandle;
    bool             mRecStart;
    bool             mRecBufClearFlag;
    int              mStartPos;
    int              mUsedPos;

};
#endif

class AutoLink : public IUserLinkPlayer
{

public:
    AutoLink();
    virtual ~AutoLink();

#ifdef USE_AUTO
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
    virtual bool open_page(AppPage appPage);
    virtual void request_status(RequestAppStatus requestAppStatus, void *reserved = nullptr);
    virtual void send_license(const string& license){}

    friend class IUserAutoImpl;

protected:
    PhoneType getPhoneType() {return mPhoneType;}

    string  getRecData() {return mRecData;}

    void setRecData(string str);

private:

     LinkMode                             mLinkMode;
     AndroidAuto*                         mHandle;
     IUserAutoImpl*                       mAutoCbs;
     PhoneType                            mPhoneType;
     string                               mRecData;
     std::mutex                           mMutex;
#endif

};

#endif // AUTOLINK_H
