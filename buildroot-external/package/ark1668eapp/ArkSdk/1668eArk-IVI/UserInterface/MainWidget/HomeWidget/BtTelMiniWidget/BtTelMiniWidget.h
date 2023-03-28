#ifndef BTTELMINIWIDGET_H
#define BTTELMINIWIDGET_H

#include <QObject>
class BtTelMiniWidgetPrivate;
class BtTelMiniWidget : public QObject
{
    Q_OBJECT
public:
    explicit BtTelMiniWidget(QObject *parent = nullptr);
    void setBtTelMiniWidgetObject(QObject* qmlObject);
protected slots:
    void onDialInfo(const int type,const QString& phone);
    void onConnectStatusChange(const int status);
public slots:
    void onToolButtonRelease();
private:
    BtTelMiniWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(BtTelMiniWidget)
};

#endif // BTTELMINIWIDGET_H
