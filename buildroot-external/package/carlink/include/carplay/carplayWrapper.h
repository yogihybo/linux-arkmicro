#ifndef __CARPLAY_INTERFACE_H
#define __CARPLAY_INTERFACE_H
#include <string>
#include <stdint.h>
#include "carplayVideoWrapper.h"
#include "carplayAudioWrapper.h"

typedef int		CarplayEntity;
#define CarplayEntity_NotApplicable			0
#define CarplayEntity_Controller			1
#define CarplayEntity_Accessory				2

typedef int		CarplayTransferType;
#define CarplayTransferType_NotApplicable				0
#define CarplayTransferType_Take						1 // Transfer ownership permanently.
#define CarplayTransferType_Untake						2 // Release permanent ownership.
#define CarplayTransferType_Borrow						3 // Transfer ownership temporarily.
#define CarplayTransferType_Unborrow					4 // Release temporary ownership.


typedef int		CarplayTransferPriority;
#define CarplayTransferPriority_NotApplicable				0
#define CarplayTransferPriority_NiceToHave					100 // Transfer succeeds only if constraint is <= Anytime.
#define CarplayTransferPriority_UserInitiated				500 // Transfer succeeds only if constraint is <= UserInitiated.


typedef int		CarplayConstraint;
#define CarplayConstraint_NotApplicable			0
#define CarplayConstraint_Anytime					100  // Resource may be taken/borrowed at any time.
#define CarplayConstraint_UserInitiated			500  // Resource may be taken/borrowed if user initiated.
#define CarplayConstraint_Never					1000 // Resource may never be taken/borrowed.


typedef int		CarplayTriState;
#define CarplayTriState_NotApplicable		0
#define CarplayTriState_False				-1
#define CarplayTriState_True				1


typedef int		CarplaySpeechMode;
#define CarplaySpeechMode_NotApplicable			0
#define CarplaySpeechMode_None						-1 // No speech-related states are active.
#define CarplaySpeechMode_Speaking					1  // Device is speaking to the user.
#define CarplaySpeechMode_Recognizing				2  // Device is recording audio to recognize speech from the user.

typedef struct
{
    CarplayEntity			entity;
    CarplaySpeechMode		mode;

}	CarPlaySpeechState;

typedef struct
{
    CarplayEntity			screen;		// Owner of the screen.
    CarplayEntity			mainAudio;	// Owner of main audio.
    CarplayEntity			phoneCall;	// Owner of phone call.
    CarPlaySpeechState		speech;		// Owner of speech and its mode.
    CarplayEntity			turnByTurn;	// Owner of navigation.

} CarPlayModeState;

typedef enum
{
	UsbHost = 0,
	UsbDevice
} UsbMode;


class CarplayWrapper;
class ICarplayCallbacks
{
public:
    virtual void iap2LinkStatus(int status) = 0;
    
    virtual int iap2WriteData(char *buf, int len) = 0;

    virtual void carplaySessionStart() = 0;

    virtual void carplaySessionStop() = 0;

	virtual int switchUsbModeCB(UsbMode mode) = 0;

    virtual void appleTimeUpdateCB(long long time, int zone_offset) = 0;
    
    virtual void appleLanguageUpdateCB(const char *lang) = 0;
    
    virtual void NotifyDeviceNameCB(const char *name, int name_len) = 0;
    
    /* *
     * @brief 退出的通知
     * */
    virtual void carplayExitCB() = 0;
    /* *
     * @brief 点击返回本地界面的按钮通知
     * */
    virtual void returnNativeUICB() = 0;
    /* *
     * @brief carplay模式改变的通知
     * */
    virtual void modesChangeCB(CarPlayModeState *modes) = 0;
    /* *
     * @brief carplay禁止蓝牙
     * */
	virtual void disableBluetoothCB() = 0;

	/* *
     * @brief 压低其他音频流音量
     * */
    virtual void caplayDuckAudioCB(double inDurationSecs, double inVolume) = 0;
    /* *
     * @brief 还原其他音频流音量
     * */
    virtual void caplayUnduckAudioCB(double inDurationSecs) = 0;
};

class ICarplayVideoCallbacks;
class ICarplayAudioCallbacks;
class ICarplayCallbacks;
class Mutex;
class CarplayWrapper
{
public:

	CarplayWrapper();
	
	bool init();
	void registerCallbacks(ICarplayCallbacks *pcbs);
	void registerVideoCallbacks(ICarplayVideoCallbacks *pcbs);
	void registerAudioCallbacks(ICarplayAudioCallbacks *pcbs);

	int32_t CarplayStart();
	void CarplayStop();
	void CarplaySetIntParameter(const std::string& key, const int32_t& value);
	void CarplaySetStringParameter(const std::string& key, const std::string& value);
	int  CarplayGetIntValue(const std::string& key);
    std::string CarplayGetStringValue(const std::string& key);
	
	void CarplaySendMediaKey(const int32_t& button, const bool& pressed);
	void CarplaySendTelephoneKey(const int32_t& button, const bool& pressed);
	void CarplaysendSiriButton(const bool& pressed);
	void CarplaySendSingleTouchPoint(const uint16_t& x, const uint16_t& y, const bool& pressed);
	void CarplaySendKnob(const uint8_t& selectButton, const uint8_t& homeButton, const uint8_t& backButton, const double& x, const double& y, const uint8_t& wheelPositionRelative);
	void CarplayForceKeyFrame();
	void CarplayRequestUI(const std::string& url);
	void CarplayChangeModes(const int32_t& ScreenType, const int32_t& ScreenPriority, const int32_t& ScreenTake, const int32_t& ScreenBorrow, const int32_t& AudioType, const int32_t& AudioPriority, const int32_t& AudioTake, const int32_t& AudioBorrow, const int32_t& Phone, const int32_t& Speech, const int32_t& TurnByTurn);
	
	
	/* *
     * @brief 获取carplay送过来的音频数据,然后送到声卡播放
     * @param 		handle 该路音频流操作句柄
     * @param 		buffer 数据缓存
     * @param 		len	数据缓存大小
     * @param 		frames	音频数据帧大小 frames = len / (bits / 8 * channels)
     * @timestamp	timestamp 播放时间戳
     * */
	void AudioPlayStream(int handle, void *buffer, int len, int frames, unsigned long long timestamp);
	
	/* *
     * @brief 从声卡获取录音数据,然后发给carplay
     * @param 		handle 该路音频流操作句柄
     * @param 		buffer 数据缓存
     * @param 		len	数据缓存大小
     * @param 		frames	音频数据帧大小 frames = len / (bits / 8 * channels)
     * @timestamp	timestamp 播放时间戳
     * */
	void AudioRecordStream(int handle, void *buffer, int len, int frames, unsigned long long timestamp);


	void stopTimer();
	
	friend class CarplayCallbacksPriv;
protected:
	static void* CarplayStartProc(void *ctx);
	static void* CarplayStopProc(void *ctx);
	static void timeoutExpiredProc(void *ctx);
private:
	Mutex*				mLock;
	void*				mTimeout;
	bool				mStart;
	
	ICarplayCallbacks		*mCbs;
	ICarplayVideoCallbacks *mVideoCbs;
	ICarplayAudioCallbacks *mAudioCbs;
	
};

#endif//__DEMO_ECHO_SERVER_H
