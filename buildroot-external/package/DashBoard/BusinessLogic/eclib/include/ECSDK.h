/*****************************************************************
* Project ECSDK.
* (c) copyright 2017-2020.
* Company Carbit.
* All rights reserved. Copy without permission
****************************************************************/
///\file ECSDK.h
#ifndef CARBIT_EC_SDK_H
#define CARBIT_EC_SDK_H

#include <stdint.h>
#include "ECTypes.h"

#ifdef PLATFORM_WIN32
#ifdef ECLIBRARY_EXPORTS
#define DLL_EXPORT _declspec(dllexport)
#else
#define DLL_EXPORT _declspec(dllexport)
#endif
#else // !PLATFORM_WIN32
#define DLL_EXPORT
#endif

namespace  CarbitECSDK
{
/**
* @class  IECCallback
*
* @brief  ECSDK's callback interface.
*         Use needs to implement this interface to receive callbacks from ECSDK.
*         Its instance is used to initialize ECSDK. After initialization,
*         the instance will get notification message or receive data from ECSDK.
*
* @note   DO NOT call any ECSDK's methods in IECCallback's callback stack 
*         except onMirrorVideoReceived. otherwise, user may get threads dead lock.
*
* @see    ECSDK::initialize
*/
class DLL_EXPORT IECCallback
{
public:
    virtual  ~IECCallback();

    /**
    * @brief  Called when a phone is connected to ECSDK via USB cable or WiFi.
    *
    * @param  type  Tells by which transport the phone is connected to ECSDK.
    *               It's used to open corresponding transport.
    *
    * @see    ECSDK::openTransport
    */
    virtual void       onPhoneConnected(ECTransportType type) = 0;

    /**
    * @brief  Called when a phone is disconnected.
    */
    virtual void       onPhoneDisconnected() = 0;

    /**
    * @brief  Called when EasyConnected status changed.
    *
    * @param  status  The changed EasyConnected message.
    */
    virtual void        onECStatusMessage(ECStatusMessage status) = 0;

    /**
    * @brief  Called when the phone app sends down HUD information.
    *
    * @param  data  HUD information.
    */
    virtual void        onPhoneAppHUD(const ECNavigationHudInfo& data) = 0;

	/*
	* @brief  Called when phone app tell the music info.
	*
	* @param  data The information of music.
	*/
	virtual void        onPhoneAppMusicInfo(const ECAppMusicInfo& data) {};

    /**
    * @brief  Called when the phone app start to send down audio data.
    *
    * @param  type  Audio type, TTS, VR, IM or music.
    *
    * @param  info  Audio information.
    *
    * @note   Car needs to initialize audio player when this method is triggered. 
    *         There may be multiple audio types started. That means phone may 
    *         send down TTS and music at the same time.
    */
    virtual void        onPhoneAudioStart(ECAudioType type, const ECAudioInfo& info) = 0;

    /**
    * @brief  Called when the phone app sends down audio data. Audio data is raw PCM.
    *
    * @param  type    Audio type, TTS, VR, IM or music.
    *
    * @param  data    Buffer of audio data.
    *
    * @param  length  Buffer length.
    *
    * @note   ECSDK manages an audio data queue thread for each audio type and 
    *         this method is call in corresponding thread. So, car needs to play 
    *         audio data in this method's call stack. Car should neither try play data 
    *         in another thread nor try to cache audio data(in a queue).
    */
    virtual void        onPhoneAudioData(ECAudioType type, const void *data, uint32_t length) = 0;

    /**
    * @brief  Called when the phone app asks car to set audio volume for corresponding audio type.
    *
    * @param  type    Audio type, TTS, VR, IM or music.
    *
    * @param  volume  Volume value, range[0, 100]. 0 means mute and 100 means the max volume.
    *
    * @note   This method is called when the phone app wants to do sound mixing for multiple audio type.
    */
    virtual void        onPhoneAudioSetVolume(ECAudioType type, uint32_t volume) = 0;

    /**
    * @brief  Called when the phone app stops to send down audio data.
    *
    * @param  type    Audio type, TTS, VR, IM or music.
    */
    virtual void        onPhoneAudioStop(ECAudioType type) = 0;

	/**
	* @brief  Called when the phone app interrupts audio play.
	*
	* @param  type    Audio type, TTS, VR, IM or music.
	*/
	virtual void		onPhoneAudioInterrupt(ECAudioType type) = 0;

    /**
    * @brief  Called when the phone app asks car to start to record audio via its microphone.
    *
    * @param  info    Audio info that the phone app wants.
    *
    * @return EC_OK if car is able to record audio with the specified audio info.
    *         Otherwise, return EC_ERR_OP_FAIL.
    *
    * @note   If car does not have a microphone or want to upload any audio data to the phone app,
    *         it can set micType(in structure ECCarDescription) to EC_MIC_TYPE_PHONE when call
    *         ECSDK::initialize, then the phone app never calls this method. If set micType to
    *         EC_MIC_TYPE_NATIVE, the phone app will call this method whenever it needs audio data.
    *         After this method is invoked, car can upload recorded audio data to phone app via
    *         ECSDK::uploadAudioData.
    *
    * @see    ECSDK::initialize, ECSDK::uploadAudioData, ECCarDescription
    */
    virtual int32_t     onStartCarMicRecord(const ECAudioInfo& info) = 0;

