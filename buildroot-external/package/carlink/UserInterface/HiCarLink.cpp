#include "HiCarLink.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include "ark_hicar_api.h"
#include "BufferQueue.h"

#define BLUETOOTHADDR "/tmp/bluetoothaddress"
#define BLUETOOTHNAME "/tmp/bubluetoothname"
#define BLUETOOTHPIN  "/tmp/bubluetoothpin"
#define WIFI_ADDR "/tmp/hostapd.conf"

#ifdef USE_HICAR
static void resetUsb(int index){
/*
    char peripheral[256] = {0,}, otg[256] = {0,};
    sprintf(peripheral,"echo peripheral > /sys/devices/platform/soc/e0100000.usb/musb-hdrc.%d/mode", index);
    system(peripheral);
    usleep(500000);
    sprintf(otg,"echo otg > /sys/devices/platform/soc/e0100000.usb/musb-hdrc.%d/mode", index);
    system(otg);
*/

    if(index == 0){
        system("echo peripheral > /sys/devices/platform/soc/e0100000.usb/musb-hdrc.0/mode");
        usleep(500000);
        system("echo otg > /sys/devices/platform/soc/e0100000.usb/musb-hdrc.0/mode");
    }
    else if(index == 1){
        system("echo peripheral > /sys/devices/platform/soc/e0400000.usb/musb-hdrc.1/mode");
        usleep(500000);
        system("echo otg > /sys/devices/platform/soc/e0400000.usb/musb-hdrc.1/mode");
    }
}

HiCarLinkImpl::HiCarLinkImpl(HiCarLink *handle): mHandle(handle),mHiCarState(0)
{
    ArkHicarRegisterAppRcvHicarDataCallback(HiCarSendVHiCarEventCallBack, this);
    HicarDongleRegisterGetVideoDataCallBack(HiCarSendVideoDataCallBack, this);
    HicarDongleRegisterGetAudioDataCallBack(HiCarGetAudioDataCallBack, this);
    HicarDongleRegisterGetAudioMicDataCallBack(HiCarGetAudioMicDataCallBack, this);
    ArkHicarRegisterAppRcvBtDataCallback(HiCarSendVHiCarBTCallBack, this);
    //ArkHicarRegisterAudioCallback(HiCarAudioFoucsCallBack, this);
}

HiCarLinkImpl::~HiCarLinkImpl()
{

}

int HiCarLinkImpl::HiCarSendVideoDataCallBack(int type, void *data, int len, void *parameters)
{
    HiCarLinkImpl *pthis = (HiCarLinkImpl *)parameters;
 //   printf("event type:%d len:%d\n", type, len);
    if(type == ARK_HICAR_VIDEO_START){
        ArkHicarDongleVideoDataParam *param = (ArkHicarDongleVideoDataParam *)data;
        if(param)
            pthis->mHandle->video_start(param->width, param->height, param->xpos, param->ypos);
    }
    else if(type == ARK_HICAR_VIDEO_STOP){
        pthis->mHandle->video_stop();
    } else if(type == ARK_HICAR_VIDEO_STREAM) {
        pthis->mHandle->video_play(data,len);
    }
    return 0;
}

