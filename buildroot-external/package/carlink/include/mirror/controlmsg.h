#ifndef CONTROLMSG_H
#define CONTROLMSG_H

#include "inputconvertbase.h"
#include <stdint.h>
#include "input.h"
#include "keycodes.h"
#include <string>

#define CONTROL_MSG_TEXT_MAX_LENGTH 300
#define CONTROL_MSG_CLIPBOARD_TEXT_MAX_LENGTH 4093
// ControlMsg


class ControlMsg
{
public:    
    enum ControlMsgType {
        CMT_NULL = -1,
        CMT_INJECT_KEYCODE = 0,
        CMT_INJECT_TEXT,
        CMT_INJECT_MOUSE,
        CMT_INJECT_SCROLL,
        CMT_BACK_OR_SCREEN_ON,
        CMT_EXPAND_NOTIFICATION_PANEL,
        CMT_COLLAPSE_NOTIFICATION_PANEL,
        CMT_GET_CLIPBOARD,
        CMT_SET_CLIPBOARD,
        CMT_SET_SCREEN_POWER_MODE,        
        CMT_INJECT_TOUCH,
        CMT_SET_MIRROR,
    };



    ControlMsg(ControlMsgType controlMsgType);
    virtual ~ControlMsg();

    void setInjectKeycodeMsgData(AndroidKeyeventAction action, AndroidKeycode keycode, AndroidMetastate metastate);
    void setInjectTextMsgData(std::string text);
    void setInjectMouseMsgData(AndroidMotioneventAction action, AndroidMotioneventButtons buttons, Rect position);
    // id 代表一个触摸点，最多支持10个触摸点[0,9]
    // action 只能是AMOTION_EVENT_ACTION_DOWN，AMOTION_EVENT_ACTION_UP，AMOTION_EVENT_ACTION_MOVE
    // position action动作对应的位置
    void setInjectTouchMsgData(unsigned int id, AndroidMotioneventAction action, Rect position);
    void setInjectScrollMsgData(Rect position, unsigned int hScroll, unsigned int vScroll);
    void setSetClipboardMsgData(std::string text);
    void setSetScreenPowerModeData(ScreenPowerMode mode);
    void setMirrorModeData(MirrorMode mode);

    uint8_t* serializeData();
    void writePosition(uint8_t* buffer, const Rect& value);

    void WriteButton(uint8_t* buffer, unsigned int value);
private:    


private:
    struct ControlMsgData {
        ControlMsgType type = CMT_NULL;
        //union {
            struct {
                AndroidKeyeventAction action;
                AndroidKeycode keycode;
                AndroidMetastate metastate;
            } injectKeycode;
            struct {
                char* text;
            } injectText;
            struct {
                AndroidMotioneventAction action;
                AndroidMotioneventButtons buttons;
                Rect position;
            } injectMouse;
            struct {
                unsigned int id;
                AndroidMotioneventAction action;
                Rect position;
            } injectTouch;
            struct {
                Rect position;
                unsigned int hScroll;
                unsigned int vScroll;
            } injectScroll;
            struct {
                char *text ;
            } setClipboard;
            struct {
                ScreenPowerMode mode;
            } setScreenPowerMode;

            struct{
                MirrorMode mode;
            } setMirrorMode;
        //};

        ControlMsgData(){}
        ~ControlMsgData(){}
    };

    ControlMsgData m_data;
};

#endif // CONTROLMSG_H
