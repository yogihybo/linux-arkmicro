/*****************************************************************
* Project ECSDK.
* (c) copyright 2017-2020.
* Company Carbit.
* All rights reserved. Copy without permission
****************************************************************/
///\file ECTypes.h
#ifndef CARBIT_EC_SDK_TYPES_H
#define CARBIT_EC_SDK_TYPES_H

#include <stdint.h>

/**
* @namespace CarbitECSDK
*
* @brief namespace includes Data Type definition and API in SDK
*/
namespace  CarbitECSDK
{

/**
* @brief return code of ECSDK APIs.
*/
#define EC_OK                     0              ///< success
#define EC_ERR_INVAL_OP          -1              ///< invalid operation
#define EC_ERR_INVAL_PARAM       -2              ///< invalid parameter(s)
#define EC_ERR_OP_FAIL           -3              ///< operation failed
#define EC_ERR_LICENSE_AUTH_FAIL -4              ///< license authorize failed
#define EC_ERR_APP_NOT_STARTED   -5              ///< phone app not started
#define EC_ERR_APP_AUTH_FAIL     -6              ///< phone app authorization failed.
#define EC_ERR_APP_AUTH_PENDING  -7              ///< phone app authorization pending.
#define EC_ERR_PERMISSION_DENIED -8              ///< without permission.

/**
* @enum ECMobileType
*
* @brief phone type
*/
enum ECMobileType
{
    EC_MOBILE_TYPE_ANDROID = 0,                  ///< android
    EC_MOBILE_TYPE_IPHONE                        ///< iphone
};

/**
* @enum ECStatusMessage
*
* @brief status message code
*
* @see IECCallback::onECStatusMessage
*/
enum ECStatusMessage
{
    EC_STATUS_MESSAGE_USER_NOT_AUTH_DEBUG,       ///< user did not authorize car to use phone via usb debug mode.
    EC_STATUS_MESSAGE_APP_NOT_RUNNING,           ///< phone app is not running
    EC_STATUS_MESSAGE_APP_FOREGROUND,            ///< phone app runs in foreground,deprecated ,instead of EC_STATUS_MESSAGE_APP_FOREGROUND_SAME or EC_STATUS_MESSAGE_APP_FOREGROUND_SPLIT
	EC_STATUS_MESSAGE_APP_FOREGROUND_SAME,       ///< phone app runs in foreground with same screen mode
	EC_STATUS_MESSAGE_APP_FOREGROUND_SPLIT,      ///< phone app runs in foreground with split screen mode
    EC_STATUS_MESSAGE_APP_BACKGROUND_SAME,       ///< phone app runs in background with same screen mode
	EC_STATUS_MESSAGE_APP_BACKGROUND_SPLIT,      ///< phone app runs in background with split screen mode
    EC_STATUS_MESSAGE_APP_SCREENLOCKED,          ///< screen locked
    EC_STATUS_MESSAGE_APP_UNSCREENLOCKED,        ///< screen unlocked
    EC_STATUS_MESSAGE_APP_NAVI_STARTED,          ///< phone app started to navigate
    EC_STATUS_MESSAGE_APP_NAVI_STOPPED,           ///< phone app stopped to navigate  
    EC_STATUS_MESSAGE_SWITCH_AOA_FAIL,           ///< failed to switch AOA mode for current android phone
    EC_STATUS_MESSAGE_3RD_APP_HORIZONTAL_NOT_SUPPORT,    ///< 3rd app does not support horizontal screen
    EC_STATUS_MESSAGE_3RD_APP_HORIZONTAL_SUPPORT,        ///< 3rd app supports horizontal screen  
    EC_STATUS_MESSAGE_APP_VR_STARTED,            ///< vr of phone app started.
    EC_STATUS_MESSAGE_APP_VR_STOPPED,            ///< vr of phone app stopped.
	EC_STATUS_MESSAGE_ACQUIRE_BLUETOOTH_A2DP,    ///< HU needed to acquire bluetooth for music play and send play event to phone via bluetooth.
	EC_STATUS_MESSAGE_ACQUIRE_BLUETOOTH_A2DP_WITHOUT_SEND_PLAY,    ///< HU needed to acquire bluetooth for music play but not send play event to phone.
	EC_STATUS_MESSAGE_SWITCH_TO_SYSTEM_MAIN_PAGE,///< HU switch to system main page. 
	EC_STATUS_MESSAGE_LAUNCH_PHONE_APP,          ///< phone request HU to launch app. 
	EC_STATUS_MESSAGE_APP_TALKIE_STARTED,        ///< talkie of phone app started.
	EC_STATUS_MESSAGE_APP_TALKIE_STOPPED,        ///< talkie of phone app stopped.
	EC_STATUS_MESSAGE_APP_MUSIC_PENDING,         ///< music of phone app is pending, it will be deprecated.
	EC_STATUS_MESSAGE_APP_MUSIC_PLAYING,         ///< music of phone app is playing, it will be deprecated.
	EC_STATUS_MESSAGE_APP_MUSIC_PAUSED,          ///< music of phone app paused, it will be deprecated. 
	EC_STATUS_MESSAGE_APP_MUSIC_STOPPED,	     ///< music of phone app stopped, it will be deprecated. 
	EC_STATUS_MESSAGE_SWITCH_TO_FRONT,           ///< make easyconn switch to front.
	EC_STATUS_MESSAGE_OPEN_BLUETOOTH_PHONE,      ///< make HU open bluetooth phone.
	EC_STATUS_MESSAGE_OPEN_BLUETOOTH_SETTING,    ///< make HU open bluetooth setting.
	EC_STATUS_MESSAGE_MAX                        ///< reserve
};

/**
* @enum ECCallType
*
* @brief call type
*
* @see ECSDK::onCallAction
*/
enum ECCallType
{
    EC_CALL_TYPE_DAIL = 0,                       ///< ring up
    EC_CALL_TYPE_HANG_UP,                        ///< ring off
    EC_CALL_TYPE_MAX,                            ///< reserve
};

/**
* @enum ECMicType
*
* @brief microphone type
*/
enum ECMicType
{
    EC_MIC_TYPE_NATIVE = 0,                      ///< microphone of car
    EC_MIC_TYPE_PHONE,                           ///< microphone of phone
    EC_MIC_TYPE_UNSUPPORT,                       ///< no microphone
};

/**
* @enum ECMediaLevel
*
* @brief car multimedia level
*/
enum ECMediaLevel
{
    EC_DVD_LEVEL_LOW = 0,                        ///< low-end
    EC_DVD_LEVEL_MIDDLE,                         ///< middle-end
    EC_DVD_LEVEL_HIGH,                           ///< high-end
};

/**
*
* @enum ECScreenType
*
* @brief HU screen type
*/
enum ECScreenType
{	
	EC_CAR_SCREEN_HORIZONTAL,                    ///< horizontal screen type
	EC_CAR_SCREEN_VERTICAL,                      ///< vertical screen type   
	EC_CAR_SCREEN_UNKNOWN						 ///< unknown screen type
};

/**
*
* @enum ECProjectFlavor
*
* @brief HU Project market for sale,This field will affect the function of SDK,
 * SDK will carry a flavor by default,EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_CN or
 * EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_OVERSEA.
*/
enum ECProjectFlavor
{
    EC_PROJECT_FLAVOR_DEFAULT = 0,                                   ///< default value
	EC_PROJECT_FLAVOR_AFTER_MARKET_INSTALLED_PRODUCTS_CN,            ///< aftermarket installed products in China
	EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_CN,                 ///< factory-installed products in China
	EC_PROJECT_FLAVOR_FACTORY_INSTALLED_PRODUCTS_OVERSEA,            ///< factory-installed products oversea
	EC_PROJECT_FLAVOR_AFTER_MARKET_INSTALLED_PRODUCTS_OVERSEA        ///< aftermarket installed products oversea
};

/**
* @struct ECAuthentication
*
* @brief car authorization info
*/
struct ECAuthentication
{
    char         uuid[1024];                     ///< the universally unique identifier of the car
    char         pwd[1024];                      ///< the specific password for authentication powered by Carbit.
    char         versionName[1024];              ///< the version name of EasyConn.
    uint32_t     versionCode;                    ///< the version code of EasyConn.
	bool         autoAuthViaCar;                 ///< specify whether make automatic authentication via car's network.
    ECProjectFlavor     flavor;                  ///< specify the HU Project market for sale,SDK will carry a flavor by default.see enum ECProjectFlavor in ECTypes.h
    char         reserve[256];                   ///< reserve
};

enum ECSupportConnect
{
	EC_SUPPORT_CONNECT_ADB = 0x01,
	EC_SUPPORT_CONNECT_AOA = 0x02,
	EC_SUPPORT_CONNECT_WIFIDIRECT = 0x08,
	EC_SUPPORT_CONNECT_EAP = 0x20,
	EC_SUPPORT_CONNECT_MUX = 0x40,
	EC_SUPPORT_CONNECT_LIGHTNING = 0x80,
    EC_SUPPORT_CONNECT_IPHONE_WIFI = 0x100,
    EC_SUPPORT_CONNECT_ANDROID_WIFI = 0x200,
    EC_SUPPORT_CONNECT_USB_AIRPLAY = 0x400,
    EC_SUPPORT_CONNECT_WIFI_AIRPLAY = 0x800
};

enum ECSupportFunction
{
    EC_SUPPORT_FUNCTION_INPUT = 0x01,
    EC_SUPPORT_FUNCTION_SPEECH_WAKE = 0x02
};

enum ECMicFeature
{
	EC_MIC_SUPPORT_ECHO_CANCELLATION = 0x0001,
	EC_MIC_SUPPORT_ECHO_CANCELLATION_VIA_PHONE = 0x0002,
	EC_MIC_SUPPORT_LEFT_CHANNEL_RECORD = 0x0004,
};

/**
* @struct ECCarDescription
*
* @brief  car description info
*/
struct  ECCarDescription
{
    ECMediaLevel mediaLevel;                     ///< level of multimedia，reserved field
    char         mediaOS[64];                    ///< os of multimedia，reserved field
    char         mediaModel[64];                 ///< multimedia system model.
	ECScreenType screenType;					 ///< HU screen type
    char         carBrand[64];                   ///< car brand
    char         carModel[64];                   ///< car model
    char         carConfig[64];                  ///< car config information
    ECMicType    micType;                        ///< microphone type
	bool		 supportBTCall;					 ///< whether HU support bluetooth call.
	bool         supportBTSetting;               ///< whether HU support bluetooth setting.
    char         btName[64];                     ///< bluetooth name
    char         btAddress[64];                  ///< bluetooth mac addrss
    char         btPin[32];                      ///< bluetooth pin code
	uint32_t     dpi;                            ///< dpi of screen of HU.
	uint32_t     supportConnect;                 ///< the supported connects of HU.
												 ///< the value of supportConnect can be the combination of ECSupportConnect
												 ///< such as (EC_SUPPORT_CONNECT_ADB | EC_SUPPORT_CONNECT_WIFIDIRECT).
    uint32_t     supportFunction;                ///< such as (EC_SUPPORT_FUNCTION_INPUT)
	uint32_t     micSupportFeature;              ///< the feature of car's microphone.
	                                             ///< the value of micSupportFeature can be the combination of ECMicFeature
	                                             ///< such as (EC_MIC_SUPPORT_ECHO_CANCELLATION | EC_MIC_SUPPORT_LEFT_CHANNEL_RECORD)