int HiCarLinkImpl::HiCarSendVHiCarEventCallBack(int type, void *data, int dataLen, void *parameters)
{
    HiCarLinkImpl *pthis = (HiCarLinkImpl *)parameters;
    ArkHicarDataInfo *dataInfo = (ArkHicarDataInfo *)data;

   printf("event type:%d len:%d\n", type, dataLen);
    if(!dataInfo && type!=ARK_HICAR_INIT_DONE )
        return -1;

    if(type == ARK_HICAR_LINK_STATE_CHANGE){

        printf("hicarState = %d, data status = %d\r\n",pthis->mHiCarState, dataInfo->status);
        if(pthis->mHiCarState != dataInfo->status){

            printf("###### hicar status === %d\r\n", dataInfo->status);
            switch (dataInfo->status) {
            case LINK_STARTING:
                ark_hicar_link_state_change(pthis->mHandle->mLinkType, LINK_STARTING);
                printf("link_startting===========\r\n");
                break;
            case LINK_EXITING:
                pthis->mHandle->app_status(APP_BACKGROUND);
                break;
            case LINK_EXITED:
            case LINK_REMOVED:
            {
                pthis->mHandle->onSdkConnectStatus(CONNECT_STATUS_DISCONNECTED, UnKnown);
                pthis->mHandle->video_stop();
            }
                break;
            case LINK_SUCCESS:
            {
                pthis->mHandle->app_status(APP_FOREGROUND);
                pthis->mHandle->onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED, pthis->mHandle->getPhoneType());
            }
                break;
            default:
                break;
            }
            pthis->mHiCarState = dataInfo->status;
            printf("hicarState = %d, dataInfo->status = %d\r\n",pthis->mHiCarState, dataInfo->status);
        }
    }
    else if(type == ARK_HICAR_BT_PIN_CODE){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(data && (dataLen == 6)) {
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            string pinCode((char*)data, dataLen);
            pthis->mHandle->app_status(APP_PINCODE, (void*)&pinCode);
            printf("pincode = %s\r\n", data);
        }
    }
    else if(type == ARK_HICAR_INIT_DONE){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        pthis->mHandle->app_status(APP_CARLINK_INIT_DONE);
        printf("============ ark hicar init done \r\n");
    }
    return 0;
}

int HiCarLinkImpl::HiCarSendVHiCarBTCallBack(int type, void *data, int dataLen, void *parameters)
{
    HiCarLinkImpl *pthis = (HiCarLinkImpl *)parameters;

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    static string cmd = "";
    cmd = string((char*)data,dataLen);
    if(pthis)
        pthis->mHandle->app_status(APP_BT_CMD, (void*)&cmd);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    return 0;
}

AudioType HiCarLinkImpl::ChangeAudioType(HiCarAudioStreamType type)
{
    AudioType ret  = AUDIO_TYPE_MUSIC;
    if(type == HiCar_AUDIO_STREAM_MEDIA){
        ret = AUDIO_TYPE_MUSIC;
    }
    else if(type == HiCar_AUDIO_STREAM_Alt){
        ret = AUDIO_TYPE_VR;
    }
    else if(type == HiCar_AUDIO_STREAM_VR){
        ret = AUDIO_TYPE_TTS;
    }
    else if(type == HiCar_AUDIO_STREAM_TELEPHONY){
        ret = AUDIO_TYPE_CALL;
    }
    return ret;
}

int HiCarLinkImpl::HiCarGetAudioDataCallBack(int streamType, void *data, int dataLen, void *parameters)
{
    HiCarLinkImpl *pthis = (HiCarLinkImpl *)parameters;

    if(data) {
        int *tmp = (int *)data;

        if(streamType == ARK_HICAR_AUDIO_FLAGS) {
            int status = tmp[0];
            AudioType audioType = pthis->ChangeAudioType((HiCarAudioStreamType)tmp[1]);
            if(audioType < 0) {
                printf("%s:%d: Invalid audioType:%d", __func__, __LINE__, audioType);
                return -1;
            }
            int rate = tmp[2];
            int channels = tmp[3];
            int bits = tmp[4];
            if(status == ARK_HICAR_AUDIO_OPEN) {
                pthis->mHandle->audio_start(audioType, rate, bits, channels);
            } else if(status == ARK_HICAR_AUDIO_CLOSE) {
                if(audioType == AUDIO_TYPE_MUSIC){
                    pthis->mHandle->app_status(APP_MUSIC_STOPPED, nullptr);
                }
                else if(audioType == AUDIO_TYPE_TTS){
                    pthis->mHandle->app_status(APP_NAVI_SOUND_STOPPED, nullptr);
                }
                else if(audioType == AUDIO_TYPE_VR){
                    pthis->mHandle->app_status(APP_VR_STOPPED, nullptr);
                }
            }
        } else {
            AudioType audioType = pthis->ChangeAudioType((HiCarAudioStreamType)streamType);
            if(audioType < 0) {
                printf("%s:%d: Invalid audioType:%d", __func__, __LINE__, audioType);
                return -1;
            }
            pthis->mHandle->audio_play(audioType, (char *)data, dataLen);
//            printf("streamType:%d, datalen = %d\r\n", streamType, dataLen);
        }
    }
    return 0;
}

