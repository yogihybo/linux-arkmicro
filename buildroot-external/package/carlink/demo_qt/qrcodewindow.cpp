#include "qrcodewindow.h"
#include "ECSDKToolKit.h"



QRCodeWindow::QRCodeWindow(QWidget *parent) : QWidget(parent)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    QPalette pal(this->palette());
    pal.setColor(QPalette::Background, Qt::white);
    this->setAutoFillBackground(true);
    this->setPalette(pal);

    int screen_width = 1920;
    int screen_height = 720;

    setGeometry(0, 0, screen_width, screen_height);

    m_IosQRCode = new QPushButton(QString("IOS"), this);
    m_IosQRCode->setGeometry(width() / 4 * 3, 0, width() / 4, height()/2);
    QObject::connect(m_IosQRCode, SIGNAL(clicked()), this, SLOT(onClicked()));

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    m_AndroidQRCode = new QPushButton(QString("Android"), this);
    m_AndroidQRCode->setGeometry(width() / 4 * 3, height() / 2 * 1, width() / 4, height()/2);
    QObject::connect(m_AndroidQRCode, SIGNAL(clicked()), this, SLOT(onClicked()));

    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    m_Close = new QPushButton(QString("Close"), this);
    m_Close->setGeometry(width() / 4 * 2, height(), width() / 4, height());
    QObject::connect(m_Close, SIGNAL(clicked()), this, SLOT(onClicked()));

    m_Label = new QLabel(QString(""),this);
    m_Label->setGeometry(width() / 4 * 0, 0, 3* width() / 4, height());
    m_Label->setText("select phone type,please scan code!");
    m_Label->show();
    //p2p
}

void QRCodeWindow::resizeEvent(QResizeEvent *event)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    resize(1920, 720);
}

void QRCodeWindow::onQrcodeInfo(char *qrcode)
{
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    string str = qrcode;
    m_Label->setPixmap(createQRCode(str));

}

void QRCodeWindow::getPlayer(IUserLinkPlayer *player)
{
    mPlayer = player;
}

void QRCodeWindow::onUIChanged(bool visible)
{
    if(visible){

    }
    else{
    }
}

QPixmap QRCodeWindow::createQRCode(string str)
{
#ifdef SCAN_QRCODE

    int margin = 2;
    if (str.length() == 0)
       return QPixmap();

    QRcode *qrcode = QRcode_encodeString(str.c_str(), 2, QR_ECLEVEL_L, QR_MODE_8, 0);
    if (qrcode == NULL) {
        return QPixmap();
    }

    unsigned char *p, *q;
    p = NULL;
    q = NULL;
    int x, y, bit;
    int realwidth;

    realwidth = qrcode->width;
    QImage image = QImage(realwidth, realwidth, QImage::Format_Indexed8);
    QRgb value;
    value = qRgb(255, 255, 255);
    image.setColor(0, value);
    value = qRgb(0, 0, 0);
    image.setColor(1, value);
    image.setColor(2, value);
    image.fill(0);
    p = qrcode->data;

    for (y = 0; y<qrcode->width; y++) {
        bit = 7;
        q += margin / 8;
        bit = 7 - (margin % 8);
        for (x = 0; x<qrcode->width; x++) {
            if ((*p & 1) << bit)
                image.setPixel(x, y, 1);
            else
                image.setPixel(x, y, 0);
            bit--;
            if (bit < 0)
          {
            q++;
            bit = 7;
          }
            p++;
        }
    }
    return QPixmap::fromImage(image.scaledToWidth(200));
#endif
}

void QRCodeWindow::onClicked()
{
    const QPushButton* const ptr = static_cast<const QPushButton* const >(sender());
    printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

    if (ptr == m_IosQRCode) {

        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);

#ifdef SCAN_QRCODE
        ECQRInfo info;
        info.ssid = "ark169_wifi";
        info.pwd = "12345678";
        info.action = EC_QR_ACTION_WIFI_AP_MODE_CUSTOMIZED;
        info.auth = "WPA-PSK";
        info.name = "ark169_wifi";
        info.mac = "34:72:4a:00:1a:e9";
        ECSDKToolKit::getInstance()->generateQRCodeUrl(info);
        //mPlayer->RequestStatus(QUERYQRCODE);
#endif

    } else if (ptr == m_AndroidQRCode) {
#ifdef SCAN_QRCODE
        ECQRInfo info ;
        info.ssid = "";
        info.pwd = "";
        info.action = EC_QR_ACTION_WIFI_P2P_MODE;
        info.auth = "";
        info.name = "ark169_wifi";
        info.mac = "36:72:4A:00:1A:E9";
        ECSDKToolKit::getInstance()->generateQRCodeUrl(info);
#endif
        printf("%s:%s:%d\r\n",__FILE__,__func__,__LINE__);
    }
    else if(ptr == m_Close){
        close();
    }
}
