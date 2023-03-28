#ifndef VOLUMESETTINGWIDGET_H
#define VOLUMESETTINGWIDGET_H

#include <QObject>
class VolumeSettingWidgetPrivate;
class VolumeSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit VolumeSettingWidget(QObject *parent = nullptr);
    void setVolumeSettingWidgetObject(QObject* qmlObject);
private:
    VolumeSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(VolumeSettingWidget)
};

#endif // VOLUMESETTINGWIDGET_H
