#ifndef PHONELINKMSGSHOWWIDGET_H
#define PHONELINKMSGSHOWWIDGET_H

#include <QObject>
class PhoneLinkMsgShowWidgetPrivate;
class PhoneLinkMsgShowWidget : public QObject
{
    Q_OBJECT
public:
    explicit PhoneLinkMsgShowWidget(QObject *parent = nullptr);
    void setPhoneLinkMsgShowWidgetObject(QObject* qmlObject);
public slots:
    void onPhoneLinkMsgShowWidgetShow(QString msg);
private:
    PhoneLinkMsgShowWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(PhoneLinkMsgShowWidget)
};

#endif // PHONELINKMSGSHOWWIDGET_H
