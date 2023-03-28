#include "MainWidget.h"
#include "AutoConnect.h"
#include "Utility.h"
#include "UserInterfaceUtility.h"
#include "RunnableThread.h"
#include "BusinessLogic/Multimedia.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/carback.h"
#include "BusinessLogic/carlink.h"
#include "BusinessLogic/Widget.h"
#include "BusinessLogic/HostApd.h"
#include "BusinessLogic/WiFiManager.h"
#include "BusinessLogic/Setting.h"
#include <QSocketNotifier>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <QTimer>
#include <QDebug>
#include <QQmlProperty>

class MainWidgetPrivate
{
    Q_DISABLE_COPY(MainWidgetPrivate)
public:
    explicit MainWidgetPrivate(MainWidget* parent);
    ~MainWidgetPrivate();
    void initializeNetlink();
    void initializeMultiMediaWidget();
    void initializeTimer();
    void initializeToolWiget();
    void initializeVideoWidget();
    void initializeSettingWidget();
    void initializeTelePhoneWidget();
    void initializePhoneLinkWidget();
    void initializeHomeWidget();
    void initializeCarBack();
    void initializeKeyBoardWidget();
    void initializeAuxWidget();
    void initializeBackWidget();
    int  detectMediaDevice(char* cmd);
    void connectAllSlots();
public:
    QSocketNotifier* m_SocketNotifier;
    QObject*   m_MainWidgetObject;
    QTimer*    m_Timer;
    MultiMediaWidget* m_MultiMediaWidget;
    ToolWiget*     m_ToolWiget;
    VideoWidget*   m_VideoWidget;
    SettingWidget* m_SettingWidget;
    TelePhoneWidget* m_TelePhoneWidget;
    PhoneLinkWidget* m_PhoneLinkWidget;
    HomeWidget* m_HomeWidget;
    KeyBoardWidget* m_KeyBoardWidget;
    AuxWidget*  m_AuxWidget;
    BackWidget* m_BackWidget;
private:
    Q_DECLARE_PUBLIC(MainWidget)
    MainWidget* const q_ptr;
};

MainWidget::MainWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new MainWidgetPrivate(this))
{

}
void MainWidget::setMainWidgetObject(QObject* qmlObject)
{
    Q_D(MainWidget);
    if(d->m_MainWidgetObject == NULL)
    {
        d->m_MainWidgetObject = qmlObject;
    }
    d->initializeToolWiget();
    d->initializeHomeWidget();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_MainWidgetObject, ARKSENDER(mousePressed(double,double)),
                     this,      ARKRECEIVER(onMousePressed(double,double)),
                     type);
    QObject::connect(d->m_MainWidgetObject, ARKSENDER(mouseRelease(double,double)),
                     this,      ARKRECEIVER(onMouseRelease(double,double)),
                     type);
    QObject::connect(d->m_MainWidgetObject, ARKSENDER(mouseMove(double,double)),
                     this,      ARKRECEIVER(onMouseMove(double,double)),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(multiMediaComponentLoaderComplete()),
                     this,      ARKRECEIVER(onMultiMediaComponentLoaderComplete()),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(phoneLinkComponentLoaderComplete()),
                     this,      ARKRECEIVER(onPhoneLinkComponentLoaderComplete()),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(telephoneComponentLoaderComplete()),
                     this,      ARKRECEIVER(onTelephoneComponentLoaderComplete()),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(videoMediaComponentLoaderComplete()),
                     this,      ARKRECEIVER(onVideoMediaComponentLoaderComplete()),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(settingComponentLoaderComplete()),
                     this,      ARKRECEIVER(onSettingComponentLoaderComplete()),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(keyBoardComponentLoaderComplete()),
                     this,      ARKRECEIVER(onKeyBoardComponentLoaderComplete()),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(auxComponentLoaderComplete()),
                     this,      ARKRECEIVER(onAuxComponentLoaderComplete()),
                     type);

    QObject::connect(d->m_MainWidgetObject, ARKSENDER(backComponentLoaderComplete()),
                     this,      ARKRECEIVER(onBackComponentLoaderComplete()),
                     type);

}

void MainWidget::onMultiMediaComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializeMultiMediaWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}
void MainWidget::onPhoneLinkComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializePhoneLinkWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}
void MainWidget::onTelephoneComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializeTelePhoneWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}

void MainWidget::onVideoMediaComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializeVideoWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}

