#ifndef HICARWIDGET_H
#define HICARWIDGET_H

#include <QObject>
#include <string>
#include "BusinessLogic/Audio.h"
using namespace std;
class HicarWidgetPrivate;
class HicarWidget : public QObject
{
    Q_OBJECT
public:
    explicit HicarWidget(QObject *parent = nullptr);
    void setHicarWidgetObject(QObject* qmlObject);
    void setHicarWidgetParentObject(QObject* qmlObject);
public slots:
    void onToolButtonRelease();
    void onTimeout();
protected slots:
    void onLinkStatus(int type, int mode, int status);
    void onPinCode(const int type, const QString pincode); //send carlink's pincode(hicar)
    void onBlueToothCmd(const int type, const QString cmd);
    void onPhoneType(int type, int inserted);
    void onCarLinkInitDone(const int type);
    void onHolderChange(const AudioSource oldHolder, const AudioSource newHolder);
private:
    HicarWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(HicarWidget)
};

#endif // HICARWIDGET_H
