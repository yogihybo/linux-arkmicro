#ifndef WIDGET_H
#define WIDGET_H

#include <QObject>
#include <QGuiApplication>
#include "ark_api.h"
class WidgetPrivate;
class Widget : public QObject
{
    Q_OBJECT
#ifdef g_Widget
#undef g_Widget
#endif
#define g_Widget (Widget::instance())
public:
    enum WidgetType{
        T_Undefine = -1,
        T_Home,
        T_MusicPlay,
        T_PhoneLink,
        T_BluetoothTel,
        T_VideoPlay,
        T_Aux,
        T_Setting,
    };
    inline static Widget* instance() {
        static Widget *widget(new Widget(qApp));
        return widget;
    }
    disp_handle* getUIDispLayer();
    disp_handle* getVideoDispLayer();
    void setPhoneLinkStatus(int status);
    int  getPhoneLinkStatus();
    void setPreemptiveWidget(int value);
    int  getPreemptiveWidget();
signals:
    void onWidgetTypeChange(const int destinationType, const int requestType, const QString &status);
    void onUsbMediaPlayExit();
    void onSdMediaPlayExit();
private:
    explicit Widget(QObject *parent = nullptr);
    ~Widget();
    WidgetPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(Widget)
};

#endif // WIDGET_H
