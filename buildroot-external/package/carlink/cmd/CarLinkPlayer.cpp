#include "CarLinkPlayer.h"


#ifdef USE_DBUS
void InterfaceCallbackImpl::start(int type, int mode, int state)
{
    if(mHandle){
        mHandle->requestLink((LinkType)type, (LinkMode)mode, (DbusReceive)state);
    }
}

void InterfaceCallbackImpl::touch(int x, int y, int press)
{
    if(mHandle){
        mHandle->requestTouch(x,y, (TouchCode)press);
    }
}

void InterfaceCallbackImpl::multi_touch(int x1, int y1, int press1, int x2, int y2, int press2)
{
    if(mHandle){
        mHandle->requestMultiTouch(x1,y1, (TouchCode)press1, x2, y2, (TouchCode)press2);
    }

}

void InterfaceCallbackImpl::key(int key){
    if(mHandle){
        mHandle->requestKey((KeyCode)key);
    }
}

void InterfaceCallbackImpl::wheel(int wheel, bool foucs)
{
    if(mHandle){
        mHandle->requestWheel((WheelCode)wheel, foucs);
    }
}

void InterfaceCallbackImpl::night_mode(bool night)
{
    if(mHandle){
        mHandle->requestNightMode(night);
    }
}

void InterfaceCallbackImpl::right_hand_dirver(bool right)
{
    if(mHandle){
        mHandle->requestRightHandDriver(right);
    }
}

void InterfaceCallbackImpl::page(int page)
{
    if(mHandle){
        mHandle->requestPage((AppPage)page);
    }
}

void InterfaceCallbackImpl::phone_ip(string ipstring)
{
    if(mHandle){
        mHandle->requestPhoneIPAddress(ipstring);
    }
}

void InterfaceCallbackImpl::phone_bt(string btstring){

    if(mHandle){
        mHandle->requestPhoneBTAddress(btstring);
    }
}

void InterfaceCallbackImpl::car_bluetooch(string name, string address, string pin)
{
    if(mHandle){
        mHandle->requestCarBluetooth(name, address, pin);
    }

}

void InterfaceCallbackImpl::wifi(string ssid, string passphrase, string channel_id){
    if(mHandle){
        mHandle->requestWifi(ssid, passphrase, channel_id);
    }
}

void InterfaceCallbackImpl::license(string lisence){
    if(mHandle){
        mHandle->requestLicense(lisence);
    }
}

void InterfaceCallbackImpl::input_text(string text){
    if(mHandle){
        mHandle->requestInputText(text);
    }
}

void InterfaceCallbackImpl::input_selection(int start, int stop){
    if(mHandle){
        mHandle->requestInputSelection(start, stop);
    }
}

void InterfaceCallbackImpl::input_action(int actionId, int keyCode){
    if(mHandle){
        mHandle->requestInputAction(actionId, keyCode);
    }
}

void InterfaceCallbackImpl::screen_size(int width, int height){
    if(mHandle){
        mHandle->requestScreenSize(width, height);
    }
}

void InterfaceCallbackImpl::bluetooth_cmd(string cmd){
    if(mHandle){
        mHandle->requestBluetoothCmd(cmd);
    }
}

void InterfaceCallbackImpl::broadcast(bool enable){
    if(mHandle){
        mHandle->requestBroadcast(enable);
    }
}

void InterfaceCallbackImpl::delay_record(int millisecond){
    if(mHandle){
        mHandle->requestDelayRecord(millisecond);
    }
}

void InterfaceCallbackImpl::wifi_state_changed(int action, int state, string phoneIp, string carIp){
    if(mHandle){
        mHandle->requestWifiStateChanged((WifiStateAction)action, (WifiState)state, phoneIp, carIp);
    }
}

#endif

