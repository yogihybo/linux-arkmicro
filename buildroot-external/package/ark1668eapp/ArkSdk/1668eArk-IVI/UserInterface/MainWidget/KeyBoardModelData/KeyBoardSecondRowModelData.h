#ifndef KEYBOARDSECONDROWMODELDATA_H
#define KEYBOARDSECONDROWMODELDATA_H

#include <QObject>
#include "../ToolWidget/StatusBar/myModel/myModel.h"
class KeyBoardSecondRowModelData : public QObject
{
    Q_OBJECT
public:
    explicit KeyBoardSecondRowModelData(QObject *parent = nullptr);
    Q_INVOKABLE myModel* objectModel();
public slots:
    void onCaseSwitchBtnClicked();
    void onSymbolsBtnClicked();
protected slots:
    void onKeyBoardWidgetVisibel(bool visible);
private:
    myModel* m_myModel;
    QStringList m_UppercaseList;
    QStringList m_LowcaseList;
    QStringList m_SymbolList;
    int  m_CaseType;
    bool m_SymbolType;
};

#endif // KEYBOARDSECONDROWMODELDATA_H
