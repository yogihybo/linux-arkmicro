#include "KeyBoardFirstRowModelData.h"
#include "AutoConnect.h"
#include "BusinessLogic/QmlWidget.h"
#include <QDebug>
KeyBoardFirstRowModelData::KeyBoardFirstRowModelData(QObject *parent) : QObject(parent)
{
    m_myModel  = NULL;
    m_CaseType = 0;
    m_SymbolType = false;
    m_UppercaseList.clear();
    m_UppercaseList<<"Q"<<"W"<<"E"<<"R"<<"T"<<"Y"<<"U"<<"I"<<"O"<<"P";
    m_LowcaseList.clear();
    m_LowcaseList<<"q"<<"w"<<"e"<<"r"<<"t"<<"y"<<"u"<<"i"<<"o"<<"p";
    m_SymbolList.clear();
    m_SymbolList<<"!"<<"@"<<"#"<<"$"<<"%"<<"^"<<"&"<<"*"<<"("<<")";
    connectSignalAndSlotByNamesake(QmlWidget::instance(), this, ARKRECEIVER(onCaseSwitchBtnClicked()));
    connectSignalAndSlotByNamesake(QmlWidget::instance(), this, ARKRECEIVER(onSymbolsBtnClicked()));
    connectSignalAndSlotByNamesake(QmlWidget::instance(), this, ARKRECEIVER(onKeyBoardWidgetVisibel(bool)));
}

myModel* KeyBoardFirstRowModelData::objectModel()
{
    if(m_myModel == NULL)
    {
        m_myModel = new myModel();
        for(int i=0;i<m_UppercaseList.size();i++)
        {
            QString data = m_UppercaseList.at(i);
            myData d(data);
            m_myModel->Add(d);
        }
    }
    return m_myModel;
}

void KeyBoardFirstRowModelData::onCaseSwitchBtnClicked(){
    qDebug()<<"++++++onCaseSwitchBtnClicked+++++++";
    if(m_CaseType == 0)
    {
        m_CaseType = 1;
        m_myModel->clear();
        for(int i=0;i<m_LowcaseList.size();i++)
        {
            QString data = m_LowcaseList.at(i);
            myData d(data);
            m_myModel->Add(d);
        }
    }
    else if(m_CaseType == 1)
    {
        m_CaseType = 0;
        m_myModel->clear();
        for(int i=0;i<m_UppercaseList.size();i++)
        {
            QString data = m_UppercaseList.at(i);
            myData d(data);
            m_myModel->Add(d);
        }
    }
}

void KeyBoardFirstRowModelData::onSymbolsBtnClicked()
{
    if(m_SymbolType == false)
    {
        m_SymbolType = true;
        m_myModel->clear();
        for(int i=0;i<m_SymbolList.size();i++)
        {
            QString data = m_SymbolList.at(i);
            myData d(data);
            m_myModel->Add(d);
        }
    }
    else{
        m_SymbolType = false;
        if(m_CaseType == 1)
        {
            m_myModel->clear();
            for(int i=0;i<m_LowcaseList.size();i++)
            {
                QString data = m_LowcaseList.at(i);
                myData d(data);
                m_myModel->Add(d);
            }
        }
        else if(m_CaseType == 0)
        {
            m_myModel->clear();
            for(int i=0;i<m_UppercaseList.size();i++)
            {
                QString data = m_UppercaseList.at(i);
                myData d(data);
                m_myModel->Add(d);
            }
        }
    }
}

void KeyBoardFirstRowModelData::onKeyBoardWidgetVisibel(bool visible){
    if(visible == true)
    {
        m_CaseType = 0;
        m_SymbolType = false;
        m_myModel->clear();
        for(int i=0;i<m_UppercaseList.size();i++)
        {
            QString data = m_UppercaseList.at(i);
            myData d(data);
            m_myModel->Add(d);
        }
    }
}
