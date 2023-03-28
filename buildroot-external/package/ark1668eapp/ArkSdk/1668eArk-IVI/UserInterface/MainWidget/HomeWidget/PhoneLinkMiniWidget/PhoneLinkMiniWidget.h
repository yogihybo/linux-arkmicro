#ifndef PHONELINKMINIWIDGET_H
#define PHONELINKMINIWIDGET_H

#include <QObject>
class PhoneLinkMiniWidgetPrivate;
class PhoneLinkMiniWidget : public QObject
{
    Q_OBJECT
public:
    explicit PhoneLinkMiniWidget(QObject *parent = nullptr);
    void setPhoneLinkMiniWidgetObject(QObject* qmlObject);
public slots:
    void onPhoneLinkMiniWidgetClicked();
protected slots:
    void onLinkStatus(int type, int mode, int status);
private:
    PhoneLinkMiniWidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(PhoneLinkMiniWidget)
};

#endif // PHONELINKMINIWIDGET_H
