#ifndef _ARK_HICAR_API_H
#define _ARK_HICAR_API_H
#ifdef __cplusplus
extern "C" {
#endif
typedef int (*ArkHicarSdkCommunicateCallBack)(int , void *, int, void *);

typedef struct _ArkHicarPrivataDataInfo{
	int type;
	int status;
	char *data;
	int dataLen;
}ArkHicarDataInfo;

//for touchscreen
#define  ARK_MAX_TOUCH_NUMS   10
typedef enum _ArkHicarTouchEventType
{
    ARK_TOUCH_DOWN,
    ARK_TOUCH_UP,
    ARK_TOUCH_MOVE
}ArkHicarTouchEventType;

typedef struct _ArkHicarTouchDataInfo{
    ArkHicarTouchEventType type;         // touch event type
    int numPointers;                     // Number of pointers of a multi-touch motion event.
    int pointerId[ARK_MAX_TOUCH_NUMS];   // The identification number of this pointer.
    long coordinateX[ARK_MAX_TOUCH_NUMS];  // X-coordinate for touch down event
    long coordinateY[ARK_MAX_TOUCH_NUMS];  // Y-coordinate for touch down event
}ArkHicarTouchDataInfo;
//end for touchscreen

typedef struct _ArkHicarMicDataInfo{
	int state;		//0:close, 1:open.
	int channels;	//1:mono, 2:stereo.		(default: hicar use mono)
	int format;		//16, 24 ...			(default: hicar use 16bit)
	int sampleRate;	//16KHz, 24KHz, 48KHz ...	(default: hicar use 16KHz)
} ArkHicarMicDataInfo;

typedef enum {
	/* audio play */
	//hicar send audio data to app,
    ARK_HICAR_SEND_REQUEST_AUDIO_FOCUS,
    ARK_HICAR_SEND_ABANDON_AUDIO_FOCUS,
	//hicar receive audio data from app.
	ARK_HICAR_RCV_AUDIO_FOCUS_CHANGE,

	/* mic record */
	//hicar send mic state to app. 
    ARK_HICAR_SEND_MIC_RECORD_STATE = 0x60,	//open/close state.
    //...
    //hicar receive mic param from app.
	ARK_HICAR_RCV_MIC_RECORD_PARAM = 0x70,	//reserved.
	//...

	/* Others type */
	//...
}ArkAudioCommunicateType;

typedef enum {
    ARK_HICAR_USB_STATE_CHANGE,
	ARK_HICAR_LINK_STATE_CHANGE,
	ARK_HICAR_CONNECTING_PHONE_TYPE,
	ARK_HICAR_HICAR_VERDION,
	ARK_HICAR_BT_PIN_CODE,
	ARK_HICAR_CAR_DATA,
    ARK_HICAR_INIT_DONE,
}ArkHicarSendDataType;

typedef enum
{
    LINK_UNSUPPORTED= 0xff,     //手机类型未匹配或手机连接中转后台点击另外一个连接模式
    LINK_CONNECTED = 1,         //暂时未用
    LINK_DISCONNECTED = 2,      //请求连接后没连接上，可能没插入手机
    LINK_STARTING = 3,          //发起连接
    LINK_SUCCESS = 4,           //连接成功
    LINK_FAIL = 5,              //连接失败
    LINK_EXITING = 6,           //退到后台
    LINK_EXITED = 7 ,           //退出carlife，完全是释放解码库，切源的时候调用
    LINK_REMOVED = 8,           //手机连接上carlife后拔出手机
    LINK_INSERTED = 9,          //手机插入包括第一次上电
    LINK_NOT_INSERTED = 10,     //手机未进行连接拔出,暂时可以没用
    LINK_NOT_INSTALL = 11,      //未安装APK
    LINK_CALL_PHONE = 12,       //carlife拨打电话，呼入电话和通话中
    LINK_CALL_PHONE_EXITED = 13,//carlife拒绝来电或者挂断电话
    LINK_MUTE =14,              //禁掉声音
    LINK_UNMUTE  = 15,          //恢复声音
    LINK_NODATA = 16,           //苹果手机锁屏或者在后针对iphone5，iphone5s,iphone6等机型
    LINK_VIDEOREADY = 17,       //苹果手机解锁并且在前台针对iphone5，iphone5s,iphone6等机型
    LINK_BT_DISCONNECT = 18,    //断掉蓝牙
    LINK_FAILED_EAP = 19,       //苹果手机eap失败,提示用户要重启手机端carlife
    LINK_FAILED_UNSTART = 20,   //手机端还未拉起完成就通信,可建议用户手机端手动启动carlife
    LINK_AUTO_BT_UNPAIRED = 21, //return the status of vehicle bluetooth with target android phone
    LINK_AUTO_BT_PAIRED = 22,
    LINK_AUTO_BT_REQUEST = 23,
    LINK_EXIT_PROCESS = 24,     //guanbi整个carlife应用
    LINK_KILL_PROCESS = 25,
    LINK_START_PROCESS = 30,     //
    LINK_SOCKET_TRUST = 26,
    LINK_OPEN_APP  = 27,
    LINK_VOLUME_START = 28,
    LINK_VOLUME_STOP = 29,

	LINK_ILLIGHT_ON = 31,//打开黑夜模式
	LINK_ILLIGHT_OFF = 32,//关闭黑夜模式

    LINK_SIRI_START = 33,  //vr铃声
    LINK_SIRI_STOP = 34,   //vr铃声
    LINK_ASSIST_START = 35, //辅助音量比如导航
    LINK_ASSIST_STOP  = 36, //辅助音量比如导航
    LINK_TEL_START = 37, //电话声音
    LINK_TEL_STOP = 38,  //电话声音
    LINK_MUSIC_START = 39, //音乐
    LINK_MUSIC_STOP = 40,   //音乐
    LINK_HEARTBEAT = 50,
    LINK_AUTHFAIL = 51,
    LINK_AUTHSUCCESS = 52,

    LINK_BT_CALL_INCOMMING = 100,
    LINK_BT_CALL_OUT = 101,     //
    LINK_BT_CALL_TERMINAL = 102,
    LINK_BT_CALL_ANSWER = 103,  //BT来电
    LINK_BT_CALL_REJECT = 104,  //BT拒绝接听
    LINK_BT_CALL_MUTE  = 105, //
    LINK_BT_CALL_UNMUTE  = 106, //
    LINK_BT_CALL_DTMF_0  = 107, //0
    LINK_BT_CALL_DTMF_1  = 108, // 1
    LINK_BT_CALL_DTMF_2  = 109, // 2
    LINK_BT_CALL_DTMF_3  = 110, //3
    LINK_BT_CALL_DTMF_4  = 111, //4
    LINK_BT_CALL_DTMF_5  = 112, //5
    LINK_BT_CALL_DTMF_6  = 113, //6
    LINK_BT_CALL_DTMF_7  = 114, //7
    LINK_BT_CALL_DTMF_8  = 115, //8
    LINK_BT_CALL_DTMF_9  = 116, //9
    LINK_BT_CALL_DTMF_STAR  = 117, //*
    LINK_BT_CALL_DTMF_SHARP  = 118, //#
    LINK_VERSION = 301,

    LINK_START_OTA_UPDATE = 302,
    LINK_STOP_OTA_UPDATE = 303,
}LinkStatus;

typedef enum _KeyStatus
{
	KEY_STATUS_IGNORE,	//按键触发(不区分up/down).
	KEY_STATUS_UP,		//按键弹起(区分up/down).
	KEY_STATUS_DOWN		//按键按下(区分up/down).
} KeyStatus;
#define KeyStatus int

typedef enum _KeyValue
{
//1.Old Type(Compatible with carlife/carplay)
	/* 虚拟按键 */
	HOME_KEY = 0x01,         //进入HiCar主界面(从原车界面进入HiCar界面，预留)
	MENU_KEY= 0x02,          //进入HiCar App菜单界面(预留)
	BACK_KEY = 0x03,         //返回HiCar上级菜单(预留)
	MIC_KEY = 0x04,          //语音唤醒按键
	/* 物理按键*/
	MEDIA_PLAY_KEY = 0x05,   //播放.
	MEDIA_PAUSE_KEY = 0x06,  //暂停.
	MEDIA_NEXT_KEY = 0x7,    //播放下一曲.
	MEDIA_PREVIOUS_KEY = 0x8,//播放上一曲.
	PICKUP_PHONE_KEY = 0x09, //接听电话.
	HANGUP_PHONE_KEY = 10,   //挂断电话.

	/* 虚拟按键拓展*/
	NAVI_KEY = 11,           //进入导航(预留)。
	CAR_HOME_KEY = 12,       //返回原车界面(HiCar切换到后台，预留)。
	//////

	/* 物理按键拓展 */
//2. New Type.(HiCar Physical key extension)
	/* 物理按键*/
	MEDIA_PLAY_PAUSE_KEY = 0xA0,   //媒体播放/暂停一体按键.
	MEDIA_STOP_KEY,			//多媒体键停止.
	MEDIA_REWIND_KEY,		//多媒体键快退.
	MEDIA_FAST_FORWARD_KEY,	//多媒体键快进.
	//方向键
	DPAD_UP_KEY,			//方向键上(可以转换为上一首MEDIA_PREVIOUS_KEY);
	DPAD_DOWN_KEY,			//方向键下(可以转换为下一首MEDIA_NEXT_KEY);
	DPAD_LEFT_KEY,			//旋钮-左旋(导航方向键:向左)
	DPAD_RIGHT_KEY,			//旋钮-右旋(导航方向键:向右)
	//摇杆.
	CARROCKER_UP_KEY,		//摇杆-上（可以翻页、切区块等-依赖应用适配）
	CARROCKER_DOWN_KEY,		//摇杆-下（可以翻页、切区块等-依赖应用适配）
	CARROCKER_LEFT_KEY,		//摇杆-左（可以翻页、切区块等-依赖应用适配）
	CARROCKER_RIGHT_KEY,	//摇杆-右（可以翻页、切区块等-依赖应用适配）
	//
	DPAD_CENTER_KEY,		//确认键(导航方向键-确认键)
	DPAD_BACK_KEY,			//返回键(预留)
	//adv
	ADV_ON_KEY,				//开始靠近发现广播.
	ADV_OFF_KEY,			//停止靠近发现广播.
}KeyValue;
#define KeyValue int



/***************************************************************
 ******
 ****** 		BT Api(Common Interface).
 ******
 ***************************************************************/
/*	Function	   : ArkHicarRcvBtStackAtCmd.
 *		app register callback to receive hicar bt data.
 *	Parameters :
 *		cmd : BT data. (AT cmd for rtl8821 module.
 *						For example: App Receive BT data from BT module is:"AT#XXxxxx\r\n",
 *						then cmd parameters format should be "XXxxxxxx".
 *						HiCar only support six AT cmd:	SRxxx,SIxxx,SVxxx,SSxxx,MXxxx,MZ12xxx.)
 *	Return :	0: success, others:  failed.
 */
int ArkHicarRcvBtStackAtCmd(char *cmd);	//hicar receive AT cmd form app

/*	Function	: ArkHicarRegisterAppRcvBtDataCallback.
 *		hicar send bt data to app.
 *	Parameters	:
 *		callback: BT data callback function
 *			callback parameters:
 *				(int type, void *data, int len, void *priv)
 *					type: LinkType(HicarWireless)
 *					data: BT data(AT cmd for rtl8821 module, HiCar send BT data format:"AT#XXxxxx\r\n")
 *					len	: data length.
 *					priv: priv parameters when register.
 *		priv : parameters.
 *	Return	:
 *			0: success, others:  failed.
 */
int ArkHicarRegisterAppRcvBtDataCallback(ArkHicarSdkCommunicateCallBack callback, void *priv);



/***************************************************************
 ******
 ****** 		Hicar send data to app api(Common Interface).
 ******
 ***************************************************************/
/*	Function	   : ArkHicarRegisterAppRcvHicarDataCallback.
 *		app register callback to receive hicar data.
 *	Parameters :
 *		callback	: app callback function
 *		priv		: app private data.
 *	Return :	0: success, others:  failed.
 */
int ArkHicarRegisterAppRcvHicarDataCallback(ArkHicarSdkCommunicateCallBack callback, void *priv);

/***************************************************************
 ******
 ******		App send data to hicar api(Common Interface).
 ******
 ***************************************************************/
 
/*	Function : ark_hicar_init.
 *		hicar initialize.
 *	Parameters:
 *		type	: 
 *			enum {
 *				HICAR_INIT_TYPE_COMMON,	//
 *				HICAR_INIT_TYPE_DONGLE,	//without display, for dongle board.
 *				HICAR_INIT_TYPE_END		//type end.
 *			};
 *	Return:	0: success, 1: hicar has already initialized, -1: failed.
 */
int ark_hicar_init(int type);

/*	Function	: ark_hicar_release,
 *		hicar release.
 *	Parameters :	none.
 *	Return :	none.
 */
void ark_hicar_release(void);

/*	Function	   : ark_hicar_link_state_change.
 *		App tell hicar sdk link status change.
 *	parameters :
 *		type		: LinkType(must be HICAR(0x09)).
 *		status	: LinkStatus.
 *	Return :	0: success, -1:  failed.
 */
int ark_hicar_link_state_change(int type, int status);

/*	Function	   : ark_hicar_touch_state_change.
 *		App tell hicar touch screen data.
 *	Parameters :
 *		data		: ArkHicarTouchDataInfo.
 *		dataLen	: sizeof(ArkHicarTouchDataInfo)
 *	Return :	0: success, -1:  failed.
 */
int ark_hicar_touch_state_change(ArkHicarTouchDataInfo *data, int dataLen);

/*	Function   : ark_hicar_get_trust_phone_list.
 *		Start or stop ble advertising.
 *	Parametrs :
 *		enable	: 0: disable, others: enable.
 *	Return :	0: success, -1:  failed.
 */
int ark_hicar_advertisement(int enable);

/*	Function:ark_hicar_send_car_data.
 *		send car data.
 *	Parameters:
 *		type		: CarDataType
 *			typedef enum {
 *  				HICAR_DATA_UNKNOWN = 0,
 *    			HICAR_DATA_VEHICLE_CONTROL   = 500,
 *    			HICAR_DATA_DAY_NIGHT         = 501,
 *    			HICAR_DATA_BRAND_ICON_DATA   = 502,
 *    			HICAR_DATA_NAV_FOCUS         = 503,
 *    			HICAR_DATA_CALL_STATE_FOCUS  = 504,
 *    			HICAR_DATA_VOICE_STATE       = 505,
 *    			HICAR_DATA_DRIVING_MODE      = 506,
 *   				HICAR_DATA_CAR_STATE         = 507,
 *				HICAR_DATA_SERVICE_CHANNEL   = 508,
 *				HICAR_DATA_KEYCODE           = 509,
 *				HICAR_DATA_SENSOR_DATA       = 510,
 *				HICAR_DATA_NET_SERVICE       = 511,
 *				HICAR_DATA_BLUETOOTH         = 512,
 *				HICAR_DATA_SCREEN_SERVICE    = 513,
 *				HICAR_DATA_AA_DATA           = 514,
 *				HICAR_DATA_AA_MEDIA_DATA     = 515,
 *				HICAR_DATA_AA_CALL_DATA      = 516,
 *				HICAR_DATA_AA_NAVIGATION_DATA = 517,
 *				HICAR_DATA_MANUAL_DISCONNECT = 518,
 *			} CarDataType;
 *		data		:
 *			see the specification.
 *		dataLen	:
 *			data length.
 *	Return :	0: success, others:  failed.
 */
int ark_hicar_send_car_data(int type, char *data, int dataLen);

/*	Function	   : ark_hicar_whell_state_change.
 *		App tell hicar sdk whell state change.
 *	Parameters :
 *		type		: LinkType: HICAR/HicarWireless..
 *		pressed	: 1:pressed(whell enter), 0:not pressed(whell left or right).
 *		step		: -1:left; 1:right.
 *	Return :	0: success, -1:  failed.
 */
int ark_hicar_whell_state_change(int type, int pressed, int step);

/*	Function	   : ark_hicar_key_value_change.
 *		App tell hicar sdk key value change.
 *	Parameters :
 *		type		: LinkType: HICAR/HicarWireless.
 *		status	: KeyStatus.
 *		key		: KeyValue.
 *	Return :	0: success, -1:  failed.
 */
int ark_hicar_key_value_change(int type, KeyStatus status, KeyValue key);


#if 0
/*	Function   :ark_hicar_start_wireness_reconnect.
 *		wireless reconnect.
 *	Parameters :
 *		remoteMac	: phone mac address which is connecd with hicar bt.
 *		realLen		: the length of remoteMac.
 *	Note :
 *		If you use ArkHicarRcvBtStackAtCmd(), there is no need to use this interface.
 */
int ark_hicar_start_wireness_reconnect(char *remoteMac, int realLen);
#endif


/*	Function   : ark_hicar_get_connect_type.
 *		get hicar connect device type.
 *	Return :
 *		success:
 *	    		HICAR_CONNECTION_UNKNOWN     = 0,
 *	    		HICAR_CONNECTION_WIFI        = 3,
 *	    		HICAR_CONNECTION_USB         = 6,
 *		failed:
 *			-1
 */
int ark_hicar_get_connect_type(void);

/*	Function	   :ark_hicar_get_trust_phone_list.
 *		App get trust device list.
 *	Parameters :
 *		list	: device list buffer = PHONE_TRUST_MAX_LIST * sizeof(TrustPhoneInfo).
 *				#define PHONE_TRUST_MAX_LIST	16
 *				#define PHONE_ID_MAX_LEN		128
 *				#define PHONE_NAME_MAX_LEN		256
 *				#define BR_MAC_MAX_LEN			20
 *				typedef struct {
 *					uint32_t phoneIdRealLen;
 *					uint8_t phoneId[PHONE_ID_MAX_LEN];
 *					uint32_t phoneNameRealLen;
 *					uint8_t phoneName[PHONE_NAME_MAX_LEN];
 *					uint32_t phoneBrMacRealLen;
 *					uint8_t phoneBrMac[BR_MAC_MAX_LEN];
 *					int64_t lastConnectTime;
 *					uint32_t connectStatus;
 *				} TrustPhoneInfo;
 *	Return :	list number.
 */
int ark_hicar_get_trust_phone_list(void *list);

/*	Function:ark_hicar_delete_trust_phone_list.
 *		delete trust device list.
 *	Parameters:
 *		phoneId	: phoneId which get in ark_hicar_get_trust_phone_list().
 *		idLen	: phoneId length.
 *	Return :	0: success, others:  failed.
 */
int ark_hicar_delete_trust_phone_list(char *phoneId, int idLen);

/*	Function:ark_hicar_disconnect_device.
 *		disconnect device(phone) which is connecting with hicar.
 *	Parameters:
 *		phoneId	: phoneId which get in ark_hicar_get_trust_phone_list().
 *		idLen	: phoneId length.
 *	Return :	0: success, others:  failed.
 */
int ark_hicar_disconnect_device(char *phoneId, int idLen);

/*	Function	: ark_hicar_sleep.
 *		hicar go to sleep mode when system swtich to other carlink type(carplay,carlife ...).
 *	Parameters 	:
 *		state	: 1:sleep mode, 0: resume form sleep mode.
 *	Return :	none.
 */
void ark_hicar_sleep(int state);

/*	Function	   : ark_hicar_cut_screen.
 *		cut screen(Save picture in pathName).
 *	Parameters :
 *		pathName : Storage path(For example: /media/udisk/file.yuv).
 *	Return :	0: success, -1: failed.
 */

int ark_hicar_cut_screen(char *pathName);

/*	Function	   : ark_hicar_app_req_usb_monitor_management.
 *		app want to manage usb hotplug, can use it.
 *		by default, hicar library will management usb hotplug.
 *		if call it, you must notify usb insert/remove state to hicar by call
 *		ark_hicar_app_notify_usb_status_change()
 *		Recommand: Do not use it,because hicar can management by itself. 
 *	Parameters :
 *		none.
 *	Return :	0: success; -1: failed.
 */
int ark_hicar_req_usb_monitor_management(void);

/*	Function	   : ark_hicar_app_notify_usb_status_change.
 *		app notify usb insert/remove state to hicar.
 *		if app call ark_hicar_app_req_usb_monitor_management to manage usb hotplug, you must
 *		call this function to notify hicar when usb status change.
 *		Recommand: DO not use it, because hicar can management by itself.
 *	Parameters :
 *		usbId : usb index(reserved.)
 *		state : 0: remove; 1: insert.
 *	Return :	0: success; -1: failed.
 */
int ark_hicar_notify_usb_status_change(int type, int state);

/*	Function	: ark_hicar_request_video_size.
 *		app request video size to hicar.
 *	Parameters	:
 *		width	: request video soruce width.
 *		height	: request video soruce height.
 *		xpos	: lcd display x start position(not effect in dongle board).
 *		ypos	: lcd display y start position(not effect in dongle board).
 *	Return :	0: success; -1: failed.
 */
int ark_hicar_request_video_size(int width, int height, int xpos, int ypos);

/*	Function	: ark_hicar_get_video_size.
 *		app get jocar video size information.
 *	Parameters	:
 *		width	: video soruce width.
 *		height	: video soruce height.
 *	Return :	0: success; -1: failed.
 */
int ark_hicar_get_video_size(int *width, int *height);

/***************************************************************
 ******
 ******		 Audio Api(Common Interface, for all board type.).
 ******
 ***************************************************************/
//app register callback to receive data from hicar
int ArkHicarRegisterAudioCallback(ArkHicarSdkCommunicateCallBack callback, void *priv);

//app unregister callback to release audio resource.
int ArkHicarUnregisterAudioCallback(void);

//hicar receive audio data from UI.
int ArkHicarRcvAudioData(int type, void *data, int arg);

#if 0
//reset record parameters.(default: 16K rate, 1 channel, 16 bits.)
int ArkHicarAudioResetRecordParameters(int rate, int channels, int bits);

//audio record use software denosize
int ArkHicarAudioRecordSetSoftwareDenosize(void);	//default: not used.
//audio record use software echo calcelation.
int ArkHicarAudioRecordSetSoftwareEchoCancel(void)	//default: not used.
#endif



//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//========================== A Beautiful Dividing Line ==============================
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||



/***************************************************************
 ******
 ******		Dongle Board Api(App manage the audio or video stream).
 ******
 ***************************************************************/

typedef enum {
	ARK_HICAR_VIDEO_STOP = 0,
	ARK_HICAR_VIDEO_START,
	ARK_HICAR_VIDEO_STREAM,
}ArkHicarDongleVideoOpreationType;

typedef enum {
	ARK_HICAR_AUDIO_CLOSE = 0,
	ARK_HICAR_AUDIO_OPEN,
	ARK_HICAR_AUDIO_READ,
	ARK_HICAR_AUDIO_FLAGS = 0xFFEE,	//open/close flags
}ArkHicarDongleAudioOpreationType;

typedef struct _ArkHicarDongleAudioDataParam {
	int flags;		//ArkHicarDongleAudioOpreationType.
	int streamType;	//hicar audio stream type
	int rates;		//sample rates
	int channels;	//sample channels
	int bits;		//sample bits
} ArkHicarDongleAudioDataParam;

typedef struct _ArkHicarDongleVideoDataParam {
	int width;		//hicar video source width.
	int height;		//hicar video source height.
	int xpos;		//display x position.
	int ypos;		//display y position.
} ArkHicarDongleVideoDataParam;



//app register audio callback to get hicar audio play stream
void HicarDongleRegisterGetAudioDataCallBack(ArkHicarSdkCommunicateCallBack callback, void *parameters);
//app register audio callback to get hicar audio record stream
void HicarDongleRegisterGetAudioMicDataCallBack(ArkHicarSdkCommunicateCallBack callback, void *parameters);
//app register video callback to get hicar video stream
void HicarDongleRegisterGetVideoDataCallBack(ArkHicarSdkCommunicateCallBack callback, void *parameters);



#if 1	//Abandon the following old interface.
/***************************************************************
 ******
 ******		Dongle Board Api(Only for Dongle Board(Do not display on board)).
 ******
 ***************************************************************/

typedef enum _ArkHiCarDonglePlayerCmdType{
	AHDP_START,
	AHDP_STOP,
	AHDP_SET_VIDEO_INFO,
	AHDP_TOUCH,
	AHDP_KEY,
	AHDP_KNOB,
	AHDP_FOREGROUND,
	AHDP_BACKGROUND,
	AHDP_START_ADV,
	AHDP_GET_CONNECT_TYPE,
}ArkHiCarDonglePlayerCmdType;

typedef enum _ArkHiCarDonglePlayerVideoType{
	AHDP_VIDEO_STOP,
	AHDP_VIDEO_START,
	AHDP_VIDEO_PALY,
}ArkHiCarDonglePlayerVideoType;

typedef enum _ArkHiCarDonglePlayerNotifiyType{
	//notify app status event
    AHDP_NOTIFY_INSERT, 		//device insert(reserve)
    AHDP_NOTIFY_HOME,   		//back to car when click home button(reserve)
    AHDP_NOTIFY_FOREGROUND,		//switch to foreground.
    AHDP_NOTIFY_BACKGROUND,		//switch to background.
    AHDP_NOTIFY_REMOVE,			//device remove or link  disconnect.
    AHDP_NOTIFY_LINK_SUCCESS,	//link success.

	//notify self-process event
	AHDP_NOTIFY_PIN_CODE = 0xA0,   //hicar pinCode
    AHDP_NOTIFY_HEARTBEAT_START,
    AHDP_NOTIFY_HEARTBEAT_STOP,
}ArkHiCarDonglePlayerNotifiyType;

void ark_hicar_dongle_register_video_data_callback(ArkHicarSdkCommunicateCallBack callback, void *parameters);
void ark_hicar_dongle_register_notify_event_callback(ArkHicarSdkCommunicateCallBack callback, void *parameters);
int ark_hicar_dongle_send_cmd(int cmd, char *data, int dataLen);
int ark_hicar_dongle_init(void);
int ark_hicar_dongle_release(void);
#endif
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
