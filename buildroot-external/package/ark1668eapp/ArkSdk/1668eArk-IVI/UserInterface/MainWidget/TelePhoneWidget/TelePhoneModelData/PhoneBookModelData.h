#ifndef PHONEBOOKMODELDATA_H
#define PHONEBOOKMODELDATA_H

#include <QObject>
#include "PhoneBookModel.h"
#include "./BusinessLogic/Bluetooth.h"
struct InitialPosition
{
    QString str;
    int index;
};
class PhoneBookModelDataPrivate;
class PhoneBookModelData : public QObject
{
    Q_OBJECT
public:
    explicit PhoneBookModelData(QObject *parent = nullptr);
    Q_INVOKABLE PhoneBookModel* getObjectModel();
    Q_INVOKABLE int modelDataCount();
    Q_INVOKABLE int getModelDataHead(QString str);
protected slots:
    void onSyncPhoneBook();
    void onConnectStatusChange(const int status);
private:
    PhoneBookModelDataPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(PhoneBookModelData)
};

#endif // PHONEBOOKMODELDATA_H
