#include "UsbScanWidget.h"

class UsbScanWidgetPrivate
{
    Q_DISABLE_COPY(UsbScanWidgetPrivate)
public:
    explicit UsbScanWidgetPrivate(UsbScanWidget* parent);
    ~UsbScanWidgetPrivate();
public:
    QObject* m_UsbScanWidgetObject;
    QObject* m_UsbScanWidgetTmObject;
private:
    Q_DECLARE_PUBLIC(UsbScanWidget)
    UsbScanWidget* const q_ptr;
};


UsbScanWidget::UsbScanWidget(QObject *parent)
    : QObject(parent),
      d_ptr(new UsbScanWidgetPrivate(this))
{

}
void UsbScanWidget::setUsbScanWidgetObject(QObject* qmlObject)
{
    Q_D(UsbScanWidget);
    if(d->m_UsbScanWidgetObject == NULL)
    {
        d->m_UsbScanWidgetObject = qmlObject;
    }
    if(d->m_UsbScanWidgetObject != NULL)
    {
        d->m_UsbScanWidgetTmObject = d->m_UsbScanWidgetObject->findChild<QObject*>("timeObject");
    }
}

QObject* UsbScanWidget::getUsbScanWidgetTmObject()
{
    Q_D(UsbScanWidget);
    if(d->m_UsbScanWidgetTmObject != NULL)
    {
        return d->m_UsbScanWidgetTmObject;
    }

}

UsbScanWidgetPrivate::UsbScanWidgetPrivate(UsbScanWidget *parent)
    : q_ptr(parent)
{
    m_UsbScanWidgetObject = NULL;
    m_UsbScanWidgetTmObject = NULL;
}
UsbScanWidgetPrivate::~UsbScanWidgetPrivate()
{

}

