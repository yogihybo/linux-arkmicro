#ifndef CARLIFELINK_H
#define CARLIFELINK_H

#include "IUserLinkPlayer.h"


enum BTCodeKey
{
    BT_CALLOUT_DTMF_0     = 0,                   ///< 0
    BT_CALLOUT_DTMF_1     = 1,                   ///< 1
    BT_CALLOUT_DTMF_2     = 2,                   ///< 2
    BT_CALLOUT_DTMF_3     = 3,
    BT_CALLOUT_DTMF_4     = 4,
    BT_CALLOUT_DTMF_5     = 5,
    BT_CALLOUT_DTMF_6     = 6,
    BT_CALLOUT_DTMF_7     = 7,
    BT_CALLOUT_DTMF_8     = 8,
    BT_CALLOUT_DTMF_9     = 9,
    BT_CALLOUT_DTMF_START = 10,
    BT_CALLOUT_DTMF_SHARP = 11,
};

typedef enum
{
    KEYCODE_Selector_Next = 0x06,     //切换到下一个焦点
    KEYCODE_Selector_Previous = 0x07, //切换到上一个焦点
    KEYCODE_Move_Left = 0x15,         //21 切换到当前小区域的左边小区域中,焦点将切换到该小区域记录的最后一个焦点
    KEYCODE_Move_Right = 0x16,        //22 切换到当前小区域的右边小区域中,焦点将切换到该小区域记录的最后一个焦点
    KEYCODE_Move_Up = 0x17,           //23 切换到当前小区域的上边小区域中,焦点将切换到该小区域记录的最后一个焦点
    KEYCODE_Move_Down = 0x18,         //24 切换到当前小区域的下边小区域中,焦点将切换到该小区域记录的最后一个焦点
    KEYCODE_Move_Up_Left = 0x19,      //25 地图界面支持
    KEYCODE_Move_Up_Right = 0x1A,     //26 地图界面支持
    KEYCODE_Move_Down_Left = 0x1B,    //27 地图界面支持
    KEYCODE_Move_Down_Right = 0x1C,   //28 地图界面支持
}WheelCmd;


typedef enum
{
    KEYCODE_Home = 0x01,
    KEYCODE_Phone_Call = 0x02,
    KEYCODE_Phone_End,
    KEYCODE_HFP = 0x05,
    KEYCODE_Media = 0x09,
    KEYCODE_Navi = 0x0B,
    KEYCODE_Back = 0x0E,
    KEYCODE_Seek_Sub = 0x0F,  //15 播放上一首
    KEYCODE_Seek_Add = 0x10,  //16 播放下一首
    KEYCODE_Mute = 0x13,      //19
    KEYCODE_OK = 0x14,        //20
    KEYCODE_TEL = 0x1D,
    KEYCODE_MAIN = 0x1E,
    KEYCODE_Media_Start = 0x1F,
    KEYCODE_Media_Stop = 0x20,
    KEYCODE_VR_Start = 0x21,
    KEYCODE_VR_Stop = 0x22,
}HardKeyCmd;


class CarlifePlayer;
class CarlifeLink : public IUserLinkPlayer
{

public:
    CarlifeLink();
    virtual ~CarlifeLink();

#ifdef USE_CARLIFE
    void set_mac(string mac);

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
    virtual void send_phone_bluetooth(const string& address){}
    virtual void send_car_wifi(WifiInfo& info){}
    virtual void send_touch(int x, int y, TouchCode touchCode);
    virtual void send_multi_touch(int x1, int y1, TouchCode touchCode1, int x2, int y2, TouchCode touchCode2){}
    virtual bool send_key(KeyCode keyCode);
    virtual bool send_wheel(WheelCode wheel, bool foucs);
    virtual bool send_night_mode(bool night);
    virtual bool send_right_hand_driver(bool right){}
    virtual bool open_page(AppPage appPage);
    virtual void request_status(RequestAppStatus requestAppStatus, void *reserved = nullptr);
    virtual void send_license(const string& license){}
    virtual void send_input_text(const string& text) {}
    virtual void send_input_selection(const int start, const int stop){}
    virtual void send_input_action(const int acionId, const int keyCode){}
private:
    void bt_call_status(int status);
    void status_callback_func(int status, void* parameter);
    void video_start_callback_func(bool start, int width, int height, void* parameter);
    void video_callback_func(int width, int height, unsigned char* data, int length, void* parameter);
    void audio_start_callback_func(bool start, int type, int rate, int bit, int channel,void* parameter);
    void audio_callback_func(int type, unsigned char* data, int length,  void* parameter);
    void phone_number_callback_func(string number, void* parameter);
private:
    CarlifePlayer *m_pCarlifePlayer;
    bool mBRecorder;
    int m_src_x[2];
    int m_src_y[2];
    int m_old_src_x[2];
    int m_old_src_y[2];
    bool m_blongpress;
    string mPhoneNumber;
    PhoneType mPhoneType;
#endif
};

#endif // CARLIFELINK_H
