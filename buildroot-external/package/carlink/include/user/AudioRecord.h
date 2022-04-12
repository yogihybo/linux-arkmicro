#ifndef AUDIORECORD_H
#define AUDIORECORD_H


#include "alsa/asoundlib.h"
#include "AudioDecoder.h"
#include <functional>
//#include <atomic>
#include "Thread.h"

typedef std::function<void(unsigned char* ,int)> AUDIORECORDFUNC;

struct AudioData
{
    unsigned char data[4096];
    uint length;
};

class AudioRecord : public ArkThread
{
public:
    AudioRecord();

    virtual ~AudioRecord();
    static AudioRecord *instance();

    //设置音频参数
    void setMediaParam(const uint rate, const uint bits, const uint channels);

    void registerRecordAudio(AUDIORECORDFUNC func);

    void stopRecordSound()
    {
        mbRecordStart = true;
    }
    void resumeRecordSound()
    {
        mbRecordStart = false;

    }
    int  initDevice();
    void Release();

    int  ReadMicAudioData(AudioData &buff);

protected:
    virtual void run();

    int recSound(unsigned char *data, uint& len);
    int xrunRecover(snd_pcm_t *handle, int err);

private:
    static AudioRecord *mInstance;

    snd_pcm_t   *mpHandle;
    AUDIORECORDFUNC mFuncAudioRecord;
    bool mbRecordStart;

    int         mBufferSize;
    int         mFrameSize;

    AudioParam  mParam;
    AudioData   mData;
};


#endif // AUDIORECORDER_H
