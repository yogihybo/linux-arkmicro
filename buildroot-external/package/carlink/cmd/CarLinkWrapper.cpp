#ifndef CARLINKWRAPPER_H
#define CARLINKWRAPPER_H
#include "CarLinkWrapper.h"
#include "CarLinkPlayer.h"
#include "LinkBase.h"

#ifdef __cplusplus
extern "C" {
#endif
void* GetInstance()
{
    return new CarLinkPlayer();
}

void  InitCarLinkPlayer(void *handle)
{
    static_cast<CarLinkPlayer *>(handle)->initialize();
}

void RegisterUsbState(void *handle, void(*pUsbStateCallback)(int,int))
{
    static_cast<CarLinkPlayer *>(handle)->registerUsbState(pUsbStateCallback);
}

void RegisterAppStatus(void *handle, void(*pAppStatusCallback)(int,void*))
{
    static_cast<CarLinkPlayer *>(handle)->registerAppStatus(pAppStatusCallback);
}

void Start(void *handle, int linkType, int linkMode)
{
    static_cast<CarLinkPlayer *>(handle)->Start(LinkType(linkType), (LinkMode)linkMode);
}

void ReleaseInstance(void *handle)
{
    static_cast<CarLinkPlayer *>(handle)->Stop();
}

void SetBackground(void *handle)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SetBackground();
}

void SetForeground(void *handle)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SetForeground();
}

void Touch(void *handle, int x, int y, int pressed)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SendTouch(x, y, (TouchCode)pressed);
}

void Key(void *handle, int key)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SendKey((KeyCode)key);
}

void Wheel(void *handle, int wheelCode, bool bFoucs)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SendWheel((WheelCode)wheelCode, bFoucs);
}

void OpenPage(void *handle, int appPage)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->OpenPage((AppPage)appPage);
}

void SendCarBluetooth(void *handle,char* name, char* address, char* pin)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SendCarBluetooth(name, address, pin);
}

void SendPhoneBluetooth(void *handle, char* address)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SendPhoneBluetooth(address);
}

void SendIphoneMacAddress(void *handle, char* address)
{
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SendIphoneMacAddress(address);
}

void SendCarWifi(void *handle, char* ssid, char* passphrase, char* channel_id)
{
    WifiInfo info = {ssid, passphrase, channel_id};
    static_cast<CarLinkPlayer *>(handle)->UIPlayer()->SendCarWifi(info);
}

#ifdef __cplusplus
}
#endif


#endif
