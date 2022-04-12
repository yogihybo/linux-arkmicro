#ifndef VIDEOCHANEL_H
#define VIDEOCHANEL_H

#include "thread.h"
#include "types.h"
#include "CCarLifeLibWrapper.h"
#include "videodecode.h"
#include <stdio.h>

class VideoChannel : public Thread
{
public:
    VideoChannel();
    virtual ~VideoChannel();

    static VideoChannel *instance();

    //初始化视频解码参数
    void setVideoParam(const uint width, const uint height, const uint frameRate);

    //车机端显示回调注册
    void SetVideoCallback(void (*callback)(int,int ,int ,void*), void *parameter);

    void SetVideoInfoCallback(bool bControlVideo, void (*callback)(int, int ,unsigned char*, int ,void*), void *parameter);

    void ShowPlayer(bool bVisible);

    bool IsPlayerVisible() const
    {
        return m_bVisible;
    }

    uint width() const
    {
        return mParam.width;
    }

    uint height() const
    {
        return mParam.height;
    }

    uint frameRate() const
    {
        return mParam.frameRate;
    }

    bool recvData() const
    {
        return mRecvData;
    }
    void setRecvData(bool recv)
    {
        mRecvData = recv;
    }

    VideoDecode *decoder()
    {
        return m_pVideoDecode;
    }

    VideoParam GetVideoParam() const
    {
         return  mParam;
    }

protected:
    virtual void run();         //线程执行函数


private:
    //数据接收回调函数
    static void recvData(u8 *data, u32 len);        //接收视频数据
    static void recvHeartBeat();                    //接收心跳包
    //视频显示通知
    static void video_callback_func(int ready, void* parameter);
    static void video_info_callback_func(int width,int height, unsigned char* data, int length, void* parameter);
private:
    static VideoChannel *mInstance;

    VideoParam mParam;

    bool mRecvData;

    VideoDecode *m_pVideoDecode;

    bool m_bVisible;
    bool mbControlVideo;

    int m_screen_width;
    int m_screen_height;

    void (*m_video_callback)(int, int, int, void *);
    void (*m_video_info_callback)(int,int,unsigned char*,int,void*);
    void *m_parameter;
};

#endif // VIDEOCHANEL_H
