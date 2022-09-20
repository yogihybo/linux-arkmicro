#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "IUserLinkPlayer.h"
#include <QMouseEvent>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include "UsbHostService.h"
#include <QPushButton>
#include <QLabel>
//#include <QWSServer>
#define FBPATH  "/dev/fb0"



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow),mChangeMode(false),mPlayer(nullptr),mIsRunningBackGround(false),mConnectStatus(CONNECT_STATUS_DISCONNECTED)
{
    ui->setupUi(this);
    system("ifconfig lo up");

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    int screen_width = 1920;
    int screen_height = 720;

    setGeometry(0, 0, screen_width, screen_height);


    m_CarplayLink = new QPushButton(QString("Carplay"), this);
    m_CarplayLink->setGeometry(width() / 6 * 0, 0, width() / 6, height());
    QObject::connect(m_CarplayLink, SIGNAL(clicked()),
                     this,          SLOT(onClicked()));


    m_AutoLink = new QPushButton(QString("Auto"), this);
    m_AutoLink->setGeometry(width() / 6 * 1, 0, width() / 6, height());
    QObject::connect(m_AutoLink, SIGNAL(clicked()),
                     this,         SLOT(onClicked()));


    m_CarlifeLink = new QPushButton(QString("Carlife"), this);
    m_CarlifeLink->setGeometry(width() / 6 * 2, 0, width() / 6, height());
    QObject::connect(m_CarlifeLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));


    m_HiCarLink = new QPushButton(QString("HiCar"), this);
    m_HiCarLink->setGeometry(width() / 6 * 3, 0, width() / 6, height());
    QObject::connect(m_HiCarLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));


    m_MirrorLink = new QPushButton(QString("Mirror"), this);
    m_MirrorLink->setGeometry(width() / 6 * 4, 0, width() / 6, height());
    QObject::connect(m_MirrorLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));

    m_EasyConnectLink = new QPushButton(QString("Easy Connect"), this);
    m_EasyConnectLink->setGeometry(width() / 6 * 5, 0, width() / 6, height());
    QObject::connect(m_EasyConnectLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));

    QObject::connect(this, SIGNAL(Change(bool)), this, SLOT(onUIChanged(bool)));


    mQRCodeWindow = new QRCodeWindow(this);
    mQRCodeWindow->hide();

    QObject::connect(this, SIGNAL(ChangeQRCode(bool)), this, SLOT(onChangeQRCode(bool)));


    QObject::connect(this, SIGNAL(QrcodeInfo(char*)), mQRCodeWindow, SLOT(onQrcodeInfo(char*)));

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    mpLinkAssist = new LinkAssist();
    mpLinkAssist->RegisterUsbCallback(std::bind(&MainWindow::usb_state,this, std::placeholders::_1,std::placeholders::_2));
    mPlayer = mpLinkAssist->Initialize(Carplay);
  //  Start(Mirror);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete mpLinkAssist;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("x:%d,y:%d\r\n",event->pos().x(),event->pos().y());
    if(mPlayer){
        mPlayer->SendTouch(event->pos().x(),event->pos().y(), Touch_Press);
 //       mPlayer->RequestStatus(QUERYTIME);
 //       string a  = "test";
 //       mPlayer->RequestStatus(QUERYINPUT, (void*)a.c_str());
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("x:%d,y:%d\r\n",event->pos().x(),event->pos().y());
    if(mPlayer)
        mPlayer->SendTouch(event->pos().x(),event->pos().y(), Touch_Up);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    printf("x:%d,y:%d\r\n",event->pos().x(),event->pos().y());
    if(mPlayer)
        mPlayer->SendTouch(event->pos().x(),event->pos().y(), Touch_Move);
}

