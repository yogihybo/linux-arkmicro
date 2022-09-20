#ifndef CARLINKPLAYER_H
#define CARLINKPLAYER_H
#ifdef __cplusplus
#ifdef USE_DBUS
#include "ArkDbus.h"
#include "InterfaceCallback.h"
#endif

#include "IUserLinkPlayer.h"
#include "LinkAssist.h"

#include <vector>
#include <mutex>
using namespace std;



#define ArkMicroLinkService     "com.arkmicro.carlink"
#define ArkMicroLinkPath        "/com/arkmicro/carlink"
#define ArkMicroLinkInterface   "Local.DbusServer.CarLink"



class CarLinkPlayer;

#ifdef USE_DBUS
class InterfaceCallbackImpl : public InterfaceCallback
{
public:
    InterfaceCallbackImpl(CarLinkPlayer* handle) : mHandle(handle) {}

protected:
    void start(int type, int mode, int state);
    void touch(int x, int y, int press);
    void multi_touch(int x1, int y1, int press1, int x2, int y2, int press2);
    void key(int key);
    void wheel(int wheel, bool foucs);
    void night_mode(bool night);
    void right_hand_dirver(bool right);
    void page(int page);
    void phone_ip(string ipstring);
    void phone_bt(string btstring);
    void car_bluetooch(string name, string address, string pin);
    void wifi(string ssid, string passphrase, string channel_id);
    void license(string lisence);
    void input_text(string text);
    void input_selection(int start, int stop);
    void input_action(int actionId, int keyCode);
    void screen_size(int width, int height);
    void bluetooth_cmd(string cmd);
    void broadcast(bool enable);
    void delay_record(int millisecond);
    void wifi_state_changed(int action, int state, string phoneIp, string carIp);
private:
    CarLinkPlayer* mHandle;
};
#endif
class CarLinkPlayer
{
public:
    CarLinkPlayer();
    virtual ~CarLinkPlayer();

    void initialize();
    void registerAppStatus(void(*pAppStatusCallback)(int,void*));
    void registerUsbState(void(*pUsbStateCallback)(int,int));

#ifdef dbus_send_signal
    void onLinkStatus(const int type, const int mode, const int status);                   //send carlink's status to UI
    void onDateTime(const int type, const long long time);                                 //send carlink's timer to UI
    void onTelephone(const int type, const string name, const string number);              //send carlink's information to buletooth
    void onPhoneType(const int type, int inserted);                                        //send carlink's type to UI
    void onCarCmd(const int type, const int carcmd);                                       //send carlink's voices controls to UI change state (eclink)
    void onLicenseStatus(const int type, int activate, int code, string msg, string ver);  //send carlink's license to UI (eclink)
    void onInputStart(const int type, const int inputType, const int imeOptions, const string rawText, const int minLines, const int maxLines, const int maxLength); //send carlink's keyboard state to UI(eclink)
    void onInputSelection(const int type, const int start, const int stop);                //send carlink's keyboard select area (elink)
    void onVrTextinfoChange(const int type, const int vrtype, const int sequence, const string plainText, const string htmlText);  //send link's voices change text to UI (eclink)
    void onCarLinkVersion(const int type,  const string ver);                              //send carlink's version to UI
    void onLinkDuckAudio(const int type, double durationSecs, double volume);              //send carlink's audio duck volume
    void onLinkUnduckAudio(const int type, double durationSecs);                           //send carlink's audio unduck volume
    void onPinCode(const int type, const string pincode);                                  //send carlink's pincode(hicar)
    void onBlueToothCmd(const int type, const string cmd);                                 //send carlink's bluetooth of "AT" cmd
    void onCarLinkInitDone(const int type);                                                //send carlink's initialize finished
#endif

public:
#ifdef USE_DBUS
    void requestLink(LinkType linkType, LinkMode linkMode, DbusReceive status);                         //UI request carlink's connect
    void requestTouch(int x, int y, TouchCode pressed);                                                 //UI request carlink's touch coordinate
    void requestMultiTouch(int x1, int y1, TouchCode pressed1,int x2, int y2, TouchCode pressed2);      //UI request carlink's multi touch coordinate
    void requestKey(KeyCode key);                                                                       //UI request carlink's key value
    void requestWheel(WheelCode wheelCode, bool bFoucs);                                                //UI request carlink's wheel value
    void requestPage(AppPage appPage);                                                                  //UI request carlink's open page
    void requestNightMode(bool night);                                                                  //UI request carlink's night mode
    void requestRightHandDriver(bool right);                                                            //UI request carlink's driver mode
    void requestPhoneIPAddress(string str);                                                             //UI request carlink's connected phone's ip address
    void requestPhoneBTAddress(string str);                                                             //UI request carlink's connected phone's bt address
    void requestCarBluetooth(string name, string address, string pin);                                  //UI request carlink's bluetooth address
    void requestWifi(string ssid, string passphrase, string channel_id);                                //UI request carlink's wifi information
    void requestLicense(string license);                                                                //UI request carlink's license(eclink)
    void requestInputText(string text);                                                                 //UI request carlink's keyboard input text(eclink)
    void requestInputSelection(int start, int stop);                                                    //UI request carlink's keyboard input selection(eclink)
    void requestInputAction(int actionId, int keyCode);                                                 //UI request carlink's keyboard input action(eclink)
    void requestScreenSize(int width, int height);                                                      //UI request carlink's screen size
    void requestBluetoothCmd(string cmd);                                                               //UI requset carlink's bt cmd
    void requestBroadcast(bool enable);                                                                 //UI requset carlink's wireless pincode
    void requestDelayRecord(int millisecond);                                                           //UI requset carlink's delay record
    void requestWifiStateChanged(WifiStateAction action, WifiState state, string phoneIp, string carIp);//UI request carlink's wireless wifi state
#endif
public:
    void Start(LinkType linkType, LinkMode linkMode);
    void Stop();


    IUserLinkPlayer* UIPlayer() const {return mPlayer;}
protected:
    void app_status(AppStatusMessage appStatusMessage, void *reserved);
    void usb_state(ConnectedStatus status, PhoneType type);
    void carlink_connect_state(ConnectedStatus status, PhoneType type);
private:

    IUserLinkPlayer     *mPlayer;
    LinkAssist          *mpLinkAssist;
#ifdef USE_DBUS
    InterfaceCallback   *mpCallbacks;
#endif
    bool                 mChangeMode;
    ConnectedStatus      mConnectStatus;
    PhoneType            mPhonetype;
    LinkType             mLinkType;
    LinkMode             mLinkMode;
    bool                 mIsRunningBackGround;
    string               mstrBtAddress;
    string               mstrIpAddress;

    WifiInfo             mWifiInfo;
    BlueToothInfo        mBlueToothInfo;
    string               mLicense;
    int                  mNightMode;
    int                  mRightHandDriver;
    int                  mScreenWidth;
    int                  mScreenHeight;
    int                  mMillisecond;
    vector<string>       mVecBlueToothCmd;
    bool                 mStartCarLink;
    std::mutex           mMutex;
    void (*mAppStatus)(int, void*);
    void (*mUsbState)(int, int);
};
#endif
#endif // CARLINKPLAYER_H
