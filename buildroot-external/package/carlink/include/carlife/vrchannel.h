#ifndef VRCHANNEL_H
#define VRCHANNEL_H

#include "thread.h"
#include "mediadecode.h"
#include "CCarLifeLibWrapper.h"

using namespace CommonUtilH;
using namespace CCarLifeLibH;


class VRChannel : public Thread
{
public:
    virtual ~VRChannel();
    static VRChannel *instance();

    MediaDecode *decoder()
    {
        return mDecoder;
    }

     void SetVRInfoCallback(void (*callback)(unsigned char*, int ,void*), void *parameter);
     void SetVRStartCallback(void (*callback)(bool,int,int,int, int, void*), void *parameter);
protected:
    virtual void run();

private:
    VRChannel();

    //数据接收回调函数
    static void recvInit(S_AUDIO_INIT_PARAMETER *);         //接收手机端发送给车机端的vr初始化信息
    static void recvNormalData(u8 *data, u32 len);          //接收车机端接收手机端发送的vr数据
    static void recvStop();                                 //接收手机端通知车机端vr停止状态

private:
    static VRChannel *mInstance;

    MediaDecode *mDecoder;
    void (*m_vr_info_callback)(unsigned char*,int,void*);
    void (*m_vr_start_callback)(bool,int,int,int,int,void*);
    void *m_parameter;
};

#endif // VRCHANNEL_H
