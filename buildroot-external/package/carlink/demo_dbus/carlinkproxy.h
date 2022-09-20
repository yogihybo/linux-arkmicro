#ifndef CARLINKPROXY_H
#define CARLINKPROXY_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>
#include <string>
using namespace std;
/*
 * Proxy class for interface Local.DbusServer.CarLink
 */
class LocalDbusServerCarLinkInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "Local.DbusServer.CarLink"; }

public:
    LocalDbusServerCarLinkInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~LocalDbusServerCarLinkInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> requestLink(int type, int mode, int status)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(type) << qVariantFromValue(mode) << qVariantFromValue(status);
        return asyncCallWithArgumentList(QLatin1String("requestLink"), argumentList);
    }

    inline QDBusPendingReply<> requestTouch(int x, int y, int pressed)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(x) << qVariantFromValue(y) << qVariantFromValue(pressed);
        return asyncCallWithArgumentList(QLatin1String("requestTouch"), argumentList);
    }

    inline QDBusPendingReply<> requestWifi(string ssid, string passphrase, string channel_id)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(QString::fromStdString(ssid)) << qVariantFromValue(QString::fromStdString(passphrase)) << qVariantFromValue(QString::fromStdString(channel_id));
        return asyncCallWithArgumentList(QLatin1String("requestWifi"), argumentList);
    }

    inline QDBusPendingReply<> requestCarBluetooth(string name, string address, string pin)
    {
        QList<QVariant> argumentList;
        argumentList << qVariantFromValue(QString::fromStdString(name)) << qVariantFromValue(QString::fromStdString(address)) << qVariantFromValue(QString::fromStdString(pin));
        return asyncCallWithArgumentList(QLatin1String("requestCarBluetooth"), argumentList);
    }


Q_SIGNALS: // SIGNALS
    void onLinkStatus(int type, int mode, int status);
    void onCarLinkVersion(const int type,  const QString ver);
    void onPhoneType(int type, int inserted);
    void onDateTime(const int type, const long long time);
};

namespace Local {
  namespace DbusServer {
    typedef ::LocalDbusServerCarLinkInterface CarLink;
  }
}
#endif
