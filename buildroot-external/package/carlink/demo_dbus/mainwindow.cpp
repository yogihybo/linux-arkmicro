#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sys/ipc.h>
#include <sys/msg.h>

#define KEY 1234
#define MESSAGEMAXSIZE 1024
#define RECEIVETYPE 0
struct msg
{
    long int messageType;
    char message[MESSAGEMAXSIZE];
};


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),mIsRunningBackGround(false),mDbusSend(DBUS_DISCONNECTED)
{
    ui->setupUi(this);
    onUIInit();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onUIInit()
{
    int screen_width = 1920;
    int screen_height = 720;

    setGeometry(0, 0, screen_width, screen_height);
    setAttribute(Qt::WA_TranslucentBackground);

    QObject::connect(g_Link, SIGNAL(onLinkStatus(int,int,int)),
                     this,   SLOT(onLinkStatus(int,int,int)));

    QObject::connect(g_Link, SIGNAL(onCarLinkVersion(int,QString)),
                     this,   SLOT(onCarLinkVersion(int,QString)));

    QObject::connect(g_Link, SIGNAL(onPhoneType(int,int)),
                     this,   SLOT(onPhoneType(int,int)));

    QObject::connect(g_Link, SIGNAL(onDateTime(int,long long)),
                     this,   SLOT(onDateTime(int,long long)));

    m_CarplayWiredLink = new QPushButton(QString("Carplay Wired"), this);
    m_CarplayWiredLink->setGeometry(width() / 6 * 0, 0, width() / 6, height()/2);
    QObject::connect(m_CarplayWiredLink, SIGNAL(clicked()),
                     this,          SLOT(onClicked()));

    m_CarplayWirelessLink = new QPushButton(QString("Carplay Wireless"), this);
    m_CarplayWirelessLink->setGeometry(width() / 6 * 0, height()/2, width() / 6, height()/2);
    QObject::connect(m_CarplayWirelessLink, SIGNAL(clicked()),
                     this,          SLOT(onClicked()));

    m_AutoWiredLink = new QPushButton(QString("Auto Wired"), this);
    m_AutoWiredLink->setGeometry(width() / 6 * 1, 0, width() / 6, height()/2);
    QObject::connect(m_AutoWiredLink, SIGNAL(clicked()),
                     this,         SLOT(onClicked()));

    m_AutoWirelessLink = new QPushButton(QString("Auto Wireless"), this);
    m_AutoWirelessLink->setGeometry(width() / 6 * 1, height()/2, width() / 6, height()/2);
    QObject::connect(m_AutoWirelessLink, SIGNAL(clicked()),
                     this,         SLOT(onClicked()));

    m_CarlifeWiredLink = new QPushButton(QString("Carlife Wired"), this);
    m_CarlifeWiredLink->setGeometry(width() / 6 * 2, 0, width() / 6, height()/2);
    QObject::connect(m_CarlifeWiredLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));

    m_CarlifeWirelessLink = new QPushButton(QString("Carlife Wireless"), this);
    m_CarlifeWirelessLink->setGeometry(width() / 6 * 2, height()/2, width() / 6, height()/2);
    QObject::connect(m_CarlifeWirelessLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));

    m_HiCarWiredLink = new QPushButton(QString("HiCar Wired"), this);
    m_HiCarWiredLink->setGeometry(width() / 6 * 3, 0, width() / 6, height()/2);
    QObject::connect(m_HiCarWiredLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));


    m_HiCarWirelessLink = new QPushButton(QString("HiCar Wireless"), this);
    m_HiCarWirelessLink->setGeometry(width() / 6 * 3, height()/2, width() / 6, height()/2);
    QObject::connect(m_HiCarWirelessLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));


    m_MirrorWiredLink = new QPushButton(QString("Mirror Wired"), this);
    m_MirrorWiredLink->setGeometry(width() / 6 * 4, 0, width() / 6, height()/2);
    QObject::connect(m_MirrorWiredLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));

    m_MirrorWirelessLink = new QPushButton(QString("Mirror Wireless"), this);
    m_MirrorWirelessLink->setGeometry(width() / 6 * 4, height()/2, width() / 6, height()/2);
    QObject::connect(m_MirrorWirelessLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));


    m_EasyWiredConnectLink = new QPushButton(QString("ECLink Wired"), this);
    m_EasyWiredConnectLink->setGeometry(width() / 6 * 5, 0, width() / 6, height()/2);
    QObject::connect(m_EasyWiredConnectLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));

    m_EasyWirelessConnectLink = new QPushButton(QString("ECLink Wireless"), this);
    m_EasyWirelessConnectLink->setGeometry(width() / 6 * 5, height()/2, width() / 6, height()/2);
    QObject::connect(m_EasyWirelessConnectLink, SIGNAL(clicked()),
                     this,      SLOT(onClicked()));
}