    /**
    * @brief  Called when the phone app asks car to stop recording audio.
    */
    virtual void        onStopCarMicRecord() = 0;

    /**
    * @brief  Called when the phone app sends down some information.
    *
    * @param  data    Buffer of app information.
    *
    * @param  length  Buffer length.
    *
    * @note  data is json string, the fields includes os, osVersion and ip. 
	*        Called when ECSDK::openTransport succeed.		 
    */
    virtual void        onPhoneAppInfo(const void *data, uint32_t length) = 0;

    /**
    * @brief  Called when ECSDK wants car to do call operations(dial or hang up) via Bluetooth.
    *
    * @param  type    Operation type.
    *
	* @param  name    The person's name of corresponding number.
	*	
    * @param  number  Phone numbers.
    *
    * @note   Phone app is not able to dial or hang up automatically due to the latest system access limitation,
    *         however, car is able to do it via Bluetooth. Therefore, ECSDK moves the call operations
    *         to car, which can dial or hang up when this method is called.
    */
    virtual void        onCallAction(ECCallType type, const char* name, const char* number) = 0;
        
    /**
    * @brief  Called when a frame of mirror video data is received.
    *
    * @param  data    Buffer of the frame.
    *
    * @param  length  Buffer length.
    *
    * @param  videoInfo  detail info of video
    *
    * @note   The mirror data format is set by "openMirrorConnection". When receives a frame,
    *         car can decode and render the frame. ECSDK would not acquire the next frame from
    *         phone until this method returned, so if car's decoding and rendering is low performance,
    *         suggest car to apply frame cache strategy instead of handling the frame in current
    *         callback thread.
    *
    * @see    ECSDK::openMirrorConnection
    */
    virtual void        onMirrorVideoReceived(const void *data, uint32_t length, const ECVideoInfo& videoInfo) = 0;

    /**
   *
   * @deprecated use onMirrorVideoReceived instead
   *
   * @brief  Called when a Mirror width/height or rotation changed which android Mirror connected with AOA .
   *
   * @param  mirrorWidth    real mirror width encode by phone.
   *
   * @param  mirrorHeight    real mirror height encode by phone.
   *
   * @param  rotation  the phone rotation.
   *
   * @note   The API is used to notify the UI,they can do something with HMI. It is different with
   * openMirrorConnection,which returned MirrorWidth/MirrorHeight is fill with the black area and
   * rotate by encoder.for example,the car(1024x600) called openMirrorConnection required a mirror
   * data,then phone returned a mirror width 1024x592,in stead of the onMirrorVideoInfoChanged returned a mirror
   * info with width=270 height=585,rotation=0.
   *
   *
   *  @see    ECSDK::openMirrorConnection
   */
    virtual void        onMirrorVideoInfoChanged(int mirrorWidth,int mirrorHeight, int rotation){};

    /**
    * @brief  Called when bulk data is received(RESERVED).
    *
    * @param  data    Buffer of bulk data.
    *
    * @param  length  Buffer length.
    *
    * @note   THIS IS A RESERVED METHOD. PLEASE IGNORE IT NOW.
    */
    virtual void        onBulkDataReceived(const void *data, uint32_t length) = 0;

    /**
    * @brief  Called when mirror connection disconnected.
    *
    * @note   This method is usually called when the phone app was killed by system or an error happened.
    *         After this method is called, car should close the mirror connection and try to ask user to
    *         launch app again. After that, car can open mirror connection again.
    *         "onPhoneDisconnected" will not trigger this method.
    *
    * @see    ECSDK::openMirrorConnection, ECSDK::closeMirrorConnection
    */
    virtual void        onMirrorDisconnected() = 0;

    /**
    * @brief  Called when current mirror connect was closed, then mirror reconnect was requested.
    *
    * @note   the mirror should be reconnected by ECSDK::startMirror.
    */
    virtual void        onMirrorRequestReconnect(){};

    /**
    * @brief  Called when the transport was stopped by user. The phone app has a button 
    *         that can manually stop current connection.
    */
    virtual void        onTransportStopedByApp() = 0;

    /**
    * @brief  Called when the license authorization failed. After this interface was called, 
    *         all connections would be forced closed.
    *
    * @param  errCode Error code.
    *
    * @param  errMsg Error message.
    */
    virtual void        onLicenseAuthFail(int32_t errCode, const char* errMsg) = 0;

	/**
	* @brief  Called when the license authorization succeed. 
	*
	* @param  code success code. The code can gain specific meaning by ECAuthSuccessCode.
	*
	* @param  msg success information.
	*
	* @param  msg the description information.
	*
	*/
	virtual	void		onLicenseAuthSuccess(int code, const char* msg) = 0;

    /**
    * @brief  Called when the phone app exit. Maybe killed by system.
    */
    virtual void        onPhoneAppExited() = 0;     

	/**
	* @brief  Called when registered command was triggered by VR.
	* 
	* @param  carCmd The triggered command.
	*
	* @note   Voice control can be implemented with this method by VR.
	*
	* @see    ECSDK::registerCarCmds
	*/
	virtual void        onCarCmdNotified(const ECCarCmd& carCmd) = 0;

