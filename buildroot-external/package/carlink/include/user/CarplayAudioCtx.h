#ifndef CARPLAYAUDIOCTX_H
#define CARPLAYAUDIOCTX_H
#include "Thread.h"
//#include "ICarplayAudioCallbacksImpl.h"
#include "CarplayLink.h"

class CarplayAudioRecordCtx;
class CarplayAudioPlayCtx;
class CarplayLink;
class CarplayAudioCtx
{
public:
    CarplayAudioCtx(CarplayLink *carplayLink, int handle, AudioStreamType type, int rate, int bits, int channels);
    ~CarplayAudioCtx();
    int getStreamHandle() {
        return mStreamHandle;
    }

private:

    CarplayAudioRecordCtx*      mRecordHandle;
    CarplayAudioPlayCtx*        mPlayHandle;
    int                         mStreamHandle;
};


#endif // CARPLAYAUDIOCTX_H
