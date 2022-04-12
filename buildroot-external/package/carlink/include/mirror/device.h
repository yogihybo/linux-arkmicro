#ifndef DEVICE_H
#define DEVICE_H


//#include "videodecode.h"
#include <stdint.h>
#include <string>
#include "common.h"
#include "stream.h"
//#include "controller.h"



using namespace std;
class Server;
class Stream;
class Controller;

class Device
{

public:
    struct DeviceParams {
        string recordFileName;    // 视频录制文件名
        string serial;            // 设备序列号
        unsigned short localPort;      // reverse时本地监听端口
        unsigned short maxSize;          // 视频分辨率
        unsigned int bitRate;      // 视频比特率
        bool closeScreen;       // 启动时自动息屏
        bool useReverse;         // true:先使用adb reverse，失败后自动使用adb forward；false:直接使用adb forward
        bool display;            // 是否显示画面（或者仅仅后台录制）
        string gameScript;        // 游戏映射脚本
    };
    explicit Device(DeviceParams params);
    virtual ~Device();

    Controller *getController();
    Server *getServer();
   // VideoDecode *getDecoder() const {return m_decoder;}

    void setDeviceDisconnectCallback(void (*callback)(string ,void*), void* parameter);
    void setServerFinishedCallback(void (*callback)(void*), void* parameter);
    void SetVideoStartCallback(FUNCVIDEOSTART funcVideoStart);
    void SetVideoInfoCallback(FUNCVIDEOINFO funcVideoInfo);

    void startServer();

    void eventOperation();
    void onSendVertical(bool bVertical);

private:    

    void GrabFrameThread();
    static void *GrabFrameThreadFunc(void *parameter);

    void onVideoStartCallback(bool start, int width, int height);
    void onVideoInfoCallback(int width, int height, unsigned char* data, int length);


    static void server_start_result_callback_func(bool success,void* parameter);
    static void connect_to_result_callback_func(bool success, string deviceName, Size size, void* parameter);
private:

    Server* m_server;
    Controller* m_controller;
    Stream* m_stream;
    FUNCVIDEOSTART  mFuncVideoStart;
    FUNCVIDEOINFO   mFuncVideoInfo;
   // VideoDecode *m_decoder;
    pthread_t     mGrabFrameThread;
    // ui

    DeviceParams m_params;

    void (*m_device_disconnected_callback)(std::string ,void*);
    void (*m_server_finished_callback)(void*);
    void *m_parameter;
};

#endif // DEVICE_H
