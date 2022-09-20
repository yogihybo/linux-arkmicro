#ifndef __WEBRTC_H__
#define __WEBRTC_H__
#ifdef AEC_DELAY
#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/interface/module_common_types.h"
#endif

class Webrtc
{
public:
    Webrtc();
	~Webrtc();
#ifdef AEC_DELAY
	void SetFrameParam(int rate, int step, int chans);
	void FrameProcess(short *data, int len);
    
private:
    webrtc::AudioProcessing* apm;
	webrtc::AudioFrame *frame;
#endif
};
#endif 
