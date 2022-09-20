#include "WebrtcWrapper.h"
#include "webrtc.h"
 
#ifdef __cplusplus
extern "C" {
#endif

#ifdef AEC_DELAY
void *GetInstance()
{
    return new Webrtc();
}

void ReleaseInstance(void *handle)
{
	delete static_cast<Webrtc *>(handle);
}

void SetFrameParam(void *handle, int rate, int step, int chans)
{
	static_cast<Webrtc *>(handle)->SetFrameParam(rate, step, chans);
}

void FrameProcess(void *handle, short *data, int len)
{
	static_cast<Webrtc *>(handle)->FrameProcess(data, len);
}
#endif
#ifdef __cplusplus
};
#endif
