#ifndef ABOUTSETTINGNATIVEINFOWIDGET_H
#define ABOUTSETTINGNATIVEINFOWIDGET_H

#include <QObject>
class AboutSettingNativeInfoWidgetPrivate;
class AboutSettingNativeInfoWidget : public QObject
{
    Q_OBJECT
public:
    explicit AboutSettingNativeInfoWidget(QObject *parent = nullptr);
    void setAboutSettingNativeInfoWidgetObject(QObject* qmlObject);
public slots:
    void onVisibleChanged();
private:
    AboutSettingNativeInfoWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AboutSettingNativeInfoWidget)
};

#endif // ABOUTSETTINGNATIVEINFOWIDGET_H