	/* 
	* @brief  Called when checkOTAUpdate was called, it will tell the result of checkOTAUpdate. 
	*
	* @param  downloadableSoftwares It pointer to a array of ECOTAUpdateSoftware, which is downloadable software.
	*
	* @param  downloadableLength  The length of the downloadable array, if downloadableLength < 0, means network of the HU is unavailable.
	*
	* @param  downloadedSoftwares It pointer to a array of ECOTAUpdateSoftware, which is downloaded software.
	*
	* @param  downloadedLength  The length of the downloaded array.
	*/
	virtual void        onOTAUpdateCheckResult(const ECOTAUpdateSoftware* downloadableSoftwares, int32_t downloadableLength, const ECOTAUpdateSoftware* downloadedSoftwares, uint32_t downloadedLength) {};
	                     
	/**
	* @brief Called when startOTAUpdate is called, it will notify the progress of downloading.
	*
	* @param downloadingSoftwareId The id of the downloading software.
	*
	* @param progress The progress of the downloading software,which is a percentage.
	*
	* @param softwareLeftTime The left time of the downloading software.
	*
	* @param otaLeftTime The left time of all the specified software by startOTAUpdate.
	*/
	virtual void        onOTAUpdateProgress(const char* downloadingSoftwareId, float progress, uint32_t softwareLeftTime, uint32_t otaLeftTime) {};

	/**
	* @brief Called when startOTAUpdate is called, it will notify software is downloaded.
	*
	* @param downloadedSoftwareId The id of the downloaded software.
	*
	* @param md5Path The md5 file path.
	*
	* @param packagePath The software path.
	*
	* @param iconPath The icon path.
	*
	* @param leftSoftwareNum The amount of software remaining to be downloaded.
	*/
	virtual void        onOTAUpdateCompleted(const char* downloadedSoftwareId, const char* md5Path, const char* packagePath, const char* iconPath, uint32_t leftSoftwareNum) {};

	/*
	* @brief  Called when checkOTAUpdate or startOTAUpdate failed. 
	*
	* @param  errCode error code, see ECOTAUpdateErrorCode.
	*
	* @param  softwarId  the id of software.
	*/ 
	virtual void        onOTAUpdateError(int32_t errCode, const char* softwareId) {};

	/**
	 * @brief  Called when phone app request the HU to start input.
	 *
	 * @param info relevant parameters about the input.
	 */
	virtual void        onInputStart(const ECInputInfo& info) {};

    /**
     * @brief  Called when phone app request the HU to cancel input.
     */
	virtual void        onInputCancel() {};

    /**
     * @brief  Called when phone app tell the selection of input.
     */
	virtual void        onInputSelection(int start, int stop) {};
};

/**
* @class  IECAccessDevice
*
* @brief  An interface class to implement special usb device access.
*/
class DLL_EXPORT IECAccessDevice
{
public:
    virtual  ~IECAccessDevice();

    /**
    * @brief  Open usb device.
    *
    * @return Zero on success, others on fail.
    */
    virtual int32_t     open() = 0;

    /**
    * @brief  Read data from usb devices.
    *
    * @param  data    Read buffer.
    *
    * @param  length  Read size.
    *
    * @return The size of data read in bytes, zero if currently no read data available, or negative number on fail. 
    */
    virtual int32_t     read(void *data, uint32_t length) = 0;

    /**
    * @brief  Write data into usb devices.
    *
    * @param  data    Write buffer.
    *
    * @param  length  Write size.
    *
    * @return The size of data written in bytes, negative number on fail.
    */
    virtual int32_t     write(void *data, uint32_t length) = 0;

    /**
    * @brief  Close usb device. Opposite to open.
    *
    * @see    open
    */
    virtual void        close() = 0;
};

/**
* @class  ECSDK
*
* @brief  The main class providing all ECSDK methods.
*
* @note   Most methods of class ECSDK return an int value. The returned value is named as
*         EC_OK or other EC_ERR_* macros(defined in class @link ECTypes.h).
*/
class DLL_EXPORT ECSDK
{
public:
    /**
    * @brief  Get the singleton instance of class ECSDK.
    *
    * @return Pointer of ECSDK instance.
    *
    * @note   User is NOT allowed to construct ECSDK object.
    */
    static ECSDK*   getInstance();

    /**
    * @brief  Get the ECSDK version.
    *
    * @return The ECSDK version in string format, etc. "1.0.0".
    */
    virtual const char* getVersion() = 0;

	/** 
	* @brief  Get the version code of ECSDK.
	*
	* @return The version code of ECSDK.
	*/
	virtual uint64_t  getVersionCode() = 0;

    /**
    * @brief  Initialize ECSDK environment.
    *         After ECSDK was initialized, user can wait until IECCallback::onPhoneConnected 
    *         is invoked and then start to use other ECSDK methods to communicate with the
    *         connected phone. ECSDK supports finding an connected phone(Android or iOS)
    *         via USB cable and WiFi.
    *
    * @param  authentication ECSDK authentication.
    *
    * @param  carDesc        Car description.
    *
    * @param  options        Initialization options.
    *
    * @param  listener       Pointer of IECCallback instance.
    *
    * @return EC_OK on success, others on fail.
    *
    * @see    IECCallback
    */
    virtual int32_t     initialize(const ECAuthentication& authentication, const ECCarDescription& carDesc, const ECOptions& options, IECCallback *listener) = 0;

    /**
    * @brief  Release all ECSDK resources. Opposite to ECSDK::initialize.
    */
    virtual void        release() = 0;

