#include "CarlifeLink.h"
#include "carlifeplayer.h"
#include "util.h"
#include "AudioRecord.h"
#include "CCarLifeLibWrapper.h"
#include "UsbHostService.h"
using namespace CCarLifeLibH;



CarlifeLink::CarlifeLink()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_CARLIFE
    m_pCarlifePlayer = new CarlifePlayer();
    mBRecorder = true;
    m_blongpress = false;
#endif
}

CarlifeLink::~CarlifeLink(){

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_CARLIFE
    if(m_pCarlifePlayer){
        delete m_pCarlifePlayer;
        m_pCarlifePlayer = nullptr;
    }
#endif
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}
#ifdef USE_CARLIFE
extern int gvid;
bool CarlifeLink::init(LinkMode linkMode)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(m_pCarlifePlayer){
        m_pCarlifePlayer->SetEapLink(true);
        m_pCarlifePlayer->SetAoaLink(true);
        m_pCarlifePlayer->SetidVendor(gvid);
        m_pCarlifePlayer->Initialize();
        //m_pCarlifePlayer->GetCarScreenSize(mLinkConfig.screen_width,mLinkConfig.screen_height);
        //printf("mLinkConfig.screen_width:%d,mLinkConfig.screen_height:%d\r\n",mLinkConfig.screen_width, mLinkConfig.screen_height);
        m_pCarlifePlayer->SetLinkstatusCallback(std::bind(&CarlifeLink::status_callback_func,this, std::placeholders::_1,std::placeholders::_2),this);
        m_pCarlifePlayer->SetVideoStartCallback(std::bind(&CarlifeLink::video_start_callback_func,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4),this);
        m_pCarlifePlayer->SetVideoInfoCallback(true, std::bind(&CarlifeLink::video_callback_func,this, std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4,std::placeholders::_5),this);
        m_pCarlifePlayer->SetAudioStartCallback(std::bind(&CarlifeLink::audio_start_callback_func,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4,std::placeholders::_5,std::placeholders::_6),this);
        m_pCarlifePlayer->SetAudioInfoCallback(std::bind(&CarlifeLink::audio_callback_func,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4),this);
        m_pCarlifePlayer->SetCmdPhoneNumberCallback(std::bind(&CarlifeLink::phone_number_callback_func,this,std::placeholders::_1,std::placeholders::_2),this);
    }
    return true;
}

void CarlifeLink::set_inserted(bool inserted, PhoneType phoneType)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(m_pCarlifePlayer){
        if(phoneType == Phone_IOS)
            m_pCarlifePlayer->SetLinkType(IOS_CARLIFE);
        else if(phoneType == Phone_Android)
            m_pCarlifePlayer->SetLinkType(ANDROID_CARLIFE);
        m_pCarlifePlayer->SetInserted(inserted);
        mPhoneType = phoneType;
    }
}

bool CarlifeLink::release()
{
    if(m_pCarlifePlayer){
        m_pCarlifePlayer->UnInitialize();
        delete m_pCarlifePlayer;
        m_pCarlifePlayer = nullptr;
    }
}

bool CarlifeLink::start()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(m_pCarlifePlayer){
        onSdkConnectStatus(CONNECT_STATUS_CONNECTING, mPhoneType);
        m_pCarlifePlayer->StartLink(NULL);
    }
    return true;
}

bool CarlifeLink::stop()
{
    if(m_pCarlifePlayer){
        m_pCarlifePlayer->ExitProcess();
    }
    return true;
}

bool CarlifeLink::start_mirror()
{
    if(m_pCarlifePlayer)
        m_pCarlifePlayer->VideoStart();
    return true;
}

bool CarlifeLink::stop_mirror()
{
    if(m_pCarlifePlayer)
        m_pCarlifePlayer->VideoPause();
    return true;
}


void CarlifeLink::set_mac(string mac)
{
    if(m_pCarlifePlayer)
        m_pCarlifePlayer->SetIPAddress((char*)mac.c_str());
}

