#ifndef VOLUMEVALUESETTINGWIDGET_H
#define VOLUMEVALUESETTINGWIDGET_H

#include <QObject>
#include "BusinessLogic/Audio.h"
class VolumeValueSettingWidgetPrivate;
class VolumeValueSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit VolumeValueSettingWidget(QObject *parent = nullptr);
    void setVolumeValueSettingWidgetObject(QObject* qmlObject);
protected slots:
    void onHolderChange(const AudioSource oldHolder, const AudioSource newHolder);
public slots:
    void onValueChanged();
    void onMediaSliderMoveFinish();
    void onVisibleChanged();
    void onNavigationMoveFinish();
    void onTelephoneMoveFinish();
private:
    VolumeValueSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(VolumeValueSettingWidget)
};

#endif // VOLUMEVALUESETTINGWIDGET_H
