#ifndef CARPLAYLINKCBSIMPL_H
#define CARPLAYLINKCBSIMPL_H

#include "carplayWrapper.h"

class CarplayLink;
class CarplayLinkCbsImpl : public ICarplayCallbacks
{
public:
    CarplayLinkCbsImpl(CarplayLink *handle) : mHandle(handle) {
        mEntityScreen = 2;
        mEntityPhoneCall = -1;
        mSpeechMode = 0;
        mEntityAudio = 0;
        mEntityByTurn = 0;
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    ~CarplayLinkCbsImpl() {
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    void iap2LinkStatus(int status);
    int iap2WriteData(char *buf, int len);
    void carplaySessionStart();
    void carplaySessionStop();
    int switchUsbModeCB(UsbMode mode);
    void appleTimeUpdateCB(long long time, int zone_offset);
    void appleLanguageUpdateCB(const char *lang);
    void NotifyDeviceNameCB(const char *name, int name_len);
    void carplayExitCB();
    void returnNativeUICB();
    void modesChangeCB(CarPlayModeState *modes);
    void disableBluetoothCB();
    void caplayDuckAudioCB(double inDurationSecs, double inVolume);
    void caplayUnduckAudioCB(double inDurationSecs);
private:
    CarplayLink* mHandle;
    CarplayEntity mEntityScreen;
    CarplayEntity mEntityPhoneCall;
    CarplayEntity mSpeechMode;
    CarplayEntity mEntityAudio;
    CarplayEntity mEntityByTurn;
};
#endif // CARPLAYLINKCBSIMPL_H
