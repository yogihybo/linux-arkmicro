#ifndef LINKASSIST_H
#define LINKASSIST_H
#ifdef __cplusplus
#include "IUserLinkPlayer.h"
#include <vector>
using namespace std;

class UsbHostService;
class AudioDecoder;
class CarlifeLink;
class CarplayLink;
class HiCarLink;
class EasyConnectLink;
class MirrorLink;
class UsbStateWatcher;

typedef function<void (ConnectedStatus, PhoneType)> FUNCUSBCALLBACK;


class LinkAssist
{
public:
    LinkAssist();
    virtual ~LinkAssist();

public:
    void RegisterUsbCallback(FUNCUSBCALLBACK funcUSBCallback);

    LinkConfig GetConfigInfo() {return mLinkConfig;}

    CarplayConfig GetCarplayInfo() {return mCarplayConfig;}

    CarlifeConfig GetCarlifeInfo() {return mCarlifeConfig;}

    void onSdkConnectStatus(int status, int type);

    IUserLinkPlayer* Initialize(LinkType linkType);

    void Release();

private:
    void ReadConfig();
private:
    UsbHostService  *m_pUsbHost;
    UsbStateWatcher *m_pUsbStateWatcher;

    vector<IUserLinkPlayer *> mpIULPlayerVector;
    FUNCUSBCALLBACK  mFuncUSBCallback;
    LinkConfig       mLinkConfig;
    CarlifeConfig    mCarlifeConfig = {1, 0, 0};
    CarplayConfig    mCarplayConfig = {154, 87, 50, 30};

};
#endif
#endif // LINKASSIST_H