    char         reserve[256];                   ///< reserve
};

enum ECMirrorMode
{
	EC_MIRROR_MODE_DEFAULT = 0,                  ///< default mirror mode.
	EC_MIRROR_MODE_FIXED_NAVIGATION              ///< fixed navigation mirror mode.
};

/**
* @brief log output destination
*
* @see ECOptions::logOutputType
*/
enum ECLogOutputType
{
    EC_LOG_OUT_STD = 0,                              ///< log to std out
    EC_LOG_OUT_FILE,                                 ///< log to file
    EC_LOG_OUT_LOGCAT,                               ///< log to file only for android
    EC_LOG_OUT_SLOGINFO,                             ///< loginfo  only for qnx
};

/**
* @brief log module
*
* @see ECSDK::setLogLevel
*/
enum ECLogModule
{
    EC_LOG_MODULE_SDK = 0x01,                       ///< output sdk module log
    EC_LOG_MODULE_ADB = 0x02,                       ///< output adb module log
    EC_LOG_MODULE_MUX = 0x04,                       ///< output mux module log
    EC_LOG_MODULE_USB = 0x08,                       ///< output usb module log
    EC_LOG_MODULE_APP = 0x10                        ///< output ec app module log
};

/**
* @brief log level
*
* @see ECSDK::setLogLevel
*/
enum ECLogLevel
{
    EC_LOG_LEVEL_ALL = 0,                              ///< all log
    EC_LOG_LEVEL_DEBUG,                                ///< debug log
    EC_LOG_LEVEL_INFO,                                 ///< info log
    EC_LOG_LEVEL_WARN,                                 ///< warn log
    EC_LOG_LEVEL_ERROR,                                ///< error log
    EC_LOG_LEVEL_FATAL,                                ///< fatal log
    EC_LOG_LEVEL_OFF                                   ///< no log
};

/**
* @struct ECOptions
*
* @brief  options info
*/
struct ECOptions 
{           
    char         workspace[4096];                ///< absolute path of readable and writable directory 
    bool         scanADB;                        ///< whether ECSDK scan adb devices.  
    bool         scanAOA;                        ///< whether ECSDK scan aoa devices.  
    bool         scanIOSUsb;                     ///< whether ECSDK scan ios devices, RESERVED.
	bool         supportRVForAdb;                ///< whether ECSDK support rv for adb.
	bool         isAppMirrorForAdb;              ///< whether ECSDK use app mirror for adb, 
	                                             ///< if supportRVForAdb is false, isAppMirrorForAdb would be used as false.  	
	bool         supportScreenMirroring;         ///< whether the app of connected phone support screen mirroring.
	bool         supportThirdPartyApp;           ///< whether the app of connected phone support the third-party app.
	bool         supportLandscapeAdaptive;       ///< whether the app of connected phone support landscape adaptive.
	bool         supportScreenTouch;             ///< whether the HU support screen touch, it determines whether the app of the phone displays a mask.          
	bool         supportOTAUpdate;               ///< make the app of connected phone decide whether support OTA Update.
	bool         supportBackDesktop;             ///< whether the app of connected phone support back desktop of HU.
	ECMirrorMode mirrorMode;                     ///< tell the app of connected phone which mirror mode would be used.
    uint32_t     bluetoothPolicy;                ///< the policy of A2DP message phone sent to the car.
	char         reserve[256];                   ///< reserve
};

/**
* @enum ECTransportType
*
* @brief transport type
*
* @see ECSDK::onPhoneConnected,  openTransport
*/
enum ECTransportType
{
    EC_TRANSPORT_ANDROID_USB_ADB = 0,            ///< android usb adb, system screen
    EC_TRANSPORT_ANDORID_USB_AOA,                ///< android usb aoa, app screen
    EC_TRANSPORT_ANDROID_WIFI,                   ///< android wifi, app screen
    EC_TRANSPORT_IOS_USB_EAP,                    ///< iphone usb eap, app screen
    EC_TRANSPORT_IOS_USB_MUX,                    ///< iphone usb mux, app screen
    EC_TRANSPORT_IOS_USB_AIRPLAY,                ///< iphone usb airplay, system screen   
    EC_TRANSPORT_IOS_WIFI_APP,                   ///< iphone wifi app, app screen
    EC_TRANSPORT_IOS_WIFI_AIRPLAY,               ///< iphone wifi airplay, system screen
	EC_TRANSPORT_IOS_USB_LIGHTNING,              ///< iphone usb lightning connect(闪连)
    EC_TRANSPORT_MAX,                            ///< reserve
};

/**
* @enum ECVideoType
*
* @brief video type
*
* @see ECSDK::ECMirrorConfig
*/
enum ECVideoType
{
    EC_VIDEO_TYPE_H264 = 0,                      ///< H264
    EC_VIDEO_TYPE_MPEG4,                         ///< MPEG4
    EC_VIDEO_TYPE_JPEG,                          ///< JPEG
    EC_VIDEO_TYPE_MAX,                           ///< reserve
};

/**
* @struct ECMirrorConfig
*
* @brief mirror config type
*
* @see ECSDK::openMirrorConnection
*/
struct ECMirrorConfig
{
    ECVideoType type;                            ///< video type
    
