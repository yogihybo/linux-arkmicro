#ifndef TOOLWIGET_H
#define TOOLWIGET_H

#include <QObject>
class ToolWigetPrivate;
class ToolWiget : public QObject
{
    Q_OBJECT
public:
    explicit ToolWiget(QObject *parent = nullptr);
    void setToolWigetObject(QObject* qmlObject);
    QObject* getToolWigetObject();
protected slots:
    void onDataTimeSetting();
    void onTimeout();
private:
    ToolWigetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(ToolWiget)
};

#endif // TOOLWIGET_H
