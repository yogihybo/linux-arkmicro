#ifndef ABOUTSETTINGRESETWIDGET_H
#define ABOUTSETTINGRESETWIDGET_H

#include <QObject>
class AboutSettingResetWidgetPrivate;
class AboutSettingResetWidget : public QObject
{
    Q_OBJECT
public:
    explicit AboutSettingResetWidget(QObject *parent = nullptr);
    void setAboutSettingResetWidgetObject(QObject* qmlObject);
public slots:
    void onToolButtonRelease();
private:
    AboutSettingResetWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AboutSettingResetWidget)
};

#endif // ABOUTSETTINGRESETWIDGET_H