void MainWindow::onClicked()
{
    const QPushButton* const ptr = static_cast<const QPushButton* const >(sender());
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mConnectStatus == CONNECT_STATUS_DEVICE_ATTACHED)
    {
        if (ptr == m_AutoLink) {
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_AUTO
            Start(Android_Auto, Wired);
#endif
        } else if (ptr == m_CarplayLink) {
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_CARPLAY
            Start(Carplay, Wired);
#endif
        } else if (ptr == m_CarlifeLink) {
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_CARLIFE
            Start(Carlife, Wired);
#endif
        } else if(ptr == m_HiCarLink){
#ifdef USE_HICAR
            Start(HiCar, Wired);
#endif
        }
        else if(ptr == m_MirrorLink){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_MIRROR
            Start(Mirror,Wired);
#endif
        } else if(ptr == m_EasyConnectLink){
#ifdef USE_EASYCONNECT
            Start(ECLink, Wired);
#endif
        }
    }
    else if(mConnectStatus == CONNECT_STATUS_CONNECT_SUCCEED){

        if(mIsRunningBackGround){
            mPlayer->SetForeground();
            emit Change(false);
         }
    }
    else if(mConnectStatus == CONNECT_STATUS_DISCONNECTED
            || mConnectStatus == CONNECT_STATUS_DEVICE_DEATTACHED
            || mConnectStatus == CONNECT_STATUS_APP_EXIT
            || mConnectStatus == CONNECT_STATUS_CONNECT_FAILED
            || mConnectStatus == CONNECT_STATUS_INTERRUPTED_BY_APP)
    {
        if(ptr == m_EasyConnectLink){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
#ifdef USE_EASYCONNECT
/*            system("insmod /lib/modules/4.19.192/kernel/drivers/net/wireless/realtek/rtl8821cs/rtl8821cs.ko");
            system("ifconfig wlan1 up");
            system("ifconfig wlan1 192.168.2.1");
            system("udhcpd /etc/udhcpd_p2p.conf");
            system("wpa_supplicant -Dnl80211 -iwlan1 -c /etc/p2p_supplicant.conf  -B");
            system("wpa_cli -i wlan1 p2p_group_add persistent");
            system("wpa_cli -i wlan1 wps_pbc");

            printf("width = %d, height = %d\r\n",width(), height());
            sleep(2);
            mQRCodeWindow->show();
*/
            Start(ECLink, Wireless);
#endif
        }
/*
        else if(ptr == m_CarplayLink){
#ifdef USE_CARPLAY
            Start(Carplay, Wireless);
#endif
        }
*/
    }
}


