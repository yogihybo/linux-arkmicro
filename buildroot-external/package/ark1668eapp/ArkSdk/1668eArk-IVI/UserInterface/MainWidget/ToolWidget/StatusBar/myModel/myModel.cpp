#include "myModel.h"

myModel::myModel(QObject *parent) : QAbstractListModel(parent)
{

}

void myModel::Add(myData&  md)
{
    beginInsertRows(QModelIndex(),rowCount(),rowCount());
    m_datas.append(md);
    endInsertRows();
}

void myModel::clear()
{
    //清除rows 界面将不显示
    beginRemoveRows(QModelIndex(),0,m_datas.size());
    //清空动态数组
    m_datas.clear();
    endRemoveRows();
    //end
}

//外部接口 QML调用 添加数据在指定行
void  myModel::minsert(int index, const QString& data1)
{
    myData  d(data1);
    beginInsertRows(QModelIndex(),index,index);
    m_datas.insert(m_datas.begin()+index,d);
    endInsertRows();
}

void  myModel::mremove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    m_datas.erase(m_datas.begin()+index);
    endRemoveRows();
}

void myModel::pushdata(const QString& data1)
{
    myData  d(data1);
    Add(d);
}


int myModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_datas.size();
}

QVariant myModel::data(const QModelIndex &index, int role)  const
{
    if(!index.isValid())
    {
        return QVariant();
    }
    if (index.row() < 0 || (index.row() > m_datas.size()-1))
    {
        return QVariant();
    }
    const myData& _ModeData = m_datas.at(index.row());
    switch (role)
    {
        case myModelDataType::type1:
           return QVariant::fromValue(_ModeData.m_data);
        case myModelDataType::type2:
            return QVariant::fromValue(_ModeData.obj);
        default:
           return QVariant();
    }
    return QVariant();
}

//定义数据别名  QHash<int, QByteArray> 父类规定的 //定义角色名称
QHash<int, QByteArray> myModel::roleNames() const
{
    QHash<int, QByteArray>  role;
    role[myModelDataType::type1] = "data";
    role[myModelDataType::type2] = "curitem";
    return  role;
}
