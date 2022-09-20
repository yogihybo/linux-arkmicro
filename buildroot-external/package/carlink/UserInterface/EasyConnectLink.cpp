#include "EasyConnectLink.h"
#include <unistd.h>
string gStringRec;
EasyConnectLink::EasyConnectLink()
{
#ifdef USE_EASYCONNECT
    mHasInitRec = false;
#endif
}

EasyConnectLink::~EasyConnectLink()
{

}

#ifdef  USE_EASYCONNECT
bool EasyConnectLink::init(LinkMode linkMode)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    ECLogConfig logCfg;
    logCfg.logLevel = ECSDKFrameWork::EC_LOG_LEVEL_ALL;
    logCfg.logModule = ECSDKFrameWork::EC_LOG_MODULE_SDK | ECSDKFrameWork::EC_LOG_MODULE_APP;
    logCfg.logType = ECSDKFrameWork::EC_LOG_OUT_STD;
    logCfg.logFile = "";
    ECSDKFramework::getInstance()->setECSDKLogConfig(logCfg);

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    ECSDKFrameWork::ECMirrorConfig mirrorCfg;    // ECMirrorConfig has a default constructor and does not require memset
    int width = 1920;
    int height = 720;
    mirrorCfg.width = width;
    mirrorCfg.height = height;
    mirrorCfg.type = ECSDKFrameWork::EC_VIDEO_TYPE_H264;
    mirrorCfg.quality = 4 * 1024 * 1024;

    mirrorListener.SetMirrorDirCallback(mirror_direction_callback,this);
    mirrorListener.SetMirrorStatusCallback(mirror_status_callback, this);

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    videoplayer.registerStartVideoCallback(std::bind(&EasyConnectLink::start_video_callback_func, this,std::placeholders::_1,std::placeholders::_2, std::placeholders::_3));
    videoplayer.registerVideoDataCallback(std::bind(&EasyConnectLink::video_data_callback_func, this,std::placeholders::_1,std::placeholders::_2));
    ECSDKFrameWork::ECSDKMirrorManager::getInstance()->initialize(&videoplayer, &mirrorListener);
    ECSDKFrameWork::ECSDKMirrorManager::getInstance()->setMirrorConfig(mirrorCfg);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    audioPlayer.registerStartAudioCallback(std::bind(&EasyConnectLink::start_audio_callback_func, this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4,std::placeholders::_5));
    audioPlayer.registerAudioDataCallback(std::bind(&EasyConnectLink::audio_data_callback_func, this,std::placeholders::_1,std::placeholders::_2, std::placeholders::_3));
    audioRecoder.registerRecStartCallback(std::bind(&EasyConnectLink::start_rec_callback_func, this,std::placeholders::_1,std::placeholders::_2, std::placeholders::_3,std::placeholders::_4));
    audioRecoder.registerRecDataCallback(std::bind(&EasyConnectLink::rec_data_callback_func, this,std::placeholders::_1));
    ECSDKFrameWork::ECSDKAudioManager::getInstance()->initialize(&audioPlayer,&audioRecoder);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    appListener.registerAppStatusCallback(std::bind(&EasyConnectLink::appstatus_callback_func, this,std::placeholders::_1));
    appListener.registerPhoneCallCallback(std::bind(&EasyConnectLink::phonecall_callback_func, this,std::placeholders::_1,std::placeholders::_2, std::placeholders::_3));
    appListener.registerInputStartCallback(std::bind(&EasyConnectLink::inputstart_callback_func, this,std::placeholders::_1,std::placeholders::_2, std::placeholders::_3,std::placeholders::_4,std::placeholders::_5, std::placeholders::_6));
    appListener.registerInputSelectionCallback(std::bind(&EasyConnectLink::inputselection_callback_func, this,std::placeholders::_1,std::placeholders::_2));
    appListener.registerVRTextInfoCallback(std::bind(&EasyConnectLink::vr_textinfo_callback_func, this,std::placeholders::_1,std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    appListener.registerPhoneInfoCallback(std::bind(&EasyConnectLink::phoneinfo_callback_func, this,std::placeholders::_1));
    appListener.registerCarCmdCallback(std::bind(&EasyConnectLink::carcmd_callback_func, this,std::placeholders::_1));
    ECSDKAPPManager::getInstance()->initialize(&appListener);

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    if(linkMode == Wireless){
        toolkitListener.SetQRCallback(qr_callback_func,this);
        ECSDKToolKit::getInstance()->initialize(&toolkitListener);
    }

/*
    ECOTAResourceVersion OTARes;
    OTARes.softWareId = "123f071c";
    OTARes.softVersion = 5;

    ECOTAConfig OTAConf;
    OTAConf.configPath = "/tmp";
    OTAConf.packagePath = "/tmp";
    OTAConf.otaResourceVersionList.push_back(OTARes);
    ECSDKOTAManager::getInstance()->initialize(&otaListener,OTAConf);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
*/

    // 设置sdk初始化参数

    string verion = "1.0";

    if(mLicense.empty()){
        mLicense = "TEST036514313165461313";
    }
    ECSDKFrameWork::ECSDKConfig cfg;
    cfg.setUUID(mLicense);
    cfg.setVersion(verion);
    cfg.setWorkSpace("/data");

    string uuid;
    cfg.getCommonConfig("auth_uuid", uuid);

    uint32_t supportFunction = EC_SUPPORT_FUNCTION_INPUT;
    cfg.setCommonConfig("car_supportFunction", supportFunction);


    sdkListener.SetLicenseAuthSuccessCallback(license_auth_success_func, this);
    sdkListener.SetLicenseAuthFailCallback(license_auth_failed_func,this);
    sdkListener.registerStatusCallback(std::bind(&EasyConnectLink::status_callback_func, this,std::placeholders::_1,std::placeholders::_2));
    ECSDKFramework::getInstance()->initialize(&cfg, &sdkListener);    
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    string ver = ECSDKFramework::getInstance()->getECVersion();
    static const char *p = ver.c_str();
    printf("==============carlink version = %s =============\n", p);
    app_status(APP_VERSION, (void*)p);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    return true;
}

bool EasyConnectLink::release()
{

    ECSDKFramework::getInstance()->release();

    return true;
}

bool EasyConnectLink::start()
{
    ECSDKFramework::getInstance()->start();
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    return true;
}

bool EasyConnectLink::stop()
{

    ECSDKFramework::getInstance()->stop();

    return true;
}

bool EasyConnectLink::start_mirror()
{
    ECSDKFrameWork::ECSDKMirrorManager::getInstance()->startMirror();
}

bool EasyConnectLink::stop_mirror()
{
    ECSDKFrameWork::ECSDKMirrorManager::getInstance()->stopMirror();
}

bool EasyConnectLink::set_background()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    stop_mirror();
    app_status(APP_BACKGROUND);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

bool EasyConnectLink::set_foreground()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    start_mirror();
    app_status(APP_FOREGROUND);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

bool EasyConnectLink::get_audio_focus()
{

}

bool EasyConnectLink::release_audio_focus()
{

}

void EasyConnectLink::set_inserted(bool inserted, PhoneType phoneType)
{

}

void EasyConnectLink::send_screen_size(int width, int height)
{

}

#define DUMP_REC_FILE 0
#if DUMP_REC_FILE
static FILE *pRecfile = NULL;
#endif


void EasyConnectLink::record_audio_callback(unsigned char *data, int len)
{
       printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);


       if(mHasInitRec){
           // mRecData += string((char*)data, len);
            //printf("record_audio_callback = %d\r\n", mRecData.length());
            //audioRecoder.setRecData(mRecData);
           std::lock_guard<std::mutex> lock(mMutex);
           gStringRec += string((char*)data, len);
           //mRecSemaphore.up();
       }

        usleep(10000);
   #if DUMP_REC_FILE
       if (NULL == pRecfile) {
           pRecfile = fopen("/tmp/rec_in.pcm", "w");
       }

       if (pRecfile) {
           fwrite(data, 1, len, pRecfile);
           //printf("rec len:%d\r\n ", len);
       }
   #endif
       printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void EasyConnectLink::send_car_bluetooth(const string& name, const string& address, const string& pin)
{
    mBlueToothInfo.name = name;
    mBlueToothInfo.addr = address;
    mBlueToothInfo.pin = pin;
}

void EasyConnectLink::send_phone_bluetooth(const string& address)
{

}

void EasyConnectLink::send_touch(int x, int y, TouchCode touchCode)
{
    if(touchCode == 1)
    {
        ECSDKFrameWork::ECTouchEventData data;
        data.pointX = x;
        data.pointY = y;
        data.slot = 0;
        ECSDKTouchKeyManager::getInstance()->sendTouchEvent(data, EC_TOUCH_DOWN);
    }
    else if(touchCode == 0)
    {
        ECSDKFrameWork::ECTouchEventData data;
        data.pointX = x;
        data.pointY = y;
        data.slot = 0;
        ECSDKTouchKeyManager::getInstance()->sendTouchEvent(data, EC_TOUCH_UP);
    }
    else if(touchCode == 2)
    {
        ECSDKFrameWork::ECTouchEventData data;
        data.pointX = x;
        data.pointY = y;
        data.slot = 0;
        ECSDKTouchKeyManager::getInstance()->sendTouchEvent(data, EC_TOUCH_MOVE);
    }

}
bool EasyConnectLink::send_key(KeyCode keyCode)
{
    if(keyCode == KEY_VOICE_ASSISTANT){
        ECSDKTouchKeyManager::getInstance()->sendBtnEvent(EC_BTN_VOICE_ASSISTANT, EC_BTN_TYPE_CLICK);
    }
    else if(keyCode == KEY_MUSIC_PLAY){
        ECSDKTouchKeyManager::getInstance()->sendBtnEvent(EC_BTN_MUSIC_PLAY, EC_BTN_TYPE_DOWN);
    }
    else if(keyCode == KEY_MUSIC_PAUSE){
        ECSDKTouchKeyManager::getInstance()->sendBtnEvent(EC_BTN_MUSIC_PAUSE, EC_BTN_TYPE_DOWN);
    }
    else if(keyCode == KEY_MUSIC_NEXT){
        ECSDKTouchKeyManager::getInstance()->sendBtnEvent(EC_BTN_MUSIC_NEXT, EC_BTN_TYPE_DOWN);
    }
    else if(keyCode == KEY_MUSIC_PREVIOUS){
        ECSDKTouchKeyManager::getInstance()->sendBtnEvent(EC_BTN_MUSIC_PREVIOUS, EC_BTN_TYPE_DOWN);
    }
}

bool EasyConnectLink::send_wheel(WheelCode wheel, bool foucs)
{

}

bool EasyConnectLink::send_night_mode(bool night)
{

    return true;
}

bool EasyConnectLink::open_page(AppPage appPage)
{

}

void EasyConnectLink::request_status(RequestAppStatus requestAppStatus, void *reserved)
{

    if(requestAppStatus == QUERYQRCODE){

        //app_status(APP_RESERVED, (void*)mQRCode.c_str());
    }
    else if(requestAppStatus == QUERYINPUT){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        char *ptr = (char*)reserved;
        ECSDKAPPManager::getInstance()->sendInputText(ptr);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
}

void EasyConnectLink::send_license(const string& license)
{
    mLicense = license;
}

void EasyConnectLink::send_input_text(const string& text)
{
    ECSDKAPPManager::getInstance()->sendInputText(text);
}

void EasyConnectLink::send_input_selection(const int start, const int stop)
{
     ECSDKAPPManager::getInstance()->sendInputSelection(start, stop);
}

void EasyConnectLink::send_input_action(const int actionId, const int keyCode)
{
    ECSDKAPPManager::getInstance()->sendInputAction(actionId, keyCode);
}

void EasyConnectLink::send_wifi_state_changed(WifiStateAction action, WifiState state, string& phoneIp, string& carIp)
{
    ECNetWorkInfo info = {(int32_t)EC_WIFI_STATE_CONNECTED, phoneIp, carIp};
    ECSDKFramework::getInstance()->notifyWifiStateChanged((ECWifiStateAction)action, info);
}

void EasyConnectLink::start_video_callback_func(bool start, int width, int height)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(start)
        video_start(0, 0, width, height);
    else
        video_stop();
}

void EasyConnectLink::video_data_callback_func(unsigned char* data, int length)
{
    //printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    video_play(data, length);
}

void EasyConnectLink::start_audio_callback_func(bool start, int type, int rate, int bit, int channel)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(start)
        audio_start((AudioType)type, rate, bit, channel);
    else
        audio_stop((AudioType)type);
}

