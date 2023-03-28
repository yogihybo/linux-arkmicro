#include "mainwindow.h"
#include <QPainter>

#include <QPainter>
#include <QPropertyAnimation>
#include <QDebug>
#include "BusinessLogic/carback.h"
#include <QResource>
#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
#ifdef __ARM__
    QResource::registerResource("/usr/share/images.rcc");
    this->setWindowFlag(Qt::FramelessWindowHint);
#endif
    QPalette pal = palette();
    pal.setColor(QPalette::Background, QColor(0x00,0x00,0x00,0x00));
    setPalette(pal);

    pixBackground = QPixmap(":/images/HomeBackground.png");
    m_Background = new QLabel(this);
    m_Background->setPixmap(pixBackground);
    m_Background->setGeometry(0, 0, 1920, 720);
    m_Background->setVisible(true);

    logoPainter = new LogoPainter(this);
    logoPainter->setGeometry(0,18,652,684);
    logoPainter->show();

    speedPainter = new SpeedPainter(this);
    speedPainter->setGeometry(1268, 18, 652,686);
    speedPainter->show();

    m_CornerLampWidget = new CornerLampWidget(this);
    //m_CornerLampWidget->setGeometry(764, 154, 372, 52);
    m_CornerLampWidget->setGeometry(764, 154, 390, 52);
    m_CornerLampWidget->show();

    g_Carback->initialize();

    m_LinkWidget = new LinkWidget(this);
    m_LinkWidget->setGeometry(654, 214, 616, 436);
    m_LinkWidget->show();
}

MainWindow::~MainWindow()
{
    delete logoPainter;
    delete speedPainter;
    delete m_Background;
    delete m_LinkWidget;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    resize(1920,720);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    qDebug() << "MainWindow::paintEvent";
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event){

    if((event->x() >= 960 && event->x() <= 1010) &&
            (event->y() >= 660 && event->y() <= 710))
    {
        qDebug()<<"++++++++mouseReleaseEvent+++++++++";
        system("echo b > /data/processType");
        system("sync");
        reboot(LINUX_REBOOT_CMD_RESTART);
    }
}