void MainWindow::app_status(AppStatusMessage appStatusMessage, void *reserved)
{
 //   lock_guard<mutex> lock(mStateMutex);

    if(appStatusMessage == APP_FOREGROUND){
        printf("++ app_foreground ++\r\n");
        mIsRunningBackGround = false;
        emit Change(false);
        printf("-- app_foreground --\r\n");
    }
    else if(appStatusMessage == APP_BACKGROUND){
        printf("++ app_background ++\r\n");
        mIsRunningBackGround = true;
        emit Change(true);
        printf("-- app_background --\r\n");
    }
    else if(appStatusMessage == APP_RESERVED){

        long long time = (long long )reserved;
        //char *ctime(long *clock);

        struct tm *tdate;
        time_t t = time;
        tdate=localtime(&t);        
        char *ptime = ctime(&t);

        printf("app_reserved long time = %lld, string time =%s\r\n", time, ptime);
    }
    else if(appStatusMessage == APP_QRCODE){

        char* rqcode = (char* )reserved;
        emit QrcodeInfo(rqcode);
        printf("rqcode = %s\r\n", rqcode);
    }
    else if(appStatusMessage == APP_LICENSE){
        License *pLicense = (License*)reserved;
        printf("activate = %d, msg = %s\r\n", pLicense->activate, pLicense->msg.c_str());
    }
    else if(appStatusMessage == APP_INPUTINFO){        
        InputInfo *pInputInfo = (InputInfo*)reserved;
        printf("inputType = %d\r\n", pInputInfo->inputType);
    }
    else if(appStatusMessage == APP_VRTEXT){
        VRTextInfo *pVRTextInfo = (VRTextInfo*)reserved;
        printf("vr type = %d\r\n", pVRTextInfo->type);
    }
    else if(appStatusMessage == APP_VOICE_CMD){

        int cmd = (int )reserved;
        printf("cmd = %d\r\n", cmd);
    }
    else if(appStatusMessage == APP_MUSIC_STARTED){
        //mPlayer->SendKey(KEY_MUSIC_PLAY);
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else if(appStatusMessage == APP_MUSIC_STOPPED){
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else if(appStatusMessage == APP_VOICE_DUCK){
        DuckInfo *pDuckInfo = (DuckInfo*)reserved;
        printf("duck appStatusMessage inDurationSecs = %lf, inVolume = %lf\r\n", pDuckInfo->inDurationSecs, pDuckInfo->volume);
    }
    else if(appStatusMessage == APP_VOICE_UNDUCK){
        double* inDurationSecs = (double*) reserved;
        printf("unduck appStatusMessage inDurationSecs = %lf\r\n", *inDurationSecs);
    }
}

void MainWindow::onUIChanged(bool visible)
{
    printf("++ ui visible %d ++\r\n",visible);
    m_CarplayLink->setVisible(visible);
    m_CarlifeLink->setVisible(visible);
    m_AutoLink->setVisible(visible);
    m_HiCarLink->setVisible(visible);
    m_MirrorLink->setVisible(visible);
    m_EasyConnectLink->setVisible(visible);
    printf("-- ui visible %d --\r\n",visible);
}

void MainWindow::onChangeQRCode(bool visible)
{
    if(visible){
        mQRCodeWindow->show();
    }
    else
        mQRCodeWindow->hide();
}


void MainWindow::carlink_connect_state(ConnectedStatus status, PhoneType type)
{
    switch (status)
    {
        case CONNECT_STATUS_CONNECTING:
        {
            // todo 正在连接
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mChangeMode = true;
            break;
        }
        case CONNECT_STATUS_CONNECT_SUCCEED:
        {
            // todo 连接成功
            mChangeMode = false;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            emit Change(false);
            emit ChangeQRCode(false);
            mPlayer->RequestStatus(QUERYTIME);
            printf("connect succeed\r\n");
            break;
        }
        case CONNECT_STATUS_CONNECT_FAILED:
        case CONNECT_STATUS_DISCONNECTED:
        case CONNECT_STATUS_APP_EXIT:
        case CONNECT_STATUS_INTERRUPTED_BY_APP:
        {
            // todo 连接断开
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("connect disconnected :%d\r\n",status);
            Stop();
            emit Change(true);
            mChangeMode = false;
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            break;
        }
        default:
            break;
    }
    mConnectStatus = status;
    mPhonetype = type;
}

void MainWindow::usb_state(ConnectedStatus status, PhoneType type)
{
    std::lock_guard<std::mutex> lock(mMutex);

    mConnectStatus = status;
    mPhonetype = type;

    printf("=== usb state = %d ===\r\n",status);
    if(status == CONNECT_STATUS_DEVICE_ATTACHED){

        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(!mChangeMode){
  //          Start(Carlife, Wired);
        }
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    }
    else if(status == CONNECT_STATUS_DEVICE_DEATTACHED){
/*
        if(mPlayer){
            mPlayer->SetInserted(false, UnKnown);
        }
*/
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
}

void MainWindow::Start(LinkType linkType, LinkMode linkMode)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer = mpLinkAssist->Initialize(linkType);
    mPlayer->GetIniConfig(mpLinkAssist);
    mPlayer->RegisterConnectCallback(std::bind(&MainWindow::carlink_connect_state,this, std::placeholders::_1,std::placeholders::_2));

    if(linkMode == Wireless)
    {
        //printf("str bt address %s\r\n", mstrBtAddress.c_str());
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        if(linkType == Carlife)
            mPlayer->SendIphoneMacAddress("192.168.2.20");

        if(!mstrBtAddress.empty()){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mPlayer->SendPhoneBluetooth(mstrBtAddress);
        }   
        if(!(mWifiInfo.ssid.empty()| mWifiInfo.passphrase.empty() | mWifiInfo.channel_id.empty())){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            mPlayer->SendCarWifi(mWifiInfo);
        }
    }
    if(linkType == ECLink){
        mPlayer->SendCarBluetooth("car","12:34:56:78:90", "0000");
        mPlayer->SendLisenceCode("TEST036514313165461313");
    }
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    mPlayer->RegisterAppStatusCallback(std::bind(&MainWindow::app_status,this, std::placeholders::_1,std::placeholders::_2));
    mPlayer->Initialize(linkMode, mPhonetype);
    mPlayer->Start();
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}
void MainWindow::Stop()
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    if(mpLinkAssist)
        mpLinkAssist->Release();
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
}
