
#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

#include "ECSDKAudioManager.h"

using namespace ECSDKFrameWork;
#include <mutex>
/*
 * 用户实现：声音播放器功能
 * start：初始化并启动播放器
 * stop：停止并释放播放器
 * play：播放手机传输过来的音频数据
 * setVolume：设置音量
 * 所有接口最好保证线程安全
 * */
#include <functional>
typedef std::function<void(bool, int,int,int,int)> FUNCAUDIOSTART;
typedef std::function<void(int, unsigned char*, int)> FUNCAUDIODATA;

class AudioPlayer : public IECAudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    void start(ECAudioType type, const ECAudioInfo& info) override;

    void stop(ECAudioType type) override;

    void play(ECAudioType type, const void* data, uint32_t len) override;

    void setVolume(ECAudioType type, uint32_t vol) override;

public:
    void registerAudioDataCallback(FUNCAUDIODATA func);
    void registerStartAudioCallback(FUNCAUDIOSTART func);

private:
    bool mHasInit = false;
    std::mutex mMutex;

    FUNCAUDIODATA  mFuncAudioData;
    FUNCAUDIOSTART mFuncAudioStart;
};


#endif //AUDIOPLAYER_H