    /**
    * @brief  set log information.
    *
    * @param  level        log level.Default is EC_LOG_LEVEL_INFO.
    *
    * @param  type         the type of log output.Default is EC_LOG_OUT_STD.
    *
    * @param  logDirectory      absolute path of log out when type is EC_LOG_OUT_FILE.default directory is "/tmp/",and logfile is "/tmp/ECSDK_H_M_S.log"
    *
    * @param  module       specify output log module.Default is EC_LOG_MODULE_SDK.Yout can do like this EC_LOG_MODULE_SDK|EC_LOG_MODULE_APP.
    */
    virtual void        setLogInfo(const ECLogLevel level,const ECLogOutputType type,const char*logDirectory,int module=EC_LOG_MODULE_SDK) = 0;

    /**
    * @brief  Open WIFI service to find a phone IP in local WIFI network. Use WIFI service only when car supports WIFI.
    *         If find a phone, IECCallback::onPhoneConnected will be triggered with transport
    *         type=EC_TRANSPORT_ANDROID_WIFI or EC_TRANSPORT_IOS_WIFI.
    *
    * @return EC_OK on success, others on fail.
    *
    */
    virtual int32_t     openWiFiService() = 0;

    /**
    * @brief  Close WIFI service. Opposite to openWiFiService.
    */
    virtual void        closeWiFiService() = 0;

	virtual int32_t		openNetLinkService() = 0;

	virtual void		closeNetLinkService() = 0;

    /**
    * @brief  Bind the transport with a special USB device. 
    *         After binding, ECSDK can access the device via the provided IECAccessDevice.
    *         Once the transport is bound successfully, IECCallback::onPhoneConnected will be invoked.
    *
    * @param  type  The transport to be bound.
    *
    * @param  dev   The device access methods, see IECAccessDevice.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   Only platform-dependent transport(i.e EAP) needs to bind a USB device.
    */
    virtual int32_t     bindTransportDevice(ECTransportType type, IECAccessDevice *dev) = 0;

    /**
    * @brief  Unbind USB device for the given transport. Opposite to bindTransportDevice.
    *
    * @param  type  The transport to be unbound.
    *
    * @see    ECSDK::bindTransportDevice
    */
    virtual void        unbindTransportDevice(ECTransportType type) = 0;

	/**
	* @brief  Set specified phone ip.
	*		  This method gives chance to directly connect to the specified phone via wifi.
	*
	* @param  ip The specified ip address.
	*  
	* @return EC_OK on success, others on fail.
	*
	* @note   This method would fail if ECSDK::openTransport has been successfully  called.
	*         IECCallback::onPhoneAppInfo can tell discovered phone info include ip.
	*/
	virtual int32_t		setWirelessDirectConnectPhoneIp(const char* ip) = 0;

    /**
    * @brief  Open specified transport. 
    *         Please call this method after IECCallback::onPhoneConnected is invoked, 
    *         which tells the transport type. Only after the transport was successfully opened,
    *         user can start to use mirror and other methods.
    *
    * @param  type  Transport type.
    *
    * @return EC_OK on success, others on fail. 
    *         EC_ERR_APP_NOT_STARTED if the phone app is not started or installed, 
    *                                needs to guide user to install and start the app.
    *         EC_ERR_APP_AUTH_PENDING if authorization is pending, needs to guide user to approve it.
    *
    * @see    IECCallback::onPhoneConnected
    *
    * @note   DO NOT call this method in IECCallback::onPhoneConnected call stack. User should 
    *         call this method in his/her own thread.
    */
    virtual int32_t     openTransport(ECTransportType type) = 0;

    /**
    * @brief  Close transport. Opposite to openTransport.
    *
    * @see    ECSDK::openTransport
    */
    virtual void        closeTransport() = 0;

    /**
    * @brief  Open mirror connection.
    *
    * @param  config            Mirror configurations.
    *
    * @param[out] mirrorWidth   Returned mirror width.
    *
    * @param[out] mirrorHeight  Returned mirror height.
    *
    * @return EC_OK on success, others on fail. Returning EC_ERR_APP_AUTH_FAIL means the authorization failed.
    *
    * @note   The eventual output mirror size is {mirrorWidth} x {mirrorHeight}(in pixels), which is equal to
    *         or litter than the configured width and height. Please use the returned size to
    *         initialize video decoder and render window.
    */
    virtual int32_t     openMirrorConnection(const ECMirrorConfig& config, uint32_t& mirrorWidth, uint32_t& mirrorHeight) = 0;

    /**
    * @brief  Start mirror transmission.
    *
    * @return EC_OK on success, others on fail. Returning EC_ERR_APP_AUTH_FAIL means user needs to allow phone app to capture screen and 
    *         returning EC_ERR_APP_AUTH_PENDING means waiting for the user to allow phone app to capture screen. 
    *
    * @note   After started mirror, car would receive mirror data via IECCallback::onMirrorVideoReceived.
    *
    * @see    IECCallback::onMirrorVideoReceived
    */
    virtual int32_t     startMirror() = 0;

    /**
    * @brief  Pause mirror transmission.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   After paused, car would not receive mirror data until mirror resumed.
    *
    * @see    ECSDK::resumeMirror
    */
    virtual int32_t     pauseMirror() = 0;

    /**
    * @brief  Resume mirror transmission. Opposite to pauseMirror.
    *
    * @return EC_OK on success, others on fail.
    *
    * @see    ECSDK::pauseMirror
    */
    virtual int32_t     resumeMirror() = 0;