CarLinkPlayer::CarLinkPlayer():mPlayer(nullptr),mIsRunningBackGround(false),
    mpLinkAssist(nullptr),mLinkMode(Auto),mChangeMode(false),mNightMode(0),mRightHandDriver(0),mScreenWidth(0),mScreenHeight(0),mMillisecond(0),mStartCarLink(false),mAppStatus(nullptr),mUsbState(nullptr)
{
#ifdef USE_DBUS
    mpCallbacks = new InterfaceCallbackImpl(this);
#endif
}

CarLinkPlayer::~CarLinkPlayer()
{
#ifdef USE_DBUS
    delete mpCallbacks;
#endif
    if(mpLinkAssist)
        delete mpLinkAssist;
}

void CarLinkPlayer::initialize()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_DBUS
    ArkDbus::instance()->start();
    ArkDbus::instance()->registerCallbacks(mpCallbacks);
#endif
    mpLinkAssist = new LinkAssist();
    mpLinkAssist->RegisterUsbCallback(std::bind(&CarLinkPlayer::usb_state,this, std::placeholders::_1,std::placeholders::_2));
/*    if(handle){
        mHandle = (void(*)(int,int))handle;
    }
*/
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

#ifdef USE_DBUS
void CarLinkPlayer::requestLink(LinkType linkType, LinkMode linkMode, DbusReceive status)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mLinkMode = linkMode;
    if(status == DBUS_REQUEST_CONNECT){
        ArkDbus::instance()->SendLinkStatus(linkType, linkMode, DBUS_CONNECTTING);
        mLinkType = (LinkType)linkType;
        Start((LinkType)linkType, linkMode);
    }else if(status == DBUS_REQUEST_BACKGROUND){
        mPlayer->SetBackground();
    }else if(status == DBUS_REQUEST_FOREGROUND){
        mPlayer->SetForeground();
    }else if(status == DBUS_REQUEST_EXITED){
        mPlayer->Stop();
    }
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void CarLinkPlayer::requestTouch(int x, int y, TouchCode pressed)
{
    if(mPlayer){
        mPlayer->SendTouch(x, y, pressed);
    }
}

void CarLinkPlayer::requestMultiTouch(int x1, int y1, TouchCode pressed1, int x2, int y2, TouchCode pressed2)
{
    if(mPlayer){
        mPlayer->SendMultiTouch(x1, y1, pressed1, x2, y2, pressed2);
    }
}

void CarLinkPlayer::requestKey(KeyCode key)
{
    if(mPlayer){
        mPlayer->SendKey(key);
    }
}

void CarLinkPlayer::requestWheel(WheelCode wheelCode, bool bFoucs)
{
    if(mPlayer){
        mPlayer->SendWheel(wheelCode, bFoucs);
    }
}

void CarLinkPlayer::requestPage(AppPage appPage)
{
    if(mPlayer){
        mPlayer->OpenPage(appPage);
    }
}

void CarLinkPlayer::requestNightMode(bool night)
{
    mNightMode = night;
}

void CarLinkPlayer::requestRightHandDriver(bool right)
{
    mRightHandDriver = right;
}

void CarLinkPlayer::requestPhoneIPAddress(string str)
{
    mstrIpAddress = str;
    printf("requestPhoneIPAddress str = %s", str.c_str());
}

void CarLinkPlayer::requestPhoneBTAddress(string str)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mstrBtAddress = str;
}

void CarLinkPlayer::requestCarBluetooth(string name, string address, string pin)
{
    BlueToothInfo blueToothInfo = {name, address, pin};
    mBlueToothInfo = blueToothInfo;
}

void CarLinkPlayer::requestWifi(string ssid, string passphrase, string channel_id)
{
    printf("requestWifi ssid = %s, passphrase = %s, channel_id = %s\n",ssid.c_str(), passphrase.c_str(), channel_id.c_str());
    mWifiInfo.ssid = ssid;
    mWifiInfo.passphrase = passphrase;
    mWifiInfo.channel_id = channel_id;
}

void CarLinkPlayer::requestLicense(string license)
{
    mLicense = license;
}

