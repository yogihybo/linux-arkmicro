#include "PhoneBookModelData.h"
#include "AutoConnect.h"
#include <QDebug>
#include <pthread.h>
#include <stdio.h>
QList<struct PhoneBookInfo> getSortedList(QList<struct PhoneBookInfo> list)
{
    for(int i = 0; i<list.length(); i++)
    {
        for (int j = i+1; j < list.length(); j++)
        {
            QByteArray ba1 = list.at(i).head.toLower().toLatin1();
            const char *ch1 = ba1.data();
            QByteArray ba2 = list.at(j).head.toLower().toLatin1();
            const char *ch2 = ba2.data();
            if(*ch1 > *ch2)
            {
                list.swap(i,j);
            }
        }
    }//end.for
    return list;
}
class PhoneBookModelDataPrivate
{
    Q_DISABLE_COPY(PhoneBookModelDataPrivate)
public:
    explicit PhoneBookModelDataPrivate(PhoneBookModelData* parent);
    ~PhoneBookModelDataPrivate();
    void connectAllSlots();
public:
    PhoneBookModel* m_PhoneBookModel;
    QList<struct PhoneBookInfo> phoneBookList ;
    QList<struct PhoneBookInfo> _PhoneBookList;
    QList<struct InitialPosition>_InitialPositionList;
private:
    Q_DECLARE_PUBLIC(PhoneBookModelData)
    PhoneBookModelData* const q_ptr;
};

PhoneBookModelData::PhoneBookModelData(QObject *parent) :
    QObject(parent),
    d_ptr(new PhoneBookModelDataPrivate(this))
{

}

PhoneBookModel* PhoneBookModelData::getObjectModel()
{
    Q_D(PhoneBookModelData);
    if(d->m_PhoneBookModel == NULL)
    {
        d->m_PhoneBookModel = new PhoneBookModel();
    }
    return d->m_PhoneBookModel;
}
static void* addModelData(void* arg){
    PhoneBookModelDataPrivate* pThis = (PhoneBookModelDataPrivate*)arg;
    int count = 0;
    int index = 0;
    int lastIndex = 0;
    while('A'+count < ('Z'+1)){
        if(index >= pThis->_PhoneBookList.size()){
            index = lastIndex;
        }
        while(index < pThis->_PhoneBookList.size())
        {
            if(pThis->_PhoneBookList.at(index).head.toUpper().toStdString().c_str()[0] == 'A'+count)
            {
                InitialPosition _InitialPositionInfo;
                _InitialPositionInfo.str = pThis->_PhoneBookList.at(index).head.toUpper();
                _InitialPositionInfo.index = index;
                pThis->_InitialPositionList.append(_InitialPositionInfo);
                lastIndex = index;
                break;
            }
            index++;
        }
        count++;
    }

}
void PhoneBookModelData::onSyncPhoneBook(){
    Q_D(PhoneBookModelData);
    if(d->m_PhoneBookModel != NULL)
    {
        d->m_PhoneBookModel->clear();
    }
    d->phoneBookList.clear();
    d->_PhoneBookList.clear();
   // qDebug()<<"=========onSyncPhoneBook=======";
    d->phoneBookList  = g_Bluetooth->getRecordList();
    d->_PhoneBookList = getSortedList(d->phoneBookList);
    for(int i = 0;i < d->_PhoneBookList.size();i++)
    {
        QString data1 = d->_PhoneBookList.at(i).head;
        QString data2 = d->_PhoneBookList.at(i).name;
        QString data3 = d->_PhoneBookList.at(i).phone;
        PhoneBookListModeData data(data1,data2,data3);
        if(d->m_PhoneBookModel != NULL)
        {
             d->m_PhoneBookModel->Add(data);
        }

    }
    pthread_t pthead;
    int ret = pthread_create(&pthead, NULL,addModelData, d);
    if(ret != 0) {
        qDebug()<<"pthread_create failed!";
    }
}
void PhoneBookModelData::onConnectStatusChange(const int status){
    Q_D(PhoneBookModelData);
    if(status < 3)
    {
        if(d->m_PhoneBookModel != NULL)
        {
             d->m_PhoneBookModel->clear();
        }

    }
}
int PhoneBookModelData::modelDataCount(){
    Q_D(PhoneBookModelData);
    QList<PhoneBookInfo> _PhoneBookList  = g_Bluetooth->getRecordList();
    return _PhoneBookList.size();
}
int PhoneBookModelData::getModelDataHead(QString str){
    Q_D(PhoneBookModelData);
    int index = 0;
    //qDebug()<<"+++++++d->_InitialPositionList.size()+++++++"<<d->_InitialPositionList.size();
    if(d->_InitialPositionList.size() > 0)
    {
        for(int i=0;i < d->_InitialPositionList.size();i++)
        {
            if(d->_InitialPositionList.at(i).str == str){
                index = d->_InitialPositionList.at(i).index;
                break;
            }
        }
    }
   // qDebug()<<"+++++++index+++++++"<<index;
    return index;
}
PhoneBookModelDataPrivate::PhoneBookModelDataPrivate(PhoneBookModelData *parent)
    : q_ptr(parent)
{
    m_PhoneBookModel =NULL;
    connectAllSlots();
}
PhoneBookModelDataPrivate::~PhoneBookModelDataPrivate()
{

}

void PhoneBookModelDataPrivate::connectAllSlots()
{
    Q_Q(PhoneBookModelData);
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onSyncPhoneBook()));
    connectSignalAndSlotByNamesake(g_Bluetooth, q, ARKRECEIVER(onConnectStatusChange(const int)));
}
