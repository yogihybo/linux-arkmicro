#ifndef INPUTCONVERTBASE_H
#define INPUTCONVERTBASE_H


#include "common.h"
#include "controlmsg.h"
//#include "mirrordbus.h"
//#include "generalplayer.h"
struct TouchEvent;
class Controller;
class ControlMsg;

class InputConvertBase
{

public:

    InputConvertBase(Controller* controller);
    virtual ~InputConvertBase();

    // the frame size may be different from the real device size, so we need the size
    // to which the absolute position apply, to scale it accordingly
    virtual void mouseEvent(const TouchEvent* from, const Size& frameSize, const Size& showSize) = 0;
    virtual void PhoneScreen(int& VideoWidth, int& VideoHeight, int ScreenWidth, int ScreenHeight) = 0;
    virtual void ValidRect(int VideoWidth, int VideoHeight, int ScreenWidth, int ScreenHeight) = 0;

    Controller *m_controller;
protected:
    void sendControlMsg(ControlMsg* msg);

private:

};

#endif // INPUTCONVERTBASE_H
