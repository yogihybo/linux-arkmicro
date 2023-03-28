#ifndef BRIGHTNESSSETTINGWIDGET_H
#define BRIGHTNESSSETTINGWIDGET_H

#include <QObject>
class BrightnessSettingWidgetPrivate;
class BrightnessSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit BrightnessSettingWidget(QObject *parent = nullptr);
    void setBrightnessSettingWidgetObject(QObject *qmlObject);
public slots:
    void onValueChanged();
    void onSliderMoveFinish();
private:
    BrightnessSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(BrightnessSettingWidget)
};

#endif // BRIGHTNESSSETTINGWIDGET_H
