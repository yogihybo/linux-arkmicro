#pragma once

#include <atomic>
#include <mutex>


#include "ECSDKMirrorManager.h"
//#include "BufferQueue.h"
//#include "videodecode.h"

using namespace ECSDKFrameWork;

/*
 * 用户实现：视频播放器功能
 * start：初始化并启动播放器
 * stop：停止并释放播放器
 * play：播放手机传输过来的视频数据
 * */
#include <functional>
typedef std::function<void (unsigned char*, int)> FUNCVIDEODATA;
typedef std::function<void (bool, int, int)> FUNCVIDEOSTART;

typedef struct ScreenSize
{
    int Width;
    int Height;

}_ScreenSize;

class VideoPlayer : public IECVideoPlayer
{
public:
    VideoPlayer();
    ~VideoPlayer();

    virtual void play(const void *data, int32_t len) override;
    virtual void start(int width, int height) override;
    virtual void stop() override;

    void setScreenSize(ScreenSize size);
    void registerStartVideoCallback(FUNCVIDEOSTART func);
    void registerVideoDataCallback(FUNCVIDEODATA func);
private:
    void                DeliverFrameThread();
    static void        *DeliverFrameThreadFunc(void *parameter);
private:
    FUNCVIDEOSTART      mFuncVideoStart = nullptr;
    FUNCVIDEODATA       mFuncVideoData = nullptr;
    //BufferQueue*        mPtrVideoQueue;
    pthread_t           mDeliverFrameThread;
    //VideoDecode         *m_pVideoPlayer;
    ScreenSize          mScreenSize;

    std::atomic_bool mHasInit;
    std::mutex    mMutex;
};

