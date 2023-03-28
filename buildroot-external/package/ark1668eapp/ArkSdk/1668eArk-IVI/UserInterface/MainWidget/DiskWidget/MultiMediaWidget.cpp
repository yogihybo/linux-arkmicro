#include "MultiMediaWidget.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Multimedia.h"
#include "./UsbDiskWidget/UsbScanWidget.h"
#include "SdDiskWidget/SdScanWidget.h"
#include "./MultiMediaPlayWIdget/MultiMediaPlayWidget.h"
#include "./UsbDiskWidget/UsbListModelData.h"
#include "BusinessLogic/Bluetooth.h"
#include "BusinessLogic/Audio.h"
#include <QQmlProperty>
#include <QQuickItem>
class MultiMediaWidgetPrivate
{
    Q_DISABLE_COPY(MultiMediaWidgetPrivate)
public:
    explicit MultiMediaWidgetPrivate(MultiMediaWidget* parent);
    ~MultiMediaWidgetPrivate();
    void initializeUsbScanWidget();
    void initializeSdScanWidget();
    void initializeMultiMediaPlayWidget();
    void connectAllSlots();
public:
    DeviceWatcherStatus m_DeviceWatcherStatus;
    QObject*  m_MultiMediaWidgetObject;
    UsbScanWidget* m_UsbScanWidget;
    SdScanWidget*  m_SdScanWidget;
    MultiMediaPlayWidget* m_MultiMediaPlayWidget;
    int  m_UsbType;
    int  m_SdType;
    bool m_BtLastConnectStatus;
private:
    Q_DECLARE_PUBLIC(MultiMediaWidget)
    MultiMediaWidget* const q_ptr;
};
MultiMediaWidget::MultiMediaWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new MultiMediaWidgetPrivate(this))
{

}
void MultiMediaWidget::setMultiMediaWidgetObject(QObject* qmlObject)
{
    Q_D(MultiMediaWidget);
    if(d->m_MultiMediaWidgetObject == NULL)
    {
        d->m_MultiMediaWidgetObject = qmlObject;
    }
    d->initializeMultiMediaPlayWidget();
    d->connectAllSlots();
}
void MultiMediaWidget::onDeviceWatcherStatus(const int type, const int status){
    Q_D(MultiMediaWidget);
    if (DWT_USBDisk == type) {
        d->m_DeviceWatcherStatus = status;
        switch (status) {
        case DWS_Empty: {
            d->m_UsbType = UsbNotConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(0);
            break;
        }
        case DWS_Unsupport: {
            d->m_UsbType = UsbNotConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(0);
            break;
        }
        case DWS_Busy: {
            d->m_UsbType = UsbScaning;
            d->initializeUsbScanWidget();
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(1);
            QObject* UsbScanWidgetTmObject =  d->m_UsbScanWidget->getUsbScanWidgetTmObject();
            QQmlProperty(UsbScanWidgetTmObject,"running").write(true);
            break;
        }
        case DWS_Ready: {
            d->m_UsbType = UsbConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(2);
            QObject* UsbScanWidgetTmObject =  d->m_UsbScanWidget->getUsbScanWidgetTmObject();
            QQmlProperty(UsbScanWidgetTmObject,"running").write(false);
            if(d->m_MultiMediaPlayWidget != NULL)
            {
                QObject* usbListWidgetObject  = d->m_MultiMediaWidgetObject->findChild<QObject*>("usbListWidgetObject");
                d->m_MultiMediaPlayWidget->setUsbMusicListviewObject(usbListWidgetObject);
            }
            if(d->m_MultiMediaPlayWidget != NULL)
            {
                d->m_MultiMediaPlayWidget->startMusicTypeTimer();
            }
            break;
        }
        case DWS_Remove: {
            d->m_UsbType = UsbNotConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(0);
            QQmlProperty(d->m_MultiMediaWidgetObject,"usbWidgetType").write(0);
            if(d->m_MultiMediaPlayWidget != NULL)
            {
                d->m_MultiMediaPlayWidget->startMusicTypeTimer();
            }
            break;
        }
        default: {
            break;
        }
        }
    }

    if (DWT_SDDisk == type) {
        d->m_DeviceWatcherStatus = status;
        switch (status) {
        case DWS_Empty: {
            d->m_SdType = SdNotConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(3);
            break;
        }
        case DWS_Unsupport: {
            d->m_SdType = SdNotConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(3);
            break;
        }
        case DWS_Busy: {
            d->m_SdType = SdScaning;
            d->initializeSdScanWidget();
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(4);
            QObject* SdScanWidgetTmObject =  d->m_SdScanWidget->getSdScanWidgetTmObject();
            QQmlProperty(SdScanWidgetTmObject,"running").write(true);
            break;
        }
        case DWS_Ready: {
            d->m_SdType = SdConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(5);
            QObject* SdScanWidgetTmObject =  d->m_SdScanWidget->getSdScanWidgetTmObject();
            QQmlProperty(SdScanWidgetTmObject,"running").write(false);
            if(d->m_MultiMediaPlayWidget != NULL)
            {
                QObject* sdListWidgetObject  = d->m_MultiMediaWidgetObject->findChild<QObject*>("sdListWidgetObject");
                d->m_MultiMediaPlayWidget->setSdMusicListviewObject(sdListWidgetObject);
            }
            if(d->m_MultiMediaPlayWidget != NULL)
            {
                d->m_MultiMediaPlayWidget->startMusicTypeTimer();
            }
            break;
        }
        case DWS_Remove: {
            d->m_SdType = SdNotConnect;
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(3);
            QQmlProperty(d->m_MultiMediaWidgetObject,"sdWidgetType").write(0);
            if(d->m_MultiMediaPlayWidget != NULL)
            {
                d->m_MultiMediaPlayWidget->startMusicTypeTimer();
            }
            break;
        }
        default: {
            break;
        }
        }
    }
}
//BT INFO
void MultiMediaWidget::onConnectStatusChange(const int status){
    Q_D(MultiMediaWidget);
    if(status >= 3)
    {
        d->m_BtLastConnectStatus  = true;
        if(d->m_MultiMediaWidgetObject != NULL)
        {
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(7);
        }

    }
    else{
        if(d->m_MultiMediaWidgetObject != NULL)
        {
            if(d->m_BtLastConnectStatus  == true)
            {
                d->m_BtLastConnectStatus = false;
                QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(6);
            }
        }
    }

    if(d->m_MultiMediaPlayWidget != NULL)
    {
        d->m_MultiMediaPlayWidget->startMusicTypeTimer();
    }
}

