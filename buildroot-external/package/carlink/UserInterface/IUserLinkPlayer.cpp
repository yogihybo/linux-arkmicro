#include "IUserLinkPlayer.h"
#include "CarlifeLink.h"
#include "CarplayLink.h"
#include "EasyConnectLink.h"
#include "MirrorLink.h"
#include "UsbHostService.h"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "AudioRecord.h"
#include "ConfigParser.h"
#include "util.h"
#include "LinkAssist.h"
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <sys/time.h>
using namespace XEngine;

#define MIRROR_WIDTH 1280
#define MIRROR_HEIGHT 720

uint64_t getTickCount()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

IUserLinkPlayer::IUserLinkPlayer():mpIULPlayer(this),mpMusicDecoder(new AudioDecoder()),
mpTTSDecoder(new AudioDecoder()),mpVRDecoder(new AudioDecoder()),mpCallDecoder(new AudioDecoder()),mFuncAppStatusCallback(nullptr),mFuncConnectCallback(nullptr),mConnected(false),mVideoStart(false)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

IUserLinkPlayer::~IUserLinkPlayer()
{
    printf("%d:%s\r\n",__LINE__, __func__);
    delete mpMusicDecoder;
    delete mpTTSDecoder;
    delete mpVRDecoder;
    delete mpCallDecoder;
}

void IUserLinkPlayer::RegisterConnectCallback(FUNCCONNECTCALLBACK funcConnectCallback)
{
    mFuncConnectCallback = funcConnectCallback;
}

void IUserLinkPlayer::RegisterAppStatusCallback(FUNCAPPSTATUSCALLBACK funcAppStatusCallback)
{
    mFuncAppStatusCallback = funcAppStatusCallback;
}

void IUserLinkPlayer::GetIniConfig(LinkAssist *pLinkAssist)
{
    mLinkConfig = pLinkAssist->GetConfigInfo();
    mCarplayConfig = pLinkAssist->GetCarplayInfo();
}

bool IUserLinkPlayer::Initialize(LinkMode linkMode, PhoneType phoneType)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("mpIULPlayer = %x, linkMode = %d, phoneType = %d\n",mpIULPlayer, linkMode, phoneType);
    mpIULPlayer->set_inserted(true, phoneType);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mpIULPlayer->init(linkMode);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mpIULPlayer->SendScreenSize(MIRROR_WIDTH, MIRROR_HEIGHT); //请求大小,不一定是车机大小
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    return true;
}

bool IUserLinkPlayer::Start()
{
    if(mpIULPlayer)
        mpIULPlayer->start();
    return true;
}

bool IUserLinkPlayer::Stop()
{
    if(mpIULPlayer)
        mpIULPlayer->stop();
    return true;
}

void IUserLinkPlayer::Release()
{
    printf("%d:%s\r\n",__LINE__, __func__);
    if(mpIULPlayer){
        mpIULPlayer->release();
    }
}

bool IUserLinkPlayer::StartMirror()
{
    if(mpIULPlayer)
        mpIULPlayer->start_mirror();
    return true;
}

bool IUserLinkPlayer::StopMirror()
{
    if(mpIULPlayer)
        mpIULPlayer->stop_mirror();
    return true;
}

bool IUserLinkPlayer::SetBackground()
{
    //VideoDecoder::instance()->Show(false);
    if(mpIULPlayer)
        mpIULPlayer->set_background();
    return true;
}

bool IUserLinkPlayer::SetForeground()
{
    //VideoDecoder::instance()->Show(true);
    if(mpIULPlayer)
        mpIULPlayer->set_foreground();
    return true;
}

void IUserLinkPlayer::SendScreenSize(int width, int height)
{
    if(mpIULPlayer)
        mpIULPlayer->send_screen_size(width, height);
}

void IUserLinkPlayer::SendTouch(int x, int y, TouchCode touchCode)
{
    if(mpIULPlayer){
        mpIULPlayer->send_touch(x, y, touchCode);
    }
}

void IUserLinkPlayer::SendMultiTouch(int x1, int y1, TouchCode touchCode1,int x2, int y2, TouchCode touchCode2)
{
    if(mpIULPlayer){
        mpIULPlayer->send_multi_touch(x1, y1, touchCode1, x2, y2, touchCode2);
    }
}

void IUserLinkPlayer::SendKey(KeyCode keyCode)
{
    if(mpIULPlayer){
        mpIULPlayer->send_key(keyCode);
    }
}

void IUserLinkPlayer::SendWheel(WheelCode wheelCode, bool bFoucs)
{
    if(mpIULPlayer){
        mpIULPlayer->send_wheel(wheelCode, bFoucs);
    }
}