void EasyConnectLink::audio_data_callback_func(int type, unsigned char* data, int length)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    audio_play((AudioType)type, data, length);
}

void EasyConnectLink::start_rec_callback_func(bool start, int rate, int bit, int channel)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    AudioInfo info = {(uint32_t)rate, (uint32_t)bit, (uint32_t)channel};
    mHasInitRec = start;

    if(start){
        record_start(info);
    }
    else
        record_stop();
}

void EasyConnectLink::rec_data_callback_func(string& recData)
{
    //printf("EasyConnectLink rec_data_callback_func==============\r\n");
    //sleep(20);

   // printf("EasyConnectLink rec_data_callback_func==============1111\r\n");
    //std::lock_guard<std::mutex> lock(mMutex);
    //printf("EasyConnectLink recdata1 = %d\r\n", recData.length());
    recData = gStringRec;
    //printf("EasyConnectLink recdata2 = %d\r\n", recData.length());
    //mRecData.clear();

}

void EasyConnectLink::status_callback_func(int status, int type)
{
#ifdef USE_EASYCONNECT
    printf("%s:%s:%d status = %d\r\n",__FILE__,__func__,__LINE__, status);

    PhoneType phoneType = UnKnown;
    if(type == EC_CONNECT_TYPE_ANDROID_USB){
        phoneType = Phone_Android;
    }
    else if(type == EC_CONNECT_TYPE_IOS_USB_EAP ||
            type == EC_CONNECT_TYPE_IOS_USB_MUX ||
            type == EC_CONNECT_TYPE_IOS_USB_AIRPLAY||
            type == EC_CONNECT_TYPE_IOS_USB_LIGHTNING)
    {
        phoneType = Phone_IOS;
    }
    if(EC_CONNECT_STATUS_DEVICE_ATTACHED == status){
        onSdkConnectStatus(CONNECT_STATUS_DEVICE_ATTACHED, phoneType);
    }
    else if(EC_CONNECT_STATUS_DEVICE_DEATTACHED == status){
        onSdkConnectStatus(CONNECT_STATUS_DISCONNECTED, UnKnown);
    }
    else if(EC_CONNECT_STATUS_CONNECT_SUCCEED == status){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        onSdkConnectStatus(CONNECT_STATUS_CONNECT_SUCCEED, UnKnown);       
        ECSDKAPPManager::getInstance()->sendCarBluetooth(mBlueToothInfo.name,mBlueToothInfo.addr,mBlueToothInfo.pin);

        vector<ECCarCmd> cmdList;
        ECCarCmd carCmd1 = {EC_CAR_CMD_TYPE_EFFECTIVE_GLOBAL,"0", "(播放|切)上一曲", "上一曲", false};
        ECCarCmd carCmd2 = {EC_CAR_CMD_TYPE_EFFECTIVE_GLOBAL,"1", "(播放|切)下一曲", "下一曲", false};
        ECCarCmd carCmd3 = {EC_CAR_CMD_TYPE_EFFECTIVE_GLOBAL,"2", "声音(高|大)一(点|些)|[把|将](音量|声音)调高|[把|将](音量|声音)调(高|大)一(点|些)|(我要|我想)调高音量", "音量升高", false};
        ECCarCmd carCmd4 = {EC_CAR_CMD_TYPE_EFFECTIVE_GLOBAL,"3", "声音(低|小)一(点|些)|[把|将](音量|声音)调低|[把|将](音量|声音)调(低|小)一(点|些)|(我要|我想)调低音量", "音量降低", false};
        ECCarCmd carCmd5 = {EC_CAR_CMD_TYPE_EFFECTIVE_GLOBAL,"4", "[把|将]音量调节到最(大|高)|声音最(大|高)|最(大|高)声音|音量最(大|高)|最(大|高)音量", "音量调节到最大", false};
        ECCarCmd carCmd6 = {EC_CAR_CMD_TYPE_EFFECTIVE_GLOBAL,"5", "[把|将]音量调节到最(小|低)|声音最(小|低)|最(小|低)声音|音量最(小|低)||最(小|低)音量", "音量调节到最小", false};
        cmdList.push_back(carCmd1);
        cmdList.push_back(carCmd2);
        cmdList.push_back(carCmd3);
        cmdList.push_back(carCmd4);
        cmdList.push_back(carCmd5);
        cmdList.push_back(carCmd6);
        ECSDKAPPManager::getInstance()->registerCarCmds(cmdList);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    }
    else if(EC_CONNECT_STATUS_CONNECT_FAILED == status){
        onSdkConnectStatus(CONNECT_STATUS_CONNECT_FAILED, UnKnown);
    }
    else if(EC_CONNECT_STATUS_DISCONNECTED == status){
        onSdkConnectStatus(CONNECT_STATUS_DISCONNECTED, UnKnown);
    }
    else if(EC_CONNECT_STATUS_APP_EXIT == status){
        onSdkConnectStatus(CONNECT_STATUS_APP_EXIT, UnKnown);
    }
#endif
}

