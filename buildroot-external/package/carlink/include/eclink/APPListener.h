#ifndef APPMANAGERLISTENERIMPL_H
#define APPMANAGERLISTENERIMPL_H

#include "ECSDKAPPManager.h"
#include <functional>
using namespace ECSDKFrameWork;

/*
 * 用户实现：手机app信息管理类
 * 通过以下回调函数，车机端能获得互联中的各种状态
 *
 * onECStatusMessage：互联状态
 * onPhoneAppHUD：导航数据信息
 * onPhoneAppInfo：手机信息
 * */

#define APP_INPUTSTART 100
#define APP_INPUTSTOP  101

typedef std::function<void (ECStatusMessage)> FUNCAPPSTATUS;

class APPListener : public IECAPPManagerListener
{
public:
    APPListener();
    virtual ~APPListener();

    virtual void onECStatusMessage(ECStatusMessage status) override;

    virtual void onPhoneAppHUD(const ECNavigationHudInfo& data) override;

    virtual void onPhoneAppInfo(const string& info) override;

    virtual void onCallAction(ECCallType type, const string& name, const string& number) override;

	virtual void onCarCmdNotified(const ECCarCmd& carCmd) override;

	virtual void onInputStart(const ECInputInfo& info) override;


	virtual void onInputCancel() override;


	virtual void onInputSelection(int start, int stop) override;


	virtual void onInputText(const char* text) override;


	virtual void onVRTextReceived(const ECVRTextInfo& info) override;


	virtual void onPageListReceived(const vector<ECPageInfo>& pages) override;


	virtual void onPageIconReceived(const vector<ECIconInfo> icons) override;


	virtual void onWeatherReceived(const string& data) override;


	virtual void onVRTipsReceived(const string& data) override;
public:
    void registerAppStatusCallback(FUNCAPPSTATUS func);

    void SetPhoneCallCallback(void (*callback)(int ,string ,string ,void*), void *parameter);
    void SetStatusCallback(void (*callback)(int,void*), void *parameter);
    void SetInputStartCallback(void (*callback)(int,int, string, int,int, int,void*), void *parameter);
    void SetInputSelectionCallback(void (*callback)(int,int,void*), void *parameter);
    void SetPhoneInfoCallCallback(void (*callback)(string,void*), void *parameter);
private:
    void      (*m_phonenumber_callback)(int, string ,string ,void*);
    void      (*m_status_callback)(int,void*);
    void      (*m_input_start_callback)(int, int,string,int,int,int,void*);
    void      (*m_input_selection_callback)(int,int,void*);
    void      (*m_phoneinfo_callback)(string,void*);

   FUNCAPPSTATUS  mFuncAppStatus;
    void*         m_parameter;
};


#endif //APPMANAGERLISTENERIMPL_H
