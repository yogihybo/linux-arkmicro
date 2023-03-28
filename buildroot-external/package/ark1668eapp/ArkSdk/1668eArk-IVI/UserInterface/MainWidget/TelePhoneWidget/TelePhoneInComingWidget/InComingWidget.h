#ifndef INCOMINGWIDGET_H
#define INCOMINGWIDGET_H

#include <QObject>
class InComingWidgetPrivate;
class InComingWidget : public QObject
{
    Q_OBJECT
public:
    explicit InComingWidget(QObject *parent = nullptr);
    void setInComingWidgetObject(QObject* qmlObject);
protected slots:
    void onDialInfo(const int type,const QString& phone);
    void onConnectStatusChange(const int status);
public slots:
    void onToolButtonRelease();
    void onTimeout();
private:
    InComingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(InComingWidget)
};

#endif // INCOMINGWIDGET_H
