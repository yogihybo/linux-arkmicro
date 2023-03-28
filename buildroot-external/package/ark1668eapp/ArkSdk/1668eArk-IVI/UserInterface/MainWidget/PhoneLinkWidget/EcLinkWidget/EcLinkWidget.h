#ifndef ECLINKWIDGET_H
#define ECLINKWIDGET_H

#include <QObject>
class EcLinkWidgetPrivate;
class EcLinkWidget : public QObject
{
    Q_OBJECT
public:
    explicit EcLinkWidget(QObject *parent = nullptr);
    void setEcLinkWidgetObject(QObject* qmlObject);
    void setEcLinkWidgetParentObject(QObject* qmlObject);
public slots:
    void onToolButtonRelease();
protected slots:
    void onLinkStatus(int type, int mode, int status);
private:
    EcLinkWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(EcLinkWidget)
};

#endif // ECLINKWIDGET_H
