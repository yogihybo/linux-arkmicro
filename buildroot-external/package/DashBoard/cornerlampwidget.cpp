#include "cornerlampwidget.h"
#include <QPainter>
#include <QDebug>

CornerLampWidget::CornerLampWidget(QWidget *parent) : QOpenGLWidget(parent)
{
    pixLeftCorner.load(":/images/LeftCornerActivited.png");
    pixRightCorner.load(":/images/RightCornerAcitivated.png");
    pixCornerLampBg.load(":/images/CornerLampBg.png");

    timer = new QTimer(this);
    timer->setInterval(500);
    QObject::connect(timer, &QTimer::timeout, this, &CornerLampWidget::onTimeout);
    timer->start();
}

CornerLampWidget::~CornerLampWidget()
{
    delete timer;
    delete leftCorner;
    delete rightCorner;
}

void CornerLampWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0,0.0,0.0,0.0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glEnable(GL_BLEND);
}

void CornerLampWidget::resizeGL(int width, int height)
{

}

void CornerLampWidget::paintGL()
{
    static bool flag = true;
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
    painter.drawPixmap(0, 0, 400, 52, pixCornerLampBg);
    if (flag) {
        painter.drawPixmap(0, 0, 68, 52, pixLeftCorner);
        painter.drawPixmap(304, 0, 68, 52, pixRightCorner);
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform,false);
    painter.end();
    flag = !flag;
}

//void CornerLampWidget::paintEvent(QPaintEvent *e)
//{
//    qDebug() << "CornerLampWidget::paintEvent";
//}

void CornerLampWidget::onTimeout()
{
    update();
}
