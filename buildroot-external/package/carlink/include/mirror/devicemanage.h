#ifndef DEVICEMANAGE_H
#define DEVICEMANAGE_H

#include "device.h"
#include <map>
#include <string>
#include <stdint.h>
using namespace std;
class DeviceManage
{

public:
    explicit DeviceManage();
    virtual ~DeviceManage();

    bool connectDevice(Device::DeviceParams params);
    bool disconnectDevice(const string &serial);


    //VideoDecode* getDeviceDecoder() const;
    Controller* getController();

    void SetVideoStartCallback(FUNCVIDEOSTART funcVideoStart);
    void SetVideoInfoCallback(FUNCVIDEOINFO funcVideoInfo);

    void setServerFinishedCallback(void (*callback)(void*), void* parameter);
    void setServerRemovedCallback(void  (*callback)(void*), void* parameter);
private:
    static void device_disconnect_callback_func(string serial, void* parameter);
    static void server_finished_callback_func(void* parameter);

    void onVideoStartCallback(bool start, int width, int height);
    void onVideoInfoCallback(int width, int height, unsigned char* data, int length);
    //VideoDecode *m_pVideoDecoder;
    FUNCVIDEOSTART  mFuncVideoStart;
    FUNCVIDEOINFO   mFuncVideoInfo;

    Controller *m_pController;

    void  (*m_sfcallback)(void *);
    void  (*m_srcallback)(void *);
    void   *m_parameter;

private:
    //QMap<string, Device* > m_devices;
   // map<string, Device*> m_devices;
    Device *mDevice;
    unsigned short m_localPortStart = 27183;
};

#endif // DEVICEMANAGE_H