    int         width;                           ///< video width in pixels
    
    int         height;                          ///< video height in pixels
    
    int         quality;                         ///< video quality, 
                                                 ///< bitrate for H264 or MPEG4(typically, 1024*1024*4)
                                                 ///< picture quality factor[0~100] for JPEG, 0 is worst, 100 is best. Typically, 50.
    int         touchMode;                       ///< touch mode : 0x0 Single-Touch ;0x01 Multi-touch;0x02 not support touch; Single-Touch is default mode.

    char        reserve[252];                    ///< reserve(must be zero clearing)
};

/**
* @enum ECSystemKeyCode
*
* @brief phone key event code
*
* @see ECSDK::sendSystemKeyEvent
*/
enum ECSystemKeyCode
{
    //The Android phone's HOME/MENU/BACK key.
    EC_SYSTEM_KEYCODE_HOME = 0,                         ///< home
    EC_SYSTEM_KEYCODE_MENU,                             ///< menu
    EC_SYSTEM_KEYCODE_BACK,                             ///< back

    //Media
    EC_SYSTEM_KEYCODE_VOLUME_UP,                        ///< volume up
    EC_SYSTEM_KEYCODE_VOLUME_DOWN,                      ///< volume down
    EC_SYSTEM_KEYCODE_MEDIA_PALY,                       ///< media play
    EC_SYSTEM_KEYCODE_MEDIA_PAUSE,                      ///< media pause
    EC_SYSTEM_KEYCODE_MEDIA_STOP,                       ///< media stop
    EC_SYSTEM_KEYCODE_MEDIA_NEXT,                       ///< media next
    EC_SYSTEM_KEYCODE_MEDIA_PREVIOUS,                   ///< media previous
    EC_SYSTEM_KEYCODE_MEDIA_REWIND,                     ///< media backward
    EC_SYSTEM_KEYCODE_MEDIA_FAST_FORWARD,               ///< media forward
    EC_SYSTEM_KEYCODE_MUTE,                             ///< media mute
    EC_SYSTEM_KEYCODE_POWER,                            ///< power