void EasyConnectLink::appstatus_callback_func(ECStatusMessage status)
{
    static int prev_status = -1;
    if(prev_status != status)
    {
       if(status == EC_STATUS_MESSAGE_APP_BACKGROUND_SAME || status == EC_STATUS_MESSAGE_APP_FOREGROUND_SAME){
           printf("same mirror mode\r\n");
       }
       else if(status == EC_STATUS_MESSAGE_APP_FOREGROUND ||
               status == EC_STATUS_MESSAGE_APP_FOREGROUND_SPLIT || status == EC_STATUS_MESSAGE_APP_BACKGROUND_SPLIT)
       {
            printf("app running mode\r\n");
       }
       else if(status == EC_STATUS_MESSAGE_SWITCH_TO_SYSTEM_MAIN_PAGE){
           printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            app_status(APP_BACKGROUND);
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
       }

       prev_status = status;
    }
}

void EasyConnectLink::phonecall_callback_func(ECCallType type, const string name, const string number)
{

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    bt_call_action((CallType)type, name.c_str(), number.c_str());

    printf("type = %d, name = %s, number = %s\r\n", type, name, number);
}

void EasyConnectLink::inputstart_callback_func(int inputType, int  imeOptions, string rawText, int minLines, int maxLines,  int maxLength)
{
    static InputInfo inputInfo = {0, 0, 0, 0, 0, 0};
    inputInfo.inputType = inputType;
    inputInfo.imeOptions = imeOptions;
    inputInfo.rawText = rawText;
    inputInfo.minLines = minLines;
    inputInfo.maxLines = maxLines;
    inputInfo.maxLength = maxLength;
    app_status(APP_INPUTINFO, (void*)&inputInfo);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void EasyConnectLink::inputselection_callback_func(int start, int stop)
{
    static InputPos inputPos = {0, 0};
    inputPos.start = start;
    inputPos.start = stop;
    app_status(APP_INPUTPOS, (void*)&inputPos);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void EasyConnectLink::phoneinfo_callback_func(const string& info)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("phone info = %s\r\n", info.c_str());
}

void EasyConnectLink::vr_textinfo_callback_func(int vrtype, int sequence, string plainText, string htmlText)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    static VRTextInfo vrTextInfo = {VR_TEXT_FROM_UNKNOWN, 0, "", ""};
    vrTextInfo.type = (VRTextType)vrtype;
    vrTextInfo.sequence = sequence;
    vrTextInfo.plainText = plainText;
    vrTextInfo.htmlText = htmlText;
    app_status(APP_VRTEXT, (void*)&vrTextInfo);

}

