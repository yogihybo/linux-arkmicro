#include "CarplayLink.h"
#include "CarplayLinkCbsImpl.h"
#include "CarplayAudioCtx.h"

#ifdef USE_CARPLAY

int ICarplayVideoCallbacksImpl::carplayVideoStartCB()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->video_start(0, 0, mHandle->getLinkConfig().screen_width, mHandle->getLinkConfig().screen_height);
        mHandle->onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED, mHandle->getPhoneType());
    }
    return 0;
}

void ICarplayVideoCallbacksImpl::carplayVideoStopCB()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle)
        mHandle->video_stop();
}

int ICarplayVideoCallbacksImpl::carplayVideoDataProcCB(const char *buf, int len)
{
    //printf("%s:%s:%d len:%d\r\n",__FILE__,__func__,__LINE__, len);
    if(mHandle)
        mHandle->video_play(buf, len);
    return 0;
}

void ICarplayAudioCallbacksImpl::carplayAudioStartCB(int handle, AudioStreamType type, int rate, int bits, int channels)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    if (type == AudioStreamRec)
        return;

    printf("type:%d, rate:%d, bits:%d, channels:%d\n",type,rate, bits, channels);

    if (type == AudioStreamRECOGNITION || type == AudioStreamCall) {
        if(mHandle){
            AudioInfo info = {rate, channels,bits };
            mHandle->record_start(info);
            if(type == AudioStreamRECOGNITION){
                mHandle->app_status(APP_RECOGNITION_STARTED);
            }
            else if(type == AudioStreamCall){
                mHandle->app_status(APP_PHONE_STARTED);
            }
        }
        //sendAudioMsg(mHandle, true, ChannelAudioIn, type, rate, bits, channels);
    }

    mHandle->audio_start((AudioType)type,rate, bits, channels);

    CarplayAudioCtx* audioHandle = new CarplayAudioCtx(mHandle, handle, type, rate, bits, channels);
    //sendAudioMsg(mHandle, true, ChannelAudioOut, type, rate, bits, channels);


    Autolock l(&mLock);
    mAudioHandlList.push_back(audioHandle);

}

void ICarplayAudioCallbacksImpl::carplayAudioStopCB(int handle, AudioStreamType type)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    if (type == AudioStreamRec)
        return;
    if (type == AudioStreamRECOGNITION || type == AudioStreamCall) {
        if(mHandle){
            mHandle->record_stop();
        }
        if(type == AudioStreamRECOGNITION){
            mHandle->app_status(APP_RECOGNITION_STOPPED);
        }
        else if(type == AudioStreamCall){
            mHandle->app_status(APP_PHONE_STOPPED);
        }
    }

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    std::list<CarplayAudioCtx *>::iterator itor;
    CarplayAudioCtx *tmpAudioHandle = NULL;
    bool isFound = false;

    Autolock l(&mLock);
    for (itor = mAudioHandlList.begin(); itor != mAudioHandlList.end();) {
        tmpAudioHandle = *itor;
        if (tmpAudioHandle->getStreamHandle() == handle) {
            isFound = true;
            mAudioHandlList.erase(itor);
            break;
        } else {
            itor++;
        }
    }

    if (isFound && tmpAudioHandle)
        delete tmpAudioHandle;

    mHandle->audio_stop((AudioType)type);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}
#endif

CarplayLink::CarplayLink()
{
#ifdef USE_CARPLAY
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mHandle = new CarplayWrapper();
    mVideoCbs = new ICarplayVideoCallbacksImpl(this);
    mAudioCbs = new ICarplayAudioCallbacksImpl(this);
    mCbs = new CarplayLinkCbsImpl(this);
    mDefaultWifi = true;
    mDdefaultPhonebtMac = true;
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#endif
}

CarplayLink::~CarplayLink()
{
#ifdef USE_CARPLAY
    delete mHandle;
    delete mVideoCbs;
    delete mAudioCbs;
    delete mCbs;
#endif
}

