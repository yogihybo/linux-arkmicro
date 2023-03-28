#ifndef TELEPHONEWIDGET_H
#define TELEPHONEWIDGET_H

#include <QObject>
class TelePhoneWidgetPrivate;
class TelePhoneWidget : public QObject
{
    Q_OBJECT
public:
    explicit TelePhoneWidget(QObject *parent = nullptr);
    void setTelePhoneWidgetObject(QObject* qmlObject);
protected slots:
    void onConnectStatusChange(const int status);
    void onSyncPhoneBook();
    void onSyncAllCallLog();
public slots:
    void onToolButtonRelease();
private:
    TelePhoneWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(TelePhoneWidget)
};

#endif // TELEPHONEWIDGET_H
