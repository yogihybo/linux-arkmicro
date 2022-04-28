#include "AutoLink.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef USE_AUTO
IUserAutoImpl::IUserAutoImpl(AutoLink* handle) : mHandle(handle)/*, mRecBuf(NULL)*/, mRecStart(false),
             mUsedPos(0),mConnectStatus(CONNECT_STATUS_DISCONNECTED)
{

}

IUserAutoImpl::~IUserAutoImpl()
{

}

void IUserAutoImpl::videoStart(int width, int height, int offsetX, int offsetY)
{
    if(mHandle){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        printf("auto video width = %d, height = %d, offsetX = %d, offsetY =%d\r\n", width, height, offsetX, offsetY);
        mHandle->video_start(offsetX, offsetY,width - offsetX * 2, height - offsetY *2);
        mHandle->onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED, mHandle->getPhoneType());
        mConnectStatus = CONNECT_STATUS_CONNECT_SUCCEED;
    }
}

void IUserAutoImpl::videoStop()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle)
        mHandle->video_stop();
}

void IUserAutoImpl::videoPlay(char *buf, int len)
{
    if(mHandle)
        mHandle->video_play(buf, len);
}


void IUserAutoImpl::audioStart(int type, int rate, int channels, int bits)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->audio_start(ChangeAudioType((AutoAudioStreamType)type), rate, bits, channels);
    }
}

void IUserAutoImpl::audioStop(int type)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->audio_stop(ChangeAudioType((AutoAudioStreamType)type));
    }
}

void IUserAutoImpl::audioPlay(int type, char *buf, int len)
{
    //printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->audio_play(ChangeAudioType((AutoAudioStreamType)type), buf, len);
    }
}

void IUserAutoImpl::recordStart(int rate, int channels, int bits)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->record_pause(false);
        mHandle->app_status(APP_RECOGNITION_STARTED);
    }
}

void IUserAutoImpl::recordStop()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->record_pause(true);
        mHandle->app_status(APP_RECOGNITION_STOPPED);
    }
}

void IUserAutoImpl::recordProc(char *buf, int len)
{
    strncpy(buf , mHandle->getRecData().c_str(), len);
    mHandle->getRecData().clear();
}

