#ifndef ABOUTSETTINGWIDGET_H
#define ABOUTSETTINGWIDGET_H

#include <QObject>
class AboutSettingWidgetPrivate;
class AboutSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit AboutSettingWidget(QObject *parent = nullptr);
    void setAboutSettingWidgetObject(QObject* qmlObject);
private:
    AboutSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AboutSettingWidget)
};

#endif // ABOUTSETTINGWIDGET_H
