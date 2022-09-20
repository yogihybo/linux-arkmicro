#ifndef _WEBRTC_WRAPPER_H__
#define _WEBRTC_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif
#ifdef AEC_DELAY
//audio denoise.
extern void *GetInstance();
extern void ReleaseInstance(void *handle);
extern void SetFrameParam(void *handle, int rate, int step, int chans);
extern void FrameProcess(void *handle, short *data, int len);

//audio echo cancellation.
extern void *WebRtcAecInit(void);
extern void WebRtcAecRelease(void *AecHandle);
extern void SetWebRtcAecParam(void *AecHandle, int sampFreq, int scSampFreq, void *priv);
extern void WebRtcAecFrameProcess(void *AecHandle, short *far_frame, short *near_frame, short *out_frame, int delay_ms);
#endif
#ifdef __cplusplus
};
#endif

#endif
