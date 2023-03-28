#include "BluetoothNameModelData.h"
#include "AutoConnect.h"
#include <QDomDocument>
#include <QDebug>
class BluetoothNameModelDataPrivate
{
    Q_DISABLE_COPY(BluetoothNameModelDataPrivate)
public:
    explicit BluetoothNameModelDataPrivate(BluetoothNameModelData* parent);
    ~BluetoothNameModelDataPrivate();
    void connectAllSlots();
public:
    myModel* m_myModel;
private:
    Q_DECLARE_PUBLIC(BluetoothNameModelData)
    BluetoothNameModelData* const q_ptr;
};
BluetoothNameModelData::BluetoothNameModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new BluetoothNameModelDataPrivate(this))
{

}
myModel* BluetoothNameModelData::getObjectModel(){

    Q_D(BluetoothNameModelData);
    if(d->m_myModel == NULL)
    {
        d->m_myModel = new myModel();
    }
    return d->m_myModel;
}
void BluetoothNameModelData::onScanFinish()
{
    Q_D(BluetoothNameModelData);
    QList<struct RemoteDeviceInfo> _RemoteDeviceInfoList;
    _RemoteDeviceInfoList.clear();
    _RemoteDeviceInfoList = g_Bluetooth->getRemoteDeviceInfoList();
    if(_RemoteDeviceInfoList.size() > 0)
    {
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }
        for(int i=0;i<_RemoteDeviceInfoList.size();i++)
        {
            QString data1 = QString("%1 ").arg(i+1,3,10,QChar('0'))+ _RemoteDeviceInfoList.at(i).name;
            myData data(data1);
            if(d->m_myModel != NULL)
            {
                d->m_myModel->Add(data);
            }
        }
    }
}
void BluetoothNameModelData::onGetPairedListFinish()
{
    Q_D(BluetoothNameModelData);
    QMap<QString, QString> _PairedList = g_Bluetooth->getPairedList();
    if(_PairedList.size() > 0){
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }

        QMap<QString, QString>::iterator itor = _PairedList.begin();
        int count = 0;
        while(itor != _PairedList.end())
        {
            QString data1 = QString("%1 ").arg(count+1,3,10,QChar('0'))+ itor.value();
            myData data(data1);
            if(d->m_myModel != NULL)
            {
                d->m_myModel->Add(data);
            }
            itor++;
            count++;
        }
    }
}
void BluetoothNameModelData::onPowerChange(int mode)
{
    qDebug()<<"+++[BluetoothNameModelData::onPowerChange]+++"<<mode;
    Q_D(BluetoothNameModelData);
    if(mode == 0){
        if(d->m_myModel != NULL)
        {
            d->m_myModel->clear();
        }

    }

}
BluetoothNameModelDataPrivate::BluetoothNameModelDataPrivate(BluetoothNameModelData *parent)
    : q_ptr(parent)
{
    m_myModel = NULL;
    connectAllSlots();
}
BluetoothNameModelDataPrivate::~BluetoothNameModelDataPrivate()
{

}
void BluetoothNameModelDataPrivate::connectAllSlots()
{
    Q_Q(BluetoothNameModelData);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onScanFinish()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onGetPairedListFinish()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onPowerChange(int)));
}
