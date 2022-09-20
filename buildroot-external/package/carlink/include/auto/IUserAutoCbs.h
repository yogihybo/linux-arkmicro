#ifndef IUSERAUTOCBS_H
#define IUSERAUTOCBS_H
#ifdef __cplusplus
typedef enum
{
    LINK_UNSUPPORTED= 0xff,
    LINK_CONNECTED = 1,
    LINK_DISCONNECTED = 2,
    LINK_STARTING = 3,
    LINK_SUCCESS = 4,
    LINK_FAIL = 5,
    LINK_EXITING = 6,
    LINK_EXITED = 7 ,
    LINK_REMOVED = 8,
    LINK_INSERTED = 9,
    LINK_NOT_INSERTED = 10,
    LINK_NOT_INSTALL = 11,
    LINK_CALL_PHONE = 12,
    LINK_CALL_PHONE_EXITED = 13,
    LINK_MUTE =14,
    LINK_UNMUTE  = 15,
    LINK_NODATA = 16,
    LINK_VIDEOREADY = 17,
    LINK_BT_DISCONNECT = 18,
    LINK_FAILED_EAP = 19,
    LINK_FAILED_UNSTART = 20,
    LINK_AUTO_BT_UNPAIRED = 21,
    LINK_AUTO_BT_PAIRED = 22,
    LINK_AUTO_BT_REQUEST = 23,
    LINK_EXIT_PROCESS = 24,
    LINK_KILL_PROCESS = 25,

    LINK_SOCKET_TRUST = 26,
    LINK_OPEN_CARLIFE = 27,
    LINK_VOLUME_START = 28,
    LINK_VOLUME_STOP = 29,
    LINK_RECONNECT = 30,
    LINK_ILLLIGHT_ON = 31,
    LINK_ILLLIGHT_OFF = 32,

    LINK_SIRI_START = 33,
    LINK_SIRI_STOP = 34,
    LINK_ASSIST_START = 35,
    LINK_ASSIST_STOP  = 36,
    LINK_TEL_START = 37,
    LINK_TEL_STOP = 38,
    LINK_MUSIC_START = 39,
    LINK_MUSIC_STOP = 40,

    LINK_SCREEN_CONTROLLER = 62,
    LINK_SCREEN_ACCESSORY = 63,

    LINK_TAKE_AUDIO = 80,
    LINK_UNTAKE_AUDIO = 81,

    LINK_USE_USB0 = 82,
    LINK_USE_USB1 = 83,
    LINK_NO_ERROR = 0,
    LINK_BTCONNECT_ERROR = -1000,   //与gocsdk进程间通讯错误
    LINK_BTCOMM_ERROR = -1001,      //与蓝牙iap通讯失败
    LINK_BTAUTH_ERROR = -1002,      //认证失败
    LINK_BTIDRECJECT_ERROR = -1003, //iap2 identification 参数错误
}Link_STATUS;

class IUserAutoCbs
{
public:
    IUserAutoCbs() {}
    virtual ~IUserAutoCbs() {}

    virtual void videoStart(int width, int height, int offsetX, int offsetY) = 0;
    virtual void videoStop() = 0;
    virtual void videoPlay(char *buf, int len) = 0;

    virtual void audioStart(int type, int rate, int channels, int bits) = 0;
    virtual void audioStop(int type) = 0;
    virtual void audioPlay(int type, char *buf, int len) = 0;

    virtual void recordStart(int rate, int channels, int bits) = 0;
    virtual void recordStop() = 0;
    virtual void recordProc(char *buf, int len) = 0;

    virtual void notifyStatus(int state) = 0;
    virtual void notifyPhoneBtInfo(const char *phoneBTAddr, int pairMethod) = 0;
    virtual void getLocalBtAddr(char* mac) = 0;
};
#endif
#endif // IUSERAUTOCBS_H