    EC_SYSTEM_KEYCODE_MAX,                              ///< reserve
};

/**
* @enum ECBtnCode
*
* @brief phone app button code
*
* @see ECSDK::sendBtnEvent
*/
enum  ECBtnCode
{
    // for driving mode
    EC_BTN_TALKIE = 0x1010,                         ///< start talkie
    EC_BTN_TALKIE_GROUP_MUTE = 0x1011,              ///< talkie group mute
    EC_BTN_TALKIE_GROUP_CANCEL_MUTE = 0x1012,       ///< talkie group cancel mute
    EC_BTN_TALKIE_GROUP_SWITCH_MUTE = 0x1013,       ///< talkie group switch mute
    EC_BTN_NAVIGATION = 0x1020,                     ///< start navigation
    EC_BTN_VOICE_ASSISTANT = 0x1030,                ///< start voice assistant
    EC_BTN_MUSIC_PLAY = 0x1040,                     ///< play music
    EC_BTN_MUSIC_NEXT = 0x1041,                     ///< play next music
    EC_BTN_MUSIC_PREVIOUS = 0x1042,                 ///< play previous music
    EC_BTN_MUSIC_PAUSE = 0x1043,                    ///< music pause
    EC_BTN_MUSIC_STOP = 0x1044,			            ///< music stop        
    EC_BTN_MUSIC_PLAY_PAUSE = 0x1047,               ///< music switch between pause and stop

    EC_BTN_VOLUME_UP = 0x1050,                      ///< increase the volume of phone
    EC_BTN_VOLUME_DOWN = 0x1051,                    ///< decrease the volume of phone

    EC_BTN_TOPLEFT = 0x1060,                        ///< click top left button
    EC_BTN_TOPRIGHT = 0x1061,                       ///< click top right button
    EC_BTN_BOTTOMLEFT = 0x1062,                     ///< click bottom left button
    EC_BTN_BOTTOMRIGHT = 0x1063,                    ///< click bottom right button   

    EC_BTN_MODE = 0x1070,                           ///< click mode button    

	EC_BTN_APP_FRONT = 0x1080,						///< make the app of android phone switch to the foreground

    EC_BTN_APP_BACK = 0x1090,                       ///< it works like the function of the back button to the app of the connected phone.

    EC_BTN_ENFORCE_LANDSCAPE = 0x10A0,              ///< make the android phone enforce landscape.
	EC_BTN_CANCEL_LANDSCAPE = 0x10A1,               ///< make the android phone cancel landscape.
	EC_BTN_ENFORCE_OR_CANCEL_LANDSCAPE = 0x10A2,    ///< make the android phone enforce or cancel landscape.

    EC_BTN_MAX,                                     ///< reserve
};

/**
* @enum ECBtnEventType
*
* @brief physical button action code
*
* @see ECSDK::sendBtnEvent
*/
enum  ECBtnEventType
{
	EC_BTN_TYPE_UP = 0,                          ///< key up
    EC_BTN_TYPE_DOWN,                            ///< key down
    EC_BTN_TYPE_CLICK,                           ///< click
    EC_BTN_TYPE_DOUBLE_CLICK,                    ///< double click
    EC_BTN_TYPE_LONG_PRESS,                      ///< long press

    EC_BTN_TYPE_MAX,                             ///< reserve
};

/**
* @enum ECTouchEventType
*
* @brief touch event type
*
* @see ECSDK::sendTouchEvent
*/
enum ECTouchEventType
{
    EC_TOUCH_UP = 0,                             ///< touch up
    EC_TOUCH_DOWN,                               ///< touch down
    EC_TOUCH_MOVE,                               ///< touch move
};

/**
* @struct ECTouchEventData
*
* @brief touch data struct
*
* @see ECSDK::sendTouchEvent
*/
struct ECTouchEventData
{
    unsigned short    pointX;                    ///< touch point x
    unsigned short    pointY;                    ///< touch point y
    unsigned short    slot;                      ///< multi touch slot(default is 0)
    char              reserve[32];               ///< reserve
};

/**
* @enum ECGearType
*
* @brief car gear type
*
* @see ECSDK::uploadGearStatus
*/
enum ECGearType
{
    EC_GEAR_NEUTRUAL = 0,                        ///< neutral
    EC_GEAR_MANUAL_1st,                          ///< manual first
    EC_GEAR_MANUAL_2nd,                          ///< manual second
    EC_GEAR_MANUAL_3rd,                          ///< manual third
    EC_GEAR_MANUAL_4th,                          ///< manual forth
    EC_GEAR_MANUAL_5th,                          ///< manual fifth
    EC_GEAR_MANUAL_6th,                          ///< manual sixth
    EC_GEAR_MANUAL_7th,                          ///< manual seventh
    EC_GEAR_MANUAL_8th,                          ///< manual eighth
    EC_GEAR_MANUAL_9th,                          ///< manual ninth
    EC_GEAR_MANUAL_10th,                         ///< manual tenth

