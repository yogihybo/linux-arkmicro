#ifndef PHONELINKMSGWIDGET_H
#define PHONELINKMSGWIDGET_H

#include <QObject>
#include "BusinessLogic/Audio.h"
class PhoneLinkMsgWidgetPrivate;
class PhoneLinkMsgWidget : public QObject
{
    Q_OBJECT
public:
    explicit PhoneLinkMsgWidget(QObject *parent = nullptr);
    void setPhoneLinkMsgWidgetObject(QObject* qmlObject);
public slots:
    void onToolButtonRelease();
protected slots:
    void onLinkStatus(int type, int mode, int status);
    void onCarLinkVersion(const int type,  const QString ver);
    void onPhoneType(int type, int inserted);
    void onDateTime(const int type, const long long time);
    void onHolderChange(const AudioSource oldHolder, const AudioSource newHolder);
    void onTelephone(const int type, const QString name, const QString number);
private:
    PhoneLinkMsgWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(PhoneLinkMsgWidget)
};

#endif // PHONELINKMSGWIDGET_H
