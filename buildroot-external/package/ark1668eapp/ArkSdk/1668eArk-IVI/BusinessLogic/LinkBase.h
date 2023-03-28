#ifndef LINKBASE_H
#define LINKBASE_H

#endif // LINKBASE_H

using namespace std;
#include <string>
typedef enum
{
    Carplay,
    Android_Auto,
    Carlife,
    HiCar,
    ECLink,
    Mirror,
    Airplay,
}LinkType;

typedef enum
{
    Wired,
    Wireless,
    Auto,
}LinkMode;

//dbus mode carlink send
typedef enum
{
    DBUS_DEVICE_ATTACHED,                //usb insert
    DBUS_DEVICE_DEATTACHED,              //usb removed
    DBUS_CONNECTTING,                    //carlink connectting
    DBUS_CONNECTED,                      //carlink connected
    DBUS_FOREGROUND,                     //carlink foreground
    DBUS_BACKGROUND,                     //carlink background
    DBUS_FAILED,                         //carlink failed
    DBUS_DISCONNECTED,                   //carlink disconnected
    DBUS_APP_EXIT,                       //phone's app exited
    DBUS_INTERRUPTED_BY_APP,             //start connecting by interrupted (kill phone's app or connecting removed)
    DBUS_MUSIC_STARTED,                  //music start
    DBUS_MUSIC_STOPPED,                  //music stop
    DBUS_NAVI_STARTED,                   //navigation start
    DBUS_NAVI_STOPPED,                   //navigation stop
    DBUS_VR_STARTED,                     //voices start
    DBUS_VR_STOPPED,                     //voices stop
    DBUS_RECOGNITION_STARTED,            //voices recognition start(record start)
    DBUS_RECOGNITION_STOPPED,            //voices recognition stop(record stop)
    DBUS_PHONE_STARTED,                  //phone call start
    DBUS_PHONE_STOPPED,                  //phone call stop
    DBUS_NAVI_SOUND_STARTED,             //navigation sound start
    DBUS_NAVI_SOUND_STOPPED,             //navigation sound stop
}DbusSend;

//carlink receive or user's request
typedef enum
{
    DBUS_REQUEST_CONNECT,//连接
    DBUS_REQUEST_FOREGROUND,//前台
    DBUS_REQUEST_BACKGROUND,//后台
    DBUS_REQUEST_EXITED,//退出
}DbusReceive;

typedef enum
{
    UnKnown,
    Phone_Android,
    Phone_IOS,
}PhoneType;

typedef enum
{
    APP_NOT_RUNNING,                    //phone app not open
    APP_FOREGROUND,                     //carlink foreground
    APP_BACKGROUND,                     //carlink background
    APP_SCREENLOCKED,                   //phone screen locked
    APP_UNSCREENLOCKED,                 //phone screen unlocked
    APP_MUSIC_STARTED,                  //music start
    APP_MUSIC_STOPPED,                  //music stop
    APP_NAVI_STARTED,                   //navigation start
    APP_NAVI_STOPPED,                   //navigation stop
    APP_VR_STARTED,                     //voices start
    APP_VR_STOPPED,                     //voices stop
    APP_RECOGNITION_STARTED,            //voices recognition start(record start)
    APP_RECOGNITION_STOPPED,            //voices recognition stop(record stop)
    APP_PHONE_STARTED,                  //phone call start
    APP_PHONE_STOPPED,                  //phone call stop
    APP_NAVI_SOUND_STARTED,             //navigation sound start
    APP_NAVI_SOUND_STOPPED,             //navigation sound stop
    LAUNCH_PHONE_APP,                   //notify open phone's app
    APP_QRCODE,                         //qr code 's information
    APP_LICENSE,                        //license information
    APP_INPUTINFO,                      //keyboard input information
    APP_INPUTPOS,                       //keyboard input pos information
    APP_VRTEXT,                         //voices information
    APP_VOICE_CMD,                      //voices controls information
    APP_VERSION,                        //version
    APP_VOICE_DUCK,                     //duck
    APP_VOICE_UNDUCK,                   //unduck
    APP_PINCODE,                        //wireless pin code(hicar)
    APP_BT_CMD,                         //bt at cmd
    APP_RESERVED,                       //reserved
}AppStatusMessage;

