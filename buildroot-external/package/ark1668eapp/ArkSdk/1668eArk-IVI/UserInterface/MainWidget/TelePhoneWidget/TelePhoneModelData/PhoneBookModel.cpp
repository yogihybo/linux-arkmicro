#include "PhoneBookModel.h"
PhoneBookModel::PhoneBookModel(QObject *parent) : QAbstractListModel(parent)
{

}

void PhoneBookModel::Add(PhoneBookListModeData&  md)
{
    beginInsertRows(QModelIndex(),m_datas.size(),m_datas.size());
    m_datas.append(md);
    endInsertRows();
}

void PhoneBookModel::clear()
{
    //清除rows 界面将不显示
    beginRemoveRows(QModelIndex(),0,m_datas.size());
    //清空动态数组
    m_datas.clear();
    endRemoveRows();
    //end
}

//外部接口 QML调用 添加数据在指定行
void  PhoneBookModel::minsert(int index, const QString& data1, const QString& data2, const QString& data3)
{
    PhoneBookListModeData  d(data1,data2,data3);
    beginInsertRows(QModelIndex(),index,index);
    m_datas.insert(m_datas.begin()+index,d);
    endInsertRows();
}

void  PhoneBookModel::mremove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    m_datas.erase(m_datas.begin()+index);
    endRemoveRows();
}

void PhoneBookModel::pushdata(const QString& data1, const QString& data2, const QString& data3)
{
    PhoneBookListModeData  d(data1,data2,data3);
    Add(d);
}


int PhoneBookModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_datas.size();
}
QVariant PhoneBookModel::data(const QModelIndex &index, int role)  const
{
    if(!index.isValid())
    {
        return QVariant();
    }
    if (index.row() < 0 || (index.row() > m_datas.size()-1))
    {
        return QVariant();
    }
    const PhoneBookListModeData& _ModeData = m_datas.at(index.row());
    switch (role)
    {
        case PhoneBookModelDataType::type1:
           return QVariant::fromValue(_ModeData.mdata1);
        case PhoneBookModelDataType::type2:
            return QVariant::fromValue(_ModeData.mdata2);
        case PhoneBookModelDataType::type3:
            return QVariant::fromValue(_ModeData.mdata3);
        case PhoneBookModelDataType::type4:
             return QVariant::fromValue(_ModeData.obj);
        default:
           return QVariant();
    }
    return QVariant();
}

//定义数据别名  QHash<int, QByteArray> 父类规定的 //定义角色名称
QHash<int, QByteArray> PhoneBookModel::roleNames() const
{
    QHash<int, QByteArray>  roles;
    roles[PhoneBookModelDataType::type1] = "data1";
    roles[PhoneBookModelDataType::type2] = "data2";
    roles[PhoneBookModelDataType::type3] = "data3";
    roles[PhoneBookModelDataType::type4] = "curitem";
    return  roles;
}

