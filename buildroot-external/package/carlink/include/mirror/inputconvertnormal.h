#ifndef INPUTCONVERT_H
#define INPUTCONVERT_H

//#include "inputconvertbase.h"
#include "common.h"
#include "controlmsg.h"
//#include "mirrordbus.h"


struct TouchEvent
{
    enum TouchPointState {
        TouchPointPressed    = 0x01,
        TouchPointMoved      = 0x02,
        TouchPointStationary = 0x04,
        TouchPointReleased   = 0x08,
    };
    TouchEvent(){}
    TouchEvent(TouchEvent::TouchPointState state, int x, int y)
        : state(state)
        , y(y)
        , x(x) {}
    ~TouchEvent() {}
    int x;
    int y;
    TouchEvent::TouchPointState state;
};

struct TouchEvent;
class Controller;
class InputConvertNormal
{

public:

    InputConvertNormal(Controller* controller);
    virtual ~InputConvertNormal();

    virtual void mouseEvent(const TouchEvent* from, const Size& frameSize, const Size& showSize);

    virtual void PhoneScreen(int& VideoWidth, int& VideoHeight, int ScreenWidth, int ScreenHeight);

    virtual void ValidRect(int VideoWidth, int VideoHeight, int ScreenWidth, int ScreenHeight);

    void FixPos(int x, int y, int* phone_x, int *phone_y, double zoom, Rect rc);
private:
    Controller *m_controller;
    int m_VideoWidth;
    int m_VideoHeight;
    double mRatio;
    Rect mRect;
};

#endif // INPUTCONVERT_H
