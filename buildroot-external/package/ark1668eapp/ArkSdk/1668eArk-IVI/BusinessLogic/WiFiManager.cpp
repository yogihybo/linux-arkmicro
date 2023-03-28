#include "WiFiManager.h"
#include "BusinessLogic/Setting.h"
#include "Utility.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <QDebug>
#include <arpa/inet.h>  //for in_addr
#include <linux/rtnetlink.h>    //for rtnetlink
#include <net/if.h> //for IF_NAMESIZ, route_info
#include <stdlib.h> //for malloc(), free()
#include <string.h> //for strstr(), memset()
#include <string>
#include <QFile>
#include <pthread.h>
#include <QTcpSocket>
#include <QTime>
#define BUFSIZE 8192
using namespace std;
struct route_info{
 u_int dstAddr;
 u_int srcAddr;
 u_int gateWay;
 char ifName[IF_NAMESIZE];
};

#define MAX_PATH 128

static const char *SCAN             = "wpa_cli -i wlan0 scan";
static const char *SCAN_RES         = "wpa_cli -i wlan0 scan_result";
static const char *ADD_NETWORK      = "wpa_cli -i wlan0 add_network";
static const char *SELECT_NETWORK   = "wpa_cli -i wlan0 select_network";
static const char *STATUS           = "wpa_cli -i wlan0 status";
static const char *SET_SSID         = "wpa_cli -i wlan0 set_network ";
static const char *SET_PASSWORD     = "wpa_cli -i wlan0 set_network ";
static const char *ENABLE_NETWORK   = "wpa_cli -i wlan0 enable_network ";
static const char *SAVE_CONNET_INFO = "wpa_cli -i wlan0 save_config";
static const char *UDHCPC_NETWORK   = "udhcpc -b -i wlan0 -s /etc/udhcpc.script &\n";
static const char *CHECK_HOTSPOTSTATUS =  "cat /sys/class/net/wlan0/operstate";
static const char *CHECK_STASTATUS  = "wpa_cli status";
static const char *DISABLE_NETWORK  = "wpa_cli -i wlan0 disable_network ";
static const char *REMOVE_NETWORK   = "wpa_cli -i wlan0 remove_network ";

class WiFiManagerPrivate
{
    Q_DISABLE_COPY(WiFiManagerPrivate)
public:
    explicit WiFiManagerPrivate(WiFiManager *parent);
    ~WiFiManagerPrivate();
    int  m_WifiConnectStatus;
    void autoConncetSsid(QString ssid);
    void wifiConnectStatusChange(int status);
    void mySleep(int msec);
    int  isRunning(char *process);
    FILE *wifiFp;
    int   m_Pthread;
    int   m_NetId;
    QString m_ConnectedSsid;
private:
    Q_DECLARE_PUBLIC(WiFiManager)
    WiFiManager* const q_ptr;
};

static char* getPidFromStr(const char *str)
{
    static char sPID[8] = {0};
    int tmp = 0;
    int pos1 = 0;
    int pos2 = 0;
    int i = 0;
    int j = 0;

    for (i=0; i<strlen(str); i++) {
        if ( (tmp==0) && (str[i]>='0' && str[i]<='9') ) {
            tmp = 1;
            pos1 = i;
        }
        if ( (tmp==1) && (str[i]<'0' || str[i]>'9') ) {
            pos2 = i;
            break;
        }
    }
    for (j=0,i=pos1; i<pos2; i++,j++) {
        sPID[j] = str[i];
    }
    return sPID;
}



static void setNoblockRead(FILE *fstream)
{
    int fd;
    int flags;
    fd = fileno(fstream);
    flags = fcntl(fd,F_GETFL,0);
    flags |= O_NONBLOCK;
    fcntl(fd,F_SETFL,flags);
    return;
}

