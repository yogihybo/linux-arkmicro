#ifndef BACKWIDGET_H
#define BACKWIDGET_H

#include <QObject>
class BackWidgetPrivate;
class BackWidget : public QObject
{
    Q_OBJECT
public:
    explicit BackWidget(QObject *parent = nullptr);
    void setBackWidgetObject(QObject* qmlObject);
    void setBackLoaderObject(QObject *qmlObject);
    void setWidgetType(int widgetType);
public slots:
    void onClicked();
    void onTimeout();
    void onVisibleChanged();
private:
    BackWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(BackWidget)
};

#endif // BACKWIDGET_H