void CarLinkPlayer::requestInputText(string text)
{
    if(mPlayer){
        mPlayer->SendInputText(text);
    }
}

void CarLinkPlayer::requestInputSelection(int start, int stop)
{
    if(mPlayer){
        mPlayer->SendInputSelection(start, stop);
    }
}

void CarLinkPlayer::requestInputAction(int actionId, int keyCode)
{
    if(mPlayer){
        mPlayer->SendInputAction(actionId, keyCode);
    }
}

void CarLinkPlayer::requestScreenSize(int width, int height)
{
    if(mPlayer){
        mPlayer->SendScreenSize(width, height);
    }
}

void CarLinkPlayer::requestBluetoothCmd(string cmd){

    if(mStartCarLink){
        mPlayer->SendBlueToothCmd(cmd);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else{
        printf("%s:%s:%d cmd: %s\r\n",__FILE__,__func__,__LINE__,cmd.c_str());
        mVecBlueToothCmd.push_back(cmd);
    }
}

void CarLinkPlayer::requestBroadcast(bool enable)
{
    if(mPlayer){
        mPlayer->SendBroadcast(enable);
    }
}

void CarLinkPlayer::requestDelayRecord(int millisecond)
{
     mMillisecond = millisecond;
}

void CarLinkPlayer::requestWifiStateChanged(WifiStateAction action, WifiState state, string phoneIp, string carIp)
{
    if(mPlayer){
        mPlayer->SendWifiStateChanged(action, state, phoneIp, carIp);
    }
}

#endif

void CarLinkPlayer::app_status(AppStatusMessage appStatusMessage, void *reserved)
{
    if(appStatusMessage == APP_FOREGROUND){
        mIsRunningBackGround = false;
#ifdef USE_DBUS
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_FOREGROUND);
#endif
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else if(appStatusMessage == APP_BACKGROUND){
        mIsRunningBackGround = true;
#ifdef USE_DBUS
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_BACKGROUND);
#endif
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else if(appStatusMessage == APP_RESERVED){

        long long time = (long long )reserved;
        struct tm *tdate;
        time_t t = time;
        tdate=localtime(&t);
        char *ptime = ctime(&t);
        printf("app_reserved long time = %lld, string time =%s\r\n", time, ptime);
    }
#ifdef USE_DBUS
    else if(appStatusMessage == APP_MUSIC_STARTED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_MUSIC_STARTED);
    }
    else if(appStatusMessage == APP_MUSIC_STOPPED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_MUSIC_STOPPED);
    }
    else if(appStatusMessage == APP_NAVI_STARTED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_NAVI_STARTED);
    }
    else if(appStatusMessage == APP_NAVI_STOPPED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_NAVI_STOPPED);
    }
    else if(appStatusMessage == APP_VR_STARTED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_VR_STARTED);
    }
    else if(appStatusMessage == APP_VR_STOPPED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_VR_STOPPED);
    }
    else if(appStatusMessage == APP_RECOGNITION_STARTED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_RECOGNITION_STARTED);
    }
    else if(appStatusMessage == APP_RECOGNITION_STOPPED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_RECOGNITION_STOPPED);
    }
    else if(appStatusMessage == APP_PHONE_STARTED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_PHONE_STARTED);
    }
    else if(appStatusMessage == APP_PHONE_STOPPED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_PHONE_STOPPED);
    }
    else if(appStatusMessage == APP_NAVI_SOUND_STARTED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_NAVI_SOUND_STARTED);
    }
    else if(appStatusMessage == APP_NAVI_SOUND_STOPPED){
        ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_NAVI_SOUND_STOPPED);
    }
    else if(appStatusMessage == APP_VOICE_CMD){
        int cmd = (int )reserved;
        printf("cmd = %d\r\n", cmd);
        ArkDbus::instance()->SendCarCmd(mLinkType, cmd);
    }
    else if(appStatusMessage == APP_LICENSE){
        License *pLicense = (License*)reserved;
        ArkDbus::instance()->SendLicenseStatus(mLinkType, pLicense->activate, pLicense->code, pLicense->msg);
    }
    else if(appStatusMessage == APP_INPUTINFO){
        InputInfo *pInputInfo = (InputInfo*)reserved;
        ArkDbus::instance()->SendInputStart(mLinkType, pInputInfo->inputType, pInputInfo->imeOptions, pInputInfo->rawText,
                                            pInputInfo->minLines,pInputInfo->maxLines, pInputInfo->maxLength);
    }
    else if(appStatusMessage == APP_VRTEXT){
        VRTextInfo *pVRTextInfo = (VRTextInfo*)reserved;
        ArkDbus::instance()->SendVRTextinfoChange(mLinkType,pVRTextInfo->type, pVRTextInfo->sequence, pVRTextInfo->plainText, pVRTextInfo->htmlText );
    }
    else if(appStatusMessage == APP_VERSION){
        char*  ver = (char*)reserved;
        printf("ver = %s\n",ver);
        ArkDbus::instance()->SendCarLinkVersion(mLinkType, ver);
    }
    else if(appStatusMessage == APP_VOICE_DUCK){
        DuckInfo *pDuckInfo = (DuckInfo*)reserved;
        ArkDbus::instance()->SendLinkDuckAudio(mLinkType, pDuckInfo->inDurationSecs, pDuckInfo->volume);
        printf("duck appStatusMessage inDurationSecs = %lf, inVolume = %lf\r\n", pDuckInfo->inDurationSecs, pDuckInfo->volume);
    }
    else if(appStatusMessage == APP_VOICE_UNDUCK){
        double* inDurationSecs = (double*) reserved;
        ArkDbus::instance()->SendLinkUnduckAudio(mLinkType, *inDurationSecs);
        printf("unduck appStatusMessage inDurationSecs = %lf\r\n", *inDurationSecs);
    }
    else if(appStatusMessage == APP_PINCODE){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        string* pinCode = (string*) reserved;
        ArkDbus::instance()->SendPinCode(mLinkType, *pinCode);
    }
    else if(appStatusMessage == APP_BT_CMD){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        string *cmd = (string*)reserved;
        printf("======= send bluetooth cmd = %s", cmd->c_str());
        ArkDbus::instance()->SendBlueToothCmd(mLinkType, *cmd);
    }
    else if(appStatusMessage == APP_CARLINK_INIT_DONE){
        ArkDbus::instance()->SendCarLinkInitDone(mLinkType);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
#endif
    if(mAppStatus){
        mAppStatus(appStatusMessage, reserved);
    }
}

