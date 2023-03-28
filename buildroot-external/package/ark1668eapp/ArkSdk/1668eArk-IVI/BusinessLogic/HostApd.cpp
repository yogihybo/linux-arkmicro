#include "HostApd.h"
#include "BusinessLogic/Setting.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <QDebug>
class HostApdPrivate
{
    Q_DISABLE_COPY(HostApdPrivate)
public:
    explicit HostApdPrivate(HostApd* parent);
    ~HostApdPrivate();
    vector<string> stringSplit(string str, string pattern);
    void initHostApd();
public:
    int m_p2pStatus;
    int m_InitHostapdStatus;
    struct wpa_ctrl *ctrl_p2p;
private:
    Q_DECLARE_PUBLIC(HostApd)
    HostApd* const q_ptr;
};

HostApd::HostApd(QObject *parent) :
    QObject(parent),
    d_ptr(new HostApdPrivate(this))
{

}

HostApd::~HostApd()
{

}
void HostApdPrivate::initHostApd(){
    qDebug()<<"+++[HostApd::initHostApd():]+++";
    //加载网卡驱动
    g_Setting->executeShellCmd("mv /dev/random /dev/random.orig");
    g_Setting->executeShellCmd("ln -s /dev/urandom /dev/random");
    g_Setting->executeShellCmd("rmmod rtl8821cs");
    usleep(50000);
    g_Setting->executeShellCmd("insmod /lib/modules/4.19.192/kernel/drivers/net/wireless/realtek/rtl8821cs/rtl8821cs.ko");
    g_Setting->executeShellCmd("mkdir -p /var/lib/misc/");
    g_Setting->executeShellCmd("touch /var/lib/misc/udhcpd.leases");
    g_Setting->executeShellCmd("ifconfig wlan0 up");
    g_Setting->executeShellCmd("ifconfig wlan0 192.168.2.1 netmask 255.255.255.0");
    g_Setting->executeShellCmd("echo 1 > /proc/sys/net/ipv4/ip_forward");
    g_Setting->executeShellCmd("echo 1 > /proc/sys/net/ipv6/conf/wlan0/disable_ipv6");
    g_Setting->executeShellCmd("echo 2097152 > /proc/sys/net/core/rmem_default");
    g_Setting->executeShellCmd("echo 2097152 > /proc/sys/net/core/rmem_max");
    g_Setting->executeShellCmd("echo 1048576 > /proc/sys/net/core/wmem_default");
    g_Setting->executeShellCmd("echo 1048576 > /proc/sys/net/core/wmem_max");
    g_Setting->executeShellCmd("echo 0 > /proc/sys/net/ipv4/tcp_timestamps");
    g_Setting->executeShellCmd("echo 1 > /proc/sys/net/ipv4/tcp_sack");
    g_Setting->executeShellCmd("echo 1 > /proc/sys/net/ipv4/tcp_fack");
    g_Setting->executeShellCmd("echo 1 > /proc/sys/net/ipv4/tcp_window_scaling");
    //Wlan1 for P2P Mode
    /*
    system("ifconfig wlan1 192.168.2.1 netmask 255.255.255.0");
    system("wpa_supplicant -Dnl80211 -i wlan1 -c /etc/p2p_supplicant.conf -B");
    system("wpa_cli -i wlan1 p2p_group_add persistent=0 freq=5");*/
   /* system("ifconfig wlan0 up");
    system("ifconfig wlan0 192.168.2.1 netmask 255.255.255.0");
    system("wpa_supplicant -Dnl80211 -i wlan0 -c /etc/p2p_supplicant.conf -B");
    system("wpa_cli -i wlan0 p2p_group_add persistent freq=5");*/
    m_InitHostapdStatus = 0;
    return ;

}
void HostApd::CreatThreadInitHostApd(){
    Q_D(HostApd);
    d->initHostApd();
}
int HostApd::getInitHostapdStatus(){
    Q_D(HostApd);
    return d->m_InitHostapdStatus;
}
void HostApd::setInitHostapdStatus(int status)
{
    Q_D(HostApd);
    d->m_InitHostapdStatus = status;
}
void HostApd::restartHostApd(){
    qDebug()<<"+++[HostApd::restartHostApd():]+++";
    g_Setting->executeShellCmd("killall udhcpd");
    g_Setting->executeShellCmd("killall hostapd");
    g_Setting->executeShellCmd("killall udhcpc");
    g_Setting->executeShellCmd("killall wpa_supplicant");
    g_Setting->executeShellCmd("ifconfig wlan0 down");
    g_Setting->mySleep(50);
    g_Setting->executeShellCmd("ifconfig wlan0 up");
    g_Setting->backstageExecuteShellCmd("udhcpd /etc/udhcpd.conf wlan0 &\n");
    g_Setting->executeShellCmd("hostapd -B /tmp/hostapd.conf");
    g_Setting->setWlanApStatus(1);
    g_Setting->setWifiConfig(0);
}

