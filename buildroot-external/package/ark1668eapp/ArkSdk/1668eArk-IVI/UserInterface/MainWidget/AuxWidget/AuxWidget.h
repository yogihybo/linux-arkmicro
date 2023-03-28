#ifndef AUXWIDGET_H
#define AUXWIDGET_H

#include <QObject>
class AuxWidgetPrivate;
class AuxWidget : public QObject
{
    Q_OBJECT
public:
    explicit AuxWidget(QObject *parent = nullptr);
    void setAuxWidgetObject(QObject* qmlObject);
    void setAuxLoaderObject(QObject* qmlObject);
    QObject* getAuxWidgetObject();
    void setHthread(int value);
    void setVisibleStatus(bool status);
    bool getVisibleStatus();
    bool getHthreadExitStatus();
public slots:
    void onVisibleChanged();
    void onAuxWidgetClicked();
private:
    AuxWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AuxWidget)
};

#endif // AUXWIDGET_H