//亿连id
struct License{
    bool activate;
    int code;
    string msg;
    string ver;
};

struct InputInfo
{
    int32_t  inputType;
    int32_t  imeOptions;
    string   rawText;
    uint32_t minLines;
    uint32_t maxLines;
    uint32_t maxLength;
};

struct InputPos{
    int start;
    int stop;
};

enum VRTextType
{
    VR_TEXT_FROM_UNKNOWN = 0,
    VR_TEXT_FROM_VR,
    VR_TEXT_FROM_SPEAK
};

struct VRTextInfo
{
    VRTextType   type;
    int32_t      sequence;
    string       plainText;
    string       htmlText;
};


typedef enum
{
    APP_PAGE_NAVIGATION,
    APP_PAGE_MUSIC,
    APP_PAGE_VR,
    APP_PAGE_NAVI_HOME,
    APP_PAGE_NAVI_WORK,
    APP_PAGE_MAIN,
    APP_PAGE_NAVI_GAS_STATION,
    APP_PAGE_CAR_PARK,
    APP_PAGE_4S_SHOP,
}AppPage;

enum CallType
{
    CALL_TYPE_DAIL = 0,                       ///< ring up
    CALL_TYPE_HANG_UP,                        ///< ring off
    CALL_TYPE_MAX,                            ///< reserve
};

enum ConnectedStatus
{
    CONNECT_STATUS_DEVICE_ATTACHED,
    CONNECT_STATUS_DEVICE_DEATTACHED,
    CONNECT_STATUS_CONNECTING,
    CONNECT_STATUS_CONNECT_FAILED,
    CONNECT_STATUS_CONNECT_SUCCEED,
    CONNECT_STATUS_DISCONNECTED,
    CONNECT_STATUS_APP_EXIT,
    CONNECT_STATUS_INTERRUPTED_BY_APP,
};

enum RequestAppStatus
{
    QUERYTIME,
    QUERYGPS,
    QUERYBTSTATUS,
    QUERYQRCODE,
    QUERYINPUT,
};

enum AudioType
{
    AUDIO_TYPE_MUSIC   = 0x00,                   ///< Music audio
    AUDIO_TYPE_CALL    = 0x01,                   ///< phone call audio
    AUDIO_TYPE_VR      = 0x02,                   ///< VR audio
    AUDIO_TYPE_TTS     = 0x03,                   ///< TTS audio
    AUDIO_TYPE_Rec     = 0x04,                   ///< rec audio
    AUDIO_TYPE_Alert   = 0x05,                   ///< alert audio
};

struct AudioInfo
{
    uint32_t               sampleRate;            ///< sample rate
    uint32_t               channel;               ///< channel type
    uint32_t               format;                ///< audio format type
};

enum  WheelCode
{
    Wheel_Keep = 0,
    Wheel_Next = 1,
    Wheel_Previous = -1,
};

enum  TouchCode
{
    Touch_Up = 0,
    Touch_Press = 1,
    Touch_Move = 2,
};

struct WifiInfo{
    string ssid;
    string passphrase;
    string channel_id;
};

struct BlueToothInfo{
    string name;
    string addr;
    string pin;
};

struct BT_CMD{
    string cmd;
    int len;
};

struct DuckInfo{
    double inDurationSecs;
    double volume;
};

enum  KeyCode
{
    // for driving mode
    KEY_HOME = 1,
    KEY_PHONE = 2,
    KEY_TALKIE = 3,                         ///< start talkie
    KEY_NAVIGATION = 4,                     ///< start navigation
    KEY_VOICE_ASSISTANT = 5,                ///< start voice assistant
    KEY_MUSIC_PLAY = 6,                     ///< play music
    KEY_MUSIC_NEXT = 7,                     ///< play next music
    KEY_MUSIC_PREVIOUS = 8,                 ///< play previous music
    KEY_MUSIC_PAUSE = 9,                    ///< music pause
    KEY_MUSIC_STOP = 10,			        ///< music stop
    KEY_MUSIC_PLAY_PAUSE = 11,              ///< music switch between pause and stop

    KEY_VOLUME_UP = 12,                      ///< increase the volume of phone
    KEY_VOLUME_DOWN = 13,                    ///< decrease the volume of phone