//#define REC_FILE
int HiCarLinkImpl::HiCarGetAudioMicDataCallBack(int streamType, void *data, int dataLen, void *parameters)
{
    HiCarLinkImpl *pthis = (HiCarLinkImpl *)parameters;

    if(streamType == ARK_HICAR_AUDIO_READ) {
        if(data && dataLen) {
 //           printf("stremType = %d\r\n", streamType);
            char buf[640] = {0,};
            memcpy(data, buf, dataLen);
            int recLen = 0;
            unsigned char * pRecBuf = NULL;
            int type = 0 ;
            if(pthis->mHandle->mRecQueue->IsExitedThread(pthread_self())){
                printf("return thread=============\r\n");
               return -1;
            }

            if(pthis->mHandle->mRecQueue->ReadQueue(&type, &pRecBuf, &recLen) > 0){
//              printf("rec len = %d, dataLen = %d\r\n", recLen, dataLen);
                //data = (char *)pRecBuf;
             memcpy(data, pRecBuf, recLen);

#ifdef REC_FILE
            static FILE *fp = NULL;
            if(fp == NULL)
                fp = fopen("/tmp/hicar_local.pcm", "wb");
            fwrite(pRecBuf, 1, recLen, fp);
#endif
            }
            free(pRecBuf);
            pRecBuf = NULL;

            usleep(10000);
            //printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

        }
    } else if(streamType == ARK_HICAR_AUDIO_OPEN) {
        if(data && (dataLen == sizeof(int)*3)) {
            int *buf = (int *)data;
            int rate = buf[0];
            int channels = buf[1];
            int bits = buf[2];
            usleep(1000 * pthis->mHandle->mMillisecond);
            AudioInfo info = {rate, channels, bits};
            printf("rate:%d,channel:%d,bits:%d\r\n",rate, channels, bits);
            pthis->mHandle->record_start(info);
            pthis->mHandle->app_status(APP_RECOGNITION_STARTED);
        }
    } else if(streamType == ARK_HICAR_AUDIO_CLOSE) {
        pthis->mHandle->record_stop();
        pthis->mHandle->app_status(APP_RECOGNITION_STOPPED);
    }
    return 0;
}

int HiCarLinkImpl::HiCarAudioFoucsCallBack(int type, void *streamType, int focus, void* parameters)
{
    int *buf = (int *)streamType;
    int s = buf[0];


    printf("========== type:%d, streamType:%d,focus:%d\r\n", type, s,focus);
    return 0;
}

#endif

HiCarLink::HiCarLink()
{
#ifdef USE_HICAR
    mRecPos = 0;    
    memset(mRecBuf, 0, sizeof(mRecBuf));
    mRecQueue = new BufferQueue();
    mHandle = new HiCarLinkImpl(this);
    mLinkType = 0;
    mMillisecond = 0;

#endif
}

HiCarLink::~HiCarLink()
{
#ifdef USE_HICAR
    delete mHandle;
    delete mRecQueue;
    ark_hicar_release();
#endif
}

#ifdef USE_HICAR
bool HiCarLink::init(LinkMode linkMode)
{
    printf("hicar x:%d y:%d w:%d h:%d\r\n",mLinkConfig.offset_x,mLinkConfig.offset_y, mLinkConfig.screen_width, mLinkConfig.screen_height);

    ark_hicar_request_video_size(mLinkConfig.screen_width, mLinkConfig.screen_height, mLinkConfig.offset_x, mLinkConfig.offset_y);
    ark_hicar_init(0);
    mLinkMode = linkMode;
    if(mLinkMode == Wired){
        mLinkType = 0x09;
    }
    else if(mLinkMode == Wireless){
        mLinkType = 0x09;
    }
    return true;
}

