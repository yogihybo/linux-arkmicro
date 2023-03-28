#ifndef ONCALLWIDGET_H
#define ONCALLWIDGET_H

#include <QObject>
class OnCallWidgetPrivate;
class OnCallWidget : public QObject
{
    Q_OBJECT
public:
    explicit OnCallWidget(QObject *parent = nullptr);
    void setOnCallWidgetObject(QObject *qmlObject);
protected slots:
    void onConnectStatusChange(const int status);
    void onDialInfo(const int type,const QString& phone);
public slots:
    void onToolButtonRelease();
    void onTimeout();
    void onNumberBtnClicked(QString text);
private:
    OnCallWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(OnCallWidget)
};

#endif // ONCALLWIDGET_H
