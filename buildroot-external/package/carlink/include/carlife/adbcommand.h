#ifndef ADBCOMMAND_H
#define ADBCOMMAND_H

#include "types.h"
//#include <QString>
//#include <QStringList>


class AdbCommand
{
public:
    //    AdbCommand();

    //    static bool initialize();

    static long getFileSize(const char *filePath);
    static bool isFileExists(const char *dir, const char *name);
    static bool pushFile(const char *src, const char *dest, int timeout = 1000);
    static bool mkdir(const char *dir);

    static bool waitForDevices();                   //等待设备连接
    static int isDeviceConnected();                //获取设备序列号
    static bool IsConnected();

    static bool forward(int sport, int dport);

    static bool getAndroidVer(char *ver);           //获取安卓设备系统版本
    static bool isVerLess5(const char *ver);        //安卓设备版本是否小于5.0

    static bool isBdimExists();                     //bdmi是否存在
    static bool isBdimJarExists();                  //bdmi.jar是否存在
    static bool isBdscExists();                     //bdsc是否存在
    static bool isCarLifeExists();                  //CarLife.apk是否在

    static bool pushBdim();                         //将bdmi推送的手机端
    static bool pushBdimJar();                      //将bdmi.jar推送的手机端
    static bool pushBdsc(const char *ver);          //将bdsc推送到手机端
    static bool pushCarLife();                      //将CarLife安装包推送到手机端

    static bool installCarLife();                   //安装CarLife.apk

    static bool isAppLaunch(const char *app);       //app是否已经运行
    static bool launchBdimShell(int retry = -1);    //执行bdim脚本
    static bool launchBdim(int retry = -1);         //启动bdim
    //    static bool launchBdimJar(int retry = -1);      //启动bdim.jar
    static bool launchBdsc(int retry = -1);         //启动bdsc

    static bool isCarlifeAppInstalled();            //手机端是否已安装carlife
    static bool activateCarlifeApp();               //激活（启动）carlife app


    //private:
    //    static char version[32];
};



#endif // ADBCOMMAND_H
