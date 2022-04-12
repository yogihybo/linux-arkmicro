#ifndef SDKLISTENER_H
#define SDKLISTENER_H

#include "ECSDKFramework.h"

using namespace ECSDKFrameWork;

//#define CMD_SUCCESS 0x1001
/*
 * sdk 监听类。
 * sdk 通过以下回调函数将信息传递给实现者
 * onSdkConnectStatus：监听投屏连接状态
 * onSdkConnectType：取得连接类型
 * onLicenseAuthFail：授权失败
 * onLicenseAuthSuccess：授权成功
 * */
#include <functional>

typedef std::function<void (int)> FUNCSTATUS;

class SDKListener : public IECSDKListener
{
public:
    SDKListener();
    virtual ~SDKListener();

    virtual void    onSdkConnectStatus(ECSDKConnectedStatus status, ECSDKConnectedType type) override;
    virtual void    onLicenseAuthFail(int32_t errCode, const string& errMsg) override;
    virtual	void    onLicenseAuthSuccess(int code, const string& msg) override;

public:
    void registerStatusCallback(FUNCSTATUS func);

    void SetLinkPhoneTypeCallback(void(*callback)(int, void*), void* parameter);
    void SetLinkstatusCallback(void(*callback)(int, void*), void* parameter);
    void SetLicenseAuthSuccessCallback(void(*callback)(int, string, void*), void* parameter);
    void SetLicenseAuthFailCallback(void(*callback)(int, string, void*), void* parameter);

    void    (*m_link_phonetype_callback)(int, void *) = nullptr;
    void    (*m_link_status_callback)(int, void *) = nullptr;
    void    (*m_license_auth_success_callback)(int, string,void *) = nullptr;
    void    (*m_license_auth_fail_callback)(int, string,void *) = nullptr;
    void*     m_parameter;

    FUNCSTATUS      mFuncStatus = nullptr;

};


#endif //SDKLISTENER_H
