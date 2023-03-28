#include "OnCallWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Bluetooth.h"
#include "AutoConnect.h"
#include <QQmlProperty>
#include <QDebug>
#include <QTimer>
class OnCallWidgetPrivate
{
    Q_DISABLE_COPY(OnCallWidgetPrivate)
public:
    explicit OnCallWidgetPrivate(OnCallWidget* parent);
    ~OnCallWidgetPrivate();
    void initializeObjectWidget();
    void initializePortraitRectObject();
    void initializeKeyBoardRectObject();
    void initializeTimer();
    QString convertTime(const int time);
    void connectAllSlots();
public:
    QObject* m_OnCallWidgetObject;
    QObject* m_HungUpBtnObject;
    QObject* m_KeyboardBtnObject;
    QObject* m_MicrophoneBtnObject;
    QObject* m_TransferBtnObject;
    QObject* m_PortraitRectObject;
    QObject* m_PortraitRectPhoneNumberObject;
    QObject* m_PortraitRectInCommingTextObject;
    QObject* m_KeyBoardRectObject;
    QObject* m_KeyBoardRectInputTextObject;
    QObject* m_KeyBoardRectPhoneNumberObject;
    QObject* m_KeyBoardRectOnCallTimeObject;
    QObject* m_KeyBoardRectDelBtnObject;
    QTimer*  m_Timer;
    QString  m_PhoneNumber;
    int      m_OnCallCountTime;
    int      m_RecordIndex;
private:
    Q_DECLARE_PUBLIC(OnCallWidget)
    OnCallWidget* const q_ptr;
};