int WiFiManager::wifiScan()
{
    FILE *fp;
    char cmdresult[8]={0};
    if((fp = popen(SCAN,"r"))==NULL)
    {
        perror("SCAN");
        //exit(1);
        return -1;
    }
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("+++++scan0000%s+++++++++\n",cmdresult);
        if(strstr(cmdresult,"OK") != NULL)
        {
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}

void WiFiManager::wifiScanResult()
{
    Q_D(WiFiManager);
    FILE *fp;
    //int flags;
    char bssid[MAX_PATH];
    char frequency[MAX_PATH];
    char signal[MAX_PATH];
    char flag[MAX_PATH];
    char ssid[MAX_PATH];
    static int count=0;
    char cmdresult[MAX_PATH*2]; //设置一个合适的长度，以存储每一行输出

    if((fp=popen(SCAN_RES,"r"))==NULL)
    {
        perror("SCAN_RES");
        //exit(1);
        return;
    }
    setNoblockRead(fp);
    d->mySleep(1000);//这里必须要等待一下，否则fgets立即返回NULL,捕获不到数据
    fgets(cmdresult,sizeof(cmdresult),fp);  //过滤title行
    memset(cmdresult, 0, sizeof(cmdresult));
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL) //wifi扫描不到是否意味着就一直阻塞在这里？比如在没有WiFi的地方
    {
        printf("cmdresult: %s\n",cmdresult);
        memset(bssid, 0, sizeof(bssid));
        memset(frequency, 0, sizeof(frequency));
        memset(signal, 0, sizeof(signal));
        memset(flag, 0, sizeof(flag));
        memset(ssid, 0, sizeof(ssid));
        //sscanf(cmdresult, "%s\t%s\t%s\t%s\t%s\n", bssid, frequency, signal, flag, ssid);
        sscanf(cmdresult, "%s\t%s\t%s\t%s\n", bssid, frequency, signal, flag);
        int offset = strlen(bssid) + strlen(frequency) + strlen(signal) + strlen(flag) + 3;
        if(sizeof(cmdresult) < offset) {
            printf("### Invalid offset:%d\n", offset);
            continue;
        }
        char *tmp = cmdresult + offset;
        if(*(tmp) != '\t') {
            printf("### No char tab\n");
            continue;
        }
        tmp += 1;
        offset ++;
        if(strlen(cmdresult) > offset) {
            memcpy(ssid, tmp, strlen(cmdresult) - offset);
            if (ssid[strlen(cmdresult) - offset -1] == '\n') {
                if (ssid[strlen(cmdresult) - offset -2] == '\r') {
                    ssid[strlen(cmdresult) - offset -2] = '\0';
                }
                ssid[strlen(cmdresult) - offset -1] = '\0';
            }
        } else {
            //printf("### Invalid cmdresult len:%d\n", strlen(cmdresult));
            continue;
        }

        //printf("#111 %c, ssid: %s, len=%d\n",*ssid, ssid, strlen(ssid));
        if (strlen(ssid)==0) {
            continue;
        }
        //printf("ssid: %s\n",ssid);
        onWifiSSIDInfoChange(ssid);
    }
    if (strlen(cmdresult) == 0) {
        count++;
        printf("========= count: %d\n",count);
        if (count > 70) {
            count = 0;
            onWifiGetResultNoWifi();
        } else {
            onWifiGetResultFail();
        }
        //qDebug("============ Send Signal: onWifiGetResultFail.");
    } else {
        count = 0;
        onWifiGetResultSuccess();
        //qDebug("============ Send Signal: onWifiGetResultSuccess.");
    }
    pclose(fp);
    return ;
}

int WiFiManager::wifiAddNetwork()
{
    Q_D(WiFiManager);
    FILE *fp;
    char net_id = 0;
    char cmdresult[8]={0};
    if((fp = popen(ADD_NETWORK,"r"))==NULL)
    {
        perror("ADD_NETWORK");
        //exit(1);
        return -1;
    }

    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("++++++++wifiAddNetwork cmdresult %s\n",cmdresult);
        QString str = QString(QLatin1String(cmdresult));
        net_id = str.toInt();
    }
    pclose(fp);
    d->m_NetId = net_id;
    return net_id;
}

