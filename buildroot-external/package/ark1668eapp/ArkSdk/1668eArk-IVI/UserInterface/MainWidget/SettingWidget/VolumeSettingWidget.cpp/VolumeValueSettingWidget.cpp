#include "VolumeValueSettingWidget.h"
#include "BusinessLogic/Setting.h"
#include "BusinessLogic/Multimedia.h"
#include "AudioPersistent.h"
#include "AudioService.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <QTimer>
class VolumeValueSettingWidgetPrivate
{
    Q_DISABLE_COPY(VolumeValueSettingWidgetPrivate)
public:
    explicit VolumeValueSettingWidgetPrivate(VolumeValueSettingWidget* parent);
    ~VolumeValueSettingWidgetPrivate();
    void initializeObject();
public:
    QObject* m_VolumeValueSettingWidgetObject;
    QObject* m_NavigationSliderObject;
    QObject* m_TelephoneSliderObject;
    QObject* m_MediaSliderObject;
    bool   m_InitVolume;
private:
    Q_DECLARE_PUBLIC(VolumeValueSettingWidget)
    VolumeValueSettingWidget* const q_ptr;
};

VolumeValueSettingWidget::VolumeValueSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new VolumeValueSettingWidgetPrivate(this))
{

}
void VolumeValueSettingWidget::setVolumeValueSettingWidgetObject(QObject* qmlObject){
    //qDebug()<<"======VolumeValueSettingWidget===start===";
    Q_D(VolumeValueSettingWidget);
    if(d->m_VolumeValueSettingWidgetObject == NULL)
    {
        d->m_VolumeValueSettingWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_MediaSliderObject, ARKSENDER(valueChanged()),
                     this,      ARKRECEIVER(onValueChanged()),
                     type);

    QObject::connect(d->m_NavigationSliderObject, ARKSENDER(valueChanged()),
                     this,      ARKRECEIVER(onValueChanged()),
                     type);

    QObject::connect(d->m_TelephoneSliderObject, ARKSENDER(valueChanged()),
                     this,      ARKRECEIVER(onValueChanged()),
                     type);

    QObject::connect(d->m_VolumeValueSettingWidgetObject, ARKSENDER(mediaSliderMoveFinish()),
                     this,      ARKRECEIVER(onMediaSliderMoveFinish()),
                     type);

    QObject::connect(d->m_VolumeValueSettingWidgetObject, ARKSENDER(navigationMoveFinish()),
                     this,      ARKRECEIVER(onNavigationMoveFinish()),
                     type);
    QObject::connect(d->m_VolumeValueSettingWidgetObject, ARKSENDER(telephoneMoveFinish()),
                     this,      ARKRECEIVER(onTelephoneMoveFinish()),
                     type);
    QObject::connect(d->m_VolumeValueSettingWidgetObject, ARKSENDER(visibleChanged()),
                     this,      ARKRECEIVER(onVisibleChanged()),
                     type);
    connectSignalAndSlotByNamesake(g_Audio, this, ARKRECEIVER(onHolderChange(const int, const int)));
    //qDebug()<<"======VolumeValueSettingWidget===end===";
}
void VolumeValueSettingWidget::onValueChanged()
{
    Q_D(VolumeValueSettingWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_MediaSliderObject){
        int _VolumeValue = d->m_MediaSliderObject->property("value").toInt();
        if(g_Setting->getVolumeType() == V_MediaVolume)
            g_Audio->requestSetVolume(_VolumeValue);
    }
    else if(ptr == d->m_NavigationSliderObject){
        int _VolumeValue = d->m_NavigationSliderObject->property("value").toInt();
        if(g_Setting->getVolumeType() == V_NavigationVolume)
            g_Audio->requestSetVolume(_VolumeValue);
    }
    else if(ptr == d->m_TelephoneSliderObject){
        int _VolumeValue = d->m_TelephoneSliderObject->property("value").toInt();
        if(g_Setting->getVolumeType() == V_TelPhoneVolume)
            g_Audio->requestSetVolume(_VolumeValue);
    }

}
void VolumeValueSettingWidget::onVisibleChanged(){
    Q_D(VolumeValueSettingWidget);
    static bool _InitShow(true);
    bool _VisibleStatus =  d->m_VolumeValueSettingWidgetObject->property("visible").toBool();
    if(_InitShow && _VisibleStatus)
    {
        _InitShow = false;
        int _MediaVolumeValue = AudioPersistent::getVolume();
        if(_MediaVolumeValue < 20)
        {
            QQmlProperty(d->m_MediaSliderObject,"value").write(_MediaVolumeValue);
            g_Setting->setMediaVolume(_MediaVolumeValue);\
            d->m_InitVolume = false;
        }
        int _NavigationVolumeValue = 20;
        int _TelephoneVolumeValue  = 20;
        QQmlProperty(d->m_NavigationSliderObject,"value").write(_NavigationVolumeValue);
        g_Setting->setNavigationVolume(_NavigationVolumeValue);
        QQmlProperty(d->m_TelephoneSliderObject,"value").write(_TelephoneVolumeValue);
        g_Setting->setTelPhoneVolume(_TelephoneVolumeValue);
    }
}
void VolumeValueSettingWidget::onMediaSliderMoveFinish()
{
    Q_D(VolumeValueSettingWidget);
    int _MediaVolumeValue = d->m_MediaSliderObject->property("value").toInt();
    g_Setting->setMediaVolume(_MediaVolumeValue);
    d->m_InitVolume = false;
}
void VolumeValueSettingWidget::onNavigationMoveFinish(){
    Q_D(VolumeValueSettingWidget);
    int _NavigationValue = d->m_NavigationSliderObject->property("value").toInt();
    g_Setting->setNavigationVolume(_NavigationValue);
}
void VolumeValueSettingWidget::onTelephoneMoveFinish(){
    Q_D(VolumeValueSettingWidget);
    int _TelephoneValue = d->m_TelephoneSliderObject->property("value").toInt();
    g_Setting->setTelPhoneVolume(_TelephoneValue);
}

