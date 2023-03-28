#include "carlinkproxy.h"

/*
 * Implementation of interface class LocalDbusServerCarLinkInterface
 */

LocalDbusServerCarLinkInterface::LocalDbusServerCarLinkInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
{
}

LocalDbusServerCarLinkInterface::~LocalDbusServerCarLinkInterface()
{
}