void CarLinkPlayer::carlink_connect_state(ConnectedStatus status, PhoneType type)
{
    switch (status)
    {
        case CONNECT_STATUS_CONNECTING:
        {
            // todo 正在连接
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_DBUS
            ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_CONNECTTING);
#endif
            mChangeMode = true;
            break;
        }
        case CONNECT_STATUS_CONNECT_SUCCEED:
        {
            // todo 连接成功
            mChangeMode = false;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            //emit Change(false);
#ifdef USE_DBUS
            ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_CONNECTED);
#endif
            printf("connect succeed\r\n");
            break;
        }
        case CONNECT_STATUS_CONNECT_FAILED:
        case CONNECT_STATUS_DISCONNECTED:
        case CONNECT_STATUS_APP_EXIT:
        case CONNECT_STATUS_INTERRUPTED_BY_APP:
        {
            // todo 连接断开
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("connect disconnected :%d\r\n",status);
            Stop();
#ifdef USE_DBUS
            ArkDbus::instance()->SendLinkStatus(mLinkType, mLinkMode, DBUS_DISCONNECTED);
#endif
            if(mLinkType == Carplay && mLinkMode == Wired){
#ifdef USE_DBUS
                ArkDbus::instance()->SendPhoneType(mPhonetype, DBUS_DEVICE_DEATTACHED);
#endif
            }
            mChangeMode = false;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        default:
            break;
    }
    mConnectStatus = status;
    mPhonetype = type;


}

