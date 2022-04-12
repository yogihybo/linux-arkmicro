#ifndef LINKASSIST_H
#define LINKASSIST_H

#include "IUserLinkPlayer.h"
#include <vector>
using namespace std;

class UsbHostService;
class AudioDecoder;
class CarlifeLink;
class CarplayLink;
class EasyConnectLink;
class MirrorLink;

typedef std::function<void (ConnectedStatus, PhoneType)> FUNCUSBCALLBACK;


class LinkAssist
{
public:
    LinkAssist();
    virtual ~LinkAssist();

public:
    void RegisterUsbCallback(FUNCUSBCALLBACK funcUSBCallback);

    LinkConfig GetConfigInfo() {return mLinkConfig;}

    void onSdkConnectStatus(ConnectedStatus status, PhoneType type);

    IUserLinkPlayer* Initialize(LinkType linkType);

    void Release();

private:
    void ReadConfig();
private:
    UsbHostService  *m_pUsbHost;

    vector<IUserLinkPlayer *> mpIULPlayerVector;
    FUNCUSBCALLBACK  mFuncUSBCallback;
    LinkConfig       mLinkConfig;
    CarlifeConfig    mCarlifeConfig = {1, 0, 0};

};

#endif // LINKASSIST_H