void MainWidget::onSettingComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializeSettingWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}
void MainWidget::onKeyBoardComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializeKeyBoardWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}
void MainWidget::onAuxComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializeAuxWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}

void MainWidget::onBackComponentLoaderComplete()
{
    Q_D(MainWidget);
    d->initializeBackWidget();
    g_Setting->setLoaderInterfaceCount();
    g_Setting->loaderInterfaceCompleted();
}

void MainWidget::onLoaderCompleted()
{
    Q_D(MainWidget);
    d->initializeTimer();
}
void MainWidget::onMousePressed(double globalX,double globalY)
{
     g_Link->requestTouch(globalX, globalY, Touch_Press);
}
void MainWidget::onMouseRelease(double globalX,double globalY)
{
    g_Link->requestTouch(globalX, globalY, Touch_Up);
}
void MainWidget::onMouseMove(double globalX,double globalY)
{
    g_Link->requestTouch(globalX, globalY, Touch_Move);
}
void MainWidget::onDeviceWatcherStatus(const int type, const int status){
    Q_D(MainWidget);
    qDebug()<<"++++[MainWidget::onDeviceWatcherStatus:type]++++"<<type<<"  "<< status;
    if (DWT_USBDisk == type) {
        switch (status) {
        case DWS_Empty: {
            break;
        }
        case DWS_Unsupport: {
            break;
        }
        case DWS_Busy: {
            if(d->m_ToolWiget != NULL)
            {
                int status = g_Link->getLinkConnectStatus();
                qDebug()<< "+++g_Link->getLinkConnectStatus()+++"<< status;
                qDebug()<< "+++g_Widget->getPreemptiveWidget()+++"<< g_Widget->getPreemptiveWidget();
                if(status == 0){
                    if(g_Widget->getPreemptiveWidget() != 1)
                    {
                        QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
                        QQmlProperty(toolWigetObject,"mediaClickedStatus").write(true);
                        QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(false);
                        QQmlProperty(toolWigetObject,"telClickedStatus").write(false);
                        QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
                        QQmlProperty(toolWigetObject,"settingClickedStatus").write(false);
                        QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(1);
                    }
                }
            }
            break;
        }
        case DWS_Ready: {
            break;
        }
        case DWS_Remove: {
            if(d->m_ToolWiget != NULL)
            {
//                int status = g_Link->getLinkConnectStatus();
//                if(status == 0){
//                    if(g_Widget->getPreemptiveWidget() != 1){
//                        QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
//                        if(toolWigetObject->property("mediaClickedStatus").toBool() == true)
//                            QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
//                        if(toolWigetObject->property("auxClickedStatus").toBool() == true)
//                            QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
//                        int _TypeStatus = d->m_MainWidgetObject->property("typeStatus").toInt();
//                        if(_TypeStatus == 1 || _TypeStatus == 4)
//                        {
//                            QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(0);
//                        }
//                    }

//                }
                g_Widget->onUsbMediaPlayExit();

            }
            break;
        }
        default: {
            break;
        }
        }
    }

    if (DWT_SDDisk == type) {
        switch (status) {
        case DWS_Empty: {
            break;
        }
        case DWS_Unsupport: {
            break;
        }
        case DWS_Busy: {
            if(d->m_ToolWiget != NULL)
            {
                int status = g_Link->getLinkConnectStatus();
                qDebug()<<"+++0000g_Link->getLinkConnectStatus()+++"<< status;
                qDebug()<<"+++0000g_Widget->getPreemptiveWidget()+++"<< g_Widget->getPreemptiveWidget();
                if(status == 0){
                    if(g_Widget->getPreemptiveWidget() != 1)
                    {
                        QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
                        QQmlProperty(toolWigetObject,"mediaClickedStatus").write(true);
                        QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(false);
                        QQmlProperty(toolWigetObject,"telClickedStatus").write(false);
                        QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
                        QQmlProperty(toolWigetObject,"settingClickedStatus").write(false);
                        QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(1);
                    }
                }
            }
            break;
        }
        case DWS_Ready: {
            break;
        }
        case DWS_Remove: {
            if(d->m_ToolWiget != NULL)
            {
//                int status = g_Link->getLinkConnectStatus();
//                if(status == 0){
//                    if(g_Widget->getPreemptiveWidget() != 1){
//                        QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
//                        if(toolWigetObject->property("mediaClickedStatus").toBool() == true)
//                            QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
//                        if(toolWigetObject->property("auxClickedStatus").toBool() == true)
//                            QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
//                        int _TypeStatus = d->m_MainWidgetObject->property("typeStatus").toInt();
//                        if(_TypeStatus == 1 || _TypeStatus == 4)
//                        {
//                            QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(0);
//                        }
//                    }

//                }
                g_Widget->onSdMediaPlayExit();
            }
            break;
        }
        default: {
            break;
        }
        }
    }
    qDebug()<<"++++MainWidget::onDeviceWatcherStatus end+++++";
}
void MainWidget::onConnectStatusChange(const int status){
    Q_D(MainWidget);
    if(d->m_MainWidgetObject != NULL && d->m_ToolWiget != NULL)
    {
        if(status == Bluetooth::BCS_Incoming || status == Bluetooth::BCS_Outgoing){
            int status = g_Link->getLinkConnectStatus();
            if(status == 0){
                if(g_Widget->getPreemptiveWidget() != 1)
                {
                    QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
                    QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(3);
                    QQmlProperty(toolWigetObject,"telClickedStatus").write(true);
                    QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
                    QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(false);
                    QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
                    QQmlProperty(toolWigetObject,"settingClickedStatus").write(false);
                }
            }
        }
    }
}
void MainWidget::onTimeOut()
{
    Q_D(MainWidget);
    static bool _FirstStart(true);
    QTimer* ptr = static_cast<QTimer*>(sender());
    if(ptr == d->m_Timer)
    {
        if(_FirstStart)
        {
           d->initializeNetlink();
           _FirstStart = false;
           if(d->m_Timer->isActive())
           {
               d->m_Timer->stop();
           }
           d->m_Timer->start();
        }
        else{
            if(d->detectMediaDevice("devmem 0xec800050") == 0)
            {
                qDebug()<<"+++++++++detectMediaDevice0000+++++++++";
                g_Setting->executeShellCmd("echo 9 > /sys/class/gpio/export");
                g_Setting->mySleep(50);
                g_Setting->executeShellCmd("echo 9 > /sys/class/gpio/unexport");
                g_Setting->mySleep(50);
                g_Setting->executeShellCmd("devmem 0xe49001c0 32 0x9249249");
            }
        }
    }
}

