#ifndef AOAMODE_H
#define AOAMODE_H
#include "libusb.h"

class AOAMode
{
public:
    AOAMode();

    static bool Initialize();
    static int Initialize(int vid,libusb_device *device);
    static void UnInitialize();

    static bool StartAOAMode();

    static int SendCommand(libusb_device_handle* handle, const char* str, int index);

    static bool init(struct libusb_device* device, libusb_device_descriptor *desc);

private:
    static int parseInterfaces(libusb_device* dev, uint8_t* ifnum, uint8_t* readEndpoint, uint8_t* writeEndpoint);

    static libusb_context* mContext;
    static libusb_device_handle* mHandle;
    static uint8_t mInterface;
    static uint8_t mReadEndpoint;
    static uint8_t mWriteEndpoint;

};

#endif // AOAMODE_H
