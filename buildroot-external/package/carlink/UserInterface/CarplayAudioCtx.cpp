
#include <alsa/asoundlib.h>

#include "CarplayAudioCtx.h"

#ifdef USE_CARPLAY
int set_alsa_paramters(void *audio_handle, int rate, int channels, int bits_per_sample)
{
    int ret;
    unsigned int val;
    int dir = 0;
    char *buffer;
    int size;
    snd_pcm_uframes_t buffer_size, period_size, frame;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_t *sw_params;
    unsigned period_time = 0, buffer_time = 0;
    snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
    snd_pcm_uframes_t frames;

    snd_pcm_t *handle = (snd_pcm_t *)audio_handle;

    switch(bits_per_sample) {
    case 8:
        format = SND_PCM_FORMAT_U8;
        break;
    case 16:
        format = SND_PCM_FORMAT_S16_LE;
        break;
    case 24:
        format = SND_PCM_FORMAT_U24_LE;
        break;
    case 32:
        format = SND_PCM_FORMAT_U32_LE;
        break;
    default:
        format = SND_PCM_FORMAT_U8;
        break;
    }

    ret = snd_pcm_hw_params_malloc(&hw_params);
    if (ret < 0) {
        perror("snd_pcm_hw_params_malloc");
        return -1;
    }

    ret = snd_pcm_hw_params_any(handle, hw_params);
    if (ret < 0) {
        perror("snd_pcm_hw_params_any");
        return -1;
    }
    ret = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ret < 0) {
        perror("snd_pcm_hw_params_set_access");
        return -1;
    }
    ret = snd_pcm_hw_params_set_format(handle, hw_params, format);
    if (ret < 0) {
        perror("snd_pcm_hw_params_set_format");
        return -1;
    }

    ret = snd_pcm_hw_params_set_channels(handle, hw_params, channels);
    if (ret < 0) {
        perror("snd_pcm_hw_params_set_channels");
        return -1;
    }

    val = rate;
    ret = snd_pcm_hw_params_set_rate_near(handle, hw_params, &val, &dir);
    if (ret < 0) {
        perror("snd_pcm_hw_params_set_rate_near");
        return -1;
    }

    ret = snd_pcm_hw_params_set_periods(handle, hw_params, 3, 0);
    if (ret)
        printf("snd_pcm_hw_params_set_periods %d failed\n", ret);


    ret = snd_pcm_hw_params(handle, hw_params);
    if (ret < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(ret));
        return -1;
    }
    period_size = 0;
    snd_pcm_hw_params_get_period_size(hw_params, &period_size, 0);
    snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
    printf("%s:%d period_size=%d buffer_size=%d \n", __func__, __LINE__, period_size, buffer_size);
    if (period_size == buffer_size) {
        printf("Can't use period equal to buffer size (%lu == %lu)",
              period_size, buffer_size);
        return -1;
    }

    frame = (buffer_size / period_size) * period_size;

    if(frame > 4096){
        frame = 4096;
    }
    return frame;
}

class CarplayAudioPlayCtx : public ArkThread
{
public:
    CarplayAudioPlayCtx(CarplayLink *carplayLink, int handle, AudioStreamType type, int rate, int bits, int channels);
    ~CarplayAudioPlayCtx();
    friend class CarplayAudioCtx;
protected:
    void run();

private:
    int mHandle, mRate, mBits, mChannels;
    AudioStreamType mType;
    bool mStart;
    snd_pcm_t *mAlsaHandle;
    int mBufSize, mAlsaBufSize;
    CarplayLink*    mCarplayCtx;
};


class CarplayAudioRecordCtx : public ArkThread
{
public:
    CarplayAudioRecordCtx(CarplayLink *carplayLink, int handle, AudioStreamType type, int rate, int bits, int channels);
    ~CarplayAudioRecordCtx();
protected:
    void run();
    friend class CarplayAudioCtx;
    friend class CarplayLink;
private:
    int mHandle, mRate, mBits, mChannels;
    AudioStreamType mType;
    bool mStart;
    snd_pcm_t *mAlsaHandle;
    int mBufSize;
    CarplayLink*    mCarplayCtx;
};

uint64_t	UpTicks( void )
{
    uint64_t			nanos;
    struct timespec		ts;

    ts.tv_sec  = 0;
    ts.tv_nsec = 0;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    nanos = ts.tv_sec;
    nanos *= 1000000000;
    nanos += ts.tv_nsec;
    return( nanos );
}

