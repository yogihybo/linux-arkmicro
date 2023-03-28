#ifndef QMLLAUNCHER_H
#define QMLLAUNCHER_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQmlContext>
#include "BusinessLogic/Setting.h"
class QmlLauncher : public QObject
{
    Q_OBJECT
public:
    explicit QmlLauncher(QObject *parent = nullptr);
    void qmlLauncherInit();
public slots:
    void onLanguageChanged();
private:
    void setStatusBarContextProperty();
private:
    QQmlApplicationEngine* m_Engine;
};

#endif // QMLLAUNCHER_H