bool CarlifeLink::send_key(KeyCode keyCode)
{
    m_pCarlifePlayer->Key(keyCode);
    return true;
}

void CarlifeLink::send_touch(int x, int y, TouchCode touchCode)
{

    if(touchCode == Touch_Press)
    {
        m_src_x[0] = x;
        m_src_y[0] = y;
        m_old_src_x[0] = x;
        m_old_src_y[0] = y;
        m_blongpress = false;
        if(m_pCarlifePlayer->GetPhoneOS() == IOS){

            m_pCarlifePlayer->Touch(m_src_x[0], m_src_y[0], mVideoFrame.dst_width, mVideoFrame.dst_height, ScreenPress);
            return ;
        }
        else
        {
           m_pCarlifePlayer->Touch(m_src_x[0], m_src_y[0], mVideoFrame.dst_width, mVideoFrame.dst_height, ScreenPress);
        }
    }
    else if(touchCode == Touch_Up)
    {
        if(m_pCarlifePlayer->GetPhoneOS() != IOS){
            m_pCarlifePlayer->Touch(x, y, mVideoFrame.dst_width, mVideoFrame.dst_height, ScreenRelease);
        }
        else if(m_pCarlifePlayer->GetPhoneOS() == IOS /*&&  m_blongpress == false*/)
        {
            if(abs(m_old_src_x[0] - x) < 10 && abs(m_old_src_y[0] - y) < 10)
            {
                m_pCarlifePlayer->Touch(m_src_x[0], m_src_y[0], mVideoFrame.dst_width, mVideoFrame.dst_height,ScreenSingleClick);
            }
        }
        m_blongpress = false;
    }
    else if(touchCode == Touch_Move)
    {
        if(m_pCarlifePlayer->GetPhoneOS() == IOS && m_blongpress == false)
        {

            m_blongpress = true;
            m_pCarlifePlayer->Touch(x, y,  mVideoFrame.dst_width, mVideoFrame.dst_height,ScreenLongPress);
            m_src_x[0] = x;
            m_src_y[0] = y;
            return;
        }
        m_pCarlifePlayer->Touch(x, y, mVideoFrame.dst_width, mVideoFrame.dst_height,ScreenMove);
        m_src_x[0] = x;
        m_src_y[0] = y;
    }
}

void CarlifeLink::request_status(RequestAppStatus requestAppStatus, void *reserved)
{
     app_status(APP_RESERVED, nullptr);
}

bool CarlifeLink::send_wheel(WheelCode wheel, bool foucs)//foucs配置的时候生效
{
    if(wheel == Wheel_Next){
        m_pCarlifePlayer->Key(KEYCODE_Selector_Next);
    }
    else if(wheel == Wheel_Previous){
        m_pCarlifePlayer->Key(KEYCODE_Selector_Previous);
    }
    return true;
}

bool CarlifeLink::send_night_mode(bool night)
{

}

bool CarlifeLink::set_background()
{
    m_pCarlifePlayer->SetPlayForeground(true);
    //VideoDecoder::instance()->Show(false);
    return true;
}

bool CarlifeLink::set_foreground()
{
    m_pCarlifePlayer->SetPlayForeground(false);
    return true;
}

bool CarlifeLink::get_audio_focus()
{
    return true;
}

bool CarlifeLink::release_audio_focus()
{
    return true;
}