static void* RevP2PNetData(void *arg)
{
    FILE *fp;
    char buf[256] = {0};
    string dstStr("p2p_dev_addr");
    vector<string> strList;
    vector<string> dstStrList;
    string macAddr;
    string revData;
    string p2pPointName = "/var/run/wpa_supplicant/wlan0";
    HostApdPrivate* pthis = (HostApdPrivate*)arg;
    pthis->m_p2pStatus = 1;
    while (1)
    {
        if (access(p2pPointName.c_str(),F_OK) == 0)
        {
            pthis->ctrl_p2p = wpa_ctrl_open(p2pPointName.c_str());
            if (!pthis->ctrl_p2p)
            {
                qDebug()<<("[HostApd] P2P wpa_ctrl_open fail.\n");
                return NULL;
            }
            if (wpa_ctrl_attach(pthis->ctrl_p2p) != 0)
            {
                qDebug()<<("[HostApd] P2P wpa_ctrl_attach fail.\n");
                return NULL;
            }
            break;
        }
        else
        {
            usleep(100*1000);
        }
    }

    qDebug()<<("[HostApd] P2P wpa_ctrl_pending Enter.\n");

    while (1)
    {
        if(pthis->m_p2pStatus == 0)
        {
            qDebug()<<("[WiFiManager]++++++++++++++++++++++ m_p2pStatus:%d.\n",pthis->m_p2pStatus);
            pthis->m_p2pStatus = 1;
            while (1)
            {
                if (access(p2pPointName.c_str(),F_OK) == 0)
                {
                    pthis->ctrl_p2p = wpa_ctrl_open(p2pPointName.c_str());
                    if (!pthis->ctrl_p2p)
                    {
                        qDebug()<<("[HostApd] P2P wpa_ctrl_open fail.\n");
                        return NULL;
                    }
                    if (wpa_ctrl_attach(pthis->ctrl_p2p) != 0)
                    {
                        qDebug()<<("[HostApd] P2P wpa_ctrl_attach fail.\n");
                        return NULL;
                    }
                    break;
                }
                else
                {
                    usleep(100*1000);
                }
            }
        }
        if (wpa_ctrl_pending(pthis->ctrl_p2p))
        {
            qDebug()<<("[HostApd] P2P wpa_ctrl_pending\n");
            char buf[1024]={0};
            size_t len;
            len = sizeof(buf);
            wpa_ctrl_recv(pthis->ctrl_p2p, buf, &len);
            qDebug()<<("[HostApd] P2P wpa_ctrl_recv buf=%s\n", buf);
            revData = buf;
            strList = pthis->stringSplit(revData, string(" "));
            qDebug()<<("[HostApd] P2P wpa_ctrl_recv strList[0]=%s\n", string(strList[0]).c_str());
            if (strList[0] == string("<3>AP-STA-CONNECTED"))
            {
                qDebug()<<("[HostApd] P2P Phone Link Success.\n");
//                vector<pStaCallBack>::const_iterator iter = sStaCallBackList.begin();
//                for (; iter != sStaCallBackList.end(); iter++) {
//                    (*iter)(PT_Andriod, PTS_Connected);
//                }
            }
            else if (strList[0] == string("<3>P2P-PROV-DISC-PBC-REQ"))
            {
                for (auto str : strList)
                {
                    if (str.compare(0, dstStr.length(), dstStr) == 0)
                    {//Recv the Mac Address of Phone
                        dstStrList = pthis->stringSplit(str,"=");
                        macAddr = strList[1];
                        system("wpa_cli -i wlan0 wps_pbc");	//Connect the Phone P2P
                        break;
                    }
                }
            }
            else if (strList[0] == string("<3>AP-STA-DISCONNECTED"))
            {
                qDebug()<<("[HostApd] P2P Phone Disconnect.\n");
//                vector<pStaCallBack>::const_iterator iter = sStaCallBackList.begin();
//                for (; iter != sStaCallBackList.end(); iter++)
//                {
//                    (*iter)(PT_Andriod, PTS_DisConnect);
//                }
            }
        }
        usleep(1000*100);
    }
    wpa_ctrl_detach(pthis->ctrl_p2p);
    wpa_ctrl_close(pthis->ctrl_p2p);
    return NULL;

}
void HostApd::CreatThreadRevNetSta()
{
    Q_D(HostApd);
    int ret = -1;

    pthread_t net_pthead_p2p;
    ret = pthread_create(&net_pthead_p2p, NULL, RevP2PNetData, d);
    if(ret != 0)
    {
        qDebug()<<("[HostApd] net_pthead_p2p failed!\n");
    }
}


HostApdPrivate::HostApdPrivate(HostApd *parent)
    : q_ptr(parent)
{
       m_p2pStatus = 0;
       m_InitHostapdStatus = 1;
}

HostApdPrivate::~HostApdPrivate()
{

}

/**
 * 切割字符串
 */
vector<string> HostApdPrivate::stringSplit(string str, string pattern)
{
    string::size_type pos;
    vector<string> result;
    str += pattern;//扩展字符串以方便操作
    int size = str.size();
    for (int i = 0; i < size; i++) {
        pos = str.find(pattern, i);
        if (pos < size) {
            string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }
    return result;
}