#ifdef USE_CARPLAY
bool CarplayLink::init(LinkMode linkMode)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    printf("phone_bt_mac = %s\r\n",mLinkConfig.phone_bt_mac.c_str());
    if(!mLinkConfig.phone_bt_mac.empty()){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(mDdefaultPhonebtMac){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mDdefaultPhonebtMac = true;
            mHandle->CarplaySetStringParameter("REMOTE_BTMAC", mLinkConfig.phone_bt_mac);
        }
    }
    printf("car_wifi_ssid = %s\r\n",mLinkConfig.car_wifi_ssid.c_str());

    if(!(mLinkConfig.car_wifi_ssid.empty()| mLinkConfig.car_wifi_passphrase.empty() | mLinkConfig.car_wifi_channel.empty())){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(mDefaultWifi){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mDefaultWifi = true;
            mHandle->CarplaySetStringParameter("WIFI_SSID", mLinkConfig.car_wifi_ssid);
            mHandle->CarplaySetStringParameter("WIFI_PASSWD",mLinkConfig.car_wifi_passphrase);
            mHandle->CarplaySetIntParameter("WIFI_CHANNEL",stoi(mLinkConfig.car_wifi_channel));
        }
    }

    mLinkMode = linkMode;
    mHandle->CarplaySetIntParameter("NEED_AUTH", 1);

    mHandle->CarplaySetStringParameter("NAME", "CarPlay System");
    mHandle->CarplaySetStringParameter("MODEID", "Linux");
    mHandle->CarplaySetStringParameter("MANFACTURER", "jc");
    mHandle->CarplaySetStringParameter("SERIALNUMBER", "223000ac3987cf");
    mHandle->CarplaySetStringParameter("SW_VER", "sw1.1.0");
    mHandle->CarplaySetStringParameter("HW_VER", "hw1.1.0");
    mHandle->CarplaySetStringParameter("VEHICLE_NAME", "MZD");

    mHandle->CarplaySetIntParameter("OEM_ICON_VISIBLE", 1);
    mHandle->CarplaySetStringParameter("OEM_ICON_LABEL", "Home");
    mHandle->CarplaySetStringParameter("OEM_ICON_PATH", "/etc/icon_120x120.png");

    mHandle->CarplaySetStringParameter("OSINFO", "Android7.0");
    mHandle->CarplaySetStringParameter("IOSVER_MIN", "11D257");
    mHandle->CarplaySetStringParameter("GUUID", "a527cd2d-7a0f-4165-674b-579f625a160d");
    mHandle->CarplaySetStringParameter("DEVID", "00:11:22:33:44:55");
    mHandle->CarplaySetStringParameter("BTMAC", "00:11:22:33:44:55");
    mHandle->CarplaySetIntParameter("RIGHTHAND_DRIVER", 0);
    mHandle->CarplaySetIntParameter("NIGHTMODE", 0);
    mHandle->CarplaySetIntParameter("HAVE_TELBUTTON", 1);
    mHandle->CarplaySetIntParameter("HAVE_MEDIABUTTON", 1);
    mHandle->CarplaySetIntParameter("HAVE_KNOB", 1);
    mHandle->CarplaySetIntParameter("HAVE_PROXSENSOR", 0);
    mHandle->CarplaySetIntParameter("HAVE_ENHANCED_REQCARUI", 0);
    mHandle->CarplaySetIntParameter("HAVE_ETCSUPPORTED", 0);
    mHandle->CarplaySetIntParameter("HIFI_TOUCH", 1);
    mHandle->CarplaySetIntParameter("LOFI_TOUCH", 0);
    mHandle->CarplaySetIntParameter("USB_COUNRY_CODE", 33);
    mHandle->CarplaySetIntParameter("PRODUCT_CODE", 0xa4a1);
    mHandle->CarplaySetIntParameter("VERNDOR_CODE", 0x0525);

    mHandle->CarplaySetIntParameter("I2C_NUM", 0);
    mHandle->CarplaySetIntParameter("I2C_ADDR", 0x22);


    mHandle->CarplaySetIntParameter("VIDEO_WIDTH", mLinkConfig.screen_width);
    mHandle->CarplaySetIntParameter("VIDEO_HEIGHT", mLinkConfig.screen_height);
    mHandle->CarplaySetIntParameter("SCREEN_WIDTH_MM", mLinkConfig.screen_physical_width);
    mHandle->CarplaySetIntParameter("SCREEN_HEIGHT_MM", mLinkConfig.screen_physical_height);
    mHandle->CarplaySetIntParameter("FPS", 30);
    mHandle->CarplaySetIntParameter("SCREEN_WIDTH", mLinkConfig.screen_width);
    mHandle->CarplaySetIntParameter("SCREEN_HEIGHT", mLinkConfig.screen_height);