void MainWidget::onActivated()
{
    Q_D(MainWidget);
    QSocketNotifier* socket = static_cast<QSocketNotifier*>(sender());
    if (socket == d->m_SocketNotifier) {
        QByteArray data;
        data.resize(1024);
        size_t len = ::read(d->m_SocketNotifier->socket(), data.data(), 1024);
        data.resize(len);
        data = data.replace(0, '\n').trimmed();
        QString filter(data);
        //qDebug()<<"++++filter++++++"<<filter;
        if (filter.contains(QString("SUBSYSTEM=module"))
                || filter.contains(QString("SUBSYSTEM=mmc_host"))
                || filter.contains(QString("SUBSYSTEM=drivers"))
                || filter.contains(QString("SUBSYSTEM=platform"))
                || filter.contains(QString("SUBSYSTEM=sound"))
                || filter.contains(QString("SUBSYSTEM=udc"))
                || filter.contains(QString("SUBSYSTEM=net"))
                || filter.contains(QString("SUBSYSTEM=usb"))
                || filter.contains(QString("SUBSYSTEM=usb_device"))
                || filter.contains(QString("SUBSYSTEM=misc"))
                || filter.contains(QString("SUBSYSTEM=queues"))
                || filter.contains(QString("SUBSYSTEM=video4linux"))) {
            if (!filter.contains(QString("PRODUCT=5ac/"))) {
                return;
            }
        }
        if (filter.contains(QString("DEVNAME=sd"))
                || filter.contains(QString("DEVNAME=mmcblk1"))) {
            g_Multimedia->startMultimedia();
            d->m_SocketNotifier->setEnabled(false);
            ::close(d->m_SocketNotifier->socket());
        }
    }
}