    EC_GEAR_AUTO_DRIVE = 100,                    ///< automatic drive
    EC_GEAR_AUTO_PARK,                           ///< automatic park
    EC_GEAR_AUTO_REVERSE,                        ///< automatic reverse
};

/**
* @enum ECDrivingStatus
*
* @brief driving status
*
* @see ECSDK::uploadDrivingStatus
*/
enum ECDrivingStatus
{
    EC_DRIVING_FREE = 0x0,                       ///< no limited
    EC_DRIVING_NO_VIDEO = 0x01,                  ///< no video
    EC_DRIVING_NO_KEYBOARD_INPUT = 0x02,         ///< no keyboard input
    EC_DRIVING_NO_VOICE_INPUT = 0x04,            ///< no voice
    EC_DRIVING_NO_CONFIG = 0x08,                 ///< no config
};

/**
* @enum ECHeadLightStatus
*
* @brief car headlight status
*
* @see ECSDK::uploadLightStatus
*/
enum ECHeadLightStatus
{
    EC_HEADLIGHT_OFF = 0,                        ///< headlight off
    EC_HEADLIGHT_ON,                             ///< headlight on
    EC_HEADLIGHT_HIGH,                           ///< high-beam on
};

/**
* @enum ECTurnIndicatorStatus
*
* @brief car's turning indicator status
*
* @see ECSDK::uploadLightStatus
*/
enum ECTurnIndicatorStatus
{
    EC_TURNINDICATOR_NONE = 0,                   ///< indicator off
    EC_TURNINDICATOR_LEFT,                       ///< left-hand indicator on
    EC_TURNINDICATOR_RIGHT,                      ///< right-hand indicator on
};

/**
* @enum ECSensorType
*
* @brief car sensor type
*
* @see ECSDK::uploadSensorError
*/
enum ECSensorType
{
    EC_SENSOR_LOCATION = 1,                      ///< sensor location
    EC_SENSOR_COMPASS,                           ///< sensor compass
    EC_SENSOR_SPEED,                             ///< sensor speed
    EC_SENSOR_RPM,                               ///< sensor rpm
    EC_SENSOR_ODOMETER,                          ///< sensor odometer
    EC_SENSOR_FUEL,                              ///< sensor fuel
    EC_SENSOR_PARKING_BRAKE,                     ///< sensor handbrake
    EC_SENSOR_GEAR,                              ///< sensor gear
    EC_SENSOR_NIGHT_MODE,                        ///< sensor night-mode
    EC_SENSOR_ENV_STATUS,                        ///< sensor car environment
    EC_SENSOR_DRIVING_STATUS,                    ///< sensor driving status
    EC_SENSOR_PASSENGER_STATUS,                  ///< sensor passenger status
    EC_SENSOR_DOOR_STATUS,                       ///< sensor door status
    EC_SENSOR_LIGHT_STATUS,                      ///< sensor light status
    EC_SENSOR_TIRE_PRESSURE_STATUS,              ///< sensor tire pressure status
    EC_SENSOR_ACCLEROMETER_STATUS,               ///< sensor accelerated status
    EC_SENSOR_GYROSCOPE_STATUS,                  ///< sensor gyroscopes status
    EC_SENSOR_GPS_SATELLITE_STATUS,              ///< sensor GPS
    EC_SENSOR_MAX                                ///< reserve
};

/**
* @enum ECSensorErrorType
*
* @brief cat sensor error type
*
* @see ECSDK::uploadSensorError
*/
enum ECSensorErrorType
{
    EC_SENSORERROR_OK = 0,                       ///< sensor ok
    EC_SENSORERROR_TRANSIENT,                    ///< sensor transient error
    EC_SENSORERROR_PERMANENT,                    ///< sensor permanent error
};

enum ECNaviStatus
{
	EC_NAVI_STATUS_ACTIVE = 0,
	EC_NAVI_STATUS_INACTIVE,
	EC_NAVI_STATUS_MAX
};

enum ECNaviCameraType
{
	EC_NAVI_CAMERA_SPEED = 0,
	EC_NAVI_CAMERA_SPY,
	EC_NAVI_CAMERA_REDLIGHT,
	EC_NAVI_CAMERA_VIOLATION,
	EC_NAVI_CAMERA_BUS,
	EC_NAVI_CAMERA_EMERGENCY,
	EC_NAVI_CAMERA_MAX
};

enum ECNaviIcon
{
	EC_NAVI_ICON_UNDEFINED = 0,
	EC_NAVI_ICON_VEHICLE,
	EC_NAVI_ICON_LEFT,
	EC_NAVI_ICON_RIGHT,
	EC_NAVI_ICON_LEFT_FRONT,
	EC_NAVI_ICON_RIGHT_FRONT,
	EC_NAVI_ICON_LEFT_REAR,
	EC_NAVI_ICON_RIGHT_REAR,
	EC_NAVI_ICON_TURN_LEFT,
	EC_NAVI_ICON_GO_STRAIGHT,
	EC_NAVI_ICON_ARRIVE_VIA_POINT,
	EC_NAVI_ICON_ENTER_ROUNDABOUT,
	EC_NAVI_ICON_EXIT_ROUNDABOUT,
	EC_NAVI_ICON_ARRIVE_SERVICE_AREA,
	EC_NAVI_ICON_ARRIVE_TOLLGATE,
	EC_NAVI_ICON_ARRIVE_DESTINATION,
	EC_NAVI_ICON_ENTER_TUNNEL,
	EC_NAVI_ICON_PASS_CROSSWALK,
	EC_NAVI_ICON_PASS_OVERPASS,
	EC_NAVI_ICON_PASS_UNDERGROUND,
	EC_NAVI_ICON_MAX
};

/*
* @struct ECNavigationHudInfo
*
* @brief navigation HUD info
*
* @see   IECCallback::onPhoneAppHUD
*/
struct ECNavigationHudInfo
{
	ECNaviStatus status;
	char* currentRoad;
	int32_t	carDirection;
	ECNaviCameraType cameraType;
	int32_t	cameraSpeed;
	int32_t	cameraDistance;
	ECNaviIcon naviIcon;
	char* nextRoad;
	int32_t roadRemainingDistance;
	int32_t roadRemainingTime;
	int32_t destinationRemainingDistance;
	int32_t destinationRemainingTime;
	uint64_t arriveTime;
	char*   arriveTimeZone;
};

/*
* @enum ECAudioChannelType
*
* @brief audio channel type
*
*/
enum ECAudioChannelType
{
    EC_AUDIO_CHANNEL_MONO = 1,                   ///< mono channel
    EC_ADUIO_CHANNEL_STEREO                      ///< stereo channel
};


/*
* @enum ECAudioFormatType
*
* @brief audio format type
*
*/
enum ECAudioFormatType
{
    EC_AUDIO_FORMAT_U8 = 0,                      ///< PCM unsigned 8 bits
    EC_AUDIO_FORMAT_S8,                          ///< PCM signed 8 bits
    EC_AUDIO_FORMAT_U16_LE,                      ///< PCM unsigned little endian 16 bits
    EC_AUDIO_FORMAT_S16_LE,                      ///< PCM signed little endian 16 bits
    EC_AUDIO_FORMAT_U16_BE,                      ///< PCM unsigned big endian 16 bits
    EC_AUDIO_FORMAT_S16_BE,                      ///< PCM signed big endian 16 bits
    EC_AUDIO_FORMAT_U24_LE,                      ///< PCM unsigned little endian 24 bits
    EC_AUDIO_FORMAT_S24_LE,                      ///< PCM signed little endian 24 bits
    EC_AUDIO_FORMAT_U24_BE,                      ///< PCM unsigned big endian 24 bits
    EC_AUDIO_FORMAT_S24_BE,                      ///< PCM signed big endian 24 bits
    EC_AUDIO_FORMAT_U32_LE,                      ///< PCM unsigned little endian 32 bits
    EC_AUDIO_FORMAT_S32_LE,                      ///< PCM signed little endian 32 bits
    EC_AUDIO_FORMAT_U32_BE,                      ///< PCM unsigned big endian 32 bits
    EC_AUDIO_FORMAT_S32_BE,                      ///< PCM signed big endian 32 bits
    EC_AUDIO_FROMAT_F32                          ///< PCM float 32 bits
};

enum ECAudioType
{
    EC_AUDIO_TYPE_TTS     = 1,                   ///< TTS audio
    EC_AUDIO_TYPE_VR      = 2,                   ///< VR audio
    EC_AUDIO_TYPE_TALKIE  = 3,                   ///< IM audio
    EC_AUDIO_TYPE_MUSIC   = 4                    ///< Music audio
};

/*
* @struct ECAudioInfo
*
* @brief audio data information
*
* @see   IECCallback::onPhoneAppTTSStart, IECCallback::onPhoneAppVRStart
*/
struct ECAudioInfo
{
    uint32_t               sampleRate;            ///< sample rate
    ECAudioChannelType     channel;               ///< channel type
    ECAudioFormatType      format;                ///< audio format type
};


/*
* @enum ECAppPage
*
* @brief app page
*
*/
enum ECAppPage
{
    EC_APP_PAGE_NAVIGATION = 1,                   ///< navigation page
    EC_APP_PAGE_MUSIC = 2,                        ///< music page
    EC_APP_PAGE_VR = 3,                           ///< voice assistance page
    EC_APP_PAGE_TALKIE = 4,                       ///< talkie page
    EP_APP_PAGE_NAVI_HOME = 5,                    ///< navigation to go home
    EC_APP_PAGE_NAVI_WORK = 6,                    ///< navigation to go to work
    EC_APP_PAGE_MAIN = 7,                         ///< main page
    EC_APP_PAGE_NAVI_GAS_STATION = 8,             ///< navigation to go to gas station
    EC_APP_PAGE_CAR_PARK = 9,                     ///< navigation to go to car park
    EC_APP_PAGE_4S_SHOP = 10                      ///< navigation to 4s shop
};

enum ECCarStatusType
{
	EC_CAR_REVERSING = 1,                         ///< car reverse    
	EC_CAR_BLUETOOTH,                             ///< car bluetooth
	EC_CAR_DRIVINGMODE,                           ///< car driving mode
	EC_CAR_AUDIO_FOCUS_CHANGE,                    ///< car audio focus change
	EC_CAR_IS_AUTO_START_EASYCONN                 ///< whether car auto start easyconn
};

enum ECCarStatusValue
{
	EC_CAR_STATUS_FALSE = 0,
	EC_CAR_STATUS_TRUE,

