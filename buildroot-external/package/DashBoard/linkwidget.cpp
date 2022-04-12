#include "linkwidget.h"
#include <QPainter>
#include "BusinessLogic/carback.h"
#include <QDebug>


LinkWidget *pthis = NULL;

LinkWidget::LinkWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    m_ReverseStatus = RS_Off;
    m_USBStatus = US_PullOut;
    pthis = this;
    pixBackground.load(":/images/LinkBackground.png");
    m_Timer = new QTimer(this);
    m_Timer->setInterval(3000); //Elink startup too fast will cause Segmentation fault
    m_Timer->setSingleShot(true);
    QObject::connect(m_Timer, SIGNAL(timeout()),
                     this,    SLOT(onTimeout()));
    m_Timer->start();
    qDebug() << "***************Timer start!!!";
    QObject::connect(g_Carback, &Carback::CarbackStatusChange, this, &LinkWidget::onCarbackStatusChange);
}

LinkWidget::~LinkWidget()
{
#ifdef ECLINK
    delete m_ECLinkplayer;
#endif
}

void LinkWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0,0.0,0.0,0.0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glEnable(GL_BLEND);
}

void LinkWidget::resizeGL(int width, int height)
{

}

void LinkWidget::paintGL()
{
//    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "######## m_USBStatus:" << m_USBStatus;
//    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "######## m_ReverseStatus:" << m_ReverseStatus;
    if ((m_USBStatus == US_PullOut) && (m_ReverseStatus == RS_Off)) {
        QPainter painter;
        painter.begin(this);
        painter.drawPixmap(0, 0, pixBackground);
        painter.end();
//        qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "pixBackground";
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//清除屏幕和深度缓存
//        qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "glClearBackground";
    }
}

void LinkWidget::usbscan(bool status)
{
    qDebug() << "********************** USB Status:" << status;
    if (status) {
        pthis->m_USBStatus = US_Inset;
    } else {
        pthis->m_USBStatus = US_PullOut;
    }
    pthis->update();
}

void LinkWidget::onTimeout()
{
#ifdef ECLINK
    m_ECLinkplayer = new ECLinkplayer();
    qDebug() << "ECLinkplayer111 :";
    m_ECLinkplayer->register_usbevent(usbscan);
    qDebug() << "ECLinkplayer222";
    m_ECLinkplayer->init(654, 214, 616, 436);
    qDebug() << "ECLinkplayer333";
#endif
}

void LinkWidget::onCarbackStatusChange(int status)
{
    qDebug() << __PRETTY_FUNCTION__ << __LINE__ << "########### ReverseStatus:" << status;
    if (status) {
        m_ReverseStatus = RS_On;
    } else {
        m_ReverseStatus = RS_Off;
    }
    update();
}