void VolumeValueSettingWidget::onHolderChange(const AudioSource oldHolder, const AudioSource newHolder)
{
    Q_D(VolumeValueSettingWidget);
    qDebug() << __PRETTY_FUNCTION__ << oldHolder << newHolder;
    switch (newHolder) {
        case AS_Music:
        case AS_Video:
        case AS_CarplayMusic:
        case AS_AutoMusic:
        case AS_CarlifeMusic:
        case AS_BluetoothMusic:
        case AS_ECLinkMusic:
        case AS_ECLinkBluetoothMusic:
        case AS_HiCarMusic:
        case AS_HiCarBluetoothMusic:
        case AS_Aux:
            if(d->m_InitVolume == false)
            {
                g_Setting->setVolumeType(V_MediaVolume);
                g_Audio->requestSetVolume(g_Setting->getMediaVolume());
            }
            else {
                //qDebug()<<"+++++++++++g_Audio->getcurrentvolumevalue()++++++++++++"<<g_Audio->getcurrentvolumevalue();
                g_Setting->setVolumeType(V_MediaVolume);
                g_Audio->requestSetVolume(g_Audio->getcurrentvolumevalue());
            }
            break;
        case AS_CarplayPhone:
        case AS_AutoPhone:
        case AS_CarlifePhone:
        case AS_BluetoothPhone:
            g_Setting->setVolumeType(V_TelPhoneVolume);
            if(oldHolder == AS_Music)
            {
               //qDebug()<<"+++++++VPPS_Pause0000+++++++";
               g_Multimedia->musicPlayerSetPlayStatus(VPPS_Pause);
            }
            g_Audio->requestSetVolume(g_Setting->getTelPhoneVolume());
            break;
        default:
            break;
    }
}

VolumeValueSettingWidgetPrivate::VolumeValueSettingWidgetPrivate(VolumeValueSettingWidget *parent)
    : q_ptr(parent)
{
    m_VolumeValueSettingWidgetObject = NULL;
    m_NavigationSliderObject = NULL;
    m_MediaSliderObject = NULL;
    m_TelephoneSliderObject = NULL;
    m_InitVolume = true;
}

VolumeValueSettingWidgetPrivate::~VolumeValueSettingWidgetPrivate()
{

}
void VolumeValueSettingWidgetPrivate::initializeObject(){
    if(m_NavigationSliderObject == NULL)
    {
        m_NavigationSliderObject = m_VolumeValueSettingWidgetObject->findChild<QObject*>("navigationObject");
    }
    if(m_TelephoneSliderObject == NULL)
    {
        m_TelephoneSliderObject = m_VolumeValueSettingWidgetObject->findChild<QObject*>("telephoneObject");
    }

    if(m_MediaSliderObject == NULL)
    {
        m_MediaSliderObject = m_VolumeValueSettingWidgetObject->findChild<QObject*>("mediaObject");
    }
}
