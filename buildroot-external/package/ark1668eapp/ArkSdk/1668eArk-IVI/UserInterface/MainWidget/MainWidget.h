#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QObject>
#include <QScopedPointer>
#include "DiskWidget/MultiMediaWidget.h"
#include "ToolWidget/ToolWiget.h"
#include "DiskWidget/VideoWidget/VideoWidget.h"
#include "SettingWidget/SettingWidget.h"
#include "TelePhoneWidget/TelePhoneWidget.h"
#include "BusinessLogic/ark_api.h"
#include "PhoneLinkWidget/PhoneLinkWidget.h"
#include "HomeWidget/HomeWidget.h"
#include "KeyBoardModelData/KeyBoardWidget.h"
#include "AuxWidget/AuxWidget.h"
#include "BackWidget.h"
class MainWidgetPrivate;
class MainWidget : public QObject
{
    Q_OBJECT
public:
    explicit MainWidget(QObject *parent = nullptr);
public slots:
    void onActivated();
    void onTimeOut();
    void onDeviceWatcherStatus(const int type, const int status);
    void onConnectStatusChange(const int status);
    void onWidgetTypeChange(const int destinationType, const int requestType, const QString &status);
    void onPhoneLinkTelePhone(int calltype);
    void onLoaderCompleted();
public slots:
    void onCarbackStatusChange(int status);
    void onMousePressed(double globalX,double globalY);
    void onMouseRelease(double globalX,double globalY);
    void onMouseMove(double globalX,double globalY);
    void onMultiMediaComponentLoaderComplete();
    void onPhoneLinkComponentLoaderComplete();
    void onTelephoneComponentLoaderComplete();
    void onVideoMediaComponentLoaderComplete();
    void onSettingComponentLoaderComplete();
    void onKeyBoardComponentLoaderComplete();
    void onAuxComponentLoaderComplete();
    void onBackComponentLoaderComplete();
public:
    void setMainWidgetObject(QObject* qmlObject);
private:
    MainWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(MainWidget)
};

#endif // MAINWIDGET_H
