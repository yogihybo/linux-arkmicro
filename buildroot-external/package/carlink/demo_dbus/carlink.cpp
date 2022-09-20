#include "carlink.h"

#include <QString>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

#define CarLinkService        QString("com.arkmicro.carlink")
#define CarLinkPath           QString("/com/arkmicro/carlink")
#define CarLinkInterface      QString("Local.DbusServer.CarLink")


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

Link::Link(QObject *parent)
    : QObject(parent)
{
    m_CarLinkProxy = new Local::DbusServer::CarLink(CarLinkService,
                                                      CarLinkPath,
                                                      QDBusConnection::sessionBus(),
                                                      this);

    QObject::connect(m_CarLinkProxy, SIGNAL(onLinkStatus(int, int, int)), this, SIGNAL(onLinkStatus(int, int,int)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onCarLinkVersion(int, QString)), this, SIGNAL(onCarLinkVersion(int, QString)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onPhoneType(int, int)), this, SIGNAL(onPhoneType(int, int)));
    QObject::connect(m_CarLinkProxy, SIGNAL(onDateTime(int, long long)), this, SIGNAL(onDateTime(int, long long)));
}

Link::~Link()
{
}