CarplayAudioPlayCtx::CarplayAudioPlayCtx(CarplayLink *carplayLink, int handle, AudioStreamType type, int rate, int bits, int channels) :
    mHandle(handle), mType(type), mRate(rate), mBits(bits), mChannels(channels), mAlsaHandle(NULL), mCarplayCtx(carplayLink), mBufSize(0), mAlsaBufSize(0)
{

    printf("CarplayAudioPlayCtx::%s:%d start\r\n", __func__, __LINE__);
    int ret = snd_pcm_open(&mAlsaHandle, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0) {
        perror("snd_pcm_open rec\n");
        return;
    }
    mBufSize = set_alsa_paramters(mAlsaHandle, rate, channels, bits);
    printf("mBuffsize = %d\r\n", mBufSize);
    if (mBufSize <= 0) {
        mBufSize = 1024;
    }
    mAlsaBufSize = mBufSize;
    if (AudioStreamCall == mType)
        mBufSize = 320;

    start();
    printf("##CarplayAudioPlayCtx::%s:%d end\r\n", __func__, __LINE__);

}
CarplayAudioPlayCtx::~CarplayAudioPlayCtx()
{

    printf("CarplayAudioPlayCtx::%s:%d start", __func__, __LINE__);
    mStart = false;
    join();
    if (mAlsaHandle)
        snd_pcm_close(mAlsaHandle);
    printf("CarplayAudioPlayCtx::%s:%d end\r\n", __func__, __LINE__);

}

void CarplayAudioPlayCtx::run()
{

    const int bytesPerCh= (mBits * mChannels / 8);
    char buf[mBufSize * bytesPerCh] = {0};
    int len = mBufSize * bytesPerCh;
    printf("CarplayAudioPlayCtx::%s:%d\r\n", __func__, __LINE__, bytesPerCh);
    mStart = true;
    printf("CarplayAudioPlayCtx::%s:%d mBufSize:%d\r\n", __func__, __LINE__, mBufSize);
    while(mStart) {

        if(mCarplayCtx)
            mCarplayCtx->sendPlayData(mHandle, mType, buf, len, mBufSize, UpTicks());
    }
}


CarplayAudioRecordCtx::CarplayAudioRecordCtx(CarplayLink *carplayLink, int handle, AudioStreamType type, int rate, int bits, int channels) :
    mHandle(handle), mType(type), mRate(rate), mBits(bits), mChannels(channels), mAlsaHandle(NULL), mCarplayCtx(carplayLink)
{
    printf("CarplayAudioRecordCtx::%s:%d start", __func__, __LINE__);
}
CarplayAudioRecordCtx::~CarplayAudioRecordCtx()
{
    printf("CarplayAudioRecordCtx::%s:%d start", __func__, __LINE__);
    mStart = false;
    join();
    if (mAlsaHandle)
        snd_pcm_close(mAlsaHandle);
    printf("CarplayAudioRecordCtx::%s:%d end", __func__, __LINE__);
}

void CarplayAudioRecordCtx::run()
{
    int frames;
    mStart = true;

    frames = mBits / 8 * mChannels;
    printf("CarplayAudioRecordCtx::%s:%d", __func__, __LINE__);
    while(mStart) {
        mCarplayCtx->receiveRecordData(mHandle, frames);
    }
}


CarplayAudioCtx::CarplayAudioCtx(CarplayLink *carplayLink, int handle, AudioStreamType type, int rate, int bits, int channels) : mRecordHandle(NULL), mPlayHandle(NULL), mStreamHandle(handle)
{
    printf("CarplayAudioCtx::%s:%d start", __func__, __LINE__);
    if (type == AudioStreamRECOGNITION || type == AudioStreamCall) {
        printf("CarplayAudioCtx::%s:%d start", __func__, __LINE__);
        mRecordHandle = new CarplayAudioRecordCtx(carplayLink, handle, type, rate, bits, channels);
        mRecordHandle->start();
    }
    mPlayHandle = new CarplayAudioPlayCtx(carplayLink, handle, type, rate, bits, channels);
    printf("CarplayAudioCtx::%s:%d end\r\n", __func__, __LINE__);
}

CarplayAudioCtx::~CarplayAudioCtx()
{
    printf("CarplayAudioCtx::%s:%d start\r\n", __func__, __LINE__);

    if (mRecordHandle) {
        delete mRecordHandle;
    }

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if (mPlayHandle) {
        delete mPlayHandle;
    }

    printf("CarplayAudioCtx::%s:%d end", __func__, __LINE__);
}

#endif
