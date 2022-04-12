#ifndef BLUETOOTHCONTROL_H
#define BLUETOOTHCONTROL_H

#include <string>
#include "types.h"

using namespace std;


class BlueToothControl
{
public:
    static BlueToothControl *instance();

    string addr() const
    {
        return mAddr;
    }

    string name() const
    {
        return mName;
    }

    string pinCode() const
    {
        return mPinCode;
    }

    void updateInfo(const char *addr, const char *name, const char *pin);

    void setMacAddr(const char *addr)
    {
        mAddr = addr;
    }
    void setMacAddr(const string addr)
    {
        mAddr = addr;
    }

    void setName(const char *name)
    {
        mName = name;
    }
    void setName(const string name)
    {
        mName = name;
    }

    void setPinCode(const char *pin)
    {
        mPinCode = pin;
    }
    void setPinCode(const string pin)
    {
        mPinCode = pin;
    }

    void setMobileAddr(const char *addr)
    {
        mMobileAddr = addr;
    }
    void setMobileAddr(const string addr)
    {
        mMobileAddr = addr;
    }

    void setMobileName(const char *name)
    {
        mMobileName = name;
    }
    void setMobileName(const string name)
    {
        mMobileName = name;
    }

    void setMobilePinCode(const char *pin)
    {
        mMobilePinCode = pin;
    }
    void setMobilePinCode(const string pin)
    {
        mMobilePinCode = pin;
    }

    //modify    void sendPairStatus(BtPairStatus status);
    void sendPairStatus(BtPairStatus status, int ispaired);
    void sendIndication(BtIndication state, const char *phoneNum);
    void sendHfpConnection(HfpConnection status);


/***************************** add 20180226******************/
    void setBTstatus(int status)
    {
        BTstatus = status;
    }

    int status() const
    {
        return BTstatus;
    }

    void setCurPhoneType(int phone)
    {
        phone_type = phone;
    }
     //告知手机：车机端可支持的特性
    void configMDBT(string addr, string pin, bool connected = true);
    void BTBeginIdentify();
    void setBTIdentifyResult(void* result);
    void setCurMDBTInfo(void*);
    //保存当前已连接蓝牙的地址，默认为空
    void setCurConnectBT(char* addr);
    //比对蓝牙地址:在车机已经连接蓝牙的情况下，需要判断车机连接的蓝牙是否是当前连接的手机的蓝牙
    int compareBTAddr(string local_get, string md);
private:
    string curConnectedBT;
    int BTstatus;
    int phone_type;     // 1:android; 2: iphone;
/*******************************************************************/
private:
    BlueToothControl();

private:
    static BlueToothControl *mInstance;

    string mAddr;
    string mName;
    string mPinCode;

    string mMobileAddr;
    string mMobileName;
    string mMobilePinCode;
};

#endif // BLUETOOTHCONTROL_H