bool HiCarLink::start()
{
    if(mLinkMode == Wired){
        resetUsb(mLinkConfig.usb_index);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    ark_hicar_link_state_change(mLinkType, LINK_STARTING);
    onSdkConnectStatus(CONNECT_STATUS_CONNECTING, mPhoneType);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    return true;
}

bool HiCarLink::stop()
{
    return true;
}

bool HiCarLink::release()
{
    return true;
}

bool HiCarLink::start_mirror()
{
    return true;
}

bool HiCarLink::stop_mirror()
{
    return true;
}

bool HiCarLink::set_background()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    int ret =  ark_hicar_link_state_change( mLinkType,LINK_EXITING);
    printf("ret = %d\n", ret);
    return true;
}

bool HiCarLink::set_foreground()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    int ret = ark_hicar_link_state_change( mLinkType,LINK_STARTING);
    printf("ret = %d\n", ret);
    return true;
}

bool HiCarLink::get_audio_focus()
{
    return true;
}

bool HiCarLink::release_audio_focus()
{
    return true;
}

void HiCarLink::set_inserted(bool inserted, PhoneType phoneType)
{
    mPhoneType = phoneType;
}

void HiCarLink::send_screen_size(int width, int height)
{

}

void HiCarLink::record_audio_callback(unsigned char *data, int len)
{
     std::lock_guard<std::mutex> lock(mMutex);
/*
     static FILE *fp = NULL;
     if(fp == NULL)
         fp = fopen("/tmp/hicar_old.pcm", "wb");
     fwrite(data, 1, len, fp);
*/
     if(len <= 0)
         return;

     len = mRecPos + len;
     int times = len / PLAYLOAD;
     int remain = len % PLAYLOAD;
     static int prev = 0;
     mRecPos = 0;

     while(times){
        memcpy(mRecBuf + prev, data + mRecPos, PLAYLOAD - prev);
        mRecPos = mRecPos + PLAYLOAD - prev;
        prev = 0;
        --times;

        if(mRecQueue){
            uint8_t *p = (uint8_t*)malloc(PLAYLOAD);
            memcpy(p, mRecBuf, PLAYLOAD);
            mRecQueue->WriteQueue(0, p, PLAYLOAD);
        }
//        printf("pos = %d\n",mRecPos);
     }
     memcpy(mRecBuf, data + mRecPos, remain);
     mRecPos = remain;
     prev = remain;
 //    printf("record_audio_callback len = %d\r\n",len);
}

void HiCarLink::send_car_bluetooth(const string& name, const string& address, const string& pin)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    if(access(BLUETOOTHADDR, F_OK) && !address.empty()){
        ofstream addressfile(BLUETOOTHADDR);
        addressfile<<address<<endl;
        printf("bt address %s\r\n",address.c_str());
    }
}

void HiCarLink::send_phone_bluetooth(const string& address)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void HiCarLink::send_touch(int x, int y, TouchCode touchCode)
{
   // printf("hicar x:%d, y:%d, press:%d\r\n",x, y, touchCode);

    ArkHicarTouchEventType pressed;
    if(touchCode == Touch_Press ){
        pressed = ARK_TOUCH_DOWN ;
    }
    else if(touchCode == Touch_Move){
        pressed = ARK_TOUCH_MOVE;
    }
    else
        pressed = ARK_TOUCH_UP;
    ArkHicarTouchDataInfo touchInfo ;
    touchInfo.coordinateX[0] = x;
    touchInfo.coordinateY[0] = y;
    touchInfo.numPointers =1;
    touchInfo.pointerId[0] = 0;
    touchInfo.type = pressed;
    ark_hicar_touch_state_change(&touchInfo, sizeof(touchInfo));
}

