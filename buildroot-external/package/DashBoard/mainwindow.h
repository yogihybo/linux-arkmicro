#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <logopainter.h>
#include "speedpainter.h"
#include "cornerlampwidget.h"
#include "linkwidget.h"
#include <QLabel>
#include <QTimer>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *event);

private:
    QLabel *m_Background;
    QPixmap pixBackground;
    LogoPainter *logoPainter;
    SpeedPainter *speedPainter;
    CornerLampWidget *m_CornerLampWidget;
    LinkWidget *m_LinkWidget;
};

#endif // MAINWINDOW_H
