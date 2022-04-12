#ifndef CMDCHANNEL_H
#define CMDCHANNEL_H


#include "thread.h"
#include "CCarLifeLibWrapper.h"

using namespace CommonUtilH;

enum ModuleId
{
    CARLIFE_PHONE_MODULE_ID=1,
    CARLIFE_NAVI_MODULE_ID=2,
    CARLIFE_MUSIC_MODULE_ID=3,
    CARLIFE_VR_MODULE_ID=4, //
    CARLIFE_CONNECT_MODULE_ID=5,
    CARLIFE_MIC_MODULE_ID=6,
    CARLIFE_MEDIAPCM_MODULE_ID=7,
    CARLIFE_EDOG_MODULE_ID=8,
    CARLIFE_CRUISE_ID=9,
};

void setCarlifeConfig(int foucs, int wakeup);

class CmdChannel : public Thread
{

public:
    virtual ~CmdChannel();
    static CmdChannel *instance();


    void SetCmdStatusCallback(void (*callback)(int, void*), void *parameter);
    void SetCmdPhoneNumberCallback(void (*callback)(string, void*), void *parameter);
    void SetCmdMediaInfoCallback(void (*callback)(string,string,string, void*), void *parameter);
    bool GetBlueToothUnmanaged() const {return m_bt_unmanaged;}
    void SendCarBluetooth(const string& name, const string& address, const string& pin);
protected:
    virtual void run();             //线程执行函数

private:
    CmdChannel();

    //数据接收回调函数
    static void recvProtocolVersionMatchStatus(S_PROTOCOL_VERSION_MATCH_SATUS *);               //接收手机端发送给车机端的协议版本匹配结果
    static void recvMDInfo(S_MD_INFO *);                                                        //接收手机端发送给车机端的手机系统信息

    static void recvMdAuthenResponse(S_AUTHEN_RESPONSE *);                                      //手机CarLife将接收到的车机随机数使用私有密钥进行加密
    static void recvMdAuthenResult(S_MD_AUTHEN_RESULT *);                                       //手机端CarLife将验证结果发送给车机端
    static void recvFeatureConfigRequest();                                                     //双方握手认证通过后，手机端会发送该消息来请求车机端的功能定制项

    static void recvModuleStatus(S_MODULE_STATUS_LIST_MOBILE *);                                //接收手机端通知车机端音乐、导航、电话、VR等模块的状态

    static void recvVideoEncoderInitDone(S_VIDEO_ENCODER_INIT_DONE *);                          //接收手机端通知车机端视频编码器初始化完成
    static void recvVideoEncoderFrameRateChangeDone(S_VIDEO_ENCODER_FRAME_RATE_CHANGE_DONE *);  //接收手机端通知车机端视频编码器帧率调整完成

    static void recvCarDataSubscribe(S_VEHICLE_INFO_LIST *);                                    //接收手机端通知车机端需要订阅的车身信息
    static void recvCarDataSubscribeStart(S_VEHICLE_INFO_LIST *);                               //接收手机端通知车机端开始发送订阅信息
    static void recvCarDataSubscribeStop(S_VEHICLE_INFO_LIST *);                                //接收手机端通知车机端停止发送车身订阅信息
    static void recvCarLifeDataSubscribeDone(S_SUBSCRIBE_MOBILE_CARLIFE_INFO_LIST *);           //接收移动设备针对车机订阅CarLife相关数据的反馈信息

    static void recvNaviNextTurnInfo(S_NAVI_NEXT_TURN_INFO *);                                  //接收手机端通知车机端导航在路况转向的信息
    static void recvNaviAssistantGuideInfo(S_NAVI_ASSITANT_GUIDE_INFO *);                       //接收移动设备发送给车机的导航辅助诱导信息


//    static void recvMDBTPairInfo(S_BT_PAIR_INFO *);                         //接收手机端蓝牙匹配信息
    static void recvMDBTPairInfo(S_MD_BT_PAIR_INFO *);                         //接收手机端蓝牙匹配信息
    //    static void recvMDBTOOBInfro(S_MD_BT_OOB_INFO *);                 //接收手机端发送给车机端的手机蓝牙信息
    static void recvStartBtAutoPairRequest(S_BT_START_PAIR_REQ *);          //手机端发送给车机，断开已经存在的HFP连接，并开始蓝牙自动匹配的流程。
    //如果是IOS则不需要开始自动匹配流程
    static void recvBTIdentifyResultInd(S_BT_INDENTIFY_RESULT_IND *);       //手机发送给车机，告诉车机 蓝牙标识的结果，同时当标识状态发生变化时，通知车机
    static void recvBtHfpRequest(S_BT_HFP_REQUEST *);                       //车机发送给移动设备，用来告诉移动设备蓝牙电话的状态信息。其中需要指明该状态是来自于那个蓝牙设备
    static void recvBTHfpStatusRequest(S_BT_HFP_STATUS_REQUEST *);          //手机发送该消息来获取指定的状态，目前主要用来获取车机MIC的状态

    static void recvTelStateChangeIdle();           //接收手机端通知车机端电话空闲
    static void recvTelStateChangeIncoming();       //接收手机端通知车机端有电话接入
    static void recvTelStateChangeOutGoing();       //接收手机端通知车机端有电话打出
    static void recvTelStateChangeInCalling();      //接收手机端通知车机端手机处于通话中

    static void recvScreenOn();                     //接收手机端通知车机端手机屏幕点亮
    static void recvScreenOff();                    //接收手机端通知车机端手机屏幕关闭
    static void recvScreenUserPresent();            //接收手机端通知车机端手机屏幕解锁

    static void recvForeground();                   //接收手机端通知车机端手机CarLife处于前台
    static void recvBackground();                   //接收手机端通知车机端手机CarLife处于后台
    static void recvGoToDesktop();                  //接收手机端通知车机端手机恢复到主界面
    static void recvGotoForgroundResponse();        //接收carlife切换到前台的应答
    //车机端发送MSG_CMD_GO_TO_FOREGROUND消息给移动设备端将CarLife切回到前台以后，
    //移动设备端如果在5秒内没有切换成功则发送该条消息
    static void recvRequestGoToForeground();        //手机端通知车机端将手机端CarLife提升到前台运行

    static void recvMicRecordWakeupStart();         //接收手机端通知车机端手机开始VR的唤醒录音
    static void recvMicRecordEnd();                 //接收手机端通知车机端手机结束VR的录音
    static void recvMicRecordRecogStart();          //接收手机端通知车机端手机开始VR的识别后录音

    static void recvMediaInfo(S_MEDIA_INFO *);                  //接收手机端通知车机端当前的媒体播放信息
    static void recvMediaProgressBar(S_MEDIA_PROGRESS_BAR *);   //接收手机端通知车机端媒体播放进度条信息

    static void recvUIActionSound();                            //接收手机端通知车机端发出焦点点中通知声

    static void recvConnectException(S_CONNECTION_EXCEPTION *); //接收手机端通知车机端连接异常信息
    static void cmdMdExit();

private:
    static CmdChannel *mInstance;

    static void ReadBTInfo(string& addr, string& pin);

    void (*m_music_status_callback)(int, void*);

    void (*m_cmd_status_callback)(int, void*);

    void (*m_cmd_phonenumber_callback)(string ,void*);

    void (*m_cmd_mediainfo_callback)(string,string,string, void*);

    void *m_parameter;

    static bool m_bCall;
    static bool m_bBTConnect;
    static bool  m_bt_unmanaged;
    string mAddress;
    string mPin;

};

#endif // CMDCHANNEL_H
