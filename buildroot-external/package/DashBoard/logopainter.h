#ifndef LOGOPAINTER_H
#define LOGOPAINTER_H

#include <QObject>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <QLabel>


class LogoPainter : public QOpenGLWidget,protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit LogoPainter(QWidget *parent = nullptr);

public slots:
    void slotUpdate();

protected:
     void initializeGL() override;
     void resizeGL(int width, int height) override;
     void paintGL() override;
public:
     QTimer *timer;
     QPixmap pix,pointer;
     float angle;
     //int last_angle;
     bool flag;
};

#endif // LOGOPAINTER_H