    /**
    * @brief  Stop mirror transmission. Opposite to startMirror.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   After stopped, ECSDK would not receive mirror data any more.
    *
    * @see    ECSDK::startMirror
    */
    virtual void        stopMirror() = 0;

    /**
    * @brief  Send key event to phone.
    *
    * @param  key  Key code.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   Whether or not the key is supported depends on the phone system.
    *         Key event does NOT work without mirror connection.
    */
    virtual int32_t     sendSystemKeyEvent(ECSystemKeyCode key) = 0;

    /**
    * @brief  Send touch event to phone.
    *
    * @param  touch  Touch parameters.
    *
    * @param  type   Touch type.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   Touch event does NOT work without mirror connection.
    */
    virtual int32_t     sendTouchEvent(const ECTouchEventData& touch, ECTouchEventType type) = 0;

    /**
    * @brief Close mirror connection. Opposite to openMirrorConnection.
    */
    virtual void        closeMirrorConnection() = 0;

    /**
    * @brief  when you press button on steering wheel, send corresponding button event to phone.
    *
    * @param  btnCode  key code of the Button .
    *
    * @param  type  Event type.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   Button event does NOT work without phone app.
    */
    virtual int32_t     sendBtnEvent(ECBtnCode btnCode, ECBtnEventType type) = 0;

    /**
   * @brief  send button code to phone.
   *
   * @param  btnCode  key code of the Button .
   *
   * @param  type  Event type.
   *
   * @param  channel Which can specify the namespace of the code. Zero means standard, others means custom.
   *                 When channel is zero, the value of channel can refer to ECBtnCode.
   *
   * @return EC_OK on success, others on fail.
   *
   * @note   Button event does NOT work without phone app.
   */
    virtual int32_t     sendBtnEvent(int32_t btnCode, ECBtnEventType type, int32_t channel) = 0;

    /**
    * @brief  Stop phone navigation.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   ECSDK suggests to use only one navigation between phone and car.
    *         If car start navigation, use this method to stop phone
    *         navigation, opposite to IECCallback::onECStatusMessage.
    *
    * @see    IECCallback::onECStatusMessage
    */
    virtual int32_t     stopPhoneNavigation() = 0;

	/**
	* @brief  Stop phone voice recognition(VR).
	*
	* @return EC_OK on success, others on fail.
	*
	*/
	virtual int32_t     stopPhoneVR() = 0;

    /**
    * @brief  Upload car's audio data to phone. Audio is recorded by car microphone.
    *
    * @param  data    Buffer of audio data.
    *
    * @param  length  Buffer length.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t    uploadAudioData(const void *data, int32_t length) = 0;

    /**
    * @brief  Upload GPS location to phone.
    *
    * @param  timestamp  The timestamp(nanosecond) of this location.
    *
    * @param  latitude   Latitude value. Range is [-90.0,+90.0].
    *
    * @param  longitude  Longitude value. Range is [-180.0,+180.0].
    *
    * @param  accuracy   Horizontal 68% radius meters value.
    *
    * @param  altitude   Altitude value.
    *
    * @param  speed      Car speed(m/s).
    *
    * @param  bearing    Car bearing. Range is [0, 360).
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   Uploading frequency can be consistent with GPS hardware.
    */
    virtual int32_t     uploadLocation(uint64_t timestamp, float latitude, float longitude, float accuracy, float altitude, float speed, float bearing) = 0;

