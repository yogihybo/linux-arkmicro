#ifndef SDSCANWIDGET_H
#define SDSCANWIDGET_H

#include <QObject>
class SdScanWidgetPrivate;
class SdScanWidget : public QObject
{
    Q_OBJECT
public:
    explicit SdScanWidget(QObject *parent = nullptr);
    void setSdScanWidgetObject(QObject* qmlObject);
    QObject* getSdScanWidgetTmObject();
private:
    SdScanWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(SdScanWidget)
};

#endif // SDSCANWIDGET_H
