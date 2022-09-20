#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QMouseEvent>
#include "carlink.h"
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
private:
    void onUIChanged(bool visible);
    void onUIInit();
private:
    Ui::MainWindow *ui;

    QPushButton* m_CarplayWiredLink;
    QPushButton* m_CarplayWirelessLink;
    QPushButton* m_AutoWiredLink;
    QPushButton* m_AutoWirelessLink;
    QPushButton* m_CarlifeWiredLink;
    QPushButton* m_CarlifeWirelessLink;
    QPushButton* m_HiCarWiredLink;
    QPushButton* m_HiCarWirelessLink;
    QPushButton* m_MirrorWiredLink;
    QPushButton* m_MirrorWirelessLink;
    QPushButton* m_EasyWiredConnectLink;
    QPushButton* m_EasyWirelessConnectLink;

    LinkType mLinkType;
    LinkMode mLinkMode;
    DbusSend mInserted;
    DbusSend mDbusSend;
    bool mIsRunningBackGround;
private slots:
    void onClicked();
    void onLinkStatus(int type, int mode, int status);
    void onCarLinkVersion(const int type,  const QString ver);
    void onPhoneType(int type, int inserted);
    void onDateTime(const int type, const long long time);
};

#endif // MAINWINDOW_H
