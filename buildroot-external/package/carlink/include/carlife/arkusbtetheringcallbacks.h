#ifndef ARKUSBTETHERINGCALLBACKS_H
#define ARKUSBTETHERINGCALLBACKS_H

#include "arkIphoneusbtethering.h"
class ArkUsbTetheringCallbacks : public IUsbTetheringCallbacks
{

public:
    ArkUsbTetheringCallbacks();

public:
    void usbEtheringReadyCallback(char *ipaddr, void *parameter);
    void notifyTrustUserCallback(bool bTrust, void *parameter);
    void notifyOpenCalifeCallback(bool bOpen, void *parameter);

    void *m_parameter;
};

#endif // ARKUSBTETHERINGCALLBACKS_H
