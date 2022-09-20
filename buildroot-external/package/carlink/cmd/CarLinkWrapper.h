#ifndef CARLINKWRAPPER_H
#define CARLINKWRAPPER_H

#include "CarLinkPlayer.h"
//#include "LinkBase.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void* GetInstance();

extern void  InitCarLinkPlayer(void *handle);

extern void RegisterUsbState(void *handle, void(*pUsbStateCallback)(int,int));

extern void RegisterAppStatus(void *handle, void(*pAppStatusCallback)(int,void*));

extern void Start(void *handle, int linkType, int linkMode);

extern void ReleaseInstance(void *handle);

extern void SetBackground(void *handle);

extern void SetForeground(void *handle);

extern void Touch(void *handle, int x, int y, int pressed);

extern void Key(void *handle, int key);

extern void OpenPage(void *handle, int appPage);

extern void SendCarBluetooth(void *handle, char* name, char* address, char* pin);

extern void SendPhoneBluetooth(void *handle, char* address);

extern void SendIphoneMacAddress(void *handle, char* address);

extern void SendCarWifi(void *handle, char* ssid, char* passphrase, char* channel_id);

#ifdef __cplusplus
}
#endif



#endif
