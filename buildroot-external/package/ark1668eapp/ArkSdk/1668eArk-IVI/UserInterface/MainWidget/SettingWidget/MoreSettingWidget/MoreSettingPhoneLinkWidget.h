#ifndef MORESETTINGPHONELINKWIDGET_H
#define MORESETTINGPHONELINKWIDGET_H

#include <QObject>
class MoreSettingPhoneLinkWidgetPrivate;
class MoreSettingPhoneLinkWidget : public QObject
{
    Q_OBJECT
public:
    explicit MoreSettingPhoneLinkWidget(QObject *parent = nullptr);
    void  setMoreSettingPhoneLinkWidgetObject(QObject *qmlObject);
public slots:
    void onToolButtonRelease();
    void onVisibleChanged();
private:
    MoreSettingPhoneLinkWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MoreSettingPhoneLinkWidget)
};

#endif // MORESETTINGPHONELINKWIDGET_H