int WiFiManager::wifiSetSsid(int id, char *ssid_name)
{
    Q_D(WiFiManager);
    FILE *fp;
    d->m_ConnectedSsid = QString::fromUtf8(ssid_name);
    char cmdresult[128]={0};
    char cmd[128]="";
    strcat(cmd,SET_SSID);
    QString _Qstr = QString::number(id);
    std::string str = _Qstr.toStdString();
    strcat(cmd,str.c_str());
    strcat(cmd, " ssid");
    strcat(cmd," '\"");
    strcat(cmd,ssid_name);
    strcat(cmd,"\"'");
    printf("%s\n",cmd);
    if((fp=popen(cmd,"r"))==NULL)
    {
        perror("set ssid failed");
        //exit(1);
    }
    setNoblockRead(fp); //no block read
    sleep(1);
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("++++++++++++wifiSetSsid++++++%s\n",cmdresult);
        if(strstr(cmdresult,"OK")!=NULL)
        {
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}

int WiFiManager::wifiSetPassword(int id, char *password)
{
    FILE *fp;
    int ret = -1;
    char cmdresult[128]={0};
    char cmd[128]="";
    strcat(cmd,SET_PASSWORD);
    QString _Qstr = QString::number(id);
    std::string str = _Qstr.toStdString();
    strcat(cmd,str.c_str());
    strcat(cmd, " psk");
    strcat(cmd," '\"");
    strcat(cmd,password);
    strcat(cmd,"\"'");
    printf("%s\n",cmd);
    if((fp=popen(cmd,"r"))==NULL)
    {
        perror("set password failed");
        //exit(1);
        return -1;
    }
    setNoblockRead(fp); //no block read
    sleep(1);
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("++++++++++++wifiSetPassword%s+++++++\n",cmdresult);
        if (strstr(cmdresult,"OK")!=NULL) {
            ret = 1;
            break;
        } else if (strstr(cmdresult,"FAIL")!=NULL) {
            ret = 0;
            break;
        }
    }
    pclose(fp);
    return ret;
}

int WiFiManager::wifiEnableNetwork(int id)
{
    FILE *fp;
    char cmdresult[128]={0};
    char cmd[128]="";

    strcat(cmd,ENABLE_NETWORK);
    QString _Qstr = QString::number(id);
    std::string str = _Qstr.toStdString();
    strcat(cmd,str.c_str());
    printf("%s\n",cmd);
    if((fp=popen(cmd,"r")) == NULL) {
        perror("ENABLE_NETWORK");
        //exit(1);
        return -1;
    }
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL) {
        printf("+++++++++++++wifiEnableNetwork+++++++++%s\n",cmdresult);
        if(strstr(cmdresult,"OK")!=NULL) {
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}

int WiFiManager::checkSsidExist(QString filePath, QString ssid)
{
    int ret = -1;
    QFile file(filePath);
    QString readData;

    if (ssid.isEmpty()) {
        return 1;
    }
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Open" << filePath << "success.";
        while (!file.atEnd()) {
            readData = file.readLine();
            qDebug() << "Read data:" << readData;
            if (readData.contains(ssid)) {
                int start = readData.indexOf(ssid);
                int end = readData.indexOf('\n');
                QString temp = readData.mid(start, end -1-start);
                if (temp == ssid) {
                    qDebug() << "ssid has existed." << temp;
                    ret = 1;
                }
            }
        }
        file.close();
    } else {
        qDebug() << "Open" << filePath << "fail.";
    }
    return ret;
}

