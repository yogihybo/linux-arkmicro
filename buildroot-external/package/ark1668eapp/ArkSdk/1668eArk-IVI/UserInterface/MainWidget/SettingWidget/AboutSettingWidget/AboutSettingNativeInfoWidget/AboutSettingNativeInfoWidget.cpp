#include "AboutSettingNativeInfoWidget.h"
#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include "Utility.h"
#include <QQmlProperty>
#include <QDebug>

class AboutSettingNativeInfoWidgetPrivate
{
    Q_DISABLE_COPY(AboutSettingNativeInfoWidgetPrivate)
public:
    explicit AboutSettingNativeInfoWidgetPrivate(AboutSettingNativeInfoWidget* parent);
    ~AboutSettingNativeInfoWidgetPrivate();
    void initializeObject();
public:
    QObject* m_AboutSettingNativeInfoWidgetObject;
    QObject* m_BspVersionObject;
    QObject* m_AppVersionObject;
private:
    Q_DECLARE_PUBLIC(AboutSettingNativeInfoWidget)
    AboutSettingNativeInfoWidget* const q_ptr;
};

AboutSettingNativeInfoWidget::AboutSettingNativeInfoWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new AboutSettingNativeInfoWidgetPrivate(this))
{

}

void AboutSettingNativeInfoWidget::setAboutSettingNativeInfoWidgetObject(QObject* qmlObject){
    Q_D(AboutSettingNativeInfoWidget);
    if(d->m_AboutSettingNativeInfoWidgetObject == NULL)
    {
        d->m_AboutSettingNativeInfoWidgetObject = qmlObject;
    }
    d->initializeObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_AboutSettingNativeInfoWidgetObject, ARKSENDER(visibleChanged()),
                     this,      ARKRECEIVER(onVisibleChanged()),
                     type);
}
void AboutSettingNativeInfoWidget::onVisibleChanged()
{
    Q_D(AboutSettingNativeInfoWidget);
    static bool _FirstShow(true);
    static QString _BspVersionStr;
    static QString _AppVersionStr;
    bool _Visible = d->m_AboutSettingNativeInfoWidgetObject->property("visible").toBool();
    if(_Visible){
        QString prefix = QString("Ark1668eDevb");
        if(_FirstShow){
            _BspVersionStr = QString("BSP:") + prefix + QString("-") + osVersion() + QString("V1.0");
            _AppVersionStr = QString("APP:") + prefix + QString("-") + osVersion() + QString("V1.0");
            _FirstShow = false;
        }
        QQmlProperty(d->m_BspVersionObject,"text").write(_BspVersionStr);
        QQmlProperty(d->m_AppVersionObject,"text").write(_AppVersionStr);
    }
}
AboutSettingNativeInfoWidgetPrivate::AboutSettingNativeInfoWidgetPrivate(AboutSettingNativeInfoWidget *parent)
    : q_ptr(parent)
{
    m_AboutSettingNativeInfoWidgetObject = NULL;
    m_BspVersionObject = NULL;
    m_AppVersionObject = NULL;
}

AboutSettingNativeInfoWidgetPrivate::~AboutSettingNativeInfoWidgetPrivate()
{

}

void AboutSettingNativeInfoWidgetPrivate::initializeObject(){
    if(m_BspVersionObject == NULL)
    {
        m_BspVersionObject = m_AboutSettingNativeInfoWidgetObject->findChild<QObject*>("bspVersionObject");
    }
    if(m_AppVersionObject == NULL)
    {
        m_AppVersionObject = m_AboutSettingNativeInfoWidgetObject->findChild<QObject*>("appVersionObject");
    }
}
