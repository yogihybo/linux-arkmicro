#ifndef _CarplayVideoWrapper_H
#define _CarplayVideoWrapper_H

#ifdef __cplusplus
class ICarplayVideoCallbacks
{
public:
	/* *
     * @brief 视频流开始
     * */
    virtual int carplayVideoStartCB() = 0;
    /* *
     * @brief 视频流停止
     * */
    virtual void carplayVideoStopCB() = 0;
     /* *
     * @brief 视频流数据处理
     * */
    virtual int carplayVideoDataProcCB(const char *buf, int len) = 0;
};
#endif
#endif