    KEY_TOPLEFT = 14,                        ///< click top left button
    KEY_TOPRIGHT = 15,                       ///< click top right button
    KEY_BOTTOMLEFT = 16,                     ///< click bottom left button
    KEY_BOTTOMRIGHT = 17,                    ///< click bottom right button

    KEY_LIGHTMODE = 18,                           ///< click night mode button
    KEY_NIGHTMODE  = 19,                    ///light mode
    KEY_APP_FRONT = 20,						///< make the app of android phone switch to the foreground
    KEY_APP_BACK = 21,                       ///< it works like the function of the back button to the app of the connected phone.

    KEY_CAR_FOREGROUND = 22,                       ///it carlinks 's ui foreground
    KEY_CAR_BACKGROUND = 23,                       ///it carlinks 's ui background

    KEY_ENFORCE_LANDSCAPE = 24,              ///< make the android phone enforce landscape.
    KEY_CANCEL_LANDSCAPE = 25,               ///< make the android phone cancel landscape.
    KEY_ENFORCE_OR_CANCEL_LANDSCAPE = 26,    ///< make the android phone enforce or cancel landscape.

    KEY_SYSTEM_HOME = 27,                         ///< The Android phone's HOME key
    KEY_SYSTEM_BACK = 28,                             ///< The Android phone's BACK key
    KEY_PICKUP_PHONE = 29,                          /// pick up phone
    KEY_HANGUP_PHONE = 30,                          ///hang up phone

    KEY_CAR_OK = 31,                                ///make car links 'ui enter comfirm
    KEY_CAR_BACK = 32,                                ///make car links 'ui back page
    KEY_MAX,                                     ///< reserve
};

struct LinkConfig
{
   int offset_x;
   int offset_y;
   int screen_width;
   int screen_height;
   bool diy_screen;
   bool autostart;
   bool encryption;
   bool usb_index;
   LinkType linkType;
   string version;
   string manufacturer;
   string phone_bt_mac;
   string car_bt_mac;
   string phone_wifi_mac;
   string car_wifi_ssid;
   string car_wifi_passphrase;
   string car_wifi_channel;
};

struct CarlifeConfig
{
   int ios_link_mode;
   int android_link_mode;
   bool mic_record;
};

struct CarplayConfig
{
    int screen_physical_width;
    int screen_physical_height;
    int aec_delay;
    int fps;
    string vehicle_name;
    string vehicle_icon_label;
    string vehicle_icon_path;
};

class LinkBase
{
public:
    LinkBase(){}
    virtual ~LinkBase(){}

protected:
    virtual bool init(LinkMode linkMode) = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool release() = 0;
    virtual bool start_mirror() = 0;
    virtual bool stop_mirror() = 0;
    virtual bool set_background() = 0;
    virtual bool set_foreground() = 0;
    virtual bool get_audio_focus() = 0;
    virtual bool release_audio_focus() = 0;
    virtual void set_inserted(bool inserted, PhoneType phoneType) = 0;
    virtual void send_screen_size(int width, int height) = 0;
    virtual void record_audio_callback(unsigned char *data, int len) = 0;
    virtual void send_car_bluetooth(const string& name, const string& address, const string& pin) = 0;
    virtual void send_phone_bluetooth(const string& address) = 0;
    virtual void send_car_wifi(WifiInfo& info) = 0;
    virtual void send_touch(int x, int y, TouchCode touchCode) = 0;
    virtual void send_multi_touch(int x1, int y1, TouchCode touchCode1, int x2, int y2, TouchCode touchCode2) = 0;
    virtual bool send_key(KeyCode keyCode) = 0;
    virtual bool send_wheel(WheelCode wheel, bool foucs) = 0;
    virtual bool send_night_mode(bool night) = 0;
    virtual bool send_right_hand_driver(bool right) = 0;
    virtual bool open_page(AppPage appPage) = 0;
    virtual void request_status(RequestAppStatus requestAppStatus, void *reserved = nullptr) = 0;
    virtual void send_license(const string& license) = 0;
    virtual void send_input_text(const string& text) = 0;
    virtual void send_input_selection(const int start, const int stop) = 0;
    virtual void send_input_action(const int acionId, const int keyCode) = 0;
    virtual void send_bluetooth_cmd(const string& cmd ) = 0;
    virtual void send_broadcast(bool enable) = 0;

};

