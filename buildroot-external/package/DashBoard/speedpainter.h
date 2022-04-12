#ifndef SPEEDPAINTER_H
#define SPEEDPAINTER_H

#include <QWidget>
#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <QLabel>

class SpeedPainter : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit SpeedPainter(QWidget *parent = nullptr);

public slots:
    void slotUpdate();

protected:
     void initializeGL() override;
     void resizeGL(int width, int height) override;
     void paintGL() override;
public:
     QTimer *timer;
     QPixmap pix,pointer;
     int LogoPixNumber = 1;
     int Logo_a = 1;
     float angle;
     //int last_angle;
     bool flag;
};

#endif // SPEEDPAINTER_H
