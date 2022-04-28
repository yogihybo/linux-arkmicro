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
typedef std::function<void (ECCallType ,const string& , const string&)> FUNCPHONECALL;
typedef std::function<void (int,int, string, int,int, int)> FUNCINPUTSTART;
typedef std::function<void (int, int)> FUNCINPUTSELECTION;
typedef std::function<void (const string& )> FUNCPHONEINFO;
typedef std::function<void (ECVRTextType, int,string,string)> FUNCVRTEXTINFO;
typedef std::function<void (int )> FUNCCARCMD;



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
    void registerPhoneCallCallback(FUNCPHONECALL func);
    void registerInputStartCallback(FUNCINPUTSTART func);
    void registerInputSelectionCallback(FUNCINPUTSELECTION func);
    void registerPhoneInfoCallback(FUNCPHONEINFO func);
    void registerVRTextInfoCallback(FUNCVRTEXTINFO func);
    void registerCarCmdCallback(FUNCCARCMD func);

private:

   FUNCAPPSTATUS        mFuncAppStatus = NULL;
   FUNCPHONECALL        mFuncPhoneCall = NULL;
   FUNCINPUTSTART       mFuncInputStart = NULL;
   FUNCINPUTSELECTION   mFuncInputSelection = NULL;
   FUNCVRTEXTINFO       mFuncVRTextInfo = NULL;
   FUNCPHONEINFO        mFuncPhoneInfo = NULL;
   FUNCCARCMD           mFuncCarCmd = NULL;

};


#endif //APPMANAGERLISTENERIMPL_H
