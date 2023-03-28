#ifndef MORESETTINGDATATIMEWIDGET_H
#define MORESETTINGDATATIMEWIDGET_H

#include <QObject>
class MoreSettingDataTimeWidgetPrivate;
class MoreSettingDataTimeWidget : public QObject
{
    Q_OBJECT
public:
    explicit MoreSettingDataTimeWidget(QObject *parent = nullptr);
    void setMoreSettingDataTimeWidgetObject(QObject *qmlObject);
public slots:
    void onToolButtonRelease();
private:
    MoreSettingDataTimeWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MoreSettingDataTimeWidget)
};

#endif // MORESETTINGDATATIMEWIDGET_H
