#ifndef USBSTATEWATCHER_H
#define USBSTATEWATCHER_H
#include "Thread.h"

typedef enum __USB_MODE
{
 UNDEFINED = 0,
 HOST,
 PERIPHERAL,
 OTG
}USB_MODE;

class UsbStateWatcher : public ArkThread
{

public:
    UsbStateWatcher();
    ~UsbStateWatcher();

protected:
    void run();
    void parse_event(const char *msg);

private:
    int                 mQuitFd;
    bool                mStart;
};

#endif // USBSTATEWATCHER_H
