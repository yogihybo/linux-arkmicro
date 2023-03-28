#include "BrightnessSettingWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/Setting.h"
#include <QQmlProperty>
#include <QDebug>
#include <QFile>
#include <QSettings>
#include <unistd.h>
static const QString Brightness("/data/Brightness.ini");
class BrightnessSettingWidgetPrivate
{
    Q_DISABLE_COPY(BrightnessSettingWidgetPrivate)
public:
    explicit BrightnessSettingWidgetPrivate(BrightnessSettingWidget* parent);
    ~BrightnessSettingWidgetPrivate();
    void initializeBrightness();
    void setbacklightValue(int value);
public:
    QObject* m_BrightnessSettingWidgetObject;
    QObject* m_BrightnessSliderObject;
private:
    Q_DECLARE_PUBLIC(BrightnessSettingWidget)
    BrightnessSettingWidget* const q_ptr;
};

BrightnessSettingWidget::BrightnessSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new BrightnessSettingWidgetPrivate(this))
{

}
void BrightnessSettingWidget::setBrightnessSettingWidgetObject(QObject *qmlObject){
    Q_D(BrightnessSettingWidget);
    if(d->m_BrightnessSettingWidgetObject == NULL)
    {
        d->m_BrightnessSettingWidgetObject = qmlObject;
    }
    d->m_BrightnessSliderObject = d->m_BrightnessSettingWidgetObject->findChild<QObject*>("brightnessSliderObject");
    d->initializeBrightness();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_BrightnessSliderObject, ARKSENDER(valueChanged()),
                     this,      ARKRECEIVER(onValueChanged()),
                     type);

    QObject::connect(d->m_BrightnessSettingWidgetObject, ARKSENDER(sliderMoveFinish()),
                     this,      ARKRECEIVER(onSliderMoveFinish()),
                     type);
}
void BrightnessSettingWidget::onValueChanged()
{
    Q_D(BrightnessSettingWidget);
    int _BrightnessValue = d->m_BrightnessSliderObject->property("value").toInt();
    d->setbacklightValue(_BrightnessValue);
}
void BrightnessSettingWidget::onSliderMoveFinish()
{
    Q_D(BrightnessSettingWidget);
    int _BrightnessValue = d->m_BrightnessSliderObject->property("value").toInt();
    QFile _BrightnessFile(Brightness);
    if(!_BrightnessFile.exists())
    {
        g_Setting->executeShellCmd(QString(QString("touch ")+ Brightness).toLocal8Bit().constData());
        g_Setting->executeShellCmd("sync");
    }
    QSettings *brightnessSetFile = new QSettings(Brightness,QSettings::IniFormat);
    brightnessSetFile->setValue("Brightness", QString("%1").arg(_BrightnessValue));
    brightnessSetFile->sync();
    g_Setting->executeShellCmd("sync");
    delete brightnessSetFile;
}

BrightnessSettingWidgetPrivate::BrightnessSettingWidgetPrivate(BrightnessSettingWidget *parent)
    : q_ptr(parent)
{
    m_BrightnessSettingWidgetObject = NULL;
}

BrightnessSettingWidgetPrivate::~BrightnessSettingWidgetPrivate()
{

}
void BrightnessSettingWidgetPrivate::initializeBrightness(){
    Q_Q(BrightnessSettingWidget);
    QFile brightnessFile(Brightness);
    QSettings *brightnessSetFile = new QSettings(Brightness,QSettings::IniFormat);
    if(!brightnessFile.exists())
    {
        //qDebug()<< __PRETTY_FUNCTION__ << __LINE__<<"brightnessFile is not exist, creating...";
        g_Setting->executeShellCmd(QString(QString("touch ")+ Brightness).toLocal8Bit().constData());
        g_Setting->executeShellCmd("sync");
        int _BrightnessValue = m_BrightnessSliderObject->property("value").toInt();
        brightnessSetFile->setValue("Brightness", QString("%1").arg(_BrightnessValue));
        brightnessSetFile->sync();
        g_Setting->executeShellCmd("sync");
        setbacklightValue(_BrightnessValue);
    }
    else{

        int _BrightnessValue = brightnessSetFile->value("Brightness").toInt();
        QQmlProperty(m_BrightnessSliderObject,"value").write(_BrightnessValue);
        setbacklightValue(_BrightnessValue);
    }
    delete brightnessSetFile;

}

void BrightnessSettingWidgetPrivate::setbacklightValue(int value)
{
    QString cmd("echo ");
    QString backlight_Value = QString::number(value,10);
    cmd += backlight_Value;
    QString filename(" > /sys/devices/platform/soc/e0500000.lcd/backlight/backlight/brightness");
    cmd += filename;
    //qDebug()<<cmd;
    if (0 == access("/sys/devices/platform/soc/e0500000.lcd/backlight/backlight/brightness", F_OK))
    {
        g_Setting->executeShellCmd(cmd.toLocal8Bit().constData());
    }
}
