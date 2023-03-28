#include "QmlWidget.h"
#include "BusinessLogic/Audio.h"
#include "BusinessLogic/Setting.h"
#include <QDebug>
static QmlWidget* __QmlWidget = nullptr;
QmlWidget::QmlWidget(QObject *parent) : QObject(parent)
{
    m_KeyBoardInputStr.clear();
}
QmlWidget *QmlWidget::instance()
{
    if(__QmlWidget == nullptr){
        __QmlWidget = new QmlWidget();
    }
    return __QmlWidget;
}
void QmlWidget::caseSwitchBtnClicked()
{
    emit onCaseSwitchBtnClicked();
}
void QmlWidget::enterBtnClicked()
{
    emit onEnterBtnClicked();
}
void QmlWidget::symbolsBtnClicked(){
    emit onSymbolsBtnClicked();
}
void QmlWidget::keyBoardWidgetVisibel(bool visible)
{
    m_KeyBoardInputStr.clear();
    emit onKeyBoardWidgetVisibel(visible);
}

void QmlWidget::addKeyBoardInputStr(QString str){
    m_KeyBoardInputStr += str;
    emit onAddKeyBoardInputStr(m_KeyBoardInputStr);
}

void QmlWidget::subKeyBoardInputStr()
{
    if(m_KeyBoardInputStr.size() > 1)
    {
        m_KeyBoardInputStr = m_KeyBoardInputStr.left(m_KeyBoardInputStr.size()-1);
    }
    else{
        m_KeyBoardInputStr.clear();
    }
    emit onSubKeyBoardInputStr(m_KeyBoardInputStr);
}

void QmlWidget::clearKeyBoardInputStr(){
    m_KeyBoardInputStr.clear();
}
void QmlWidget::requestMuteToggole(){

    g_Audio->requestMuteToggole();
}

int  QmlWidget::getVolumeType()
{
    return g_Setting->getVolumeType();
}
