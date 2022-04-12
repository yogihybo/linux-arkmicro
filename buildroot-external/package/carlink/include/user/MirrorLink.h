#ifndef MIRRORLINK_H
#define MIRRORLINK_H

#include "IUserLinkPlayer.h"
class MirrorPlayer;
class MirrorLink : public IUserLinkPlayer
{
public:
    MirrorLink();
    virtual ~MirrorLink();
#ifdef  USE_MIRROR
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

private:
    static void linkstatus_callback_func(int status, void* parameter);
    void onVideoStartCallback(bool start, int width, int height);
    void onVideoInfoCallback(int width, int height, unsigned char* data, int length);
private:
     MirrorPlayer*      m_pMirrorPlayer;
#endif
};

#endif // MIRRORLINK_H
