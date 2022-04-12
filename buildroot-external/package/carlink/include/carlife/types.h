#ifndef TYPES_H
#define TYPES_H

//#include "carlifeinterface.h"

#define UNUSED(x)   (void)x;

#define msleep(x)   usleep((x) * 1000);

#define BUFFER_SIZE     524288  //数据缓冲大小(512k)


#define TMP_DIR                 "/data/local/tmp/"
#define CACHE_DIR               "/data/local/tmp/dalvik-cache"

#define BDIM_PATH               "/opt/carlife/bdim"
#define BDIM_JAR_PATH           "/opt/carlife/bdim.jar"

#define BDSC_PATH               "/opt/carlife/bdsc"
#define BDSC16_PATH             "/opt/carlife/bdsc16"
#define BDSC17_PATH             "/opt/carlife/bdsc17"
#define BDSC18_PATH             "/opt/carlife/bdsc18"
#define BDSC19_PATH             "/opt/carlife/bdsc19"
#define BDSC19_01_PATH          "/opt/carlife/bdsc19_01"

#define BDSC_DST_PATH           "/data/local/tmp/bdsc"

//#define CARLIFE_APK_SRC_PATH    "/opt/carlife/CarLife.apk"
#define CARLIFE_APK_SRC_PATH    "CarLife.apk"
#define CARLIFE_APK_DST_PATH    "/data/local/tmp/CarLife.apk"

//#define CARKIT_SCREEN_W     800
//#define CARKIT_SCREEN_H      480

typedef unsigned char   byte;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;

//手机类型
//enum Phone
//{
//    Android,
//    IPhone
//};

////状态
//enum Status
//{
//    NotRun,
//    Start,
//    WaitForDevice,
//    CarLifeNotInstall,
//    Initializing,
//    InitFinished,
//    InitFailed,
//    InForeground,
//    InBackground,
//};

////初始化进度
//enum InitProgress
//{
//    DeviceConnected = 0,
//    AdbForward = 25,
//    PushApp = 50,
//    LaunchCarLife = 75,
//    SetupConnection = 100
//};

////动作
//enum ScreenAction
//{
//    ScreenPress,
//    ScreenRelease,
//    ScreenMove,
//    ScreenSingleClick,
//    ScreenDoubleClick,
//    ScreenLongPress,
//};


//蓝牙匹配状态
enum BtPairStatus
{
    Idle = 0,
    Ready,
    Connected,
};

//蓝牙Hfp连接状态
enum HfpConnection
{
    HfpDisconnect = 1,
    HfpConnecting,
    HfpConnected,
};


enum BtIndication
{
    IncomingCall = 1,
    OutGoingCall,
    CallActive,
    CallInActive,
    MulticallActive,
    MulticallInActive
};

struct Buffer
{
    byte data[4096];
    uint length;
};

//视频参数
struct VideoParam
{
    uint width;          //宽度
    uint height;         //高度
    uint frameRate;      //帧率
};

//媒体参数
struct MediaParam
{
    uint rate;           //采样率Hz
    uint bits;           //位数8、16
    uint channels;       //通道数
};

#endif // TYPES_H
