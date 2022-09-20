

#ifndef TOOLKITLISTENERIMPL_H
#define TOOLKITLISTENERIMPL_H

#include "ECSDKToolKit.h"

using namespace ECSDKFrameWork;

/*
 * 用户实现：工具类
 * onQueryTime：时间查询
 * onQueryGPS：GPS信息
 * onBulkDataReceived：保留函数
 * */
class ToolKitListener : public IECToolKitListener
{
public:
    ToolKitListener();
    virtual ~ToolKitListener();

    virtual void        onQueryTime(uint64_t gmtTime, uint64_t localTime, const string& timeZone, const string& dateTime) override;
    virtual void        onQueryGPS(bool status, const ECGPSInfo& info) override;
    virtual void        onBulkDataReceived(const void *data, uint32_t length) override;
    virtual void		onGenerateQRCodeUrl(const string& url, const uint8_t* data, const uint32_t dataLen) override;
    void SetQRCallback(void(*callback)(string, void*), void* parameter);

    void    (*m_qr_callback)(string,void *) = NULL;
    void*     m_parameter;
};


#endif //TOOLKITLISTENERIMPL_H