OnCallWidget::OnCallWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new OnCallWidgetPrivate(this))
{

}
void OnCallWidget::setOnCallWidgetObject(QObject *qmlObject){
    Q_D(OnCallWidget);
    if(d->m_OnCallWidgetObject == NULL)
    {
        d->m_OnCallWidgetObject = qmlObject;
    }
    d->initializeObjectWidget();
    d->initializePortraitRectObject();
    d->initializeKeyBoardRectObject();
    Qt::ConnectionType type = static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::AutoConnection);
    QObject::connect(d->m_HungUpBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_KeyboardBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_MicrophoneBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_TransferBtnObject, ARKSENDER(clicked()),
                     this,      ARKRECEIVER(onToolButtonRelease()),
                     type);
    QObject::connect(d->m_KeyBoardRectObject, ARKSENDER(numberBtnClicked(QString)),
                     this,      ARKRECEIVER(onNumberBtnClicked(QString)),
                     type);

}
void OnCallWidget::onNumberBtnClicked(QString text){
    Q_D(OnCallWidget);
    g_Bluetooth->dialNumber(text);
}
void OnCallWidget::onToolButtonRelease()
{
    Q_D(OnCallWidget);
    QObject* ptr = static_cast<QObject*>(sender());
    if(ptr == d->m_HungUpBtnObject){
        g_Bluetooth->hanupPhone();
    }
    else if(ptr == d->m_KeyboardBtnObject){
        if(d->m_OnCallWidgetObject->property("showType").toInt() == 0)
        {
            QQmlProperty(d->m_OnCallWidgetObject,"showType").write(1);
            QQmlProperty(d->m_KeyBoardRectPhoneNumberObject,"text").write(d->m_PhoneNumber);
            QQmlProperty(d->m_KeyBoardRectOnCallTimeObject,"text").write(d->convertTime(d->m_OnCallCountTime));
        }
        else{
            QQmlProperty(d->m_OnCallWidgetObject,"showType").write(0);
        }
    }
    else if(ptr == d->m_MicrophoneBtnObject){
        if(g_Bluetooth->getBtMicMuteStatus() == MI_Unmute)
        {
            g_Bluetooth->BtMicMuteChanged(MI_Mute);
        }
        else
        {
            g_Bluetooth->BtMicMuteChanged(MI_Unmute);
        }
    }
    else if(ptr == d->m_TransferBtnObject){
        g_Bluetooth->voiceToggleSwitch();
    }
}
void OnCallWidget::onConnectStatusChange(const int status){
    Q_D(OnCallWidget);
    if(status == Bluetooth::BCS_Talking){
        if(d->m_MicrophoneBtnObject != NULL)
        {
            QQmlProperty(d->m_MicrophoneBtnObject,"enabled").write(true);
            QQmlProperty(d->m_KeyboardBtnObject,"enabled").write(true);
            QQmlProperty(d->m_TransferBtnObject,"enabled").write(true);
        }
        d->initializeTimer();
        if(d->m_Timer->isActive())
        {
            d->m_Timer->stop();
        }
        d->m_RecordIndex = 0;
        d->m_Timer->start();
    }
    else{
        if(d->m_MicrophoneBtnObject != NULL)
        {
            QQmlProperty(d->m_MicrophoneBtnObject,"enabled").write(false);
            QQmlProperty(d->m_KeyboardBtnObject,"enabled").write(false);
            QQmlProperty(d->m_TransferBtnObject,"enabled").write(false);
        }
        if(status != Bluetooth::BCS_Outgoing){
            d->initializeTimer();
            if(d->m_Timer->isActive())
            {
                d->m_Timer->stop();
            }
            d->m_OnCallCountTime = 0;
            QQmlProperty(d->m_OnCallWidgetObject,"showType").write(0);
            QQmlProperty(d->m_PortraitRectInCommingTextObject,"text").write(QString(""));
            QQmlProperty(d->m_KeyBoardRectOnCallTimeObject,"text").write(QString(""));
            QQmlProperty(d->m_PortraitRectPhoneNumberObject,"text").write(QString(""));
            QQmlProperty(d->m_KeyBoardRectPhoneNumberObject,"text").write(QString(""));
            QQmlProperty(d->m_KeyBoardRectInputTextObject,"text").write(QString(""));
            QQmlProperty(d->m_KeyBoardRectObject,"phoneNumberStr").write(QString(""));
        }
        else if(status == Bluetooth::BCS_Outgoing){
            d->initializeTimer();
            d->m_Timer->start();
        }
    }
}
void OnCallWidget::onDialInfo(const int type,const QString& phone){
    Q_D(OnCallWidget);
    //qDebug()<<"++++++++++onDialInfo+++++++++++"<<type;
    if(type == Bluetooth::BCS_Talking){
        if(d->m_OnCallWidgetObject->property("showType") == 0){
            if(d->m_PortraitRectPhoneNumberObject != NULL)
            {
                QQmlProperty(d->m_PortraitRectPhoneNumberObject,"text").write(phone);
            }
        }
        d->m_PhoneNumber = phone;
    }
    else if(type == Bluetooth::BCS_Outgoing){
        if(d->m_OnCallWidgetObject->property("showType") == 0){
            if(d->m_PortraitRectPhoneNumberObject != NULL)
            {
                QQmlProperty(d->m_PortraitRectPhoneNumberObject,"text").write(phone);
            }
        }
    }
}
void OnCallWidget::onTimeout(){
    Q_D(OnCallWidget);
    QTimer* ptr = static_cast<QTimer*>(sender());
    if(ptr == d->m_Timer){
        if(g_Bluetooth->connectStatus() == Bluetooth::BCS_Talking){
            d->m_OnCallCountTime++;
            if(d->m_OnCallWidgetObject != NULL && d->m_OnCallWidgetObject->property("showType") == 0){
                if(d->m_PortraitRectInCommingTextObject != NULL)
                {
                    QQmlProperty(d->m_PortraitRectInCommingTextObject,"text").write(d->convertTime(d->m_OnCallCountTime));
                }
            }
            else if(d->m_OnCallWidgetObject != NULL && d->m_OnCallWidgetObject->property("showType") == 1)
            {
                if(d->m_KeyBoardRectOnCallTimeObject != NULL)
                {
                    QQmlProperty(d->m_KeyBoardRectOnCallTimeObject,"text").write(d->convertTime(d->m_OnCallCountTime));
                }
            }
        }
        else if(g_Bluetooth->connectStatus() == Bluetooth::BCS_Outgoing)
        {
            if(d->m_OnCallWidgetObject != NULL && d->m_OnCallWidgetObject->property("showType") == 0){
                if(d->m_PortraitRectInCommingTextObject != NULL)
                {
                    if(d->m_RecordIndex == 0)
                    {
                        d->m_RecordIndex++;
                        QQmlProperty(d->m_PortraitRectInCommingTextObject,"text").write(QString("正在接入..."));
                    }
                    else if(d->m_RecordIndex == 1)
                    {
                        d->m_RecordIndex++;
                        QQmlProperty(d->m_PortraitRectInCommingTextObject,"text").write(QString("正在接入.. "));

                    }
                    else if(d->m_RecordIndex == 2)
                    {
                        d->m_RecordIndex++;
                        QQmlProperty(d->m_PortraitRectInCommingTextObject,"text").write(QString("正在接入.  "));

                    }
                    else if(d->m_RecordIndex == 3)
                    {
                        d->m_RecordIndex = 0;
                        QQmlProperty(d->m_PortraitRectInCommingTextObject,"text").write(QString("正在接入   "));
                    }
                }
            }
        }

    }
}

