#include "carlink.h"
#include <QString>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#define CarLinkService        QString("com.arkmicro.carlink")
#define CarLinkPath           QString("/com/arkmicro/carlink")
#define CarLinkInterface      QString("Local.DbusServer.CarLink")

class LinkPrivate
{
    Q_DISABLE_COPY(LinkPrivate)
public:
    explicit LinkPrivate(Link* parent);
    ~LinkPrivate();
public:
    int m_LinkType;
    int m_LinkConnectStatus;
    int m_DbusConnectStatus;
    int m_Mode;
    bool m_InitHicar;
private:
    Q_DECLARE_PUBLIC(Link)
    Link* const q_ptr;
};

void Link::setLinkType(int linkType){
    Q_D(Link);
    d->m_LinkType = linkType;
    return;
}
int Link::getLinkType(){
    Q_D(Link);
    return d->m_LinkType ;
}
void Link::setLinkConnectStatus(int status){
    Q_D(Link);
    d->m_LinkConnectStatus = status;
    return;
}
int  Link::getLinkConnectStatus(){
    Q_D(Link);
    return d->m_LinkConnectStatus ;
}
void Link::setDbusConnectStatus(int status){
    Q_D(Link);
    d->m_DbusConnectStatus = status;
    return;
}
int Link::getDbusConnectStatus()
{
    Q_D(Link);
    return d->m_DbusConnectStatus;
}
void Link::setLinkMode(int mode){
    Q_D(Link);
    d->m_Mode = mode;
    return;
}
int  Link::getLinkMode(){
    Q_D(Link);
    return d->m_Mode;
}
bool Link::getHicarInitStatus(){
    Q_D(Link);
    return d->m_InitHicar;
}
void Link::setHicarInitStatus(bool init){
    Q_D(Link);
    d->m_InitHicar = init;
}
void Link::requestLink(const int type, const int mode, const int status)
{
    QDBusPendingReply<> reply = m_CarLinkProxy->requestLink(type, mode, status);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request link error = %d\n",reply.error());
    }
}

void Link::requestTouch(int x, int y, int pressed)
{
    //printf("x = %d, y = %d, pressed = %d\n", x, y, pressed);
    QDBusPendingReply<> reply = m_CarLinkProxy->requestTouch(x, y, pressed);
    //reply.waitForFinished();
    if (reply.isError()) {
        printf("touch error = %d\n",reply.error());
    }
}

void Link::requestWifi(string ssid, string passphrase, string channel_id)
{
    QDBusPendingReply<> reply = m_CarLinkProxy->requestWifi(ssid, passphrase, channel_id);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request wifi error = %d\n",reply.error());
    }
}

void Link::requestCarBluetooth(string name, string address, string pin)
{
    QDBusPendingReply<> reply = m_CarLinkProxy->requestCarBluetooth(name, address, pin);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request car bluetooch error = %d\n",reply.error());
    }
}

void Link::requestPhoneBTAddress(string str)
{
    QDBusPendingReply<> reply = m_CarLinkProxy->requestPhoneBTAddress(str);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request Phone Bt Address error = %d\n",reply.error());
    }
}

void Link::requestPhoneIPAddress(string str)
{
    QDBusPendingReply<> reply = m_CarLinkProxy->requestPhoneIPAddress(str);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request Phone IP Address error = %d\n",reply.error());
    }
}
 //UI requset carlink's bt cmd
void Link::requestBluetoothCmd(string cmd)
{
    QDBusPendingReply<> reply = m_CarLinkProxy->requestBluetoothCmd(cmd);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request requestBluetoothCmd error = %d\n",reply.error());
    }
}
 //UI requset carlink's wireless pincode
void Link::requestBroadcast(bool enable){
    QDBusPendingReply<> reply = m_CarLinkProxy->requestBroadcast(enable);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request requestBroadcast error= %d\n",reply.error());
    }
}
void Link::requestNightMode(bool night){
    QDBusPendingReply<> reply = m_CarLinkProxy->requestNightMode(night);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request requestNightMode error = %d\n",reply.error());
    }
}
void Link::requestKey(KeyCode key)
{
    QDBusPendingReply<> reply = m_CarLinkProxy->requestKey(key);
    reply.waitForFinished();
    if (reply.isError()) {
        printf("request link error = %d\n",reply.error());
    }
}

Link::Link(QObject *parent)
    : QObject(parent),
      d_ptr(new LinkPrivate(this))
{
    m_CarLinkProxy = new Local::DbusServer::CarLink(CarLinkService,
                                                    CarLinkPath,
                                                    QDBusConnection::sessionBus(),
                                                    this);
    QObject::connect(m_CarLinkProxy, SIGNAL(onLinkStatus(int, int, int)), this, SIGNAL(onLinkStatus(int, int,int)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onCarLinkVersion(int, QString)), this, SIGNAL(onCarLinkVersion(int, QString)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onPhoneType(int, int)), this, SIGNAL(onPhoneType(int, int)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onDateTime(int, long long)), this, SIGNAL(onDateTime(int, long long)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onBlueToothCmd(const int ,const QString)),
                     this,   SIGNAL(onBlueToothCmd(const int ,const QString)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onPinCode(const int ,const QString)),
                     this,   SIGNAL(onPinCode(const int ,const QString)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onTelephone(const int ,const QString,const QString)),
                     this,   SIGNAL(onTelephone(const int ,const QString,const QString)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onCarLinkInitDone(const int)),
                     this,   SIGNAL(onCarLinkInitDone(const int)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onLinkDuckAudio(const int, double, double)),
                     this,   SIGNAL(onLinkDuckAudio(const int, double, double)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onLinkUnduckAudio(const int, double)),
                     this,   SIGNAL(onLinkUnduckAudio(const int, double)));
}

Link::~Link()
{

}

LinkPrivate::LinkPrivate(Link *parent)
    : q_ptr(parent)
{
    m_LinkType = 0;
    m_LinkConnectStatus = 0;
    m_DbusConnectStatus = 0;
    m_Mode = -1;
    m_InitHicar = true;
}

LinkPrivate::~LinkPrivate()
{

}