void CarLinkPlayer::usb_state(ConnectedStatus status, PhoneType type)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mConnectStatus = status;
    mPhonetype = type;
    mLinkMode = Wired;

     printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    if(status == CONNECT_STATUS_DEVICE_ATTACHED){

        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(!mChangeMode){
            if(mUsbState){
                mUsbState(status, type);
            }

            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_DBUS
            ArkDbus::instance()->SendPhoneType(mPhonetype, DBUS_DEVICE_ATTACHED);
#else
            if(type == Phone_IOS)
                Start(Carplay, Wired);
            else if(type == Phone_Android)
                Start(Android_Auto, Wired);
#endif
        }
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    }
    else if(status == CONNECT_STATUS_DEVICE_DEATTACHED){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(mUsbState){
            mUsbState(status, type);
        }
#ifdef USE_DBUS
        ArkDbus::instance()->SendPhoneType(mPhonetype, DBUS_DEVICE_DEATTACHED);
#endif
    }
}

void CarLinkPlayer::registerAppStatus(void(*pAppStatusCallback)(int,void*))
{
    mAppStatus = pAppStatusCallback;
}

void CarLinkPlayer::registerUsbState(void(*pUsbStateCallback)(int,int))
{
    mUsbState = pUsbStateCallback;
}

void CarLinkPlayer::Start(LinkType linkType, LinkMode linkMode)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer = mpLinkAssist->Initialize(linkType);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer->GetIniConfig(mpLinkAssist);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer->RegisterConnectCallback(std::bind(&CarLinkPlayer::carlink_connect_state,this, std::placeholders::_1,std::placeholders::_2));
    if(mLicense.empty())
        mPlayer->SendLisenceCode("TEST036514313165461313");
    else
        mPlayer->SendLisenceCode(mLicense);

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(!(mScreenWidth == 0 || mScreenHeight == 0)){
        mPlayer->SendScreenSize(mScreenWidth, mScreenHeight);
    }

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer->SendNightMode(mNightMode);
    printf("%s:%s:%d right hand driver =%d\r\n",__FILE__,__func__,__LINE__,mRightHandDriver);
    mPlayer->SendRightHandDriver(mRightHandDriver);
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer->SendDelayRecord(mMillisecond);

    mPlayer->SendCarBluetooth(mBlueToothInfo.name, mBlueToothInfo.addr, mBlueToothInfo.pin);


    if(linkMode == Wireless)
    {
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(!mstrBtAddress.empty()){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mPlayer->SendPhoneBluetooth(mstrBtAddress);
        }
        if(!mstrIpAddress.empty()){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mPlayer->SendIphoneMacAddress(mstrIpAddress);
        }
        if(!(mWifiInfo.ssid.empty()| mWifiInfo.passphrase.empty() | mWifiInfo.channel_id.empty())){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mPlayer->SendCarWifi(mWifiInfo);
        }     
    } 
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer->RegisterAppStatusCallback(std::bind(&CarLinkPlayer::app_status,this, std::placeholders::_1,std::placeholders::_2));    
    mPlayer->Initialize(linkMode, mPhonetype);
    if(linkType == HiCar && linkMode == Wireless){
        for(int i = 0; i< mVecBlueToothCmd.size();i++){
            mPlayer->SendBlueToothCmd(mVecBlueToothCmd[i]);
            printf("%s:%s:%d mVecBlueToothCmd[%d]: %s\r\n",__FILE__,__func__,__LINE__,i,mVecBlueToothCmd[i].c_str());
        }
    }
    mVecBlueToothCmd.clear();

    mPlayer->Start();
    mStartCarLink = true;

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}

void CarLinkPlayer::Stop()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mpLinkAssist)
        mpLinkAssist->Release();
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}


