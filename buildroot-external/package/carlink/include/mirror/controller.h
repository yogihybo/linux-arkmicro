#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "inputconvertnormal.h"
#include "common.h"
#include "keycodes.h"
#include "controlmsg.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "net.h"

#ifdef __cplusplus
};
#endif

struct TouchEvent;
class ControlMsg;
class InputConvertNormal;
class Controller
{

public:
    Controller(char* gameScript = "");
    virtual ~Controller();

    void setControlSocket(socket_t controlSocket);
    void postControlMsg(ControlMsg* controlMsg);

    // turn the screen on if it was off, press BACK otherwise
    void postTurnOn();
    void postGoHome();
    void postGoMenu();
    void postGoBack();
    void postAppSwitch();
    void postPower();
    void postVolumeUp();
    void postVolumeDown();

    void setScreenPowerMode(ScreenPowerMode mode);
    void StartMirror(MirrorMode mode);
    void setVideoInfo(int VideoWidth, int VideoHeight, int ScreenWidth, int ScreenHeight);
    // for input convert

    void mouseEvent(const TouchEvent* from, const Size& frameSize, const Size& showSize);

    bool sendControl(/*const QByteArray& buffer*/const uint8_t *buffer);

protected:
    //bool event(QEvent *event);

private:

    void postKeyCodeClick(AndroidKeycode keycode);    

private:    
    socket_t m_controlSocket = NULL;       
    InputConvertNormal* m_inputConvert = NULL;

};

#endif // CONTROLLER_H
