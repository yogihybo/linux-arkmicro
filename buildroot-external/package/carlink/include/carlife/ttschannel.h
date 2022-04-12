#ifndef TTSCHANNEL_H
#define TTSCHANNEL_H

#include "thread.h"
#include "CCarLifeLibWrapper.h"

using namespace CommonUtilH;
using namespace CCarLifeLibH;

class MediaDecode;

class TTSChannel : public Thread
{
public:
    virtual ~TTSChannel();
    static TTSChannel *instance();

    bool soundStart() const
    {
        return mSoundStart;
    }
    void setSoundStart(bool start)
    {
        mSoundStart = start;
    }

    MediaDecode *decoder()
    {
        return mDecoder;
    }

    void SetTTSInfoCallback(void (*callback)(unsigned char*, int ,void*), void *parameter);
    void SetTTSStartCallback(void (*callback)(bool,int,int,int, int, void*), void *parameter);
protected:
    virtual void run();             //线程执行函数

private:
    TTSChannel();

    //数据接收回调函数
    static void recvInit(S_AUDIO_INIT_PARAMETER *);         //接收手机端发送给车机端的tts初始化信息
    static void recvNormalData(u8 *data, u32 len);          //接收车机端接收手机端发送的tts数据
    static void recvStop();                                 //接收手机端通知车机端tts停止状态

private:
    static TTSChannel *mInstance;

    MediaDecode *mDecoder;

    bool mSoundStart;
    static int  mValue;

    void (*m_tts_info_callback)(unsigned char*,int,void*);
    void (*m_tts_start_callback)(bool,int,int,int,int,void*);
    void  *m_parameter;
};


#endif // TTSCHANNEL_H
