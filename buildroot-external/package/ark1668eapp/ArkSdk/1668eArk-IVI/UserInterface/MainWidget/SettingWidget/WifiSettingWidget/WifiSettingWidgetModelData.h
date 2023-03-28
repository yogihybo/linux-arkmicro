#ifndef WIFISETTINGWIDGETMODELDATA_H
#define WIFISETTINGWIDGETMODELDATA_H

#include <QObject>
#include "./BusinessLogic/Setting.h"
#include "UserInterface/MainWidget/ToolWidget/StatusBar/myModel/myModel.h"
class WifiSettingWidgetModelData : public QObject
{
    Q_OBJECT
public:
    explicit WifiSettingWidgetModelData(QObject *parent = nullptr);
    Q_INVOKABLE myModel* objectModel();
protected slots:
    void onWifiSsidListChanged();
private:
    myModel* m_myModel;
};

#endif // WIFISETTINGWIDGETMODELDATA_H
