#ifndef PHONELINKWIDGET_H
#define PHONELINKWIDGET_H

#include <QObject>
class PhoneLinkWidgetPrivate;
class PhoneLinkWidget : public QObject
{
    Q_OBJECT
public:
    explicit PhoneLinkWidget(QObject *parent = nullptr);
    void setPhoneLinkWidgetObject(QObject* qmlObject);

private:
    PhoneLinkWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(PhoneLinkWidget)
};

#endif // PHONELINKWIDGET_H
