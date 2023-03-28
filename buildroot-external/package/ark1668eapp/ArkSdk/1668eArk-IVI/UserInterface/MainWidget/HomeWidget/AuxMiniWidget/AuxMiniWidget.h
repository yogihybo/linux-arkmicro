#ifndef AUXMINIWIDGET_H
#define AUXMINIWIDGET_H

#include <QObject>
class AuxMiniWidgetPrivate;
class AuxMiniWidget : public QObject
{
    Q_OBJECT
public:
    explicit AuxMiniWidget(QObject *parent = nullptr);
    void setAuxMiniWidgetObject(QObject* qmlObject);
public slots:
    void onAuxWidgetClicked();
private:
    AuxMiniWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(AuxMiniWidget)
};

#endif // AUXMINIWIDGET_H
