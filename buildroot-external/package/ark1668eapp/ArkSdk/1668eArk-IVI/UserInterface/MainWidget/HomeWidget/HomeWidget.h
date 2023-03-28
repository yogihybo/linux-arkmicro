#ifndef HOMEWIDGET_H
#define HOMEWIDGET_H

#include <QObject>
class HomeWidgetPrivate;
class HomeWidget : public QObject
{
    Q_OBJECT
public:
    explicit HomeWidget(QObject *parent = nullptr);
    void setHomeWidgetObject(QObject* qmlObject);
private:
    HomeWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(HomeWidget)
};

#endif // HOMEWIDGET_H