bool IUserLinkPlayer::RequestStatus(RequestAppStatus requestAppStatus, void *reserved)
{
    if(mpIULPlayer){
        mpIULPlayer->request_status(requestAppStatus,reserved);
    }
}

void IUserLinkPlayer::SendCarBluetooth(const string& name, const string& address, const string& pin)
{
    if(mpIULPlayer){
        mpIULPlayer->send_car_bluetooth(name, address, pin);
    }
}

void IUserLinkPlayer::SendPhoneBluetooth(const string& address)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    printf("mpIULPlayer = %x\r\n", mpIULPlayer);
    if(mpIULPlayer){
        mpIULPlayer->send_phone_bluetooth(address);
    }
}

void IUserLinkPlayer::SendCarWifi(WifiInfo& info){

    if(mpIULPlayer){
        mpIULPlayer->send_car_wifi(info);
    }
}

void IUserLinkPlayer::onSdkConnectStatus(ConnectedStatus status, PhoneType type)
{
    // todo 获取连接状态
    // 根据不同的连接状态，车机界面给予用户不同的提示或者界面跳转
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    //第一次插入已经知晓UsbHostService notify手机类型

    switch (status)
    {
        case CONNECT_STATUS_CONNECTING:
        {
            // todo 正在连接
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        case CONNECT_STATUS_CONNECT_FAILED:
        {
            // todo 连接失败            
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        case CONNECT_STATUS_CONNECT_SUCCEED:
        {
            // todo 连接成功
            mConnected = true;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        case CONNECT_STATUS_DISCONNECTED:
        {
            // todo 连接断开
            mConnected = false;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        case CONNECT_STATUS_APP_EXIT:
        {
            // todo 手机app退出
            mConnected = false;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        case CONNECT_STATUS_INTERRUPTED_BY_APP:
        {
            // todo 手机app中断
            mConnected = false;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        default:
            break;
    }
    if(mFuncConnectCallback){
        mFuncConnectCallback(status, type);
    }
}

bool IUserLinkPlayer::OpenPage(AppPage appPage)
{
    if(mpIULPlayer)
        mpIULPlayer->open_page(appPage);
    return true;
}

bool IUserLinkPlayer::SendNightMode(bool night){

    if(mpIULPlayer)
       mpIULPlayer->send_night_mode(night);
    return true;
}

bool IUserLinkPlayer::SendRightHandDriver(bool right){
    if(mpIULPlayer)
        mpIULPlayer->send_right_hand_driver(right);
    return true;
}

bool IUserLinkPlayer::SendIphoneMacAddress(string address)
{
    if(mpIULPlayer){
        mpIULPlayer->set_mac(address);
    }
    return true;
}

void IUserLinkPlayer::SendLisenceCode(const string& license)
{
    if(mpIULPlayer){
        mpIULPlayer->send_license(license);
    }
}

void IUserLinkPlayer::SendInputText(const string &text)
{
    if(mpIULPlayer){
        mpIULPlayer->send_input_text(text);
    }
}

void IUserLinkPlayer::SendInputSelection(int start, int stop){
    if(mpIULPlayer){
        mpIULPlayer->send_input_selection(start,stop);
    }
}

void IUserLinkPlayer::SendInputAction(int action, int keyCode)
{
    if(mpIULPlayer){
        mpIULPlayer->send_input_action(action, keyCode);
    }
}

void IUserLinkPlayer::video_start(int offset_x, int offset_y, int width, int height)
{
    printf("video_start width = %d, height = %d\r\n", width, height);

    printf("video_start diy_screen = %d, screen_width = %d, screen_height = %d\r\n",
           mLinkConfig.diy_screen, mLinkConfig.screen_width, mLinkConfig.screen_height);

    if(mLinkConfig.diy_screen){
        mVideoFrame = {offset_x,offset_y,width, height,
                       mLinkConfig.offset_x, mLinkConfig.offset_y ,
                       mLinkConfig.screen_width, mLinkConfig.screen_height,VERTICAL};
    }
    else
    {
        int screen_width = 800;
        int screen_height = 480;
       // Util::getFrameBufferFixedSize(screen_width, screen_height, "/dev/fb0");
        mVideoFrame = {offset_x, offset_y ,width, height, 0, 0, screen_width, screen_height, VERTICAL};
    }
    mVideoStart = true;
    VideoDecoder::instance()->Init(&mVideoFrame);
}

void IUserLinkPlayer::video_stop()
{
    printf("video_stop\r\n");
    Autolock l(&mMutexVideo);
    mVideoStart = false;
    VideoDecoder::instance()->Uninit();
}

void IUserLinkPlayer::video_play(const void *data, int32_t len)
{
    //printf("video len = %d\r\n", len);

    static int frameCounter = 0;
    static int64_t beginTime = 0;
    int64_t currentTime;
    currentTime = getTickCount();

    if (!beginTime)
    {
        beginTime = currentTime;
    }

    frameCounter++;

    if (currentTime - beginTime >= 5000)
    {
        printf("5 seconds average fps: %d\n", (int)(frameCounter * 1000 / (currentTime - beginTime)));
        beginTime = 0;
        frameCounter = 0;
    }

    //printf("++ video len = %d ++\n", len);
    Autolock l(&mMutexVideo);
    if(mVideoStart)
        VideoDecoder::instance()->InputDecoder(data, len);
    //printf("-- video len = %d --\n", len);
}

void IUserLinkPlayer::record_start(AudioInfo &audioInfo)
{
    printf("%d:%s\r\n",__LINE__, __func__);
    AudioRecord::instance()->registerRecordAudio(std::bind(&IUserLinkPlayer::record_audio_callback,this, std::placeholders::_1,std::placeholders::_2));
    AudioRecord::instance()->setMediaParam(audioInfo.sampleRate, audioInfo.format, audioInfo.channel);
    AudioRecord::instance()->start();
}

void IUserLinkPlayer::record_stop()
{
    printf("%d:%s\r\n",__LINE__, __func__);
    AudioRecord::instance()->join();
    printf("%d:%s\r\n",__LINE__, __func__);
}

void IUserLinkPlayer::record_pause(bool pause)
{
    if(pause)
        AudioRecord::instance()->stopRecordSound();
    else
        AudioRecord::instance()->resumeRecordSound();
}

bool IUserLinkPlayer::app_status(AppStatusMessage appStatusMessage, void *reserved)
{
    printf("appStatusMessage = %d\r\n", appStatusMessage);
    if(mConnected){
        if(appStatusMessage == APP_FOREGROUND){
            VideoDecoder::instance()->Show(true);
        }
        else if(appStatusMessage == APP_BACKGROUND){
            VideoDecoder::instance()->Show(false);
        }
    }
    if(mFuncAppStatusCallback)
    {
        mFuncAppStatusCallback(appStatusMessage, reserved);
    }
}

bool IUserLinkPlayer::bt_call_action(CallType callType, const char *name, const char* number)
{
    printf("calltype = %d, name = %s, number = %s\r\n",callType, name, number);

    return true;
}

bool IUserLinkPlayer::audio_start(AudioType audioType, int rate, int bit, int channel)
{
    printf("start audio type = %d, rate = %d, bit = %d, channel = %d\r\n", audioType, rate, bit, channel);
    if(audioType == AUDIO_TYPE_MUSIC){
        mpMusicDecoder->setMediaParam(rate,bit, channel);
        app_status(APP_MUSIC_STARTED, nullptr);
    }
    else if(audioType == AUDIO_TYPE_TTS){
        mpTTSDecoder->setMediaParam(rate,bit, channel);
        app_status(APP_NAVI_STARTED, nullptr);
    }
    else if(audioType == AUDIO_TYPE_VR){
        mpVRDecoder->setMediaParam(rate,bit, channel);
        app_status(APP_VR_STARTED, nullptr);
    }    
    else if(audioType == AUDIO_TYPE_CALL){
        mpCallDecoder->setMediaParam(rate,bit, channel);
        //app_status(APP_PHONE_STARTED, nullptr);
    }

    return true;
}

bool IUserLinkPlayer::audio_stop(AudioType audioType)
{
    printf("stop audiotype = %d\r\n", audioType);

    if(audioType == AUDIO_TYPE_MUSIC){
        mpMusicDecoder->release();
        app_status(APP_MUSIC_STOPPED, nullptr);
    }
    else if(audioType == AUDIO_TYPE_TTS){
        mpTTSDecoder->release();
        app_status(APP_NAVI_STOPPED, nullptr);
    }
    else if(audioType == AUDIO_TYPE_VR){
        mpVRDecoder->release();
        app_status(APP_VR_STOPPED, nullptr);
    }
    else if(audioType == AUDIO_TYPE_CALL){
        mpCallDecoder->release();
        app_status(APP_PHONE_STOPPED, nullptr);
    }
    return true;
}

void IUserLinkPlayer::audio_play(AudioType audioType, const void* data, uint32_t len)
{
    //printf("audiotype = %d , len = %d\r\n", audioType, len);
    if(audioType == AUDIO_TYPE_MUSIC){
        mpMusicDecoder->playSound((unsigned char*)data, len);
    }
    else if(audioType == AUDIO_TYPE_TTS){
        mpTTSDecoder->playSound((unsigned char*)data, len);
    }
    else if(audioType == AUDIO_TYPE_VR){
        mpVRDecoder->playSound((unsigned char*)data, len);
    }
    else if(audioType == AUDIO_TYPE_CALL){
        mpCallDecoder->playSound((unsigned char*)data, len);
    }

}


