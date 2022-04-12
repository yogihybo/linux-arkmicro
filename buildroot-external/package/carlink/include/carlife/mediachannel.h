#ifndef MEDIACHANNEL_H
#define MEDIACHANNEL_H

#include "thread.h"
#include "CCarLifeLibWrapper.h"
#include "mediadecode.h"

using namespace CommonUtilH;
using namespace CCarLifeLibH;


class MediaDecode;

class MediaChannel : public Thread
{
public:
    virtual ~MediaChannel();
    static MediaChannel *instance();

    MediaDecode *decoder()
    {
        return mDecoder;
    }

    void SetMusicInfoCallback(void (*callback)(unsigned char*, int ,void*), void *parameter);
    void SetMusicStartCallback(void (*callback)(bool,int,int,int, int, void*), void *parameter);
protected:
    virtual void run();             //线程执行函数

private:
    MediaChannel();


    //数据接收回调函数
    static void recvInit(S_AUDIO_INIT_PARAMETER *); //接收手机端发送给车机端的media初始化信息
    static void recvNormalData(u8 *data, u32 len);  //接收车机端接收手机端发送的media数据
    static void recvPause();                        //接收手机端通知车机端media暂停状态
    static void recvResume();                       //接收手机端通知车机端media恢复播放状态

private:
    static MediaChannel *mInstance;
    MediaDecode *mDecoder;
    static int mRate;
    static int mBits;
    static int mChannels;
    void (*m_music_info_callback)(unsigned char*,int,void*);
    void (*m_music_start_callback)(bool,int,int,int,int,void*);
    void *m_parameter;

    bool mInitPlayer;
};


#endif // MEDIACHANNEL_H
