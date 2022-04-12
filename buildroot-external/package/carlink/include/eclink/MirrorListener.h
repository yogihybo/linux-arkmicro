
#ifndef ECSDKFRAMEWORKTEST_MIRRORLISTENER_H
#define ECSDKFRAMEWORKTEST_MIRRORLISTENER_H

#include "ECSDKMirrorManager.h"


using namespace ECSDKFrameWork;

/*
 * 用户实现：投屏状态监听类
 * onMirrorStatus：sdk framework 会通过 status 把投屏状态通过此回调函数传递出来。用户根据需要处理不同的状态
 * */
class MirrorListener : public IECMirrorManagerListener
{
public:
    void SetMirrorDirCallback(void(*callback)(int, void*), void* parameter);
    void SetMirrorStatusCallback(void(*callback)(int, void*), void* parameter);

    void onMirrorStatus(ECSDKMirrorStatus status);
    void onMirrorInfoChanged(ECVideoInfo info);

    void    (*m_direction_callback)(int,void *) = NULL;
    void    (*m_mirror_status_callback)(int,void *) = NULL;
    void*     m_parameter;
};


#endif //ECSDKFRAMEWORKTEST_MIRRORLISTENER_H