void IUserAutoImpl::notifyStatus(int state)
{
    printf("11 notify status  = %d\r\n", state);

    static bool running = false;

    if(state == LINK_REMOVED || state == LINK_DISCONNECTED){
        running = false;
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        mHandle->record_stop();
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        mHandle->onSdkConnectStatus(CONNECT_STATUS_DISCONNECTED, UnKnown);
        mConnectStatus = CONNECT_STATUS_DISCONNECTED;
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else if(state == LINK_SUCCESS){
        if(!running){
            AudioInfo info = {16000, 1, 16};
            mHandle->record_start(info);
            mHandle->record_pause(true);
            mHandle->onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED, mHandle->getPhoneType());
            mConnectStatus = CONNECT_STATUS_CONNECT_SUCCEED;
            running = true;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        }
    }
    else if(state == LINK_EXITING){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        mHandle->SetBackground();
        mHandle->app_status(APP_BACKGROUND, nullptr);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else if(state == LINK_STARTING){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        //mHandle->SetForeground();
        //mHandle->app_status(APP_FOREGROUND, nullptr);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    printf("22 notify status = %d\r\n", state);
}

void IUserAutoImpl::notifyPhoneBtInfo(const char *phoneBTAddr, int pairMethod)
{

}

void IUserAutoImpl::getLocalBtAddr(char* mac)
{
    string data = mHandle->getCarBtAddress();;
    mac = (char*)data.c_str();
}

AudioType IUserAutoImpl::ChangeAudioType(AutoAudioStreamType type)
{
    AudioType ret  = AUDIO_TYPE_MUSIC;
    if(type == AUTO_AUDIO_STREAM_MEDIA){
        ret = AUDIO_TYPE_MUSIC;
    }
    else if(type == AUTO_AUDIO_STREAM_GUIDANCE){
        ret = AUDIO_TYPE_VR;
    }
    else if(type == AUTO_AUDIO_STREAM_SYSTEM_AUDIO){
        ret = AUDIO_TYPE_TTS;
    }
    else if(type == AUTO_AUDIO_STREAM_TELEPHONY){
        ret = AUDIO_TYPE_CALL;
    }
    return ret;
}

#endif

AutoLink::AutoLink()
{
#ifdef USE_AUTO
    mHandle = new AndroidAuto();
    mAutoCbs = new IUserAutoImpl(this);
#endif
}

AutoLink::~AutoLink()
{
#ifdef USE_AUTO
    delete mHandle;
    delete  mAutoCbs;
#endif
}

#ifdef USE_AUTO

bool AutoLink::init(LinkMode linkMode)
{
    if(mHandle){
        printf("auto w:%d,h%d\r\n",mLinkConfig.screen_width,mLinkConfig.screen_height);
        mLinkMode = linkMode;
        mHandle->registerCallbacks(mAutoCbs);
        mHandle->setConfig("video_width",mLinkConfig.screen_width);
        mHandle->setConfig("video_height",mLinkConfig.screen_height);
        mHandle->setConfig("video_density",160);
    }
    return true;
}

bool AutoLink::start()
{
    if(mHandle){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        onSdkConnectStatus(CONNECT_STATUS_CONNECTING, mPhoneType);
        if(mLinkMode == Wired)
            mHandle->startSession(false);
        else
            mHandle->startSession(true);
    }
    return true;
}

bool AutoLink::stop()
{
    if(mHandle){
        mHandle->getVideoFocus();
        mHandle->releaseAudioFocus();
    }
    return true;
}

bool AutoLink::release()
{
    if(mHandle){
        mHandle->getVideoFocus();
        mHandle->getAudioFocus();
    }
    return true;
}

bool AutoLink::start_mirror()
{
    if(mHandle)
        mHandle->releaseVideoFocus();
    return true;
}

bool AutoLink::stop_mirror()
{
    if(mHandle)
        mHandle->getVideoFocus();
    return true;
}

bool AutoLink::set_background()
{
    if(mHandle)
        mHandle->getVideoFocus();
    return true;
}

bool AutoLink::set_foreground()
{
    if(mHandle)
        mHandle->releaseVideoFocus();
    return true;
}

bool AutoLink::get_audio_focus()
{
    if(mHandle)
        mHandle->getAudioFocus();
    return true;
}

bool AutoLink::release_audio_focus()
{
    if(mHandle)
        mHandle->releaseAudioFocus();
    return true;
}

void AutoLink::set_inserted(bool inserted, PhoneType phoneType)
{
    mPhoneType = phoneType;
}

void AutoLink::send_screen_size(int width, int height)
{

}

void AutoLink::record_audio_callback(unsigned char *data, int len)
{
     std::lock_guard<std::mutex> lock(mMutex);
     mRecData += string((char*)data, len);
}

void AutoLink::send_car_bluetooth(const string& name, const string& address, const string& pin)
{
    mbtaddress = address;
}

void AutoLink::send_phone_bluetooth(const string& address)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void AutoLink::send_touch(int x, int y, TouchCode touchCode)
{
    printf("auto x:%d, y:%d, press:%d\r\n",x, y, touchCode);
    if(mHandle){
        if(touchCode == Touch_Up)
            mHandle->sendTouchEvent(x, y, 1);
        else if(touchCode == Touch_Press)
            mHandle->sendTouchEvent(x, y, 0);
        else if(touchCode == Touch_Move)
            mHandle->sendTouchEvent(x, y, 2);
    }
}

bool AutoLink::send_key(KeyCode keyCode)
{
    AutoKeyCode ret;
    switch(keyCode) {
          case KYE_HOME:
              ret = KEYCODE_HOME;
              break;
          case KEY_SYSTEM_HOME:
              ret = KEYCODE_MENU;
              break;
          case KEY_SYSTEM_BACK:
              ret = KEYCODE_BACK;
              break;
          case KEY_VOICE_ASSISTANT:
              ret = KEYCODE_SEARCH;
              break;
          case KEY_MUSIC_PLAY_PAUSE:
              ret = KEYCODE_MEDIA_PLAY_PAUSE;
              break;
          case KEY_MUSIC_PLAY:
              ret = KEYCODE_MEDIA_PLAY;
              break;
          case KEY_MUSIC_PAUSE:
              ret = KEYCODE_MEDIA_STOP;
              break;
          case KEY_MUSIC_NEXT:
              ret = KEYCODE_MEDIA_NEXT;
              break;
          case KEY_MUSIC_PREVIOUS:
              ret = KEYCODE_MEDIA_PREVIOUS;
              break;
          case KEY_PHONE:
             {
                static bool call = false;
                if(!call)
                {
                    ret = KEYCODE_CALL;
                }else
                {
                    ret = KEYCODE_ENDCALL;
                }
                call = !call;
             }
              break;
          case KEY_LIGHTMODE:
              printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
              mHandle->setConfig("night_mode", 0);
              printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
              return true;
          case KEY_NIGHTMODE:
              printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
              mHandle->setConfig("night_mode", 1);
              return true;
              printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
          default:
              break;
      }
    if(ret == KEY_CAR_BACK){
        set_background();
    }
    else if(ret == KEY_CAR_FRONT){
        set_foreground();
    }
    else
    {
        mHandle->sendKeyEvent(ret, true);
        usleep(20000);
        mHandle->sendKeyEvent(ret, false);
    }
}

bool AutoLink::send_wheel(WheelCode wheel, bool foucs)
{
    if (!foucs) {
        mHandle->sendKeyEvent(KEYCODE_DPAD_CENTER, true);
        usleep(20000);
        mHandle->sendKeyEvent(KEYCODE_DPAD_CENTER, false);
    } else {
        mHandle->sendKnobEvent(KEYCODE_ROTARY_CONTROLLER, wheel);
    }
}

bool AutoLink::send_night_mode(bool night)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mHandle){
        mHandle->setConfig("night_mode", night);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    return true;
}

bool AutoLink::send_right_hand_driver(bool right)
{
    if(mHandle)
        mHandle->setConfig("driver_position", right);
    return true;
}

bool AutoLink::open_page(AppPage appPage)
{
    int ret = 0;
    if(appPage == APP_PAGE_NAVIGATION){
        ret = KEYCODE_NAVIGATION;
    }
    else if(appPage == APP_PAGE_MAIN){
        ret = KEYCODE_HOME;
    }
    else if(appPage == APP_PAGE_MUSIC){
        ret = KEYCODE_MUSIC;
    }
    mHandle->sendKeyEvent(ret, true);
    usleep(20000);
    mHandle->sendKeyEvent(ret, false);
}

void AutoLink::request_status(RequestAppStatus requestAppStatus, void *reserved)
{

}

void AutoLink::setRecData(string str)
{
    mRecData = str;
}
#endif
