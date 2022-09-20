#ifndef INTERFACECALLBACK_H
#define INTERFACECALLBACK_H

#include <string>
using namespace std;


class InterfaceCallback
{
public:
    InterfaceCallback(){}

public:
 virtual void start(int linkType, int linkMode, int status) = 0;

 virtual void touch(int x, int y, int press) = 0;

 virtual void multi_touch(int x1, int y1, int press1, int x2, int y2, int press2) = 0;

 virtual void key(int keyCode) = 0;

 virtual void wheel(int wheelCode, bool foucs) = 0;

 virtual void night_mode(bool night) = 0;

 virtual void right_hand_dirver(bool right) = 0;

 virtual void phone_ip(string ipstring) = 0;

 virtual void phone_bt(string btstring) = 0;

 virtual void car_bluetooch(string name, string address, string pin) = 0;

 virtual void wifi(string ssid, string passphrase, string channel_id) = 0;

 virtual void license(string lisence) = 0;

 virtual void input_text(string text) = 0;

 virtual void input_selection(int start, int stop) = 0;

 virtual void input_action(int acionId, int keyCode) = 0;

 virtual void screen_size(int width, int height) = 0;

 virtual void bluetooth_cmd(string cmd) = 0;

 virtual void broadcast(bool enable) = 0;

 virtual void delay_record(int millisecond) = 0;

 virtual void wifi_state_changed(int action, int state, string phoneIp, string carIp) = 0;
};

#endif // INTERFACECALLBACK_H
