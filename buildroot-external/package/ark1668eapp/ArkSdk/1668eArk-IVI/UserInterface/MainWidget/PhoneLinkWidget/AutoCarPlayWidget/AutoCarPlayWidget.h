#ifndef AUTOCARPLAYWIDGET_H
#define AUTOCARPLAYWIDGET_H

#include <QObject>
class AutoCarPlayWidgetPrivate;
class AutoCarPlayWidget : public QObject
{
    Q_OBJECT
public:
    explicit AutoCarPlayWidget(QObject *parent = nullptr);
    void setAutoCarPlayWidgetObject(QObject *qmlObject);
    void setAutoCarPlayWidgetParendObject(QObject *qmlObject);
public slots:
    void onToolButtonRelease();
protected slots:
    void onLinkStatus(int type, int mode, int status);
private:
    AutoCarPlayWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AutoCarPlayWidget)
};

#endif // AUTOCARPLAYWIDGET_H
