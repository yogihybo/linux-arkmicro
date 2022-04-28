#include "CarplayLinkCbsImpl.h"
#include "CarplayLink.h"
#ifdef USE_CARPLAY
void CarplayLinkCbsImpl::iap2LinkStatus(int status)
{
    (void)status;
}

int CarplayLinkCbsImpl::iap2WriteData(char *buf, int len)
{
    (void)buf;
    (void)len;
    return 0;
}

void CarplayLinkCbsImpl::carplaySessionStart()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

}

void CarplayLinkCbsImpl::carplaySessionStop()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

int CarplayLinkCbsImpl::switchUsbModeCB(UsbMode mode)
{
    (void)mode;
    return 0;
}



void CarplayLinkCbsImpl::appleTimeUpdateCB(long long time, int zone_offset)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("time = %lld, zone_offset = %d\r\n", time, zone_offset);
    if(mHandle)
        mHandle->setLocalTime(time);
}

void CarplayLinkCbsImpl::appleLanguageUpdateCB(const char *lang)
{

}

void CarplayLinkCbsImpl::NotifyDeviceNameCB(const char *name, int name_len)
{

}

void CarplayLinkCbsImpl::carplayExitCB()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle)
        mHandle->onSdkConnectStatus(CONNECT_STATUS_DISCONNECTED,UnKnown);
}

void CarplayLinkCbsImpl::returnNativeUICB()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mEntityScreen = 2;
    if(mHandle)
        mHandle->app_status(APP_BACKGROUND, nullptr);
}

void CarplayLinkCbsImpl::modesChangeCB(CarPlayModeState *modes)
{    
    if(mHandle){
        if(mEntityScreen != modes->screen){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("screen mode = %d\r\n",modes->screen);
            if(modes->screen == 1)
                mHandle->app_status(APP_FOREGROUND, nullptr);
            else if(modes->screen == 2)
                mHandle->app_status(APP_BACKGROUND, nullptr);
            mEntityScreen = modes->screen;
        }
        if(mEntityPhoneCall != modes->phoneCall){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("phone call mode = %d\r\n",modes->phoneCall);
                if(modes->phoneCall == 1)
                    mHandle->app_status(APP_PHONE_STARTED, nullptr);
                else
                    mHandle->app_status(APP_PHONE_STOPPED, nullptr);
            mEntityPhoneCall = modes->phoneCall;
        }
    }
}

void CarplayLinkCbsImpl::disableBluetoothCB()
{

}

void CarplayLinkCbsImpl::caplayDuckAudioCB(double inDurationSecs, double inVolume)
{

}

void CarplayLinkCbsImpl::caplayUnduckAudioCB(double inDurationSecs)
{

}

#endif
