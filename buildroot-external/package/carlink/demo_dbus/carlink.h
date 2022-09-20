#ifndef LINK_H
#define LINK_H

#include <QObject>
#include <QScopedPointer>
#include <QCoreApplication>
#include "../include/user/LinkBase.h"
#include "carlinkproxy.h"

class Link
        : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Link)
#ifdef g_Link
#undef g_Link
#endif
#define g_Link (Link::instance())
public:
    inline static Link* instance() {
        static Link* link(new Link(qApp));
        return link;
    }
    void requestLink(const int type, const int mode, const int status);
    void requestTouch(int x, int y, int pressed);
    void requestWifi(string ssid, string passphrase, string channel_id);
    void requestCarBluetooth(string name, string address, string pin);
signals:
    void onLinkStatus(const int type, const int mode, const int status);
    void onCarLinkVersion(const int type,  const QString ver);
    void onPhoneType(const int type, const int inserted);
    void onDateTime(const int type, const long long time);
private:
    explicit Link(QObject *parent = NULL);
    ~Link();

    Local::DbusServer::CarLink* m_CarLinkProxy;
};

#endif // LINK_H
