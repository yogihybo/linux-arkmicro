#include "VolumeBalanceSettingWidget.h"
#include "BusinessLogic/Audio.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>

class VolumeBalanceSettingWidgetPrivate
{
    Q_DISABLE_COPY(VolumeBalanceSettingWidgetPrivate)
public:
    explicit VolumeBalanceSettingWidgetPrivate(VolumeBalanceSettingWidget* parent);
    ~VolumeBalanceSettingWidgetPrivate();
    void setPosition(const short x, const short y);
    void initializeTimer();
public:
    QObject* m_VolumeBalanceSettingWidgetObject;
    QObject* m_TickImageObject;
    short m_X;
    short m_Y;
    QTimer* m_Timer;

private:
    Q_DECLARE_PUBLIC(VolumeBalanceSettingWidget)
    VolumeBalanceSettingWidget* const q_ptr;
};

VolumeBalanceSettingWidget::VolumeBalanceSettingWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new VolumeBalanceSettingWidgetPrivate(this))
{

}
void VolumeBalanceSettingWidget::setVolumeBalanceSettingWidgetObjecgt(QObject* qmlObject){
    Q_D(VolumeBalanceSettingWidget);
    if(d->m_VolumeBalanceSettingWidgetObject == NULL)
    {
        d->m_VolumeBalanceSettingWidgetObject = qmlObject;
    }
    if(d->m_TickImageObject == NULL){
        d->m_TickImageObject = d->m_VolumeBalanceSettingWidgetObject->findChild<QObject*>("tickImageObject");
    }
    SoundItem item = AudioPersistent::getSoundItem();
    int left;
    int right;
    switch (item) {
    case SI_Master: {
        left = -3;
        right = -3;
        break;
    }
    case SI_Slave: {
        left = 3;
        right = -3;
        break;
    }
    case SI_RearLeft: {
        left = -3;
        right = 3;
        break;
    }
    case SI_RearRight: {
        left = 3;
        right = 3;
        break;
    }
    default: {
        left = AudioPersistent::getLeftSound();
        right = AudioPersistent::getRightSound();
        break;
    }
    }
    d->setPosition(left, right);
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_VolumeBalanceSettingWidgetObject, ARKSENDER(mousePressed(double,double)),
                     this,      ARKRECEIVER(onMousePressed(double,double)),
                     type);
    QObject::connect(d->m_VolumeBalanceSettingWidgetObject, ARKSENDER(mouseMove(double,double)),
                     this,      ARKRECEIVER(onMouseMove(double,double)),
                     type);
    QObject::connect(d->m_VolumeBalanceSettingWidgetObject, ARKSENDER(mouseRelease(double,double)),
                     this,      ARKRECEIVER(onMouseRelease(double,double)),
                     type);
    //qDebug()<<"+++++++VolumeBalanceSettingWidget::setVolumeBalanceSettingWidgetObjecgt+++END+++++++++";
}
void VolumeBalanceSettingWidget::onTimeout()
{
    Q_D(VolumeBalanceSettingWidget);
    g_Audio->setSoundItem(SI_Custom, d->m_X, d->m_Y);
}
void VolumeBalanceSettingWidget::onMousePressed(double x,double y)
{
    //qDebug()<<"+++++++++onMousePressedxy++++++++++"<<x<<" "<<y;

    Q_D(VolumeBalanceSettingWidget);
    if (x >= 312 && x<=568 && y >= 138 &&  y<= 528) {
        short xPos = x- 321 - 42;
        xPos = (xPos + 6.03571f) / (12.07142f);
        if (xPos < 0) {
            xPos = 0;
        } else if (xPos > 14) {
            xPos = 14;
        }
        xPos -= 7;
        short yPos = y - 138 - 82;
        yPos = (yPos +  8.78571f) / (17.57142f);
        if (yPos < 0) {
            yPos = 0;
        } else if (yPos > 14) {
            yPos = 14;
        }
        yPos -= 7;
       // qDebug()<<"+++++++++onMousePressedxy++++0000++++++";
        d->setPosition(xPos, yPos);
      //  qDebug()<<"+++++++++onMousePressedxy++++1111++++++";
        d->initializeTimer();
       // qDebug()<<"+++++++++onMousePressedxy++++2222++++++";
        d->m_Timer->start();
      //  qDebug()<<"+++++++++onMousePressedxy++++3333++++++";
    }
}
void VolumeBalanceSettingWidget::onMouseMove(double x,double y)
{
    Q_D(VolumeBalanceSettingWidget);
    if (x >= 312 && x<=568 && y >= 138 &&  y<= 528) {
        short xPos = x- 321 - 42;
        xPos = (xPos + 6.03571f) / (12.07142f);
        if (xPos < 0) {
            xPos = 0;
        } else if (xPos > 14) {
            xPos = 14;
        }
        xPos -= 7;
        short yPos = y - 138 - 82;
        yPos = (yPos +  8.78571f) / (17.57142f);
        if (yPos < 0) {
            yPos = 0;
        } else if (yPos > 14) {
            yPos = 14;
        }
        yPos -= 7;
        d->setPosition(xPos, yPos);
        d->initializeTimer();
        d->m_Timer->start();
    }
}


void VolumeBalanceSettingWidget::onMouseRelease(double x,double y)
{
    Q_D(VolumeBalanceSettingWidget);
    d->initializeTimer();
    d->m_Timer->start();
}
VolumeBalanceSettingWidgetPrivate::VolumeBalanceSettingWidgetPrivate(VolumeBalanceSettingWidget *parent)
    : q_ptr(parent)
{
    m_VolumeBalanceSettingWidgetObject = NULL;
    m_TickImageObject  =NULL;
    m_X = 0;
    m_Y = 0;
    m_Timer = NULL;
}

VolumeBalanceSettingWidgetPrivate::~VolumeBalanceSettingWidgetPrivate()
{

}
void VolumeBalanceSettingWidgetPrivate::setPosition(const short x, const short y)
{
    m_X = x;
    m_Y = y;
    short xPos = 415 + x * 12.07142f;
    short yPos = 300 + y * 17.57142f;
    QQmlProperty(m_TickImageObject,"x").write(xPos);
    QQmlProperty(m_TickImageObject,"y").write(yPos);
}
void VolumeBalanceSettingWidgetPrivate::initializeTimer()
{
    Q_Q(VolumeBalanceSettingWidget);
    if (NULL == m_Timer) {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(100);
        m_Timer->setSingleShot(true);
        QObject::connect(m_Timer, ARKSENDER(timeout()),
                         q,       ARKRECEIVER(onTimeout()));
    }
}