	EC_CAR_STATUS_STARTED,
	EC_CAR_STATUS_STOPPED,

	EC_CAR_STATUS_CLOSED,
	EC_CAR_STATUS_UNCONNECTED,
	EC_CAR_STATUS_CONNECTED,
	EC_CAR_STATUS_CONNECTED_SAME,

	EC_CAR_STATUS_LONG_FUCUS_GAIN,
	EC_CAR_STATUS_LONG_FUCUS_LOSS,
	EC_CAR_STATUS_SHORT_FUCUS_GAIN,
	EC_CAR_STATUS_SHORT_FUCUS_LOSS,
	EC_CAR_STATUS_FADEDOWN_FUCUS_GAIN,
	EC_CAR_STATUS_FADEDOWN_FUCUS_LOSS
};

/*
* @struct ECCarCmd
*
* @brief the struct of voice command, the attribute of "cmd" in ECCarCmd can be regular expression,
*        such as "(打开|开启)空调", it equals "打开空调" or "开启空调".
* 
* @see   ECSDK::registerCarCmds, IECCallback::onCarCmdNotified
*/
struct ECCarCmd
{
	char* id;                                     ///< same function command can have same id. 
	char* cmd;                                    ///< the command content matched, the content can be regular expression.
	char* vrText;                                 ///< vrText is just used in IECCallback::onCarCmdNotified, the mathched command content.
	bool  pauseMusic;                             ///< pauseMusic is just used in ECSDK::registerCarCmds, it tell the connected phone whether pause music when the command was triggered.
};

enum ECOTAUpdateErrorCode
{
	EC_OTA_ERROR_SOFTWARE_DOWNLOAD_FAILED_VIA_NETWORK = -1,
	EC_OTA_ERROR_SOFTWARE_SPACE_NOT_ENOUGH = -2,
	EC_OTA_ERROR_SOFTWARE_NETWORK_UNAVAILABLE = -3,
	EC_OTA_ERROR_SOFTWARE_DOWNLOAD_FAILED_VIA_PHONE = -4
};

struct ECOTAUpdateSoftware
{
	const char* softwareId;                       ///< id of the software.
	const char* softwareName;                     ///< name of the software.	
	uint64_t    softwareSize;                     ///< size of the software.
	uint32_t    versionCode;                      ///< current version code of the software.
	const char* vcDesc;                           ///< the description of the version code.
	const char* vcTitle;                          ///< the title of current version software.
	const char* vcDetail;                         ///< the detail of current version software.
	const char* createTime;                       ///< the create time of current version software.
	const char* modifyTime;                       ///< the modify time of current version software.
	uint8_t     isFullDist;                       ///< 1 means full package, 0 means incremental package.
	const char* packagePath;                      ///< the path of the software in the HU.
	const char* md5Path;                          ///< the path of the md5 file in the HU.
	const char* iconPath;                         ///< the path of the icon in the HU. 	
};

enum ECOTAUpdateCheckMode
{
    EC_OTA_CHECK_VIA_DEFAULT = 0,                 ///< let sdk decide which way to check ota update.
    EC_OTA_CHECK_VIA_NETWORK,                     ///< use HU's network to check ota update.
    EC_OTA_CHECK_VIA_PHONE,                       ///< use the connected phone to check ota update.
	EC_OTA_CHECK_ONLY_LOCAL                       ///< check local only to gain specified downloaded software.
};

enum ECAuthSuccessCode
{
	EC_CAR_NETWORK_AUTH_CHECK_UUID_LICENSE = 0x1010,
	EC_CAR_NETWORK_AUTH_REGISTER_UUID_LICENSE = 0x1020,
	EC_CAR_NETWORK_AUTH_DOWNLOAD_UUID_LICENSE = 0x1030,
	EC_PHONE_NETWORK_AUTH_CHECK_UUID_LICENSE = 0x2010,
	EC_PHONE_NETWORK_AUTH_REGISTER_UUID_LICENSE = 0x2020,
	EC_PHONE_NETWORK_AUTH_DOWNLOAD_UUID_LICENSE = 0x2030
};

enum ECMusicStatus
{
	EC_MUSIC_STATUS_PLAYING = 1,                  ///< music is playing.
	EC_MUSIC_STATUS_PAUSED,                       ///< music was paused.
	EC_MUSIC_STATUS_STOPPED,                      ///< music was stopped it is not used yet.
	EC_MUSIC_STATUS_PENDING                       ///< music is pending, it is not used yet.
};

struct ECAppMusicInfo
{
	ECMusicStatus status;                         ///< the status of the song.
	const char*   title;                          ///< the name of the song.
	const char*   artist;                         ///< the artist of the song.
	const char*   album;                          ///< the album of the song.
	const char*   albumArtist;                    ///< the artist of the album.
	uint64_t      length;                         ///< the total time of the song, in ms.
};

enum ECDisplayRotation
{
    EC_DISPLAY_ROTATION_0 = 0,
    EC_DISPLAY_ROTATION_90,
    EC_DISPLAY_ROTATION_180,
    EC_DISPLAY_ROTATION_270
};

enum ECFlavor
{
    EC_FLAVOR_AE_AFTER_MARKET_INSTALLED = 0x00,
    EC_FLAVOR_AE_FACTORY_INSTALLED = 0x01,
    EC_FLAVOR_AE_OVERSEA_FACTORY_INSTALLED = 0x02,
    EC_FLAVOR_AE_FREE = 0x03,
    EC_FLAVOR_AE_OVERSEA_AFTER_MARKET_INSTALLED = 0x04,
    EC_FLAVOR_GWM_EC = 0x10
};


enum ECBluetoothPolicy
{
    EC_BLUETOOTH_POLICY_SEND_DEFAULT_WITH_A2DP_CONNECTED_ONLY = 0x00,       			///<The app will send A2DP message to the HU while app come out to the system Mirroring ,and phone's A2DP connected to the HU.
    EC_BLUETOOTH_POLICY_SEND_DEFAULT_WITH_HFP_OR_A2DP_CONNECTED  = 0x01 				///<The app will send A2DP message to the HU while app come out to the system Mirroring, whether phone's A2DP connected or HFP connected to HU.
};

struct ECVideoInfo
{
    uint32_t realWidth;                          ///< the real width of video frame in pixels.
    uint32_t realHeight;                         ///< the real height of video frame in pixels.
    uint32_t mirrorWidth;                        ///< the mirror width of video frame in pixels.
    uint32_t mirrorHeight;                       ///< the mirror height of video frame in pixels.
    int32_t  direction;                          ///< the direction of video frame. -1: video file stream, 0: up vertical screen, 3: down vertical screen,4: left horizontal screen,7:right horizontal screen.
};

/**
 * The type of the editable content.
 */
enum ECInputType
{
    EC_INPUT_TYPE_NONE = 0x00000000,
    EC_INPUT_TYPE_TEXT = 0x00000001,
	EC_INPUT_TYPE_TEXT_CAP_CHARACTERS = 0x00001001,
	EC_INPUT_TYPE_TEXT_CAP_WORDS = 0x00002001,
	EC_INPUT_TYPE_TEXT_CAP_SEQUENCE = 0x00004001,
	EC_INPUT_TYPE_TEXT_AUTO_CORRECT = 0x00008001,
	EC_INPUT_TYPE_TEXT_AUTO_COMPLETE = 0x00010001,
	EC_INPUT_TYPE_TEXT_MULTI_LINES = 0x00020001,
	EC_INPUT_TYPE_TEXT_IME_MULTI_LINES = 0x00040001,
	EC_INPUT_TYPE_TEXT_NO_SUGGESTIONS = 0x00080001,
	EC_INPUT_TYPE_TEXT_URI = 0x00000011,
	EC_INPUT_TYPE_TEXT_EMAIL_ADDRESS = 0x00000021,
	EC_INPUT_TYPE_TEXT_EMAIL_SUBJECT = 0x00000031,
	EC_INPUT_TYPE_TEXT_SHORT_MESSAGE = 0x00000041,
	EC_INPUT_TYPE_TEXT_LONG_MESSAGE = 0x00000051,
	EC_INPUT_TYPE_TEXT_PERSON_NAME = 0x00000061,
	EC_INPUT_TYPE_TEXT_POSTAL_ADDRESS = 0x00000071,
	EC_INPUT_TYPE_TEXT_PASSWORD = 0x00000081,
	EC_INPUT_TYPE_TEXT_VISIBLE_PASSWORD = 0x00000091,
	EC_INPUT_TYPE_TEXT_WEB_EDIT_TEXT = 0x000000a1,
	EC_INPUT_TYPE_TEXT_FILTER = 0x000000b1,
	EC_INPUT_TYPE_TEXT_PHONETIC = 0x000000c1,
	EC_INPUT_TYPE_TEXT_WEB_EMAIL_ADDRESS = 0x000000d1,
	EC_INPUT_TYPE_TEXT_WEB_PASSWORD = 0x000000e1,
	EC_INPUT_TYPE_NUMBER = 0x00000002,
	EC_INPUT_TYPE_NUMBER_SIGNED = 0x00001002,
	EC_INPUT_TYPE_NUMBER_DECIMAL = 0x00002002,
	EC_INPUT_TYPE_NUMBER_PASSWORD = 0x00000012,
	EC_INPUT_TYPE_PHONE = 0x00000003,
	EC_INPUT_TYPE_DATETIME = 0x00000004,
	EC_INPUT_TYPE_DATE = 0x00000014,
	EC_INPUT_TYPE_TIME = 0x00000024
};

/**
 * The type of the Input Method Editor (IME).
 * imeOptions may be combined with variations and flags to indicate desired behaviors.
 */
enum ECInputImeOptions
{
    EC_INPUT_IME_ACTION_UNSPECIFIED = 0x00000000,
	EC_INPUT_IME_ACTION_NONE = 0x00000001,
	EC_INPUT_IME_ACTION_GO = 0x00000002,
	EC_INPUT_IME_ACTION_SEARCH = 0x00000003,
	EC_INPUT_IME_ACTION_SEND = 0x00000004,
	EC_INPUT_IME_ACTION_NEXT = 0x00000005,
	EC_INPUT_IME_ACTION_DONE = 0x00000006,
	EC_INPUT_IME_ACTION_PREVIOUS = 0x00000007,
	EC_INPUT_IME_FLAG_NO_PERSONALIZED_LEARNING = 0x01000000,
	EC_INPUT_IME_FLAG_NO_FULL_SCREEN = 0x02000000,
	EC_INPUT_IME_FLAG_NAVIGATE_PREVIOUS = 0x04000000,
	EC_INPUT_IME_FLAG_NAVIGATE_NEXT = 0x08000000,
	EC_INPUT_IME_FLAG_NO_EXTRACT_UI = 0x10000000,
	EC_INPUT_IME_FLAG_NO_ACCESSORY_ACTION = 0x20000000,
	EC_INPUT_IME_FLAG_NO_ENTER_ACTION = 0x40000000,
	EC_INPUT_IME_FLAG_FORCE_ASCII = 0x80000000
};

/**
 * inputType see ECInputType.
 * imeOptions see ECInputImeOptions.
 * imeOptions can be combined with action and flag.
 */
struct ECInputInfo
{
    int32_t  inputType;
    int32_t  imeOptions;
	const char* rawText;
	uint32_t minLines;
	uint32_t maxLines;
	uint32_t maxLength;
};

}

#endif // CARBIT_EC_SDK_TYPES_H