void HiCarLink::send_multi_touch(int x1, int y1, TouchCode touchCode1, int x2, int y2, TouchCode touchCode2)
{
    printf("hicar x1:%d, y1:%d, press1:%d, x2:%d, y2:%d, press2:%d\r\n",x1, y1, touchCode1,x2, y2, touchCode2);

    ArkHicarTouchEventType pressed1, pressed2;
    if(touchCode1 == Touch_Press )
        pressed1 = ARK_TOUCH_DOWN ;
    else if(touchCode1 == Touch_Move)
        pressed1 = ARK_TOUCH_MOVE;
    else
        pressed1 = ARK_TOUCH_UP;

    if(touchCode2 == Touch_Press )
        pressed2 = ARK_TOUCH_DOWN ;
    else if(touchCode2 == Touch_Move)
        pressed2 = ARK_TOUCH_MOVE;
    else
        pressed2 = ARK_TOUCH_UP;

    ArkHicarTouchDataInfo touchInfo ;
    touchInfo.coordinateX[0] = x1;
    touchInfo.coordinateY[0] = y1;
    touchInfo.numPointers =2;
    touchInfo.pointerId[0] = 0;
    touchInfo.type = pressed1;

    touchInfo.coordinateX[1] = x2;
    touchInfo.coordinateY[1] = y2;
    touchInfo.numPointers =2;
    touchInfo.pointerId[1] = 1;
    touchInfo.type = pressed2;
    ark_hicar_touch_state_change(&touchInfo, sizeof(touchInfo));
}

bool HiCarLink::send_key(KeyCode keyCode)
{
    if(keyCode == KEY_MUSIC_PLAY){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, MEDIA_PLAY_KEY);
    }
    else if(keyCode == KEY_MUSIC_PAUSE){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, MEDIA_PLAY_KEY);
    }
    else if(keyCode == KEY_MUSIC_PLAY_PAUSE){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, MEDIA_PLAY_PAUSE_KEY);
    }
    else if(keyCode == KEY_MUSIC_NEXT){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, MEDIA_NEXT_KEY);
    }
    else if(keyCode == KEY_MUSIC_PREVIOUS){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, MEDIA_PREVIOUS_KEY);
    }
    else if(keyCode == KEY_PICKUP_PHONE){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, PICKUP_PHONE_KEY);
    }
    else if(keyCode == KEY_HANGUP_PHONE){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, HANGUP_PHONE_KEY);
    }
    else if(keyCode == KEY_HOME){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, HOME_KEY);
    }
    else if(keyCode == KEY_PHONE){

    }
    else if(keyCode == KEY_VOICE_ASSISTANT){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, MIC_KEY);
    }
    else if(keyCode == KEY_LIGHTMODE){

    }
    else if(keyCode == KEY_NIGHTMODE){

    }
    else if(keyCode == KEY_CAR_FOREGROUND){
        set_foreground();
    }
    else if(keyCode == KEY_CAR_BACKGROUND){
        set_background();
    }
    else if(keyCode == KEY_CAR_BACK){
        ark_hicar_key_value_change(mLinkType,KEY_STATUS_IGNORE, BACK_KEY);
    }
}

bool HiCarLink::send_wheel(WheelCode wheel, bool foucs)
{
    if(foucs)
        ark_hicar_whell_state_change(mLinkType, 0, wheel);
    else
        ark_hicar_whell_state_change(mLinkType, 1, wheel);
}

bool HiCarLink::open_page(AppPage appPage)
{

}

void HiCarLink::request_status(RequestAppStatus requestAppStatus, void *reserved)
{

}

bool HiCarLink::send_night_mode(bool night)
{

}

bool HiCarLink::send_right_hand_driver(bool right)
{

}

void HiCarLink::send_car_wifi(WifiInfo& info)
{
    if(access(WIFI_ADDR, F_OK)){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        string ssid = "ssid=" + info.ssid;
        string passphrase = "wpa_passphrase=" +info.passphrase;
        string channel_id = "channel_id=" + info.channel_id;
        ofstream addressfile(WIFI_ADDR);
        addressfile<<ssid<<endl;
        addressfile<<passphrase<<endl;
        addressfile<<channel_id<<endl;
        addressfile.close();
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
}

void HiCarLink::send_bluetooth_cmd(const string& cmd)
{
    ArkHicarRcvBtStackAtCmd((char*)cmd.c_str());
    printf("==== bluetooth cmd = %s\r\n", cmd.c_str());
}

void HiCarLink::send_broadcast(bool enable)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    ark_hicar_advertisement(enable);
}

void HiCarLink::send_delay_record(int millisecond){

    mMillisecond = millisecond;
}
#endif