/*
    mHandle->CarplaySetStringParameter("WIFI_SSID", "CAR-WiFi_49ac");
    mHandle->CarplaySetStringParameter("WIFI_PASSWD","49accb62");
    mHandle->CarplaySetIntParameter("WIFI_CHANNEL",11);
*/
    //mHandle->CarplaySetStringParameter("IAP_PATH", "/dev/iap");

    static bool flags = false;
    if(!flags){
        mHandle->init();  
        mHandle->registerCallbacks(mCbs);
        mHandle->registerVideoCallbacks(mVideoCbs);
        mHandle->registerAudioCallbacks(mAudioCbs);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        flags = true;
    }
    return true;
}

bool CarplayLink::release()
{
    if(mHandle){
        mHandle->CarplayStop();
    }
    return true;
}

bool CarplayLink::start()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        if(mLinkMode == Wired)
            mHandle->CarplaySetIntParameter("LINK_TYPE", 0);//设置成无线carplay模式 CARPLAY_WL CARPLAY
        else if(mLinkMode == Wireless){
            mHandle->CarplaySetIntParameter("LINK_TYPE", 7);
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        }
        mHandle->CarplayStart();
        onSdkConnectStatus(CONNECT_STATUS_CONNECTING, mPhoneType);
    }
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    return true;
}

bool CarplayLink::stop()
{
    if(mHandle)
        mHandle->CarplayStop();
    return true;
}

bool CarplayLink::start_mirror()
{
    return true;
}

bool CarplayLink::stop_mirror()
{
    return true;
}

bool CarplayLink::set_background()
{
    if(mHandle){
        mHandle->CarplayChangeModes(1, 500, 500, 100, 0, 0, 0, 0, 0, 0, 0);
    }
}

bool CarplayLink::set_foreground()
{
    if(mHandle){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        mHandle->CarplayRequestUI("");
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
}

bool CarplayLink::get_audio_focus()
{
    if(mHandle){
        //mHandle->CarplayChangeModes();
    }
}

bool CarplayLink::release_audio_focus()
{

}

void CarplayLink::set_inserted(bool inserted, PhoneType phoneType)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPhoneType = phoneType;
}

void CarplayLink::send_screen_size(int width, int height)
{

}

#define DUMP_PLAY_FILE 0
#if DUMP_PLAY_FILE
static FILE *pfile = NULL;
#endif

#define DUMP_REC_FILE 0
#if DUMP_REC_FILE
static FILE *pRecfile = NULL;
#endif


void CarplayLink::record_audio_callback(unsigned char *data, int len)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mRecData += string((char*)data, len);

#if DUMP_REC_FILE
    if (NULL == pRecfile) {
        pRecfile = fopen("/tmp/rec_in.pcm", "w");
    }

    if (pRecfile) {
        fwrite(data, 1, len, pRecfile);
        //printf("rec len:%d\r\n ", len);
    }
#endif

    //printf("record len = %d\r\n", len);
}

void CarplayLink::send_car_bluetooth(const string& name, const string& address, const string& pin)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void CarplayLink::send_phone_bluetooth(const string& address)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("address = %s\r\n",address.c_str());
    if(mHandle){
        mDdefaultPhonebtMac = false;
        mHandle->CarplaySetStringParameter("REMOTE_BTMAC", address);
    }
}

