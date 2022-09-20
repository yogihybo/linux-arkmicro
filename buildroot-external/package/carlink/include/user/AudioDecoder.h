#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include "alsa/asoundlib.h"
#include <mutex>
#include <atomic>
//媒体参数
struct AudioParam
{
    uint rate;           //采样率Hz
    uint bits;           //位数8、16
    uint channels;       //通道数
};

class AudioDecoder
{
public:
    AudioDecoder(const char *device = "default");
    virtual ~AudioDecoder();

public:
    void setMediaParam(const uint rate, const uint bits, const uint channels);

    void release();
    int  playSound(unsigned char *data, uint len);

    void stopPlaySound() {
        mHasInit = false;
    }
    void resumePlaySound() {
        mHasInit = true;
    }

private:

    bool Init();
    void Uninit();
    int xrunRecover(snd_pcm_t *handle, int err);
private:
     char             *mpDevice;
     snd_pcm_t        *mpHandle;
     AudioParam       mAudioParam;
     std::atomic_bool mHasInit;
     std::mutex       mMutex;

};

#endif // AUDIODECODER_H
