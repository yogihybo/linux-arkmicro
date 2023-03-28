#ifndef MORESETTINGWIDGET_H
#define MORESETTINGWIDGET_H

#include <QObject>
class MoreSettingWidgetPrivate;
class MoreSettingWidget : public QObject
{
    Q_OBJECT
public:
    explicit MoreSettingWidget(QObject *parent = nullptr);
    void setMoreSettingWidgetObject(QObject* qmlObject);
public slots:
    void onToolButtonRelease();
    void onTimeout();
    void onVisibleChanged();
    void onSimpChineseBtnClicked();
    void onEnglishBtnBtnClicked();
    void onTraditionalChineseBtnClicked();
    //void onDataTimeSetting();
private:
    MoreSettingWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MoreSettingWidget)
};

#endif // MORESETTINGWIDGET_H