    /**
    * @brief  Upload car's orientation to phone.
    *
    * @param  bearing  The compass bearing degree of car. Range is [0, 360).
    *
    * @param  pitch    The pitch degree of car. Range is [-90, 90].
    *
    * @param  roll     The roll degree of car. Range is [-90, 90].
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadCompassBearing(float bearing, float pitch, float roll) = 0;

    /**
    * @brief  Upload car's speed information to phone.
    *
    * @param  speed           Vehicle speed(m/s). Negative value when moving in reverse.
    *
    * @param  cruiseEnabled   Whether or not cruise control is enabled. True when cruise control is enabled.
    *
    * @param  cruiseSetSpeed  The speed(m/s) of the cruise control. Valid when cruiseEnabled is true.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadVehicleSpeed(float speed, bool cruiseEnabled, float cruiseSetSpeed) = 0;

    /**
    * @brief  Upload car's rotational speed to phone.
    *
    * @param  rpm  Vehicle RPM in rad/sec.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadEngineRpm(float rpm) = 0;

    /**
    * @brief  Upload car's odometer to phone.
    *
    * @param  odometer  The odometer value.
    *
    * @param  tripKms   Distance traveled since ignition was turned on in km.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadOdometer(float odometer, int32_t tripKms) = 0;

    /**
    * @brief  Upload car's fuel information to phone.
    *
    * @param  remainFuelPercent  The remaining fuel in percent values.
    *
    * @param  range              The estimated remaining range in km for the current amount of fuel.
    *
    * @param  enableFuelWarning        Whether or not enable low fuel warning.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadFuelStatus(float remainFuelPercent, int32_t range, bool enableFuelWarning) = 0;

    /**
    * @brief  Upload car's parking brake status to phone.
    *
    * @param  isBrakeOn  Whether or not the parking brake is on.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadParkingBrakeStatus(bool isBrakeOn) = 0;

    /**
    * @brief  Upload car's gear status to phone.
    *
    * @param  gear  Gear type.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadGearStatus(ECGearType gear) = 0;

    /**
    * @brief  Upload car's night mode status to phone.
    *
    * @param  isNightModeOn  Whether or not night mode is turned on.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadNightModeStatus(bool isNightModeOn) = 0;

    /**
    * @brief  Upload car's external environment status to phone.
    *
    * @param  temperature  The temperature in Celsius.
    *
    * @param  pressure     Pressure value.
    *
    * @param  rain         The rain detection level. 0 means no rain.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadEnvironmentStatus(float temperature, float pressure, int32_t rain) = 0;

    /**
    * @brief  Upload car's driving status to phone.
    *
    * @param  status  Driving status.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadDrivingStatus(ECDrivingStatus status) = 0;

    /**
    * @brief  Upload car's passenger presence status to phone.
    *
    * @param  present  True if a passenger is present, false otherwise.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadPassengerStatus(bool present) = 0;

    /**
    * @brief  Upload car's doors status to phone.
    *
    * @param  hoodOpen     True if the hood is open, false otherwise.
    *
    * @param  trunkOpen    True if the trunk is open, false otherwise.
    *
    * @param  doorsOpened  Array of door states, true if the door is open, false otherwise.
    *
    * @param  nDoors       The number of doors.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadDoorStatus(bool hoodOpen, bool trunkOpen, const bool* doorsOpened, int32_t nDoors) = 0;

    /**
    * @brief  Upload car's lights status to phone.
    *
    * @param  headlight      The status of the head lights.
    *
    * @param  indicator      The status of turn indicator.
    *
    * @param  hazardLightOn  True if hazard lights are on, false otherwise.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadLightStatus(ECHeadLightStatus headlight, ECTurnIndicatorStatus indicator, bool hazardLightOn) = 0;

    /**
    * @brief  Upload car's tire pressures to phone.
    *
    * @param  tirePressure     Array of tire pressure values in psi.
    *
    * @param  nTires           The number of tires.
    *
    * @return EC_OK on success, others on fail.
    */
    //
    virtual int32_t     uploadTirePressure(const float* tirePressure, int32_t nTires) = 0;

    /**
    * @brief  Upload car's accelerometer to phone. Units are m/s^2 multiplied by 1e3.
    *
    * @param  accX  Acceleration from left to right.
    *
    * @param  accY  Acceleration from back to head.
    *
    * @param  accZ  Acceleration from bottom to top.
    */
    virtual int32_t     uploadAccelerometerStatus(int32_t accX, int32_t accY, int32_t accZ) = 0;

    /**
    * @brief  Upload car's gyroscope to phone. Units are rad/s.
    *
    * @param  speedX  Rotation speed around axis from left to right.
    *
    * @param  speedY  Rotation speed around axis from back to head.
    *
    * @param  speedZ  Rotation speed around axis from bottom to top.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadGyroscopeStatus(float speedX, float speedY, float speedZ) = 0;

    /**
    * @brief  Upload GPS satellite status to phone.
    *
    * @param  numSatInUse    The number of satellites used in GPS fix.
    *
    * @param  numSatVisible  The number of satellites visible to the GPS receiver.
    *
    * @param  prnData        Array of PRNs of satellites in view or NULL if per-satellite info unavailable.
    *
    * @param   nPRN           The array size of prnData.
    *
    * @param  snrDBData      Array of SNRs of satellites in view in dB or NULL if per-satellite info unavailable.
    *
    * @param   nSRN           The array size of snrDBData.
    *
    * @param  usedInFix      Array of flags whether this satellite was used in GPS fix or NULL if per-satellite info unavailable.
    *
    * @param   nUsed          The array size of usedInFix.
    *
    * @param  azimuthsData   Array of azimuths of satellites in degrees clockwise from north or NULL if per-satellite info
    *                        unavailable or position data for satellites is absent.
    *
    * @param  nAzimuth       The array size of azimuthsData.
    *
    * @param  elevationData  Array of elevations of satellites in degrees from horizon to zenith or NULL if per-satellite info
    *                        unavailable or position data for satellites is absent.
    *
    * @param  nElevation     The array size of elevationData.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadGpsSatelliteStatus(int32_t numSatInUse, int32_t numSatVisible, const int32_t* prnData, int32_t nPRN,
        const int32_t* snrDBData, int32_t nSRN, const bool* usedInFix, int32_t nUsed,
        const float* azimuthsData, int32_t nAzimuth, const float* elevationData, int32_t nElevation) = 0;

    /**
    * @brief  Upload sensor error to phone.
    *
    * @param  sensor       The type of sensor having problem.
    *
    * @param  sensorError  The error type.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     uploadSensorError(ECSensorType sensor, ECSensorErrorType sensorError) = 0;

    /**
    * @brief  Upload vehicle info to phone(RESERVED).
    *
    * @param  data    Buffer of vehicle info.
    *
    * @param  length  Buffer length.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note  THIS IS A RESERVED METHOD. PLEASE IGNORE IT NOW.
    */
    virtual int32_t     uploadVehicleInfo(const void* data, uint32_t length) = 0;

    /**
    * @brief  Send bulk data to phone(RESERVED).
    *
    * @param  data    Buffer of bulk data.
    *
    * @param  length  Buffer length.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note  THIS IS A RESERVED METHOD. PLEASE IGNORE IT NOW.
    */
    virtual int32_t     sendBulkDataToPhone(const void *data, uint32_t length) = 0;

