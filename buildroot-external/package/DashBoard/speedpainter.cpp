#include "speedpainter.h"
#include <QTimer>
#include <QPainter>
#include <QPropertyAnimation>
#include <QtDebug>

#define EDGEANGLE 126.0
#define ANGLE 1

SpeedPainter::SpeedPainter(QWidget *parent) : QOpenGLWidget(parent)
{
    angle = -126.0;
    //angle=0;
    //last_angle = 0;
    flag = true;
    pix = QPixmap(":/images/SpeedBackground.png");
    pointer = QPixmap(":/images/needle.png");
    timer = new QTimer(this);
    connect(timer,SIGNAL(timeout()),this,SLOT(slotUpdate()));
    timer->start(30);
}

void SpeedPainter::slotUpdate()
{
//    qDebug()<<"LogoPainter::angle:"<<(angle + 360 - last_angle)%360;
//    last_angle = angle;
    if (angle <= -EDGEANGLE) {
        angle = -EDGEANGLE;
        flag = true;
    } else if (angle >= EDGEANGLE) {
        angle = EDGEANGLE;
        flag = false;
    }
    if (flag) {
        angle += ANGLE;
    } else {
        angle -= ANGLE;
    }
    update();
}

void SpeedPainter::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0,0.0,0.0,0.0);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glEnable(GL_BLEND);
}

void SpeedPainter::resizeGL(int width, int height)
{

}

void SpeedPainter::paintGL()
{
    QPainter painter;
    painter.begin(this);
    painter.fillRect(QRect(0,0,width(),height()), Qt::black);
    painter.drawPixmap(0,0,pix);
    painter.translate(335,374);  //移动到一个圆心
    //painter.rotate(angle++%360);  //旋转一定角度
    painter.rotate(angle);  //旋转一定角度
    painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
    painter.drawPixmap(-10,-256,pointer);
    painter.setRenderHint(QPainter::SmoothPixmapTransform,false);
    painter.end();
}