int WiFiManager::wifiSaveConfig()
{
    FILE *fp;
    char cmdresult[8]={0};
    if((fp = popen(SAVE_CONNET_INFO,"r"))==NULL)
    {
        perror("SAVE_CONNET_INFO");
        //exit(1);
        return -1;
    }
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("++++++++wifiSaveConfig000:%s\n",cmdresult);
        if(strstr(cmdresult,"OK")!=NULL)
        {
            printf("close fp and return\n");
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}

int WiFiManager::disableNetWork(int id)
{
    FILE *fp;
    char cmdresult[8]={0};
    char cmd[128]="";
    strcat(cmd,DISABLE_NETWORK);
    QString _Qstr = QString::number(id);
    std::string str = _Qstr.toStdString();
    strcat(cmd,str.c_str());
    printf("%s\n",cmd);
    if((fp = popen(cmd,"r"))==NULL)
    {
        perror("DISABLE_NETWORK");
        //exit(1);
        return -1;
    }
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("++++++++disableNetWork0000:%s\n",cmdresult);
        if(strstr(cmdresult,"OK")!=NULL)
        {
            printf("close fp and return\n");
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}

int WiFiManager::removeNetWork(int id)
{
    FILE *fp;
    char cmdresult[8]={0};
    char cmd[128]="";
    strcat(cmd,REMOVE_NETWORK);
    QString _Qstr = QString::number(id);
    std::string str = _Qstr.toStdString();
    strcat(cmd,str.c_str());
    printf("%s\n",cmd);
    if((fp = popen(cmd,"r"))==NULL)
    {
        perror("REMOVE_NETWORK");
        //exit(1);
        return -1;
    }
    while(fgets(cmdresult,sizeof(cmdresult),fp)!=NULL)
    {
        printf("++++++++removeNetWork:%s\n",cmdresult);
        if(strstr(cmdresult,"OK")!=NULL)
        {
            printf("close fp and return\n");
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}


static int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do{
        //收到内核的应答
        if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
        {
            perror("SOCK READ: ");
            return -1;
        }
        nlHdr = (struct nlmsghdr *)bufPtr;
        //检查header是否有效
        if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
        {
            perror("Error in recieved packet");
            return -1;
        }
        if(nlHdr->nlmsg_type == NLMSG_DONE)
        {
            break;
        }
        else
        {
            bufPtr += readLen;
            msgLen += readLen;
        }
        if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
        {
            break;
        }
    } while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

    return msgLen;

}

//分析返回的路由信息
static void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo,char *gateway)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;
    char *tempBuf = NULL;
    struct in_addr dst;
    struct in_addr gate;

    tempBuf = (char *)malloc(100);
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

    // If the route is not for AF_INET or does not belong to main routing table
    //then return.
    if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
        return;

    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);
    for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)){
        switch(rtAttr->rta_type) {
            case RTA_OIF:
            if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
            break;
            case RTA_GATEWAY:
            rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
            break;
            case RTA_PREFSRC:
            rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
            break;
            case RTA_DST:
            rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
            break;
        }
    }

    dst.s_addr = rtInfo->dstAddr;

    if (strstr((char *)inet_ntoa(dst), "0.0.0.0"))
    {
        printf("oif:%s",rtInfo->ifName);
        gate.s_addr = rtInfo->gateWay;
        sprintf(gateway, (char *)inet_ntoa(gate));
        printf("%s\n",gateway);
        gate.s_addr = rtInfo->srcAddr;
        printf("src:%s\n",(char *)inet_ntoa(gate));
        gate.s_addr = rtInfo->dstAddr;
        printf("dst:%s\n",(char *)inet_ntoa(gate));
    }
    free(tempBuf);
    return;
}


int WiFiManager::wifiGetGateway(char *gateway)
{
    struct nlmsghdr *nlMsg;
    struct rtmsg *rtMsg;
    struct route_info *rtInfo;
    char msgBuf[BUFSIZE];
    int sock, len, msgSeq = 0;

    if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        perror("Socket Creation: ");
        return -1;
    }
    memset(msgBuf, 0, BUFSIZE);
    nlMsg = (struct nlmsghdr *)msgBuf;
    rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.
    if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0){
        printf("Write To Socket Failed…\n");
        return -1;
    }
    if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
        printf("Read From Socket Failed…\n");
        return -1;
    }
    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

    for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len))
    {
        memset(rtInfo, 0, sizeof(struct route_info));
        parseRoutes(nlMsg, rtInfo,gateway);
    }
    free(rtInfo);
    close(sock);
    return 0;
}