void MainWindow::onClicked()
{
    const QPushButton* const ptr = static_cast<const QPushButton* const >(sender());
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    printf("dbus recevier :%d\r\n ", mDbusSend);
    if(mDbusSend == DBUS_CONNECTED ){
        if(mIsRunningBackGround){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            printf("linktype = %d, linkmode = %d\r\n", (int)mLinkType, (int)mLinkMode);
            g_Link->requestLink(mLinkType, mLinkMode, DBUS_REQUEST_FOREGROUND);
         }
    }
    else if(mInserted == DBUS_DEVICE_ATTACHED){
        if (ptr == m_CarplayWiredLink) {
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            g_Link->requestLink(Carplay, Wired, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_AutoWiredLink) {
            g_Link->requestLink(Android_Auto, Wired, DBUS_REQUEST_CONNECT);
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
        }
        else if(ptr == m_CarlifeWiredLink) {
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            g_Link->requestLink(Carlife, Wired, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_HiCarWiredLink){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            g_Link->requestLink(HiCar, Wired, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_MirrorWiredLink){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            g_Link->requestLink(Mirror, Wired, DBUS_REQUEST_CONNECT);

        } else if(ptr == m_EasyWiredConnectLink){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            g_Link->requestLink(ECLink, Wired, DBUS_REQUEST_CONNECT);
        }
    }
    else{

        if(ptr == m_CarplayWirelessLink){
            g_Link->requestWifi("ark1668e_devb_wifi", "12345678", "36");
            g_Link->requestCarBluetooth("carlink","00:11:22:33:44:55","0000");
            g_Link->requestLink(Carplay, Wireless, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_AutoWirelessLink){
            g_Link->requestLink(Android_Auto, Wireless, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_CarlifeWirelessLink){
            g_Link->requestLink(Carlife, Wireless, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_HiCarWirelessLink){
            g_Link->requestLink(HiCar, Wireless, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_MirrorWirelessLink){
            g_Link->requestLink(HiCar, Wireless, DBUS_REQUEST_CONNECT);
        }
        else if(ptr == m_EasyWirelessConnectLink){
            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
            g_Link->requestLink(ECLink, Wireless, DBUS_REQUEST_CONNECT);
        }
    }
}

void MainWindow::onUIChanged(bool visible)
{
    m_CarplayWiredLink->setVisible(visible);
    m_CarplayWirelessLink->setVisible(visible);
    m_AutoWiredLink->setVisible(visible);
    m_AutoWirelessLink->setVisible(visible);
    m_CarlifeWiredLink->setVisible(visible);
    m_CarlifeWirelessLink->setVisible(visible);
    m_HiCarWiredLink->setVisible(visible);
    m_HiCarWirelessLink->setVisible(visible);
    m_MirrorWiredLink->setVisible(visible);
    m_MirrorWirelessLink->setVisible(visible);
    m_EasyWiredConnectLink->setVisible(visible);
    m_EasyWirelessConnectLink->setVisible(visible);
}

void MainWindow::onLinkStatus(int type, int mode, int status)
{
    if(status == DBUS_CONNECTED){
        onUIChanged(false);
        mDbusSend = (DbusSend)status;
        mLinkType = (LinkType)type;
        mLinkMode = (LinkMode)mode;
    }
    else if(status == DBUS_FOREGROUND){
        mIsRunningBackGround = false;
        onUIChanged(false);
    }
    else if(status == DBUS_BACKGROUND){
        mIsRunningBackGround = true;
        onUIChanged(true);
    }
    else if(status == DBUS_DISCONNECTED ||
            status == DBUS_FAILED ||
            status == DBUS_APP_EXIT ||
            status == DBUS_INTERRUPTED_BY_APP){
        onUIChanged(true);
        mDbusSend = (DbusSend)status;
    }
    printf("onLinkStatus :type = %d, mode =%d, status = %d\n", type, mode, status);
}

void MainWindow::onPhoneType(int type, int inserted)
{
    if(inserted == 0)
        mInserted = DBUS_DEVICE_ATTACHED;
    else if(inserted == 1)
        mInserted = DBUS_DEVICE_DEATTACHED;
}

void MainWindow::onCarLinkVersion(const int type,  const QString ver)
{

    printf("onCarLinkVersion :type = %d, ver =%s\n", type, ver.toStdString().data());
}

void MainWindow::onDateTime(const int type, const long long time)
{
    time_t t = time;
    char *ptime = ctime(&t);
    printf("onDateTime :type: %d, time:%s\n", type, ptime);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    g_Link->requestTouch(event->pos().x(), event->pos().y(), Touch_Press);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    g_Link->requestTouch(event->pos().x(), event->pos().y(), Touch_Up);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    printf("x:%d,y:%d\r\n",event->pos().x(),event->pos().y());

    g_Link->requestTouch(event->pos().x(), event->pos().y(), Touch_Move);
}
