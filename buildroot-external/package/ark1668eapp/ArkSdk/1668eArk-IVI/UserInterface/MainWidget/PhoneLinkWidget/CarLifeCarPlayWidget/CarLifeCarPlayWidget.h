#ifndef CARLIFECARPLAYWIDGET_H
#define CARLIFECARPLAYWIDGET_H

#include <QObject>
class CarLifeCarPlayWidgetPrivate;
class CarLifeCarPlayWidget : public QObject
{
    Q_OBJECT
public:
    explicit CarLifeCarPlayWidget(QObject *parent = nullptr);
    void setCarLifeCarPlayWidgetObject(QObject *qmlObject);
    void setCarLifeCarPlayWidgetParendObject(QObject *qmlObject);
public slots:
    void onToolButtonRelease();
protected slots:
    void onLinkStatus(int type, int mode, int status);
    void onLinkDuckAudio(const int type, double durationSecs, double volume); //send carlink's audio duck volume
    void onLinkUnduckAudio(const int type, double durationSecs); //send carlink's audio unduck volume
    void onConnectStatusChange(int status);
private:
    CarLifeCarPlayWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(CarLifeCarPlayWidget)
};

#endif // CARLIFECARPLAYWIDGET_H
