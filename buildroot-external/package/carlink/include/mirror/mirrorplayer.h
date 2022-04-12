#ifndef MIRRORPLAYER_H
#define MIRRORPLAYER_H

#include "devicemanage.h"
#include <semaphore.h>
class MirrorPlayer
{

public:
    MirrorPlayer();

    bool Initialize();

    void SetLinkstatusCallback(void (*callback)(int, void*), void* parameter);

    void SetVideoStartCallback(FUNCVIDEOSTART funcVideoStart);
    void SetVideoInfoCallback(FUNCVIDEOINFO funcVideoInfo);

    bool GetInserted() const {
        return m_bInserted;
    }

    bool GetConnected() const {
        return m_bConnected;
    }
    bool startServer();
    void setController(Controller *controller);
    void SetInsertPath(char *pBuffer);
    bool SetPlayForeground(bool bForeground);//前后台切换

    bool ExitLink();
    bool RestartLink();

    void Start();
    void Stop();
    void Key(int key);

    bool WirelessConnect(char *pAddress);

    void WirelessDisConnect(char *pAddress);

    void SetIPAddress(char *pAddress);

    void updateDevice();

    void getIP();

    void stopServer();

private:
    void onVideoStartCallback(bool start, int width, int height);
    void onVideoInfoCallback(int width, int height, unsigned char* data, int length);
    static void usb_insert_callback_func(bool insert, void* parameter);
    static void server_finished_callback_func(void* parameter);
    static void server_removed_callback_func(void* parameter);
    static void vertical_callback(bool bVertical);

private:
    DeviceManage m_deviceManage;
    void*        m_parameter;
    void         (*m_callback)(int, void *);
    string       m_serial="";
    //UsbManager*  m_pUsbManger;
    FUNCVIDEOSTART  mFuncVideoStart;
    FUNCVIDEOINFO   mFuncVideoInfo;
    bool         m_bInserted;
    char*        m_pAddress;
    //连接上的状态
    bool         m_bConnected;
    int          m_idVendor;
    sem_t        m_signalStart;
    Controller   *m_controller;
};

#endif // MIRRORPLAYER_H
