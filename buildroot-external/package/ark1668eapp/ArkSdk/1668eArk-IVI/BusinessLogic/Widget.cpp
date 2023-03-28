#include "Widget.h"
#include <QDebug>
class WidgetPrivate
{
    Q_DISABLE_COPY(WidgetPrivate)
public:
    explicit WidgetPrivate(Widget* parent);
    ~WidgetPrivate();
public:
    disp_handle *disp_layer_ui;
    disp_handle *disp_layer_video;
    int  m_PhoneLinkStatus;
    bool m_PreemptiveWidget;
private:
    Q_DECLARE_PUBLIC(Widget)
    Widget* const q_ptr;
};


Widget::Widget(QObject *parent)
    : QObject(parent),
      d_ptr(new WidgetPrivate(this))
{

}

Widget::~Widget()
{

}

disp_handle* Widget::getUIDispLayer(){
    Q_D(Widget);
    return d->disp_layer_ui;
}
disp_handle* Widget::getVideoDispLayer(){
    Q_D(Widget);
    return d->disp_layer_video;
}

void Widget::setPhoneLinkStatus(int status)
{
    Q_D(Widget);
    d->m_PhoneLinkStatus = status;
}
int  Widget::getPhoneLinkStatus(){
    Q_D(Widget);
    return d->m_PhoneLinkStatus;
}

void Widget::setPreemptiveWidget(int value){
    Q_D(Widget);
    d->m_PreemptiveWidget = value;
}
int  Widget::getPreemptiveWidget(){
    Q_D(Widget);
    return d->m_PreemptiveWidget;
}

WidgetPrivate::WidgetPrivate(Widget *parent)
    : q_ptr(parent)
{
#ifdef __ARM__
    disp_layer_ui    = arkapi_display_open_layer(PRIMARY_LAYER); //UI
    disp_layer_video = arkapi_display_open_layer(VIDEO_LAYER); //video
#endif
    m_PhoneLinkStatus  = 0;
    m_PreemptiveWidget = 0;
}

WidgetPrivate::~WidgetPrivate()
{

}