    /**
    * @brief  Enable downloading phone audio.
    *
    * @param  enableTTS     Enable downloading TTS.
    *
    * @param  enableVR      Enable downloading VR.
    *
    * @param  enableTalkie  Enable downloading Talkie.
    *
    * @param  enableMusic   Enable downloading Music.
    *
	* @param  autoChangeToBT Whether the phone send talkie and music audio to car via Bluetooth.
	*
    * @return EC_OK on success, others on fail.
    *
    * @note  If car media supports playing TTS music, it can ask phone app to send down TTS voice,
    *        which includes navigation and/or VR voice. Driver can heart navigation voice more
    *        clearly from car speaker than from phone speaker. This is very useful in noisy driving
    *        environment. This method does not impact phone media(mp3, mp4, etc) voice, which may be
    *        transferred to car via Bluetooth.
    *
    * @see   IECCallback::onPhoneAudioStart, IECCallback::onPhoneAudioData,
    *        IECCallback::onPhoneAudioStop, IECCallback::onPhoneAudioSetVolume
    */
    virtual int32_t     enableDownloadPhoneAppAudio(bool enableTTS, bool enableVR, bool enableTalkie, bool enableMusic, bool autoChangeToBT = true) = 0;

    /**
    * @brief  Disable downloading phone audio. Opposite to enableDownloadPhoneAppAudio.
    *
    * @see    enableDownloadPhoneAppAudio
    */
    virtual void        disableDownloadPhoneAppAudio() = 0;

    /**
    * @brief  Enable downloading phone navigation HUD information.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note  If car wants to display HUD information in its screen, it can ask phone app to send down
    *        HUD information.
    *
    * @see   IECCallback::onPhoneAppHUD
    */
    virtual int32_t     enableDownloadPhoneAppHud() = 0;

    /**
    * @brief  Disable downloading phone navigation HUD. Opposite to enableDownloadPhoneAppHud.
    *
    * @see    enableDownloadPhoneAppAudio
    */
    virtual void        disableDownloadPhoneAppHud() = 0;

    /**
    * @brief  Set bluetooth mac address of car and connected phone.    .
    *
    * @param  carBtMac   Bluetooth mac address of car.
    *
    * @param  phoneBtMac Bluetooth mac address of phone.
    *
    * @return EC_OK on success, others on fail.   
    *
    * @note   This interface shall be called only when car was connected to phone via bluetooth.
    *         THIS IS A RESERVED METHOD. PLEASE IGNORE IT NOW.
    */
    virtual int32_t     setConnectedBTAddress(const char* carBtMac, const char* phoneBtMac) = 0;

    /**
    * @brief  Open ftp server which forwards the local ports on HUD to the ports on the connected phone.
    *
    * @param  userName Specifying the username of ftp.
    *
    * @param  pwd Specifying the password of ftp.
    *
    * @return EC_OK on success, others on fail.
    *
    * @note   This interface shall be called only after ECSDK::openTransport was called successfully via usb connection.
    *         
    */
    virtual int32_t     openFtpServer(const char* userName, const char* pwd) = 0;

    /**
    * @brief  Open app page.
    *
    * @param  page Specifying which page of app.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     openAppPage(ECAppPage page) = 0;

    /**
    * @brief  Open app page.
    *
    * @param  page Specifying which page of app.
    *
    * @param  channel Which can specify the namespace of the page. Zero means standard, others means custom.
	*                 When channel is zero, the value of page can refer to ECAppPage.
    *
    * @return EC_OK on success, others on fail.
    */
    virtual int32_t     openAppPage(int32_t page, int32_t channel) = 0;

	/**
	* @brief  Send to phone when audio play was end.
	*
	* @param  type  Audio type, TTS, VR, IM or music.
	*
	* @return EC_OK on success, others on fail.
	*
	* @note   This interface should be called when the connected phone need to know a piece of audio has finished play. 
	*		  It can be	called after IECCallback::onPhoneAudioStop was called.
	*/ 
	virtual int32_t     sendAudioPlayEnd(ECAudioType type) = 0;
	
	/**
	* @brief  Send an request to phone for GPS information.
	*
	* @param  status  Whether GPS information is valid, true means valid.
	*
	* @param  longitude GPS longitude.
	*
	* @param  latitude  GPS latitude.
	*
	* @return EC_OK on success, others on fail.
	*
	* @note   The query result will be filled to parameters.
	*/
	virtual	int32_t     queryGPS(bool& status, double& longitude, double& latitude) = 0;

	/**
	* @brief  Send an request to phone for time information.
	*
	* @param  gmtTime   GMT(UTC) time in milliseconds.
	*
	* @param  localTime Local time in milliseconds.
	*
	* @param  timeZone  Time zone, buffer for data to save,  etc. "Asia/Shanghai".
	*
	* @param  len       The length of the buffer.
	*
	* @return EC_OK on success, others on fail.
	*
	* @note   The query result will be filled to parameters.
	*/
	virtual	int32_t     queryTime(uint64_t& gmtTime, uint64_t& localTime, char* timeZone, uint32_t len) = 0;

	/**
	* @brief  Send car status info to phone.
	*
	* @param  carStatus The status type of car.
	*
	* @param  value     The value of the corresponding status type.
	*
	* @return EC_OK on success, others on fail.
	*
	*/
	virtual int32_t		sendCarStatus(ECCarStatusType carStatus, ECCarStatusValue value) = 0;
	