void CarlifeLink::status_callback_func(int status, void* parameter)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    CarlifeLink *pthis = (CarlifeLink*)parameter;

    if(CMD_FAILED == status){
        onSdkConnectStatus(CONNECT_STATUS_CONNECT_FAILED, UnKnown);
    }
    else if(CMD_SUCCESS == status){
        m_pCarlifePlayer->GetCarScreenSize(mVideoFrame.dst_width, mVideoFrame.dst_height);
        onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED,mPhoneType);
    }
    else if(CMD_REMOVED == status){
        onSdkConnectStatus(CONNECT_STATUS_DISCONNECTED, UnKnown);
    }
    else if(CMD_RECORDER_INIT == status){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        AudioInfo info = {16000, 1, 16};
        record_start(info);
    }
    else if(CMD_RECORDER_UNINIT == status){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        record_stop();

    }
    else if(CMD_RECORD_START == status){
        record_pause(false);
    }
    else if(CMD_RECORD_END == status)
    {
        record_pause(true);
    }
    else if(CMD_OPEN_CARLIFE == status){
        app_status(APP_NOT_RUNNING, nullptr);
    }
    else if(CMD_BACKGROUND == status){
        app_status(APP_BACKGROUND, nullptr);
    }
    else if(CMD_FOREGROUND == status){
        app_status(APP_FOREGROUND, nullptr);
    }
    else if(status >= CMD_BT_CALL_INCOMMING){
        bt_call_status(status);
    }
    else if(status == CMD_CALL_PHONE){
        bt_call_action(CALL_TYPE_DAIL, nullptr, pthis->mPhoneNumber.c_str());

    }
    else if(status == CMD_CALL_PHONE_EXIT){

        bt_call_action(CALL_TYPE_HANG_UP, nullptr, pthis->mPhoneNumber.c_str());
    }
}

void CarlifeLink::bt_call_status(int status)
{
 //   call_action(status, nullptr, mPhoneNumber);
}


#define DUMP_REC_FILE 0
#if DUMP_REC_FILE
static FILE *pRecfile = NULL;
#endif


void CarlifeLink::record_audio_callback(unsigned char *data, int len)
{
#if DUMP_REC_FILE
    if (NULL == pRecfile) {
        pRecfile = fopen("/tmp/rec_in.pcm", "w");
    }

    if (pRecfile) {
        fwrite(data, 1, len, pRecfile);
        printf("rec len:%d\r\n ", len);
    }
#endif
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    CCarLifeLib::getInstance()->sendVRRecordData(data, len, 0);
    usleep(10000);
    //printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void CarlifeLink::send_screen_size(int width, int height)
{
    if(m_pCarlifePlayer){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        m_pCarlifePlayer->GetCarScreenSize(width, height);
    }
}

void CarlifeLink::send_car_bluetooth(const string& name, const string& address, const string& pin)
{
    m_pCarlifePlayer->SendCarBluetooth(name, address,pin);
}

bool CarlifeLink::open_page(AppPage appPage)
{
    switch (appPage) {
        case APP_PAGE_NAVIGATION:
            m_pCarlifePlayer->Key(KEYCODE_Navi);
        break;
        case APP_PAGE_MAIN:
            m_pCarlifePlayer->Key(KEYCODE_Home);
        break;
        case APP_PAGE_MUSIC:
            m_pCarlifePlayer->Key(KEYCODE_Media);
        break;
        case APP_PAGE_VR:
            m_pCarlifePlayer->Key(KEYCODE_VR_Start);
        break;
        case APP_PAGE_NAVI_HOME:
        case APP_PAGE_4S_SHOP:
        case APP_PAGE_CAR_PARK:
        case APP_PAGE_NAVI_WORK:
        case APP_PAGE_NAVI_GAS_STATION:
        break;
    }
    return true;
}

void CarlifeLink::video_start_callback_func(bool start, int width, int height, void *parameter)
{
    if(start)
        video_start(0, 0, width, height);
    else
        video_stop();
}

void CarlifeLink::video_callback_func(int width, int height, unsigned char* data, int length, void* parameter)
{
     video_play(data, length);
}

void CarlifeLink::audio_start_callback_func(bool start, int type, int rate, int bit, int channel,void* parameter)
{
    if(start)
        audio_start((AudioType)type, rate, bit, channel);
    else
        audio_stop((AudioType)type);
}

void CarlifeLink::audio_callback_func(int type, unsigned char* data, int length, void* parameter)
{
    audio_play((AudioType)type, data, length);
}

void CarlifeLink::phone_number_callback_func(string number, void* parameter)
{
    mPhoneNumber = number;
}

#endif
