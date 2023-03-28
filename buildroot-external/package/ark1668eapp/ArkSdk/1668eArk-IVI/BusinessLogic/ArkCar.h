#ifndef ARKCAR_H
#define ARKCAR_H

enum ArkCarType {
    ACT_Undefine = -1,
    ACT_Version,
    ACT_Display,
    ACT_Reversing,
    ACT_Steering,
    ACT_Camera,
    ACT_Radar,
    ACT_Track,
    ACT_AssistTrack,
    ACT_Park,           //驻车
    ACT_AssistPark,     //辅助驻车
    ACT_ILLLight,
    ACT_Brake,
    ACT_Speed,
    ACT_Calibrate,
    ACT_MCUUpdate,
    ACT_Time,
    ACT_ACC,
    ACT_Can,
    ACT_Voltage,
    ACT_ReverseSignal,
    ACT_BackLight,      //背光(按键/显示屏)
    ACT_AirCondition,   //空调
    ACT_CarInfo,        //车身信息(安全带、手刹、尾箱、车门等状态;发动机转速、瞬时车速、电池电压、车外温度、剩余油量等；油量、电池电压等警告标志等)
    ACT_Source,         //源
    ACT_Icon,           //
};

#define ACS_ReversingStatus ACS_ReversingOn,    \
                            ACS_ReversingOff

#define ACS_ReverseSignal   ACS_DetectSignal,   \
                            ACS_NoDetectSignal


#define ACS_CameraStatus    ACS_CameraOriginal, \
                            ACS_CameraExtra,    \
                            ACS_Camera360

#define ACS_RadarStatus     ACS_RadarOn,        \
                            ACS_RadarOff,        \
                            ACS_RadarSoundOn,   /* 雷达声音ON */  \
                            ACS_RadarSoundOff,  /* 雷达声音OFF */ \
                            ACS_RadarRear,  /* 后雷达（包括后左后右） */ \
                            ACS_RadarRearLeft,   \
                            ACS_RadarRearRight,  \
                            ACS_RadarFront, /* 前雷达（包括前左前右） */ \
                            ACS_RadarFrontLeft,  \
                            ACS_RadarFrontRight

#define ACS_TrackStatus     ACS_TrackOn,    \
                            ACS_TrackOff

#define ACS_AssistTrackStatus     ACS_AssistTrackOn,  \
                                  ACS_AssistTrackOff

#define ACT_ParkStatus      ACT_ParkOn,     \
                            ACT_ParkOff,    \
                            ACT_ParkModeStandard, /* 标准驻车模式 */  \
                            ACT_ParkModeRoadside /* 路边驻车模式 */

#define ACT_AssistParkStatus        ACT_AssistParkOn,   \
                                    ACT_AssistParkOff

#define ACS_SteeringStatus  ACS_SteeringValue

#define ACS_ILLLightStatus  ACS_ILLLightOn, \
                            ACS_ILLLightOff

#define ACS_BrakeStatus     ACS_BrakeOn, \
                            ACS_BrakeOff

#define ACS_SpeedStatus           ACS_SpeedValue

#define ACS_CalibrateStatus     ACS_CalibrateOn,\
                                ACS_CalibrateOff

#define ACS_MCUUpdateStatus     ACS_MCUUpdateStart,\
                                ACS_MCUUpdateFileLen,\
                                ACS_MCUUpdateTransfer,\
                                ACS_MCUUpdatePercent,\
                                ACS_MCUUpdateTransferEnd,\
                                ACS_MCUUpdateSuccess,\
                                ACS_MCUUpdateFail,\
                                ACS_MCUUpdateRequest

#define ACS_ACCStatus           ACS_ACCOff,\
                                ACS_ACCOn

#define ACS_BackLightStatus     ACS_BackLightOff, \
                                ACS_BackLightOn, \
                                ACS_BackLightValue

#define ACS_AirConditionStatus  ACS_AirConditionState, /* 空调状态 */ \
                                ACS_AirConditionWindInfo, /* 风速和风向 */ \
                                ACS_AirConditionDriverZoneTemperature,  /* 驾驶位置温度 */ \
                                ACS_AirConditionCoDriverZoneTemperature, /* 副驾驶位置温度 */ \
                                ACS_AirConditionSeatHeating,  /*座椅加热 */ \
                                ACS_AirConditionInfo   /* 所有信息 */

#define ACS_CarInfoStatus       ACS_CarInfoValue

#define ACS_SourceStatus        ACS_SourceOff,      \
                                ACS_SourceTuner,    \
                                ACS_SourceDisc, /* CD,DVD */ \
                                ACS_SourceTv, /* Analog */ \
                                ACS_SourceNavi, \
                                ACS_SourcePhone, \
                                ACS_SourceIPod, \
                                ACS_SourceAux, \
                                ACS_SourceUsb, \
                                ACS_SourceSd, \
                                ACS_SourceDvb_t, \
                                ACS_SourcePhoneA2DP, \
                                ACS_SourceOther, \
                                ACS_SourceCdc /* v1.20 */

#define ACS_IconStatus          ACS_IconNoraml, \
                                ACS_IconScan, /* CD/DVD/TUNER */ \
                                ACS_IconMix, /* CD/DVD Only */ \
                                ACS_IconRpt /* CD/DVD Only */

enum ArkCarStatus {
    ACS_Undefine = -1,
    ACS_CarDisplay,
    ACS_NaviDisplay,
    ACS_ReversingStatus,
    ACS_CameraStatus,
    ACS_RadarStatus,
    ACS_TrackStatus,
    ACS_AssistTrackStatus,
    ACT_ParkStatus,
    ACT_AssistParkStatus,
    ACS_SteeringStatus,
    ACS_ILLLightStatus,
    ACS_BrakeStatus,
    ACS_SpeedStatus,
    ACS_CalibrateStatus,
    ACS_MCUUpdateStatus,
    ACS_ACCStatus,
    ACS_ReverseSignal,
    ACS_BackLightStatus,
    ACS_AirConditionStatus,
    ACS_CarInfoStatus,
    ACS_SourceStatus,
    ACS_IconStatus,
};

struct ArkCar {
    enum ArkCarType type;
    enum ArkCarStatus status;
    unsigned short int length;
    unsigned char* data;
};

struct CarDeviceState {
    enum ArkCarStatus reversing;
    enum ArkCarStatus reverse_detect;
    int illlight;
    int brake;
};

#endif //ARKCAR_H
