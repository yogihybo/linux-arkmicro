#ifndef MYMODEL_H
#define MYMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <stdio.h>
#include <QDebug>
class myData
{
public:
    myData(const QString& data):m_data(data) {}
    QVariant obj;//当前model的组件对象
    QString m_data;
};
class myModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum myModelDataType{
        type1 = Qt::UserRole,
        type2
    };
    explicit myModel(QObject *parent = nullptr);
    //外部接口 QML调用 添加数据
    Q_INVOKABLE void pushdata(const QString& data1);
    //外部接口 QML调用 添加数据在指定行
    Q_INVOKABLE void  minsert(int index, const QString& data1);
    //外部接口 删除指定行
    Q_INVOKABLE void  mremove(int index);
    //外部接口 C++调用 添加数据
    void  Add(myData&  md);
    //外部接口 清除model
    Q_INVOKABLE void clear();
    //虚函数  qml内部调用  获取第index行的内容  role 内容索引
    QVariant data(const QModelIndex &index, int role =Qt::DisplayRole) const;
    //虚函数     获取model行数
    int rowCount(const QModelIndex &parent  = QModelIndex()) const;
    // 虚函数 内容的别名  qml 内部调用
      QHash<int, QByteArray> roleNames()  const;
    //自定义  设置当前model第index行的当前组件指针
    Q_INVOKABLE void setcuritem(int index , QVariant  j)
    {
        //qDebug()<<j;
        m_datas[index].obj = j;
    }
    //~MyModel() {}
private:
    //model数据集合
    QList<myData>  m_datas;

};

#endif // MYMODEL_H
