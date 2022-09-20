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
    mEntityScreen = 2;
    mEntityPhoneCall = -1;
    mSpeechMode = 0;
    mEntityAudio = 0;
    mEntityByTurn = 0;
    mHandle->mInterfaceFrame = 0;
    if(mHandle)
        mHandle->onSdkConnectStatus(CONNECT_STATUS_DISCONNECTED,UnKnown);
}

void CarplayLinkCbsImpl::returnNativeUICB()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->set_background();
        mHandle->app_status(APP_BACKGROUND, (void*)true);

    }

}

void CarplayLinkCbsImpl::modesChangeCB(CarPlayModeState *modes)
{    
    if(mHandle){
        if(mEntityScreen != modes->screen){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("screen mode = %d\r\n",modes->screen);
            if(modes->screen == CarplayEntity_Controller)
                mHandle->app_status(APP_FOREGROUND, nullptr);
            else if(modes->screen == CarplayEntity_Accessory)
                mHandle->app_status(APP_BACKGROUND, (void*)true);
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

        if(mSpeechMode != modes->speech.entity){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("speech  mode = %d\r\n",modes->speech);
                if(modes->speech.entity == 1)
                    mHandle->app_status(APP_RECOGNITION_STARTED, nullptr);
                else
                    mHandle->app_status(APP_RECOGNITION_STOPPED, nullptr);

            mSpeechMode = modes->speech.entity;
        }

        if(mEntityAudio != modes->mainAudio){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("audio  mode = %d\r\n",modes->mainAudio);
                if(modes->mainAudio == 1)
                    mHandle->app_status(APP_MUSIC_STARTED, nullptr);
                else
                    mHandle->app_status(APP_MUSIC_STOPPED, nullptr);


            mEntityAudio = modes->mainAudio;
        }

        if(mEntityByTurn != modes->turnByTurn){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("turnByTurn  mode = %d\r\n",modes->turnByTurn);
                if(modes->turnByTurn == 1)
                    mHandle->app_status(APP_NAVI_STARTED, nullptr);
                else
                    mHandle->app_status(APP_NAVI_STOPPED, nullptr);

            mEntityByTurn = modes->turnByTurn;
        }
    }
}

void CarplayLinkCbsImpl::disableBluetoothCB()
{
    printf("disableBluetoothCB\n");

    mHandle->app_status(APP_BT_DISCONNECTED);
}

void CarplayLinkCbsImpl::caplayDuckAudioCB(double inDurationSecs, double inVolume)
{    
    static DuckInfo info = {0, 0};
    info.inDurationSecs = inDurationSecs;
    info.volume = inVolume;
    mHandle->app_status(APP_VOICE_DUCK, (void*)&info);
    printf("caplayDuckAudioCB inDurationSecs = %lf, inVolume = %lf\r\n", inDurationSecs, inVolume);
}

void CarplayLinkCbsImpl::caplayUnduckAudioCB(double inDurationSecs)
{
    static double sec = 0;
    sec = inDurationSecs;
    mHandle->app_status(APP_VOICE_UNDUCK, (void*)&sec);
    printf("caplayUnduckAudioCB inDurationSecs = %lf\r\n", inDurationSecs);
}

#endif
