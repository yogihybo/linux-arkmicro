#ifndef DIALERWIDGET_H
#define DIALERWIDGET_H

#include <QObject>
class DialerWidgetPrivate;
class DialerWidget : public QObject
{
    Q_OBJECT
public:
    explicit DialerWidget(QObject *parent = nullptr);
    void setDialerWidgetObject(QObject* qmlObject);
protected slots:
    void onHangUpPhone();
    void onConnectStatusChange(const int status);
public slots:
    void onToolButtonRelease();
    void onListViewItemClicked(QString phoneNumber);
private:
    DialerWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(DialerWidget)
};

#endif // DIALERWIDGET_H
