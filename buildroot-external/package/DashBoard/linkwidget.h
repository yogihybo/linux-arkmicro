#ifndef LINKWIDGET_H
#define LINKWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include "BusinessLogic/eclib/include/eclinkplayer.h"
#include <QTimer>

class LinkWidget  : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    enum ReverseStatus {
      RS_Undefine = -1,
      RS_Off,
      RS_On
    };
    enum USBStatus {
      US_Undefine = -1,
      US_PullOut,
      US_Inset,
    };
    explicit LinkWidget(QWidget *parent = nullptr);
    ~LinkWidget();

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    static void usbscan(bool status);
public slots:
    void onCarbackStatusChange(int status);
    void onTimeout();
private:
    QPixmap pixBackground;
    QLabel* m_Label;
    int m_ReverseStatus;
    int m_USBStatus;
    ECLinkplayer *m_ECLinkplayer;
    QTimer *m_Timer;
};

#endif // LINKWIDGET_H