void EasyConnectLink::carcmd_callback_func(int cmd)
{
    static int easycmd = 0;
    easycmd = cmd;
    app_status(APP_VOICE_CMD, (void*)easycmd);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void EasyConnectLink::mirror_direction_callback(int direction, void* parameter)
{
    EasyConnectLink *pthis = (EasyConnectLink*)parameter;
    //emit pthis->mirror_direction_signal(direction);
}

void EasyConnectLink::mirror_status_callback(int status, void* parameter)
{
    EasyConnectLink *pthis = (EasyConnectLink*)parameter;
    if(EC_MIRROR_STATUS_MIRROR_STARTED == status){
       // system("echo 24 0 > /proc/display");
    }
}

void EasyConnectLink::qr_callback_func(string qr, void* parameter)
{
    EasyConnectLink *pthis = (EasyConnectLink*)parameter;
    pthis->mQRCode = qr;

    pthis->app_status(APP_QRCODE, (void*)pthis->mQRCode.c_str());
    printf("qr code = %s\r\n", pthis->mQRCode.c_str());
}

void EasyConnectLink::license_auth_success_func(int code, string msg, void* parameter)
{
    EasyConnectLink *pthis = (EasyConnectLink*)parameter;
    static License sid = {0,0, 0};
    sid.activate = true;
    sid.code = code;
    sid.msg = msg;
    string ver = ECSDKFramework::getInstance()->getECVersion();
    sid.ver = ver;
    pthis->app_status(APP_LICENSE, (void*)(&sid));
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void EasyConnectLink::license_auth_failed_func(int code, string msg, void* parameter){

    EasyConnectLink *pthis = (EasyConnectLink*)parameter;
    static License sid = {0,0, 0};
    sid.activate = false;
    sid.code = code;
    sid.msg = msg;
    pthis->app_status(APP_LICENSE, (void*)(&sid));
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

#endif
