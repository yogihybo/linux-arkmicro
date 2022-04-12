//
// Created by carbit on 3/8/20.
//

#ifndef AUDIORECODER_H
#define AUDIORECODER_H

#include "ECSDKAudioManager.h"
#include <mutex>
#include <functional>

typedef std::function<void (bool,int,int,int)> FUNCRECORDSTART;
typedef std::function<void (string&)> FUNCRECORDDATA;

using namespace ECSDKFrameWork;

/*
 * 用户实现：声音录音功能
 * 此功能使用车机的麦克风录音，并传输给手机。
 *
 * start：初始化录音设备
 * record：上传录音数据， 此接口ECSDKFramewor每间隔50ms调用一次，将从mic设备获取的数据填充到data。
 * stop：停止录音设备
 * 所有接口最好保证线程安全
 *
 * */

class AudioRecoder : public IECAudioRecorder
{
public:
    AudioRecoder();
    virtual ~AudioRecoder();

    virtual void start(const ECAudioInfo& info) override;

    virtual int32_t record(string& data) override;

    virtual void stop() override;

public:
    void    registerRecStartCallback(FUNCRECORDSTART func);
    void    setRecData(string& recData);
    void    registerRecDataCallback(FUNCRECORDDATA func);

private:
    bool mHasInit = false;
    std::mutex mMutex;

    FUNCRECORDSTART mFuncRecordStart;
    FUNCRECORDDATA mFuncRecordData;
    string  mRecData;
};


#endif //AUDIORECODER_H
