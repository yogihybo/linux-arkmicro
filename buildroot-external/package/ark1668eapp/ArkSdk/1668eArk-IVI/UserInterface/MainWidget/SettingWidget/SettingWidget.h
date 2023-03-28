#ifndef SETTINGWIDGET_H
#define SETTINGWIDGET_H

#include <QObject>
#include "BluetoothSettingWidget/BluetoothSettingWidget.h"
#include "VolumeSettingWidget.cpp/VolumeSettingWidget.h"
#include "MoreSettingWidget/MoreSettingWidget.h"
#include "BrightnessSettingWidget/BrightnessSettingWidget.h"
#include "AboutSettingWidget/AboutSettingWidget.h"
#include "WifiSettingWidget/WifiSettingWidget.h"
class SettingWidgetPrivate;
class SettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit SettingWidget(QObject *parent = nullptr);
    void setSettingWidgetObject(QObject *qmlObject);
private:
    SettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(SettingWidget)
};

#endif // SETTINGWIDGET_H
