#ifndef QRCodeWindow_H
#define QRCodeWindow_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <string>

#include "qrencode.h"
#include "IUserLinkPlayer.h"
#include "EasyConnectLink.h"

using namespace std;

class QRCodeWindow : public QWidget
{
    Q_OBJECT
public:
    explicit QRCodeWindow(QWidget *parent = nullptr);

    void getPlayer(IUserLinkPlayer *player);

protected:
    void resizeEvent(QResizeEvent *event);

    QPixmap createQRCode(string str);
signals:    
    void UIChange(bool visible);
public slots:

    void onUIChanged(bool visible);
    void onClicked();
    void onQrcodeInfo(char *qrcode);
private:
    QPushButton* m_IosQRCode;
    QPushButton* m_AndroidQRCode;
    QPushButton* m_Close;
    QLabel*      m_Label;
    IUserLinkPlayer *mPlayer;
};

#endif // QRCodeWindow_H
