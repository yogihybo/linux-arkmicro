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
    bool Read(uint8_t* buf, int sizein);
    bool Write(const uint8_t* buf, int sizein);
private:
    bool mStart;
    UsbHostServicePrivate *mHandle;

};

#endif // USBHOSTSERVICE_H
