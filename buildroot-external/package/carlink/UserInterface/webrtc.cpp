#include <string>
#include <iostream>

#include "webrtc.h"

Webrtc::Webrtc()
{
#ifdef AEC_DELAY
	apm = webrtc::AudioProcessing::Create();
	apm->noise_suppression()->Enable(true);
	apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kHigh);
    apm->gain_control()->Enable(true);
    apm->gain_control()->set_analog_level_limits(0, 255);
	apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
	frame = new webrtc::AudioFrame();
#endif
}

Webrtc::~Webrtc()
{
#ifdef AEC_DELAY
    delete frame;
    delete apm;
#endif
}

#ifdef AEC_DELAY
void Webrtc::SetFrameParam(int rate, int step, int chans)
{
    frame->sample_rate_hz_ = rate;
    frame->samples_per_channel_ = rate * step / 1000.0;
    frame->num_channels_ = chans;
}
 
void Webrtc::FrameProcess(short *data, int len)
{
	memcpy(frame->data_, data, len);
	apm->ProcessStream(frame);
	memcpy(data, frame->data_, len);
}

#endif
