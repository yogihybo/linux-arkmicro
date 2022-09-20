
#ifndef OTALISTENER_H
#define OTALISTENER_H

#include "ECSDKOTAManager.h"

using namespace ECSDKFrameWork;

/*
 * 用户实现：OTA升级类
 * onOTAUpdateCheckResult：OTA升级检查结果
 * onOTAUpdateProgress：下载进度
 * onOTAUpdateCompleted：升级完成
 * onOTAUpdateError：升级错误信息
 * */
class OTAListener : public IECOTAManagerListener
{
public:
    OTAListener();
    virtual ~OTAListener();

    virtual void        onOTAUpdateCheckResult(const vector<ECOTAUpdateSoftware>& downloadableSoftwares, const vector<ECOTAUpdateSoftware>& downloadedSoftwares) override;
    virtual void        onOTAUpdateRequestDownload(const vector<ECOTAUpdateSoftware>& downloadableSoftwares) override;
    virtual void        onOTAUpdateProgress(const string& downloadingSoftwareId, float progress, uint32_t softwareLeftTime, uint32_t otaLeftTime) override;
    virtual void        onOTAUpdateCompleted(const string& downloadedSoftwareId, const string& md5Path, const string& packagePath, const string& iconPath, uint32_t leftSoftwareNum) override;
    virtual void        onOTAUpdateError(int32_t errCode, const string& softwareId) override;

public:
    void SetOTAUpdateProgress(void(*callback)(string, float, int ,int, void*), void* parameter);
    void SetOTAUpdateCompeleted(void(*callback)(string, string, string ,string, int, void*), void* parameter);
    void SetOTAUpdateError(void(*callback)(int, string, void*), void* parameter);
private:
    void        (*m_otaprogress_callback)(string, float, int ,int, void*) = nullptr;
    void        (*m_otacompeletedcallback)(string, string, string ,string, int, void*) = nullptr;
    void        (*m_otaerrorcallback)(int, string, void*) = nullptr;
    void*       m_parameter;
};


#endif //OTALISTENER_H