void MainWidget::onCarbackStatusChange(int status){
    Q_D(MainWidget);
    qDebug()<<"===[MainWidget::onCarbackStatusChange:status]==="<<status;
    switch (status) {
        case Carback::CBS_On:
            arkapi_display_hide_layer(g_Widget->getUIDispLayer());
            break;
        case Carback::CBS_Off:
            arkapi_display_show_layer(g_Widget->getUIDispLayer());
            break;
        default:
            break;
    }
}
void MainWidget::onWidgetTypeChange(const int destinationType, const int requestType, const QString &status){
    Q_D(MainWidget);
    switch (requestType) {
        case Widget::T_Home:
            switch (destinationType) {
                case Widget::T_BluetoothTel:
                    if(status == QString("show"))
                    {
                        if(g_Widget->getPreemptiveWidget() != 1)
                        {
                            QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
                            QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
                            QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(false);
                            QQmlProperty(toolWigetObject,"telClickedStatus").write(true);
                            QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
                            QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(3);
                        }

                    }
                    break;
                case Widget::T_MusicPlay:
                    if(status == QString("show"))
                    {
                        if(g_Widget->getPreemptiveWidget() != 1){
                            QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
                            QQmlProperty(toolWigetObject,"mediaClickedStatus").write(true);
                            QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(false);
                            QQmlProperty(toolWigetObject,"telClickedStatus").write(false);
                            QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
                            QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(1);
                        }

                    }
                    break;
                case Widget::T_PhoneLink:
                    if(status == QString("show"))
                    {
                        if(g_Link->getDbusConnectStatus() == DBUS_BACKGROUND){
                            printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
                            g_Widget->setPhoneLinkStatus(0);
                            g_Link->requestLink(g_Link->getLinkType(), g_Link->getLinkMode(), DBUS_REQUEST_FOREGROUND);
                        }
                        else{
                            if(g_Widget->getPreemptiveWidget() != 1){
                                QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
                                QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
                                QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(true);
                                QQmlProperty(toolWigetObject,"telClickedStatus").write(false);
                                QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
                                QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(2);
                            }
                        }
                    }
                    break;
                case Widget::T_Aux:
                    if((status == QString("show"))&&(d->m_AuxWidget->getHthreadExitStatus() == true))
                    {
                       qDebug()<<"========== Widget::T_Aux=====xxxx1111=======";
                       d->m_AuxWidget->setHthread(1);
                       QQmlProperty(d->m_MainWidgetObject,"auxStatus").write(1);
                       d->m_BackWidget->setWidgetType(Widget::T_Aux);
                    }
                    break;
                default:
                    break;
            }
            break;
    case Widget::T_Aux:
        switch (destinationType) {
            case Widget::T_Home:
                if(status == QString("show")){
                    qDebug()<<"==========auxWidget show======0000=======";
                    QQmlProperty(d->m_MainWidgetObject,"auxStatus").write(0);
                    d->m_AuxWidget->setHthread(0);
                    d->m_AuxWidget->setVisibleStatus(false);
                    d->m_BackWidget->setWidgetType(Widget::T_Home);
                    g_Widget->setPreemptiveWidget(0);
                }
                break;
            case Widget::T_Aux:
                if(status == QString("show") && d->m_AuxWidget->getVisibleStatus()){
                    qDebug()<<"==========auxWidget show======1111=======";
                    QQmlProperty(d->m_MainWidgetObject,"auxStatus").write(1);
                    d->m_BackWidget->setWidgetType(Widget::T_Aux);
                }
                else if(status == QString("request") && d->m_AuxWidget->getVisibleStatus()){
                    qDebug()<<"==========auxWidget show======2222=======";
                    QQmlProperty(d->m_MainWidgetObject,"auxStatus").write(2);
                    d->m_BackWidget->setWidgetType(Widget::T_Aux);
                }
                break;
            default:
                break;
        }

    default:
        break;
    }

}
void MainWidget::onPhoneLinkTelePhone(int calltype)
{
    Q_D(MainWidget);
    qDebug()<<"+++++++++onPhoneLinkTelePhone++++++++"<<calltype;
    if(calltype==0)
    {
        QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
        QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"telClickedStatus").write(true);
        QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"settingClickedStatus").write(false);
        QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(3);
    }
    else if(calltype==1)
    {
        QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
        QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(true);
        QQmlProperty(toolWigetObject,"telClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"settingClickedStatus").write(false);
        QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(-1);
    }
    else if(calltype==-1)
    {
        QObject* toolWigetObject = d->m_ToolWiget->getToolWigetObject();
        QQmlProperty(toolWigetObject,"mediaClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"phoneLinkClickedStatus").write(true);
        QQmlProperty(toolWigetObject,"telClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"auxClickedStatus").write(false);
        QQmlProperty(toolWigetObject,"settingClickedStatus").write(false);
        QQmlProperty(d->m_MainWidgetObject,"typeStatus").write(2);
    }
}
MainWidgetPrivate::MainWidgetPrivate(MainWidget *parent)
    : q_ptr(parent)
{
    m_SocketNotifier = NULL;
    m_Timer = NULL;
    m_MainWidgetObject =NULL;
    m_MultiMediaWidget = NULL;
    m_ToolWiget  = NULL;
    m_VideoWidget = NULL;
    m_SettingWidget = NULL;
    m_TelePhoneWidget = NULL;
    m_PhoneLinkWidget = NULL;
    m_HomeWidget      = NULL;
    m_KeyBoardWidget  = NULL;
    m_AuxWidget  = NULL;
    m_BackWidget = NULL;
    initializeCarBack();
    connectAllSlots();
}

MainWidgetPrivate::~MainWidgetPrivate()
{

}

void MainWidgetPrivate::initializeNetlink()
{
    Q_Q(MainWidget);
    if (NULL == m_SocketNotifier) {
        int socket = ::socket(PF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
        if (-1 == socket) {
            perror("socket");
            qDebug() << "create netlink socket fail!";
            return;
        }
        struct sockaddr_nl sock_nl;
        memset(&sock_nl, 0, sizeof(struct sockaddr_nl));
        sock_nl.nl_family = AF_NETLINK;
        sock_nl.nl_groups = 1;
        int bind = ::bind(socket, (struct sockaddr*)&sock_nl, sizeof(struct sockaddr_nl));
        if (-1 == bind) {
            perror("bind");
            qDebug() << "bind netlink socket fail!";
            return;
        }
        m_SocketNotifier = new QSocketNotifier(socket, QSocketNotifier::Read, q);
        QObject::connect(m_SocketNotifier, SIGNAL(activated(int)),
                         q,         SLOT(onActivated()));
    }
}
void MainWidgetPrivate::initializeTimer()
{
    Q_Q(MainWidget);
    if(m_Timer == NULL)
    {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(1000);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer,SIGNAL(timeout()),q,SLOT(onTimeOut()));
        m_Timer->start();
    }
}
void MainWidgetPrivate::initializeMultiMediaWidget()
{
    Q_Q(MainWidget);
    if(m_MultiMediaWidget == NULL)
    {
        m_MultiMediaWidget = new MultiMediaWidget(q);
        QObject* MultiMediaWidgetObject = m_MainWidgetObject->findChild<QObject*>("multiMediaWidgetObject");
        m_MultiMediaWidget->setMultiMediaWidgetObject(MultiMediaWidgetObject);
    }
}

void MainWidgetPrivate::initializeToolWiget()
{
    Q_Q(MainWidget);
    if(m_ToolWiget == NULL)
    {
        m_ToolWiget = new ToolWiget(q);
        QObject* toolWigetObject = m_MainWidgetObject->findChild<QObject*>("toolWidgetObject");
        m_ToolWiget->setToolWigetObject(toolWigetObject);
    }
}

void MainWidgetPrivate::initializeVideoWidget()
{
    Q_Q(MainWidget);
    if(m_VideoWidget == NULL)
    {
        m_VideoWidget = new VideoWidget(q);
        QObject* videoWidgetObject = m_MainWidgetObject->findChild<QObject*>("videoMediaWidgetObject");
        m_VideoWidget->setVideoObject(videoWidgetObject);
        m_VideoWidget->setParentObject(m_MainWidgetObject);
    }
}

void MainWidgetPrivate::initializeSettingWidget()
{
    Q_Q(MainWidget);
    if(m_SettingWidget == NULL)
    {
        m_SettingWidget = new SettingWidget(q);
        QObject* settingWidgetObject = m_MainWidgetObject->findChild<QObject*>("settingWidgetObject");
        m_SettingWidget->setSettingWidgetObject(settingWidgetObject);
    }
}

void MainWidgetPrivate::initializeTelePhoneWidget()
{
    Q_Q(MainWidget);
    if(m_TelePhoneWidget == NULL)
    {
        m_TelePhoneWidget = new TelePhoneWidget(q);
        QObject* _TelePhoneWidgetObject = m_MainWidgetObject->findChild<QObject*>("telephoneWidgetObject");
        m_TelePhoneWidget->setTelePhoneWidgetObject(_TelePhoneWidgetObject);
    }
}

void MainWidgetPrivate::initializePhoneLinkWidget()
{
    Q_Q(MainWidget);
    if(m_PhoneLinkWidget == NULL)
    {
        m_PhoneLinkWidget = new PhoneLinkWidget(q);
        QObject* _PhoneLinkWidgetObject = m_MainWidgetObject->findChild<QObject*>("phoneLinkWidgetObject");
        m_PhoneLinkWidget->setPhoneLinkWidgetObject(_PhoneLinkWidgetObject);
    }
}

void MainWidgetPrivate::initializeHomeWidget(){
    Q_Q(MainWidget);
    if(m_HomeWidget == NULL)
    {
        m_HomeWidget = new HomeWidget(q);
        QObject* _HomeWidgetObject = m_MainWidgetObject->findChild<QObject*>("homeWidgetObject");
        m_HomeWidget->setHomeWidgetObject(_HomeWidgetObject);
    }
}

void MainWidgetPrivate::initializeAuxWidget(){
    Q_Q(MainWidget);
    if(m_AuxWidget == NULL)
    {
        m_AuxWidget = new AuxWidget(q);
        QObject* _AuxWidgetObject = m_MainWidgetObject->findChild<QObject*>("auxWidgetObject");
        QObject* _AuxLoaderObject = m_MainWidgetObject->findChild<QObject*>("auxLoaderObject");
        m_AuxWidget->setAuxLoaderObject(_AuxLoaderObject);
        m_AuxWidget->setAuxWidgetObject(_AuxWidgetObject);
    }
}

void MainWidgetPrivate::initializeKeyBoardWidget(){
    Q_Q(MainWidget);
    if(m_KeyBoardWidget == NULL)
    {
        m_KeyBoardWidget = new KeyBoardWidget(q);
        QObject* _KeyBoardWidgetObject = m_MainWidgetObject->findChild<QObject*>("keyBoardWidgetObject");
        QObject* _KeyBoardLoaderObject = m_MainWidgetObject->findChild<QObject*>("keyBoardLoaderObject");
        m_KeyBoardWidget->setKeyBoardLoaderObject(_KeyBoardLoaderObject);
        m_KeyBoardWidget->setKeyBoardWidgetObject(_KeyBoardWidgetObject);
    }
}
void MainWidgetPrivate::initializeBackWidget(){
    Q_Q(MainWidget);
    if(m_BackWidget == NULL)
    {
        m_BackWidget = new BackWidget(q);
        QObject* _BackWidget = m_MainWidgetObject->findChild<QObject*>("backWidgetObject");
        QObject* _BackLoader = m_MainWidgetObject->findChild<QObject*>("backLoaderObject");
        m_BackWidget->setBackLoaderObject(_BackLoader);
        m_BackWidget->setBackWidgetObject(_BackWidget);
    }
}
void MainWidgetPrivate::initializeCarBack()
{
    Q_Q(MainWidget);
    g_Carback->initialize();
}

int MainWidgetPrivate::detectMediaDevice(char* cmd)
{
    char sys_line[128];
    char buf_ps[256];
    memset(sys_line, '\0', 128);
    memset(buf_ps, 0, 256);
    FILE * ptr;
    int outValue = -1;
    sprintf(sys_line, "%s", cmd);
    printf("res[%d]:%s :\n",(int)strlen(buf_ps),sys_line);
    if((ptr=popen(sys_line, "r"))!=NULL)
    {
        rewind(ptr);
        fread(buf_ps, 256,1, ptr);
        printf("++++buf_ps:%s++++\n",buf_ps);
        string str(buf_ps);
        QString output = QString::fromStdString(str);
        output = output.left(output.size()-1);
        bool ok(false);
        outValue = output.toInt(&ok,16);
        pclose(ptr);
        ptr = NULL;
    }
    else
    {
        outValue = -1;
        printf("popen %s error\n", sys_line);
    }
    return outValue;
}
void MainWidgetPrivate::connectAllSlots()
{
    Q_Q(MainWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onDeviceWatcherStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_Carback, q, ARKRECEIVER(onCarbackStatusChange(int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
    connectSignalAndSlotByNamesake(g_Widget, q, ARKRECEIVER(onWidgetTypeChange(const int, const int, const QString &)));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onPhoneLinkTelePhone(int)));
    connectSignalAndSlotByNamesake(g_Setting, q, ARKRECEIVER(onLoaderCompleted()));
}




