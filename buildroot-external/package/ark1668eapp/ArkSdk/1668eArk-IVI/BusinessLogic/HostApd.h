#ifndef HOSTAPD_H
#define HOSTAPD_H

#include <QObject>
#include <QGuiApplication>
#include <pthread.h>
#include <string>
#include <vector>
#include "BusinessLogic/wpa_ctrl.h"
#include <QMutex>
using namespace std;
class HostApdPrivate;
class HostApd : public QObject
{
    Q_OBJECT
#ifdef g_HostApd
#undef g_HostApd
#endif
#define g_HostApd (HostApd::instance())
public:
    inline static HostApd* instance() {
        static HostApd *hostApd(new HostApd(qApp));
        return hostApd;
    }
    //void initHostApd();
    void restartHostApd();
    void CreatThreadRevNetSta();
    void CreatThreadInitHostApd();
    int  getInitHostapdStatus();
    void setInitHostapdStatus(int status);
private:
    explicit HostApd(QObject *parent = nullptr);
    ~HostApd();
    HostApdPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(HostApd)
};

#endif // HOSTAPD_H
