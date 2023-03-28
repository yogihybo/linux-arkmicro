#ifndef LINK_H
#define LINK_H

#include <QObject>
#include <QScopedPointer>
#include <QCoreApplication>
#include "LinkBase.h"
#include "carlinkproxy.h"
class LinkPrivate;
class Link : public QObject
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
    void requestPhoneIPAddress(string str);                                                             //UI request carlink's connected phone's ip address
    void requestPhoneBTAddress(string str);
    void setLinkType(int linkType);
    int  getLinkType();
    void setLinkConnectStatus(int status);
    int  getLinkConnectStatus();
    void setDbusConnectStatus(int status);
    int  getDbusConnectStatus();
    void setLinkMode(int mode);
    int  getLinkMode();
    void requestBluetoothCmd(string cmd);   //UI requset carlink's bt cmd
    void requestBroadcast(bool enable); //UI requset carlink's wireless pincode
    bool getHicarInitStatus();
    void setHicarInitStatus(bool init);
    void requestKey(KeyCode key); //UI request carlink's key value
    void requestNightMode(bool night);
signals:
    void onLinkStatus(const int type, const int mode, const int status);
    void onCarLinkVersion(const int type,  const QString ver);
    void onPhoneType(const int type, const int inserted);
    void onDateTime(const int type, const long long time);
    void onPinCode(const int type, const QString pincode);    //send carlink's pincode(hicar)
    void onBlueToothCmd(const int type, const QString cmd);    //send carlink's bluetooth of "AT" cmd
    void onCarLinkInitDone(const int type);
    void onLinkDuckAudio(const int type, double durationSecs, double volume);//send carlink's audio duck volume
    void onLinkUnduckAudio(const int type, double durationSecs);//send carlink's audio unduck volume
    void onTelephone(const int type, const QString name, const QString number);
private:
    explicit Link(QObject *parent = NULL);
    ~Link();
    Local::DbusServer::CarLink* m_CarLinkProxy;
    LinkPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(Link)
};

#endif // LINK_H