static void* standardOutputWifiConnectSta(void* arg)
{
    //Q_D(WiFiManager);
    WiFiManagerPrivate* pThis = (WiFiManagerPrivate*)arg;
    char cmdresult[MAX_PATH];
    const QString ConectSuccesss = "wlan0: CTRL-EVENT-CONNECTED";
    const QString ConnctFail = "wlan0: CTRL-EVENT-DISCONNECTED";
    const QString Conncetting = "wlan0: CTRL-EVENT-SUBNET-STATUS-UPDATE";
    const QString PasswdIncorrect = "pre-shared key may be incorrect";
    while(1)
    {
        if(pThis->wifiFp == NULL)
        {
            qDebug()<<"+++++++++pThis->wifiFp == NUL++++++++++";
            break;
        }
        setNoblockRead(pThis->wifiFp);
        usleep(50*1000);  //这里必须要等待一下，否则fgets立即返回NULL,捕获不到数据
        memset(cmdresult, 0, sizeof(cmdresult));
        if(fgets(cmdresult,sizeof(cmdresult),pThis->wifiFp) != NULL)
        {
              qDebug()<<"+++++1111xxxx+++++";
              QString output = QString::fromLocal8Bit(cmdresult,MAX_PATH);
              qDebug() << "===========000========= output" << output;
              if (output.contains("Trying to associate with") && output.contains("SSID")) {
                  int start = output.indexOf("\'");
                  int end = output.indexOf("\'", start+1);
                  QString ssid = output.mid(start+1, end - (start+1));
                  qDebug() << "=========== Connected ssid:" << ssid;
                  pThis->autoConncetSsid(ssid);  
              } else if (output.contains(ConectSuccesss)) {
                  if (!pThis->isRunning("udhcpc")) {
                      g_Setting->backstageExecuteShellCmd(UDHCPC_NETWORK);
                  }
                  g_Setting->executeShellCmd("udhcpc -i wlan0");
                  pThis->m_WifiConnectStatus = WCS_Success;
                  pThis->wifiConnectStatusChange(pThis->m_WifiConnectStatus);
                  QTcpSocket *socket = new QTcpSocket();
                  socket->connectToHost("time.nist.gov", 13);
                  QString sysDataTime;
                  if (socket->waitForConnected())
                  {
                        if(socket->waitForReadyRead())
                        {
                           sysDataTime = socket->readAll();
                           sysDataTime = sysDataTime.trimmed();
                           sysDataTime = sysDataTime.section(" ", 1, 2);
                           qDebug() <<"================time:str000000000  :"<< sysDataTime;
                       }
                  }
                  socket->close();
                  QStringList sysDataTimeList = sysDataTime.split(" ");
                  if(sysDataTimeList.size() >= 2)
                  {
                      QStringList sysDataList = QString(sysDataTimeList.at(0)).split("-");
                      QStringList sysTimeList = QString(sysDataTimeList.at(1)).split(":");
                      int year  = QString(sysDataList.at(0)).toInt() + 2000;
                      int month = QString(sysDataList.at(1)).toInt();
                      int day   = QString(sysDataList.at(2)).toInt();
                      int hour  = QString(sysTimeList.at(0)).toInt()+8;
                      int min   = QString(sysTimeList.at(1)).toInt();
                      int second = QString(sysTimeList.at(2)).toInt();
                      setDateTime(year,
                                  month - 1,
                                  day,
                                  hour,
                                  min,
                                  second);
                  }
                  g_Setting->settingDataTime();
              } else if (output.contains(Conncetting)) {
                  pThis->m_WifiConnectStatus = WCS_Connectting;
                  pThis->wifiConnectStatusChange(pThis->m_WifiConnectStatus);
              } else if (output.contains(ConnctFail)) {
                  pThis->m_WifiConnectStatus = WCS_DisConnect;
                  pThis->wifiConnectStatusChange(pThis->m_WifiConnectStatus);
              } else if (output.contains(PasswdIncorrect)) {
                  pThis->m_WifiConnectStatus = WCS_IleaglPasswd;
                  pThis->wifiConnectStatusChange(pThis->m_WifiConnectStatus);
                  g_WiFiManager->disableNetWork(pThis->m_NetId);
                  g_WiFiManager->removeNetWork(pThis->m_NetId);
                  g_Setting->mySleep(1000);
                  g_WiFiManager->onReInputPassword();
              }
              else{
                  usleep(200*1000);
              }
        }
        if(pThis->m_Pthread == 0)
        {
            qDebug()<<"++++++++m_Pthread=00000+++++++";
            pclose(pThis->wifiFp);
            pThis->m_Pthread = 1;
            break;
        }
    }
    return NULL;
}



