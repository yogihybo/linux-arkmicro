#include "SettingWidget.h"
class SettingWidgetPrivate
{
    Q_DISABLE_COPY(SettingWidgetPrivate)
public:
    explicit SettingWidgetPrivate(SettingWidget* parent);
    ~SettingWidgetPrivate();
    void initializeWidget();
    void initializeBtSettinWidget();
    void initializeVolumeSettingWidget();
    void initializeMoreSettingWidget();
    void initializeBrightnessSettingWidget();
    void initializeAboutSettingWidget();
    void initializeWifiSettingWidget();
public:
    QObject* m_SettingWidgetObject;
    BluetoothSettingWidget* m_BtSettingWidget;
    VolumeSettingWidget*    m_VolumeSettingWidget;
    MoreSettingWidget* m_MoreSettingWidget;
    BrightnessSettingWidget* m_BrightnessSettingWidget;
    AboutSettingWidget* m_AboutSettingWidget;
    WifiSettingWidget* m_WifiSettingWidget;
private:
    Q_DECLARE_PUBLIC(SettingWidget)
    SettingWidget* const q_ptr;
};
SettingWidget::SettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new SettingWidgetPrivate(this))
{

}
void SettingWidget::setSettingWidgetObject(QObject *qmlObject){
    Q_D(SettingWidget);
    if(d->m_SettingWidgetObject == NULL)
    {
        d->m_SettingWidgetObject = qmlObject;
    }
    d->initializeWidget();
}

SettingWidgetPrivate::SettingWidgetPrivate(SettingWidget *parent)
    : q_ptr(parent)
{
    m_SettingWidgetObject = NULL;
    m_BtSettingWidget = NULL;
    m_VolumeSettingWidget = NULL;
    m_MoreSettingWidget = NULL;
    m_BrightnessSettingWidget = NULL;
    m_AboutSettingWidget = NULL;
    m_WifiSettingWidget   = NULL;
}

SettingWidgetPrivate::~SettingWidgetPrivate()
{

}
void SettingWidgetPrivate::initializeWidget(){
    initializeBtSettinWidget();
    initializeVolumeSettingWidget();
    initializeMoreSettingWidget();
    initializeBrightnessSettingWidget();
    initializeAboutSettingWidget();
    initializeWifiSettingWidget();
}

void SettingWidgetPrivate::initializeBtSettinWidget(){
    Q_Q(SettingWidget);
    if(m_BtSettingWidget == NULL)
    {
        m_BtSettingWidget = new BluetoothSettingWidget(q);
        QObject* btSettingWidgetObject = m_SettingWidgetObject->findChild<QObject*>("btSettingWidgetObject");
        m_BtSettingWidget->setBtSettingWidgetObject(btSettingWidgetObject);
    }
}
void SettingWidgetPrivate::initializeVolumeSettingWidget()
{
    Q_Q(SettingWidget);
    if(m_VolumeSettingWidget == NULL)
    {
        m_VolumeSettingWidget = new VolumeSettingWidget(q);
        QObject* volumeSettingWidgetObject = m_SettingWidgetObject->findChild<QObject*>("soundSettingWidgetObject");
        m_VolumeSettingWidget->setVolumeSettingWidgetObject(volumeSettingWidgetObject);
    }
}
void SettingWidgetPrivate::initializeMoreSettingWidget(){

    Q_Q(SettingWidget);
    if(m_MoreSettingWidget == NULL)
    {
        m_MoreSettingWidget = new MoreSettingWidget(q);
        QObject* _MoreSettingWidgetObject = m_SettingWidgetObject->findChild<QObject*>("moreSettingWidgetObject");
        m_MoreSettingWidget->setMoreSettingWidgetObject(_MoreSettingWidgetObject);
    }
}

void SettingWidgetPrivate::initializeBrightnessSettingWidget(){
    Q_Q(SettingWidget);
    if(m_BrightnessSettingWidget == NULL)
    {
        m_BrightnessSettingWidget = new BrightnessSettingWidget(q);
        QObject* _BrightnessSettingWidgetObject = m_SettingWidgetObject->findChild<QObject*>("lightSettingWidgetObject");
        m_BrightnessSettingWidget->setBrightnessSettingWidgetObject(_BrightnessSettingWidgetObject);
    }
}
void SettingWidgetPrivate::initializeAboutSettingWidget(){

    Q_Q(SettingWidget);
    if(m_AboutSettingWidget == NULL)
    {
        m_AboutSettingWidget = new AboutSettingWidget (q);
        QObject* _AboutSettingWidgetObject = m_SettingWidgetObject->findChild<QObject*>("aboutSettingWidgetObject");
        m_AboutSettingWidget->setAboutSettingWidgetObject(_AboutSettingWidgetObject);
    }
}

void SettingWidgetPrivate::initializeWifiSettingWidget()
{
    Q_Q(SettingWidget);
    if(m_WifiSettingWidget == NULL)
    {
        m_WifiSettingWidget = new WifiSettingWidget (q);
        QObject* _WifiSettingWidgetObject = m_SettingWidgetObject->findChild<QObject*>("wifiSettingWidgetObject");
        m_WifiSettingWidget->setWifiSettingWidgetObject(_WifiSettingWidgetObject);
    }

}
