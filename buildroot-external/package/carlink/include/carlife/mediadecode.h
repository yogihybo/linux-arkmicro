#ifndef MEDIADECODE_H
#define MEDIADECODE_H

#include "thread.h"
#include "types.h"
#include <list>
#include <vector>
#include <semaphore.h>

#include "alsa/asoundlib.h"
//using namespace std;



class MediaDecode : public Thread
{
public:

    MediaDecode(const char *device = "default");
    virtual ~MediaDecode();


    bool initDevice();
    void Release();

    //设置音频参数
    void setMediaParam(const uint rate, const uint bits, const uint channels);

    int playSound(byte *data, uint len);
    //        int playSound(Buffer &data);

    void stopPlaySound()
    {
        mStop = true;
    }
    void resumePlaySound()
    {
        mStop = false;
    }

protected:
    virtual void run();         //线程执行函数

private:

    int xrunRecover(snd_pcm_t *handle, int err);

private:

    bool mStop;
    char *mPDevice;
    snd_pcm_t *mPlaybackHandle;        //PCM设备句柄pcm.h
    MediaParam mParam;

};


#endif // MEDIADECODE_H
