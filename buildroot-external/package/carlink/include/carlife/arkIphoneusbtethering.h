#ifndef __ARK_IPHONE_USB_TETHERING_H_H
#define __ARK_IPHONE_USB_TETHERING_H_H
#include "thread.h"
#include "util.h"

typedef struct _iphdr //定义IP首部
{
    unsigned char h_verlen; //4位首部长度+4位IP版本号
    unsigned char tos; //8位服务类型TOS
    unsigned short total_len; //16位总长度（字节）
    unsigned short ident; //16位标识
    unsigned short frag_and_flags; //3位标志位
    unsigned char ttl; //8位生存时间 TTL
    unsigned char proto; //8位协议 (TCP, UDP 或其他)
    unsigned short checksum; //16位IP首部校验和
    unsigned int sourceIP; //32位源IP地址
    unsigned int destIP; //32位目的IP地址
}IP_HEADER;

class IUsbTetheringCallbacks {
public:
    virtual ~IUsbTetheringCallbacks() { }
    virtual void usbEtheringReadyCallback(char *ipaddr, void *parameter) = 0;
    virtual void notifyTrustUserCallback(bool bTrust, void *parameter) = 0;
    virtual void notifyOpenCalifeCallback(bool bOpen, void *parameter) = 0;
};


class ArkIphoneUsbTethering : public Thread
{    
public:
    explicit ArkIphoneUsbTethering();
    static ArkIphoneUsbTethering *instance();
    ~ArkIphoneUsbTethering();
    bool init();
    void uninit();
    void registerCallbacks(IUsbTetheringCallbacks *callbacks ,void *parameter) {
        mCallbacks = callbacks;
        m_parameter = parameter;
    }
    //void sendLinkStatus(const int type, const int status);

protected:
    void run();
    bool watchHandleIsExist();

    static ArkIphoneUsbTethering *mInstance;
    bool mRunning, mCarlifeIsAlive;
    int mQuitFd, mFd, mWd, mCarlifeHeartFd, mDNSFd;
    IUsbTetheringCallbacks *mCallbacks;
    void *m_parameter;
/*
signals:
    void sendSignal(int type, int status);
*/
};

#endif