void CarplayLink::send_touch(int x, int y, TouchCode touchCode)
{
    printf("carplay x:%d, y:%d, press:%d\r\n",x, y, touchCode);
    if(mHandle){        
            mHandle->CarplaySendSingleTouchPoint(x, y, touchCode);
    }
}

void CarplayLink::send_car_wifi(WifiInfo& info){

    if(mHandle){
        mDefaultWifi = false;
        mHandle->CarplaySetStringParameter("WIFI_SSID", info.ssid);
        mHandle->CarplaySetStringParameter("WIFI_PASSWD",info.passphrase);
        mHandle->CarplaySetIntParameter("WIFI_CHANNEL",stoi(info.channel_id));
    }
}

bool CarplayLink::send_key(KeyCode keyCode)
{
    if(mHandle){        
        if(keyCode == KEY_MUSIC_PLAY){
            mHandle->CarplaySendMediaKey(MEDIA_PLAY, true);
            mHandle->CarplaySendMediaKey(MEDIA_PLAY, false);
        }
        else if(keyCode == KEY_MUSIC_PAUSE){
            mHandle->CarplaySendMediaKey(MEDIA_PAUSE, true);
            mHandle->CarplaySendMediaKey(MEDIA_PLAY, false);
        }
        else if(keyCode == KEY_MUSIC_PLAY_PAUSE){
            mHandle->CarplaySendMediaKey(MEDIA_PLAY_PAUSE, true);
            mHandle->CarplaySendMediaKey(MEDIA_PLAY_PAUSE, false);
        }
        else if(keyCode == KEY_MUSIC_NEXT){
            mHandle->CarplaySendMediaKey(MEDIA_NEXT, true);
            mHandle->CarplaySendMediaKey(MEDIA_NEXT, true);
            mHandle->CarplaySendMediaKey(MEDIA_NEXT, false);
        }
        else if(keyCode == KEY_MUSIC_PREVIOUS){
            mHandle->CarplaySendMediaKey(MEDIA_PREVIOUS, true);
            mHandle->CarplaySendMediaKey(MEDIA_PREVIOUS, true);
            mHandle->CarplaySendMediaKey(MEDIA_PREVIOUS, false);
        }
    }
}

bool CarplayLink::send_wheel(WheelCode wheel, bool foucs)
{
    if(mHandle){
        mHandle->CarplaySendKnob(foucs,  0, 0, 0, 0, wheel);
    }
}

bool CarplayLink::open_page(AppPage appPage)
{

}

void CarplayLink::request_status(RequestAppStatus requestAppStatus, void *reserved)
{
    if(requestAppStatus == QUERYTIME){
        static long long time = mLocalTime;
        app_status(APP_RESERVED, (void*)time);
        printf("static local time = %lld\r\n",time);
    }
}

bool CarplayLink::sendPlayData(int handle, char type, const char* buf, int len, int frames, long long time_stamp)
{
   // printf("play hanle :%x type = %d, len = %d\r\n",handle, type, len);

    audio_play((AudioType)type, buf, len);

    if(mHandle){
        mHandle->AudioPlayStream(handle, (void*)buf, len, frames, time_stamp);

#if DUMP_PLAY_FILE
    if (NULL == pfile) {
        pfile = fopen("/tmp/play_out.pcm", "w");
    }

    if (pfile) {
        fwrite(buf, 1, len, pfile);
        printf("play len:%d\r\n ", len);
    }
#endif

    }
    return true;
}

bool CarplayLink::receiveRecordData(int handle, int frames)
{
    void *tmp = NULL;
    std::lock_guard<std::mutex> lock(mMutex);
     int len = mRecData.size();
     tmp = (void*)mRecData.c_str();
     if(mHandle)
          mHandle->AudioRecordStream(handle, tmp, len, len / frames, 0);
      mRecData.clear();
    return true;
}

void CarplayLink::setLocalTime(long long local)
{
    mLocalTime = local;
    printf("mLocalTime time = %lld\r\n",mLocalTime);
}
#endif
