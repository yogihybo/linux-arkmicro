#ifndef USBHOSTSERVICE_H
#define USBHOSTSERVICE_H

#ifdef __cplusplus

#include <stdint.h>
#include <memory.h>
#include <functional>

using namespace std;
typedef function<void (int, int)> funcHotplug;

class LinkAssist;
class UsbHostServicePrivate;
class UsbHostService
{
public:
    UsbHostService();
    ~UsbHostService();

    void addHotplugListener(funcHotplug hotplug);
    bool start();
    void stop();

private:
    bool mStart;
    UsbHostServicePrivate *mHandle;

};

#endif

#endif // USBHOSTSERVICE_H
