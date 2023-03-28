#include "AllCallLogModelData.h"
#include "AutoConnect.h"
#include "./BusinessLogic/Bluetooth.h"
#include <QDebug>
QList<struct CallLogInfo> getSortedList(QList<struct CallLogInfo> list)
{
    for(int i = 0; i<list.length(); i++)
    {
        for (int j = i+1; j < list.length(); j++)
        {
            QString data1 = QString(list.at(i).data) + QString(list.at(i).time);
            QString data2 = QString(list.at(j).data) + QString(list.at(j).time);
            if(data1.toLongLong() < data2.toLongLong())
            {
                list.swap(i,j);
            }
        }
    }//end.for
    return list;
}
class AllCallLogModelDataPrivate
{
    Q_DISABLE_COPY(AllCallLogModelDataPrivate)
public:
    explicit AllCallLogModelDataPrivate(AllCallLogModelData* parent);
    ~AllCallLogModelDataPrivate();
    void connectAllSlots();
public:
    AllCallLogModel* m_AllCallLogModel;
private:
    Q_DECLARE_PUBLIC(AllCallLogModelData)
    AllCallLogModelData* const q_ptr;
};
AllCallLogModelData::AllCallLogModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new AllCallLogModelDataPrivate(this))
{

}
AllCallLogModel* AllCallLogModelData::getObjectModel()
{
    Q_D(AllCallLogModelData);
    if(d->m_AllCallLogModel == NULL)
    {
        d->m_AllCallLogModel = new AllCallLogModel();
    }
    return d->m_AllCallLogModel;
}
void AllCallLogModelData::onSyncAllCallLog(){
    Q_D(AllCallLogModelData);
    if(d->m_AllCallLogModel != NULL)
        d->m_AllCallLogModel->clear();
    QList<struct CallLogInfo> allCallList  = g_Bluetooth->getCallLogInfo();
    QList<struct CallLogInfo> _AllCallList = getSortedList(allCallList);
    for(int i=0;i<_AllCallList.size();i++)
    {
        QString data1 = QString("%1").arg(_AllCallList.at(i).callType);
        QString data2 = _AllCallList.at(i).name;
        QString data3 = _AllCallList.at(i).phoneNumber;
        QString _DataStr = _AllCallList.at(i).data;
        QString _TimeStr = _AllCallList.at(i).data;
        QString data4 = _DataStr.left(4) + QString("-")+ _DataStr.right(4).left(2) + QString("-")+ _DataStr.right(2) + QString("-");
        QString data5 = _TimeStr.left(2) + ":" + _TimeStr.right(4).left(2) + ":"+ _TimeStr.right(2);
        AllCallLogListModelData data(data1,data2,data3,data4,data5);
        if(d->m_AllCallLogModel != NULL)
        {
            d->m_AllCallLogModel->Add(data);
        }
    }
}
void AllCallLogModelData::onConnectStatusChange(const int status){
    Q_D(AllCallLogModelData);
    if(status < 3)
    {
        if(d->m_AllCallLogModel != NULL)
        {
            d->m_AllCallLogModel->clear();
        }
    }
}
AllCallLogModelDataPrivate::AllCallLogModelDataPrivate(AllCallLogModelData *parent)
    : q_ptr(parent)
{
    m_AllCallLogModel =NULL;
    connectAllSlots();
}
AllCallLogModelDataPrivate::~AllCallLogModelDataPrivate()
{

}
void AllCallLogModelDataPrivate::connectAllSlots()
{
    Q_Q(AllCallLogModelData);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onSyncAllCallLog()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
}
