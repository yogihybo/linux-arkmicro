#include "CarplayLink.h"
#include "CarplayLinkCbsImpl.h"
#include "CarplayAudioCtx.h"
#include "WebrtcWrapper.h"
#include "carplayVideoWrapper.h"
#include "carplayAudioWrapper.h"
#include "carplayWrapper.h"
#include "BufferQueue.h"
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef USE_CARPLAY
#define FRAME_MAX 5

typedef enum
{
    MEDIA_NONE = 0,
    MEDIA_PLAY,
    MEDIA_PAUSE,
    MEDIA_PLAY_PAUSE,
    MEDIA_NEXT,
    MEDIA_PREVIOUS
}MEDIA;


int ICarplayVideoCallbacksImpl::carplayVideoStartCB()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->video_start(0, 0, mHandle->getLinkConfig().screen_width, mHandle->getLinkConfig().screen_height);
        //mHandle->onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED, mHandle->getPhoneType());
    }
    return 0;
}

void ICarplayVideoCallbacksImpl::carplayVideoStopCB()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
 //   mInterfaceFrame = 0;
    if(mHandle)
        mHandle->video_stop();
}

int ICarplayVideoCallbacksImpl::carplayVideoDataProcCB(const char *buf, int len)
{
    //printf("%s:%s:%d len:%d\r\n",__FILE__,__func__,__LINE__, len);

    if(mHandle->mInterfaceFrame == FRAME_MAX){
        mHandle->onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED, mHandle->getPhoneType());
    }
    ++mHandle->mInterfaceFrame;

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
        mHandle->mAudioStreamType = type;

        if(mHandle){
            AudioInfo info = {rate, channels,bits };
            mHandle->record_start(info);

            if(type == AudioStreamRECOGNITION){
                mHandle->app_status(APP_RECOGNITION_STARTED);
            }
            else if(type == AudioStreamCall){
#ifdef AEC_DELAY
                printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                mHandle->mAecHandle = WebRtcAecInit();
                SetWebRtcAecParam(mHandle->mAecHandle ,rate, rate, NULL);
                printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                mHandle->mDenoiseHandle = GetInstance();
                SetFrameParam(mHandle->mDenoiseHandle, rate, 10, 1);
#endif

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
#ifdef AEC_DELAY
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            WebRtcAecRelease(mHandle->mAecHandle);
            mHandle->mAecHandle = NULL;
            mHandle->mAecQueue->ClearBufferQueue();
            ReleaseInstance(mHandle->mDenoiseHandle);
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#endif
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
    mAecHandle = NULL;
    mAecQueue = new BufferQueue();
    mAudioStreamType = AudioStreamCall;
    mRecPos = 0;
    memset(mRecBuf, 0, sizeof(mRecBuf));
    mInterfaceFrame = 0;
    mHandle->init();
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
    delete mAecQueue;
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
    mHandle->CarplaySetStringParameter("MANFACTURER", "ark");
    mHandle->CarplaySetStringParameter("SERIALNUMBER", "223000ac3987cf");
    mHandle->CarplaySetStringParameter("SW_VER", "sw1.1.0");
    mHandle->CarplaySetStringParameter("HW_VER", "hw1.1.0");
    printf("vehicle name = %s\r\n",mCarplayConfig.vehicle_name.c_str());
    if(!mCarplayConfig.vehicle_name.empty())
       mHandle->CarplaySetStringParameter("VEHICLE_NAME", mCarplayConfig.vehicle_name);
    mHandle->CarplaySetIntParameter("OEM_ICON_VISIBLE", 1);
    if(!mCarplayConfig.vehicle_icon_label.empty())
        mHandle->CarplaySetStringParameter("OEM_ICON_LABEL", mCarplayConfig.vehicle_icon_label);
    if(!mCarplayConfig.vehicle_icon_path.empty())
        mHandle->CarplaySetStringParameter("OEM_ICON_PATH", mCarplayConfig.vehicle_icon_path);

    mHandle->CarplaySetStringParameter("OSINFO", "linux4.14");
    mHandle->CarplaySetStringParameter("IOSVER_MIN", "11D257");
    mHandle->CarplaySetStringParameter("GUUID", "a527cd2d-7a0f-4165-674b-579f625a160d");
    mHandle->CarplaySetStringParameter("DEVID", "00:11:22:33:44:55");
    mHandle->CarplaySetStringParameter("BTMAC", "00:11:22:33:44:55");
//    mHandle->CarplaySetIntParameter("RIGHTHAND_DRIVER", 0);
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
    mHandle->CarplaySetIntParameter("SCREEN_WIDTH_MM", mCarplayConfig.screen_physical_width);
    mHandle->CarplaySetIntParameter("SCREEN_HEIGHT_MM", mCarplayConfig.screen_physical_height);
    mHandle->CarplaySetIntParameter("FPS", 30);
    mHandle->CarplaySetIntParameter("SCREEN_WIDTH", mLinkConfig.screen_width);
    mHandle->CarplaySetIntParameter("SCREEN_HEIGHT", mLinkConfig.screen_height);
    mHandle->CarplaySetIntParameter("USB_INDEX",mLinkConfig.usb_index);
    if(mCarplayConfig.aec_delay == 0)
        mHandle->CarplaySetIntParameter("SW_AEC",0);
    else
    {
        mHandle->CarplaySetIntParameter("SW_AEC",1);
        mHandle->CarplaySetIntParameter("AEC_DELAY",mCarplayConfig.aec_delay);
    }
    mHandle->CarplaySetIntParameter("AUDIO_HANDLE_BY_PLUGIN",1);

/*
    mHandle->CarplaySetStringParameter("WIFI_SSID", "CAR-WiFi_49ac");
    mHandle->CarplaySetStringParameter("WIFI_PASSWD","49accb62");
    mHandle->CarplaySetIntParameter("WIFI_CHANNEL",11);
*/
    //mHandle->CarplaySetStringParameter("IAP_PATH", "/dev/iap");

    static bool flags = false;
    if(!flags){
//        mHandle->init();
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
    {
        mHandle->CarplayChangeModes(1, 500, 500, 100, 0, 0, 0, 0, 0, 0, 0);
        mHandle->CarplayChangeModes(0, 0, 0, 0, 1, 500, 500, 100, 0, 0, 0);
    }
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
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        mHandle->CarplayChangeModes(1, 500, 500, 100, 0, 0, 0, 0, 0, 0, 0);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        //mHandle->CarplayChangeModes(3, 500, 0, 1000, 0, 0, 0, 0, 0, 0, 0);  //goto carback
        //mHandle->CarplayChangeModes(4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);       //exit carback
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
    printf("request width = %d, height = %d\r\n", width, height);
    mLinkConfig.screen_width = width;
    mLinkConfig.screen_height = height;

}

#define DUMP_PLAY_FILE 0
#if DUMP_PLAY_FILE
static FILE *pfile = NULL;
#endif

#define DUMP_REC_FILE 1
#if DUMP_REC_FILE
static FILE *pRecfile = NULL;
#endif

#define AEC_REC_FILE 1
#if AEC_REC_FILE
static FILE *pAecRecfile = NULL;
#endif


#define TEST_DELAY 0

void CarplayLink::AudioRecordVoiceDenoisePorcess(char *buf, int len)
{
    if(!buf || (len <= 0)) {
        return;
    }
    if(mDenoiseHandle) {
        int frame_size = 16000 * 10 / 1000.0;
        int frame_count = len/(frame_size*sizeof(short));
        short *data = (short *)buf;
        int i;
        for(i=0; i<frame_count; i++) {
#ifdef AEC_DELAY
            FrameProcess(mDenoiseHandle, data, sizeof(short) * frame_size);
#endif
            data += frame_size;
        }
    }
}

void CarplayLink::record_audio_callback(unsigned char *data, int len)
{
    std::lock_guard<std::mutex> lock(mMutex);
 //   printf("recrod len = %d\r\n",len);

#if TEST_DELAY	//delay test
        struct timeval tv;
        gettimeofday(&tv,NULL);
        long prev = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

#if DUMP_REC_FILE
    if (NULL == pRecfile) {
        pRecfile = fopen("/tmp/rec_in.pcm", "w");
    }

    if (pRecfile) {
        fwrite(data, 1, len, pRecfile);
        //printf("rec len:%d\r\n ", len);
    }
#endif

#ifdef AEC_DELAY

    if(mAudioStreamType == AudioStreamCall && mAecHandle){

        len = mRecPos + len;
        int times = len / PLAYLOAD;
        int remain = len % PLAYLOAD;
        mRecPos = 0;
        static int prev = 0;
         while(times){
             memcpy(mRecBuf + prev, data + mRecPos, PLAYLOAD - prev);
             mRecPos = mRecPos + PLAYLOAD - prev;
             prev = 0;
             --times;

            int AecLen = 0;
            unsigned char * pAecBuf = NULL;
            int type = 0 ;
            if(mAecQueue->IsExitedThread(pthread_self())){
               printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
               return;
            }

            char out[PLAYLOAD] = {0,};
            if(mAecQueue->ReadQueue(&type, &pAecBuf, &AecLen) > 0){
//              printf("Aec len = %d\r\n", AecLen);
              short *farFrame = (short *)pAecBuf;
              short *nearFrame = (short *)mRecBuf;
              short *outFrame = (short *)out;
              int frameCount = (PLAYLOAD/2)/AECLEN;

              static FILE *pOldFarfile = NULL;
              if (NULL == pOldFarfile) {
                  pOldFarfile = fopen("/tmp/rec_old_far.pcm", "w");
              }
              if (pOldFarfile) {
                  fwrite(farFrame, 1, PLAYLOAD, pOldFarfile);
              }

              static FILE *pNearRecfile = NULL;
              if (NULL == pNearRecfile) {
                  pNearRecfile = fopen("/tmp/rec_old_near.pcm", "w");
              }
              if (pNearRecfile) {
                  fwrite(mRecBuf, 1, PLAYLOAD, pNearRecfile);
                  //printf("rec len:%d\r\n ", len);
              }

              for(int i=0; i<frameCount; i++)
              {
                  WebRtcAecFrameProcess(mAecHandle, (short*)farFrame, (short*)nearFrame, (short*)out, mCarplayConfig.aec_delay);
                  farFrame += AECLEN;
                  nearFrame += AECLEN;
                  outFrame += AECLEN;
              }
            }
            AudioRecordVoiceDenoisePorcess(out, PLAYLOAD);

        #if AEC_REC_FILE
            if (NULL == pAecRecfile) {
                pAecRecfile = fopen("/tmp/rec_aec.pcm", "w");
            }
            if (pAecRecfile) {
                fwrite(out, 1, PLAYLOAD, pAecRecfile);
                //printf("rec len:%d\r\n ", len);
            }
        #endif
            if(mHandle)
                 mHandle->AudioRecordStream(mRecHandle, out, PLAYLOAD, PLAYLOAD/mRecFrames, 0);
            free(pAecBuf);
            pAecBuf = NULL;
            usleep(10000);
       }
         memcpy(mRecBuf, data + mRecPos, remain);
         mRecPos = remain;
         prev = remain;
    }
    else
#endif
        mHandle->AudioRecordStream(mRecHandle, data, len, len/mRecFrames, 0);

    //printf("%s:%s:%d record len = %d\r\n",__FILE__,__func__,__LINE__,len);
#if TEST_DELAY
        struct timeval tv_now;
        gettimeofday(&tv_now,NULL);
        long now = tv_now.tv_sec * 1000 + tv_now.tv_usec / 1000;
        printf("echo cancel time:%ldms\n", now - prev);
#endif
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
   // printf("carplay x:%d, y:%d, press:%d\r\n",x, y, touchCode);
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
    printf("%s:%s:%d key = %d\r\n",__FILE__,__func__,__LINE__, keyCode);
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
        else if(keyCode == KEY_PICKUP_PHONE){
            mHandle->CarplaySendTelephoneKey(1, true);
            mHandle->CarplaySendTelephoneKey(0, false);
        }
        else if(keyCode == KEY_HANGUP_PHONE){
            mHandle->CarplaySendTelephoneKey(3, true);
            mHandle->CarplaySendTelephoneKey(0, false);
        }
        else if(keyCode == KEY_HOME){
            mHandle->CarplayRequestUI("");
        }
        else if(keyCode == KEY_PHONE){
            mHandle->CarplayRequestUI("mobilephone:");
        }
        else if(keyCode == KEY_VOICE_ASSISTANT){
            mHandle->CarplaysendSiriButton(true);
            sleep(1);
            mHandle->CarplaysendSiriButton(false);
        }
        else if(keyCode == KEY_LIGHTMODE){
            mHandle->CarplaySendNightMode(0);
        }
        else if(keyCode == KEY_NIGHTMODE){
            mHandle->CarplaySendNightMode(1);
        }
        else if(keyCode == KEY_CAR_FOREGROUND){
            set_foreground();
        }
        else if(keyCode == KEY_CAR_BACKGROUND){
            set_background();
        }
        else if(keyCode == KEY_CAR_BACK){
            mHandle->CarplaySendKnob(0,  0, 1, 0, 0, 0);
            mHandle->CarplaySendKnob(0,  0, 0, 0, 0, 0);
        }
    }
    return true;
}

bool CarplayLink::send_wheel(WheelCode wheel, bool foucs)
{
    if(mHandle){        
        mHandle->CarplaySendKnob(foucs,  0, 0, 0, 0, wheel);
    }
    return true;
}

bool CarplayLink::send_night_mode(bool night)
{
    if(mHandle){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        mHandle->CarplaySetIntParameter("NIGHTMODE", night);
    }
    return true;
}

bool CarplayLink::send_right_hand_driver(bool right){
    if(mHandle){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        mHandle->CarplaySetIntParameter("RIGHTHAND_DRIVER", right);
    }
}

bool CarplayLink::open_page(AppPage appPage)
{
    if(mHandle == NULL)
        return false;

    if(appPage == APP_PAGE_NAVIGATION){
        mHandle->CarplayRequestUI("maps:");
    }
    else if(appPage == APP_PAGE_MAIN){
        mHandle->CarplayRequestUI("");
    }
    return true;
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
//    printf("play hanle :%x type = %d, len = %d, time_stamp = %d\r\n",handle, type, len, time_stamp);

#if TEST_DELAY	//delay test
    struct timeval tv;
    gettimeofday(&tv,NULL);
    long prev = tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif

    if(mHandle){
        mHandle->AudioPlayStream(handle, (void*)buf, len, frames, time_stamp);

     audio_play((AudioType)type, buf, len);


#ifdef AEC_DELAY
    if(mAecQueue && mAecHandle && type == AudioStreamCall){ 

        uint8_t *p = (uint8_t*)malloc(len);
        memcpy(p, buf, len);
        mAecQueue->WriteQueue(0,  p, len);
 //       printf("insert p = %d\r\n", len);
    }
#endif
    usleep(10);

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

#if TEST_DELAY
    struct timeval tv_now;
    gettimeofday(&tv_now,NULL);
    long now = tv_now.tv_sec * 1000 + tv_now.tv_usec / 1000;
    printf("play time:%ldms\n", now - prev);
#endif
    return true;
}

bool CarplayLink::receiveRecordData(int handle, int frames)
{
    mRecHandle =  handle;
    mRecFrames = frames;
    usleep(100000);

    return true;
}

void CarplayLink::setLocalTime(long long local)
{
    mLocalTime = local;
    printf("mLocalTime time = %lld\r\n",mLocalTime);
}
#endif
