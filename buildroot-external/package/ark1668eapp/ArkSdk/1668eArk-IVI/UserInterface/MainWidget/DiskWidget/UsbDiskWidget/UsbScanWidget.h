#ifndef USBSCANWIDGET_H
#define USBSCANWIDGET_H

#include <QObject>
class UsbScanWidgetPrivate;
class UsbScanWidget : public QObject
{
    Q_OBJECT
public:
    explicit UsbScanWidget(QObject *parent = nullptr);
    void setUsbScanWidgetObject(QObject* qmlObject);
    QObject* getUsbScanWidgetTmObject();
private:
    UsbScanWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(UsbScanWidget)
};

#endif // USBSCANWIDGET_H
