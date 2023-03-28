#ifndef VOLUMEBALANCESETTINGWIDGET_H
#define VOLUMEBALANCESETTINGWIDGET_H

#include <QObject>
class VolumeBalanceSettingWidgetPrivate;
class VolumeBalanceSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit VolumeBalanceSettingWidget(QObject *parent = nullptr);
    void setVolumeBalanceSettingWidgetObjecgt(QObject *qmlObject);
public slots:
    void onMousePressed(double x,double y);
    void onMouseRelease(double x,double y);
    void onMouseMove(double x,double y);
    void onTimeout();
private:
    VolumeBalanceSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(VolumeBalanceSettingWidget)
};

#endif // VOLUMEBALANCESETTINGWIDGET_H
