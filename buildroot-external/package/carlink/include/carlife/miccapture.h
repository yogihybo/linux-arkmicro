#ifndef MICCAPTURE_H
#define MICCAPTURE_H

#include "thread.h"
#include "types.h"
#include "alsa/asoundlib.h"

class MicCapture : public Thread
{
public:
    virtual ~MicCapture();
    static MicCapture *instance();

    void stopRecordSound()
    {
        mStop = true;
    }
    void resumeRecordSound()
    {
        mStop = false;
    }

    int initDevice();
    void Release();

    int recSound(byte *data, uint len);
protected:
    virtual void run();


private:
    MicCapture();


    int readMicData(Buffer &);
    uint timeStamp();
    int xrunRecover(snd_pcm_t *handle, int err);

private:
    static MicCapture *mInstance;

    snd_pcm_t *mCaptureHandle;

    pthread_mutex_t mDeviceMutex;       //设备使用互斥锁

    char mDevice[256];

    int  mBufferSize;
    int  mFrameSize;

    MediaParam mParam;

    Buffer mData;

    unsigned char mAudio[2048];
    int mlength ;
    bool mStop;

};

#endif // MICCAPTURE_H
