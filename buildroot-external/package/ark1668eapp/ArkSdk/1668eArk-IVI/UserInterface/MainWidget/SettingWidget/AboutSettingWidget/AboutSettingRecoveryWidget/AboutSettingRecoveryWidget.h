#ifndef ABOUTSETTINGRECOVERYWIDGET_H
#define ABOUTSETTINGRECOVERYWIDGET_H

#include <QObject>
class AboutSettingRecoveryWidgetPrivate;
class AboutSettingRecoveryWidget : public QObject
{
    Q_OBJECT
public:
    explicit AboutSettingRecoveryWidget(QObject *parent = nullptr);
    void setAboutSettingRecoveryWidgetObject(QObject *qmlObject);
public slots:
    void onToolButtonRelease();
private:
    AboutSettingRecoveryWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AboutSettingRecoveryWidget)
};

#endif // ABOUTSETTINGRECOVERYWIDGET_H
