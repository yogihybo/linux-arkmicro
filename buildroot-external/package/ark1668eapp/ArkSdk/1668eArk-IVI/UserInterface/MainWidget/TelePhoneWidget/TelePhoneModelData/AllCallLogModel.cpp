#include "AllCallLogModel.h"
AllCallLogModel::AllCallLogModel(QObject *parent) : QAbstractListModel(parent)
{

}

void AllCallLogModel::Add(AllCallLogListModelData&  md)
{
    beginInsertRows(QModelIndex(),m_datas.size(),m_datas.size());
    m_datas.append(md);
    endInsertRows();
}

void AllCallLogModel::clear()
{
    //清除rows 界面将不显示
    beginRemoveRows(QModelIndex(),0,m_datas.size());
    //清空动态数组
    m_datas.clear();
    endRemoveRows();
    //end
}

//外部接口 QML调用 添加数据在指定行
void  AllCallLogModel::minsert(int index, const QString& data1, const QString& data2,
                               const QString& data3, const QString& data4, const QString& data5)
{
    AllCallLogListModelData  d(data1,data2,data3,data4,data5);
    beginInsertRows(QModelIndex(),index,index);
    m_datas.insert(m_datas.begin()+index,d);
    endInsertRows();
}

void  AllCallLogModel::mremove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    m_datas.erase(m_datas.begin()+index);
    endRemoveRows();
}

void AllCallLogModel::pushdata(const QString& data1,const QString& data2,const QString& data3
                               ,const QString& data4,const QString& data5)
{
    AllCallLogListModelData  d(data1,data2,data3,data4,data5);
    Add(d);
}


int AllCallLogModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_datas.size();
}
QVariant AllCallLogModel::data(const QModelIndex &index, int role)  const
{
    if(!index.isValid())
    {
        return QVariant();
    }
    if (index.row() < 0 || (index.row() > m_datas.size()-1))
    {
        return QVariant();
    }
    const AllCallLogListModelData& _ModeData = m_datas.at(index.row());
    switch (role)
    {
        case AllCallLogModelDataType::type1:
           return QVariant::fromValue(_ModeData.mdata1);
        case AllCallLogModelDataType::type2:
            return QVariant::fromValue(_ModeData.mdata2);
        case AllCallLogModelDataType::type3:
            return QVariant::fromValue(_ModeData.mdata3);
        case AllCallLogModelDataType::type4:
            return QVariant::fromValue(_ModeData.mdata4);
        case AllCallLogModelDataType::type5:
             return QVariant::fromValue(_ModeData.mdata5);
        case AllCallLogModelDataType::type6:
             return QVariant::fromValue(_ModeData.obj);
        default:
           return QVariant();
    }
    return QVariant();
}

//定义数据别名  QHash<int, QByteArray> 父类规定的 //定义角色名称
QHash<int, QByteArray> AllCallLogModel::roleNames() const
{
    QHash<int, QByteArray>  roles;
    roles[AllCallLogModelDataType::type1] = "data1";
    roles[AllCallLogModelDataType::type2] = "data2";
    roles[AllCallLogModelDataType::type3] = "data3";
    roles[AllCallLogModelDataType::type4] = "data4";
    roles[AllCallLogModelDataType::type5] = "data5";
    roles[AllCallLogModelDataType::type6] = "curitem";
    return roles;
}
