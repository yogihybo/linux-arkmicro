#ifndef CORNERLAMPWIDGET_H
#define CORNERLAMPWIDGET_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QLabel>
#include <QTimer>

class CornerLampWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit CornerLampWidget(QWidget *parent = nullptr);
    ~CornerLampWidget();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    //void paintEvent(QPaintEvent *e);
    void onTimeout();

private:
    QTimer *timer;
    QLabel *leftCorner, *rightCorner;
    QPixmap pixLeftCorner, pixRightCorner, pixCornerLampBg;
};

#endif // CORNERLAMPWIDGET_H