void MultiMediaWidget::onMusicStatusChange(const QString& musicName, const int status)
{
    Q_D(MultiMediaWidget);
    if(Bluetooth::BtMusic_Playing == status){
        if(d->m_MultiMediaWidgetObject != NULL)
        {
            QQmlProperty(d->m_MultiMediaWidgetObject,"mutilMediaType").write(7);
        }
    }
}


MultiMediaWidgetPrivate::MultiMediaWidgetPrivate(MultiMediaWidget *parent)
    : q_ptr(parent)
{
    m_DeviceWatcherStatus = DWS_Empty;
    m_UsbScanWidget = NULL;
    m_SdScanWidget  = NULL;
    m_MultiMediaPlayWidget = NULL;
    m_MultiMediaWidgetObject = NULL;
    m_UsbType = UsbUndefine;
    m_SdType  =  UsbUndefine;
    m_BtLastConnectStatus = false;
}
MultiMediaWidgetPrivate::~MultiMediaWidgetPrivate()
{


}

void MultiMediaWidgetPrivate::initializeUsbScanWidget()
{
    Q_Q(MultiMediaWidget);
    if(m_UsbScanWidget == NULL)
    {
        m_UsbScanWidget = new UsbScanWidget(q);
        QObject* UsbScanWidgetObject  = m_MultiMediaWidgetObject->findChild<QObject*>("usbScanWidgetObject");
        m_UsbScanWidget->setUsbScanWidgetObject(UsbScanWidgetObject);
    }
}

void MultiMediaWidgetPrivate::initializeSdScanWidget()
{
    Q_Q(MultiMediaWidget);
    if(m_SdScanWidget == NULL)
    {
        m_SdScanWidget = new SdScanWidget(q);
        QObject* SdScanWidgetObject  = m_MultiMediaWidgetObject->findChild<QObject*>("sdScanWidgetObject");
        m_SdScanWidget->setSdScanWidgetObject(SdScanWidgetObject);
    }
}

void MultiMediaWidgetPrivate::initializeMultiMediaPlayWidget()
{
    Q_Q(MultiMediaWidget);
    if(m_MultiMediaPlayWidget == NULL)
    {
        m_MultiMediaPlayWidget = new MultiMediaPlayWidget(q);
        QObject* MultiMediaPlayWidgetObject  = m_MultiMediaWidgetObject->findChild<QObject*>("multiMediaPlayWidgetObject");
        m_MultiMediaPlayWidget->setMultiMediaPlayWidgetObject(MultiMediaPlayWidgetObject);
        m_MultiMediaPlayWidget->setMultiMediaObject(m_MultiMediaWidgetObject);
    }
}
void MultiMediaWidgetPrivate::connectAllSlots()
{
    Q_Q(MultiMediaWidget);
    connectSignalAndSlotByNamesake(g_Multimedia, q, ARKRECEIVER(onDeviceWatcherStatus(const int, const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onMusicStatusChange(const QString& , const int)));
}
