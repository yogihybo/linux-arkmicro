#include "KeyBoardWidget.h"
#include "AutoConnect.h"
#include "BusinessLogic/QmlWidget.h"
#include <QQmlProperty>
#include <QDebug>

class KeyBoardWidgetPrivate
{
    Q_DISABLE_COPY(KeyBoardWidgetPrivate)
public:
    explicit KeyBoardWidgetPrivate(KeyBoardWidget* parent);
    ~KeyBoardWidgetPrivate();
    void initializeObject();
    void connectAllSlots();
public:
    QObject* m_KeyBoardWidgetObject;
    QObject* m_keyBoardLoaderObject;
private:
    Q_DECLARE_PUBLIC(KeyBoardWidget)
    KeyBoardWidget* const q_ptr;
};

KeyBoardWidget::KeyBoardWidget(QObject *parent) :
    QObject(parent),
    d_ptr(new KeyBoardWidgetPrivate(this))
{

}
void KeyBoardWidget::setKeyBoardWidgetObject(QObject *qmlObject){
    Q_D(KeyBoardWidget);
    if(d->m_KeyBoardWidgetObject == NULL)
    {
        d->m_KeyBoardWidgetObject = qmlObject;
    }
    d->connectAllSlots();
}

void KeyBoardWidget::setKeyBoardLoaderObject(QObject *qmlObject)
{
    Q_D(KeyBoardWidget);
    if(d->m_keyBoardLoaderObject == NULL)
    {
        d->m_keyBoardLoaderObject = qmlObject;
    }
}
void KeyBoardWidget::onKeyBoardWidgetVisibel(bool visible){
    Q_D(KeyBoardWidget);
    qDebug()<<"+++++++++KeyBoardWidget::onKeyBoardWidgetVisibel++++++++++++"<<visible;
    QQmlProperty(d->m_keyBoardLoaderObject,"visible").write(visible);
}

KeyBoardWidgetPrivate::KeyBoardWidgetPrivate(KeyBoardWidget *parent)
    : q_ptr(parent)
{
    m_KeyBoardWidgetObject = NULL;
    m_keyBoardLoaderObject = NULL;
}


KeyBoardWidgetPrivate::~KeyBoardWidgetPrivate()
{

}
void KeyBoardWidgetPrivate::initializeObject(){

}

void KeyBoardWidgetPrivate::connectAllSlots()
{
    Q_Q(KeyBoardWidget);
    connectSignalAndSlotByNamesake(QmlWidget::instance(), q, ARKRECEIVER(onKeyBoardWidgetVisibel(bool)));
}


