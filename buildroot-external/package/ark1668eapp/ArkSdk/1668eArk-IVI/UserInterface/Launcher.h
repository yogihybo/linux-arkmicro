#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QObject>
#include <QScopedPointer>
class LauncherPrivate;
class Launcher : public QObject
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent = nullptr);
public:
    void setLauncherObject(QObject* qmlObject);
public slots:
    void onLoaderCompleted();
private:
    LauncherPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(Launcher)
};

#endif // LAUNCHER_H