	/**
	* @brief  Check the network status of connected phone.
	*
	* @param  status The status of phone's network, true means network is available and false means network is unavailable.
	*
	* @return EC_OK on success, others on fail.
	*
	*/
	virtual int32_t		checkPhoneNetwork(bool& status) = 0;

	/**
	* @brief  Send car's blue tooth information to connected phone.
	*
	* @param  name The name of car's bluetooth.
	*
	* @param  adddress The adddress of car's bluetooth.
	*
	* @param  pin The pin code of car's bluetooth.
	*
	* @return EC_OK on success, others on fail.
	*
	*/
	virtual int32_t     sendCarBluetooth(const char* name, const char* adddress, const char* pin) = 0;

	/**
	* @brief  Specifying commands to identify by VR.
	*
	* @param  carCmds  The command array.
	*
	* @param  length  The number of command in carCmds.
	*
	* @return EC_OK on success, others on fail.
	*
	* @note   This method specify which commands can be identified by VR. 
	*         Then IECCallback::onCarCmdNotified would be trigggered if a
	*         voice control command was recognized by VR. 
	*/
	virtual int32_t     registerCarCmds(const ECCarCmd* carCmds, uint32_t length) = 0;

    /**
    * @brief  upload statistics information to connected phone.
    *
    * @param  key key with which the specified value is to be associated
    *
    * @param  value value to be associated with the specified key
    *
    * @return EC_OK on success, others on fail.
    *
    */
	virtual int32_t     uploadStatistics(const char* key, const char* value) = 0;
	
	/**
	* @brief  This method will trigger a check to ota update.
	*
	* @param  otaConfigPath This path will save the config files, such as "aaaaa.2.req".
	*
	* @param  otaPackagePath This path will save the software package and md5 file and log file.
	*                        software package such as "aaaaa.3.bin",
	*                        md5 file such as "aaaaa.3.md5",
	*                        log file such as "aaaaa.2.3.succ.log" or "aaaaa.2.3.err.log".
	* 
    * 
    * @param  language Which language's description of software will be gained when HU's network is used for ota update.
    *
    * @param  mode     Specify which way to check ota update.
    *
	* @return EC_OK on success, others on fail.
	*
	* @note   IECCallback::onOTAUpdateCheckResult will tell the result. If HU is connecting to a phone,
	*         the update is checked via phone, or via HU's network.
	*/
	virtual int32_t     checkOTAUpdate(const char* otaConfigPath, const char* otaPackagePath, const char* language, ECOTAUpdateCheckMode mode = EC_OTA_CHECK_VIA_DEFAULT) = 0;

	/**
	* @brief  This method will download specified softwares sequentially.
	*
	* @param  softwareIds The array save the software id. 
	*
	* @param  softwareNum The length of array.
	*
	* @return EC_OK on success, others on fail.
	*
	* @note   The software id would gained from IECCallback::onOTAUpdateCheckResult after ECSDK::checkOTAUpdate was called.
	*/
	virtual int32_t     startOTAUpdate(const char** softwareIds, int32_t softwareNum) = 0;

	/**
	* @brief  This method will stop downloading softwares.
	*
	* @note   OTA update doesn't support breakpoint continuation. The software which haven't been completed
	*         would be removed.
	*/
	virtual void        stopOTAUpdate() = 0;

	/**
	 * @brief  This method will enable ota network update use sandbox environment.
	 *
	 * @note   The default environment of ota network update is production.
	 */
	virtual void        enableSandbox() = 0;

	/**
	 * @brief This method will directly make phone app go to main page.
	 */
	virtual int32_t     openAppMainPage() = 0;

	/**
	 * @brief This method make phone app play the tts audio of car's text.
	 *
	 * @param text  The text of tts.
	 *
	 * @param level The priority of tts, the value will be from 1 to 10, bigger value means higher priority
	 *
	 * @return EC_OK on success, others on fail.
	 *
	 * @note  This method will send the text to connected phone, then the tts audio of specified text will be played.
	 */
    virtual int32_t     playCarTTS(const char* text, uint32_t level) = 0;

    /**
     * @brief This method send the text of HU's input to phone app.
     *
     * @param text The text of input
     *
     * @return EC_OK on success, others on fail.
     *
     * @note  This method will send the text to connected phone.
     */
    virtual int32_t     sendInputText(const char* text) = 0;

    /**
     * @brief This method send the action of HU's input to phone app.
     *
     * @param actionId The action of input
     *
     * @param keyCode The keyCode of input
     *
     * @return EC_OK on success, others on fail.
     *
     * @note  This method will send the action and keyCode to connected phone.
     */
    virtual int32_t     sendInputAction(int actionId, int keyCode) = 0;

    /**
     * @brief This method send the selection of HU's input to phone app.
     *
     * @param start The start index of the selection.
     *
     * @param stop The stop index of the selection.
     *
     * @return EC_OK on success, others on fail.
     *
     * @note  This method will send the selection to connected phone.
     */
    virtual int32_t     sendInputSelection(int start, int stop) = 0;

protected:
    ECSDK();
    virtual ~ECSDK();
}; // ECLibrary

} // namespace CarbitECLibrary

#endif // CARBIT_EC_SDK_H
