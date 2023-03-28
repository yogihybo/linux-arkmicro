#include "UsbMusicListModel.h"
UsbMusicListModel::UsbMusicListModel(QObject *parent) : QAbstractListModel(parent)
{

}

void UsbMusicListModel::Add(UsbMusicListModeData&  md)
{
    beginInsertRows(QModelIndex(),m_datas.size(),m_datas.size());
    m_datas.append(md);
    endInsertRows();
}

void UsbMusicListModel::clear()
{
    //清除rows 界面将不显示
    beginRemoveRows(QModelIndex(),0,m_datas.size());
    //清空动态数组
    m_datas.clear();
    endRemoveRows();
    //end
}

//外部接口 QML调用 添加数据在指定行
void  UsbMusicListModel::minsert(int index, const QString& data1, const QString& data2)
{
    UsbMusicListModeData  d(data1,data2);
    beginInsertRows(QModelIndex(),index,index);
    m_datas.insert(m_datas.begin()+index,d);
    endInsertRows();
}

void  UsbMusicListModel::mremove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    m_datas.erase(m_datas.begin()+index);
    endRemoveRows();
}

void UsbMusicListModel::pushdata(const QString& data1, const QString& data2)
{
    UsbMusicListModeData  d(data1,data2);
    Add(d);
}


int UsbMusicListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_datas.size();
}
QVariant UsbMusicListModel::data(const QModelIndex &index, int role)  const
{
    if(!index.isValid())
    {
        return QVariant();
    }
    if (index.row() < 0 || (index.row() > m_datas.size()-1))
    {
        return QVariant();
    }
    const UsbMusicListModeData& _ModeData = m_datas.at(index.row());
    switch (role)
    {
       case UsbMusicListModelDataType::type1:
           return QVariant::fromValue(_ModeData.mdata1);

       case UsbMusicListModelDataType::type2:
            return QVariant::fromValue(_ModeData.mdata2);
       case UsbMusicListModelDataType::type3:
            return QVariant::fromValue(_ModeData.obj);
       default:
           return QVariant();
    }
    return QVariant();
}

//定义数据别名  QHash<int, QByteArray> 父类规定的 //定义角色名称
QHash<int, QByteArray> UsbMusicListModel::roleNames() const
{
    QHash<int, QByteArray>  roles;
    roles[UsbMusicListModelDataType::type1] = "data1";
    roles[UsbMusicListModelDataType::type2] = "data2";
    roles[UsbMusicListModelDataType::type3] = "curitem";
    return  roles;
}


