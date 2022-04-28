#ifndef USBHOSTSERVICE_H
#define USBHOSTSERVICE_H
#include <stdint.h>
#include <memory.h>


class LinkAssist;
class UsbHostServicePrivate;
class UsbHostService
{
public:
    UsbHostService();
    ~UsbHostService();

    void addHotplugListener(LinkAssist *pLinkAssist);

    bool start();
    void stop();

private:
    bool mStart;
    UsbHostServicePrivate *mHandle;

};

#endif // USBHOSTSERVICE_H