void WiFiManager::startSTAThread()
{
    Q_D(WiFiManager);
    qDebug()<<"=======startSTAThread===start=======";
    if(d->m_Pthread ==0)
    {
        qDebug()<<"+++[WiFiManager::startSTAThread():m_Pthread = 0]+++";
        pclose(d->wifiFp);
        d->m_Pthread = 1;
    }
    const char *WIFI  = "wpa_supplicant -Dnl80211 -c /etc/wpa_supplicant/wpa_supplicant.conf -i wlan0 &\n";
    d->wifiFp = popen(WIFI,"r");
    if(d->wifiFp == NULL)
    {
        perror("WIFI");
        pclose(d->wifiFp);
        return;
    }
    pthread_t pthead;
    int ret = pthread_create(&pthead, NULL,standardOutputWifiConnectSta,d);
    if(ret != 0) {
        printf("pthread_create failed!\n");
    }

    qDebug()<<"=======startSTAThread===end=======";
}

void WiFiManager::exitPthread()
{
    Q_D(WiFiManager);
    d->m_Pthread = 0;
}
int  WiFiManager::getWifiStatus()
{
    Q_D(WiFiManager);
    return d->m_WifiConnectStatus;
}
WiFiManager::WiFiManager(QObject *parent)
    : QObject(parent),
    d_ptr(new WiFiManagerPrivate(this))
{

}

WiFiManager::~WiFiManager()
{

}

void WiFiManager::initializePrivate()
{

}


WiFiManagerPrivate::WiFiManagerPrivate(WiFiManager *parent)
    : q_ptr(parent)
{

    wifiFp = NULL;
    m_WifiConnectStatus = WCS_Undefine;
    m_Pthread = 1;
    m_NetId   = 0;
}

WiFiManagerPrivate::~WiFiManagerPrivate()
{

}
void WiFiManagerPrivate::autoConncetSsid(QString ssid){
    Q_Q(WiFiManager);
    emit q->onAutoConncetSsid(ssid);
}
void WiFiManagerPrivate::wifiConnectStatusChange(const int status){
    Q_Q(WiFiManager);
    qDebug()<<"==========wifiConnectStatusChange========="<<status;
    if(status == WCS_Success)
    {
        if (-1 == q->checkSsidExist(FILEPATH, m_ConnectedSsid))
        {
             q->wifiSaveConfig();
        }
    }
    emit q->onWifiConnectStatusChange(status);
}

int WiFiManagerPrivate::isRunning(char *process)
{
    int ret = 0;
    char sCurrPid[16] = {0};
    sprintf(sCurrPid, "%d", getpid());

    FILE *fstream=NULL;
    char buff[1024] = {0};

    char cmd[128]="";
    strcat(cmd,"ps -e -o pid,comm | grep ");
    strcat(cmd, process);
    strcat(cmd, " | grep -v PID | grep -v grep");
    //printf("=== isRunning cmd: %s\n",cmd);
    if(NULL==(fstream=popen(cmd, "r")))
    {
        printf("execute command failed:");
        return -1;
    }
    while(NULL!=fgets(buff, sizeof(buff), fstream)) {
        char *oldPID = getPidFromStr(buff);
        if ( strcmp(sCurrPid, oldPID) != 0 ) {
            printf("Runing，PID=%s\n", oldPID);
            ret = 1;
        }
    }
    pclose(fstream);

    return ret;
}
void WiFiManagerPrivate::mySleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while(QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