OnCallWidgetPrivate::OnCallWidgetPrivate(OnCallWidget *parent)
    : q_ptr(parent)
{
    m_OnCallWidgetObject = NULL;
    m_HungUpBtnObject    = NULL;
    m_KeyboardBtnObject  = NULL;
    m_MicrophoneBtnObject= NULL;
    m_TransferBtnObject  = NULL;
    m_PortraitRectObject = NULL;
    m_PortraitRectPhoneNumberObject = NULL;
    m_PortraitRectInCommingTextObject  = NULL;
    m_KeyBoardRectObject   = NULL;
    m_KeyBoardRectInputTextObject = NULL;
    m_KeyBoardRectPhoneNumberObject = NULL;
    m_KeyBoardRectOnCallTimeObject  = NULL;
    m_KeyBoardRectDelBtnObject   = NULL;
    m_Timer = NULL;
    m_OnCallCountTime = 0;
    m_RecordIndex     = 0;
    initializeTimer();
    connectAllSlots();
}

OnCallWidgetPrivate::~OnCallWidgetPrivate()
{

}

void OnCallWidgetPrivate::initializeObjectWidget(){
    Q_Q(OnCallWidget);
    if(m_OnCallWidgetObject != NULL)
    {
        if(m_HungUpBtnObject == NULL)
        {
            m_HungUpBtnObject = m_OnCallWidgetObject->findChild<QObject*>("hungUpBtnObject");
        }
        if(m_KeyboardBtnObject == NULL)
        {
            m_KeyboardBtnObject = m_OnCallWidgetObject->findChild<QObject*>("keyboardBtnObject");
        }
        if(m_MicrophoneBtnObject == NULL)
        {
            m_MicrophoneBtnObject = m_OnCallWidgetObject->findChild<QObject*>("microphoneBtnObject");
        }
        if(m_TransferBtnObject == NULL)
        {
            m_TransferBtnObject   = m_OnCallWidgetObject->findChild<QObject*>("transferBtnObject");
        }
    }
}
void OnCallWidgetPrivate::initializePortraitRectObject()
{
    Q_Q(OnCallWidget);
    if(m_PortraitRectObject == NULL)
    {
        m_PortraitRectObject = m_OnCallWidgetObject->findChild<QObject*>("portraitRectObject");
    }

    if(m_PortraitRectPhoneNumberObject == NULL)
    {
        m_PortraitRectPhoneNumberObject = m_PortraitRectObject->findChild<QObject*>("portraitRectPhoneNumberObject");
    }

    if(m_PortraitRectInCommingTextObject == NULL)
    {
        m_PortraitRectInCommingTextObject =  m_PortraitRectObject->findChild<QObject*>("inCommingTextObject");
    }
}
void OnCallWidgetPrivate::initializeKeyBoardRectObject(){
    if(m_KeyBoardRectObject == NULL)
    {
        m_KeyBoardRectObject = m_OnCallWidgetObject->findChild<QObject*>("keyBoardRectObject");
    }
    if(m_KeyBoardRectInputTextObject == NULL)
    {
        m_KeyBoardRectInputTextObject = m_KeyBoardRectObject->findChild<QObject*>("keyBoardRectInputTextObject");
    }
    if(m_KeyBoardRectPhoneNumberObject == NULL)
    {
        m_KeyBoardRectPhoneNumberObject = m_KeyBoardRectObject->findChild<QObject*>("keyBoardRectPhoneNumberObject");
    }
    if(m_KeyBoardRectOnCallTimeObject == NULL)
    {
        m_KeyBoardRectOnCallTimeObject = m_KeyBoardRectObject->findChild<QObject*>("keyBoardRectOnCallTimeObject");
    }
    if(m_KeyBoardRectDelBtnObject == NULL)
    {
        m_KeyBoardRectDelBtnObject  = m_KeyBoardRectObject->findChild<QObject*>("keyBoardRectDelBtnObject");
    }
}
void OnCallWidgetPrivate::initializeTimer(){
    Q_Q(OnCallWidget);
    if(m_Timer == NULL)
    {
        m_Timer = new QTimer(q);
        m_Timer->setInterval(1000);
        QObject::connect(m_Timer, ARKSENDER(timeout()),
                         q,                 ARKRECEIVER(onTimeout()));
    }
}
QString OnCallWidgetPrivate::convertTime(const int time)
{
    QString hour("%1");
    QString minute("%1");
    QString second("%1");
    return hour.arg((time / 60) / 60, 2, 10, QChar('0'))
            + QString(":") + minute.arg((time / 60) % 60, 2, 10, QChar('0'))
            + QString(":") + second.arg(time % 60, 2, 10, QChar('0'));
}
void OnCallWidgetPrivate::connectAllSlots()
{
    Q_Q(OnCallWidget);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onDialInfo(const int,const QString&)));
}

