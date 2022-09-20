#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "IUserLinkPlayer.h"
#include "LinkAssist.h"
#include "qrcodewindow.h"
#include <mutex>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;
class QLabel;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
private slots:
    void onClicked();
    void onUIChanged(bool visible);
    void onChangeQRCode(bool visible);

private:
    Ui::MainWindow *ui;
    QPushButton* m_CarplayLink;
    QPushButton* m_AutoLink;
    QPushButton* m_CarlifeLink;
    QPushButton* m_HiCarLink;
    QPushButton* m_MirrorLink;
    QPushButton* m_EasyConnectLink;

    QRCodeWindow *mQRCodeWindow;
    IUserLinkPlayer *mPlayer;

    LinkAssist      *mpLinkAssist;
    ConnectedStatus mConnectStatus;
    PhoneType mPhonetype;
    std::mutex mMutex;
    //UsbHostService  *m_pUsbHost;
private:
    void Start(LinkType linkType, LinkMode linkMode);
    void Stop();
    void app_status(AppStatusMessage appStatusMessage, void *reserved);
    void usb_state(ConnectedStatus status, PhoneType type);
    void carlink_connect_state(ConnectedStatus status, PhoneType type);

    bool mChangeMode;
    std::atomic_bool     mIsRunningBackGround;


    string               mstrBtAddress;
    string               mstrIpAddress;
    WifiInfo             mWifiInfo;
    Semaphore            mWait;

signals:
    void QrcodeInfo(char *qrcode);
    void Change(bool visible);
    void ChangeQRCode(bool visible);
};
#endif // MAINWINDOW_H
