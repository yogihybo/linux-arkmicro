#include "SdScanWidget.h"
class SdScanWidgetPrivate
{
    Q_DISABLE_COPY(SdScanWidgetPrivate)
public:
    explicit SdScanWidgetPrivate(SdScanWidget* parent);
    ~SdScanWidgetPrivate();
    QObject* m_SdScanWidgetObject;
    QObject* m_SdScanWidgetTmObject;
private:
    Q_DECLARE_PUBLIC(SdScanWidget)
    SdScanWidget* const q_ptr;
};

SdScanWidget::SdScanWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new SdScanWidgetPrivate(this))
{

}

void SdScanWidget::setSdScanWidgetObject(QObject* qmlObject)
{
    Q_D(SdScanWidget);
    if(d->m_SdScanWidgetObject == NULL)
    {
        d->m_SdScanWidgetObject = qmlObject;
    }
    if(d->m_SdScanWidgetObject != NULL)
    {
        d->m_SdScanWidgetTmObject = d->m_SdScanWidgetObject->findChild<QObject*>("timeObject");
    }
}

QObject* SdScanWidget::getSdScanWidgetTmObject()
{
    Q_D(SdScanWidget);
    if(d->m_SdScanWidgetTmObject != NULL)
    {
        return d->m_SdScanWidgetTmObject;
    }

}
SdScanWidgetPrivate::SdScanWidgetPrivate(SdScanWidget *parent)
    : q_ptr(parent)
{
    m_SdScanWidgetObject = NULL;
    m_SdScanWidgetTmObject= NULL;
}
SdScanWidgetPrivate::~SdScanWidgetPrivate()
{

}
