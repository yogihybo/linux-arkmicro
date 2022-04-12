#ifndef _CarplayAudioWrapper_H
#define _CarplayAudioWrapper_H

typedef enum
{
    AudioStreamMedia = 0, // 音乐
	AudioStreamCall,		// 电话
	AudioStreamRECOGNITION, // siri
    AudioStreamAlt,      // 辅助音,包括导航音,系统提示音
    AudioStreamRec,		 // 录音
    AudioStreamAlert
} AudioStreamType;

class ICarplayAudioCallbacks
{
public:
	/* *
     * @brief 音频流开始
     * @param 			handle 		该路音频流的操作句柄,要缓存起来,作为playStream/recordStream的第一个参数
     * @param			type  		音频流类型
     * @param 			rate		音频的采样率
     * @param 			bits		音频位宽
     * @param 			channels	音频通道数
     * */
    virtual void carplayAudioStartCB(int handle, AudioStreamType type, int rate, int bits, int channels) = 0;
    /* *
     * @brief 音频流停止
     * */
    virtual void carplayAudioStopCB(int handle, AudioStreamType type) = 0;
};


#endif
