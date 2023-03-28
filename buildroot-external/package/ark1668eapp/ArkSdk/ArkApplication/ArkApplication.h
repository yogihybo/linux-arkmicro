#ifndef ARKAPPLICATION_H
#define ARKAPPLICATION_H
#include <QGuiApplication>
#include <QScopedPointer>
#include <QDateTime>

#if defined(ArkApp)
#undef ArkApp
#endif
#define ArkApp (static_cast<ArkApplication *>(QCoreApplication::instance()))

class ArkApplicationPrivate;
class ArkApplication
        : public QGuiApplication
{
    Q_OBJECT
    Q_DISABLE_COPY(ArkApplication)
public:
    explicit ArkApplication(int &argc, char **argv);
    ~ArkApplication();
    bool installTranslatorPath(const QString& path);
    int size() const;
    QStringList arguments() const;
    void startTimer(const int second = 1);
    void resetTimer(const int second = 1);
    void setCurrentTime(const QDateTime dateTime);
public slots:
    void onTimeout();
signals:
    void currentDateTime(const QDateTime &dateTime);
private:
    friend class ArkApplicationPrivate;
    QScopedPointer<ArkApplicationPrivate> m_Private;
};

#endif // ARKAPPLICATION_H
