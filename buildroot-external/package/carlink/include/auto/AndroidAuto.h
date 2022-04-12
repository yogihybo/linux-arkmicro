#ifndef ANDROIDAUTO_H
#define ANDROIDAUTO_H

class AndroidAutoPrivate;
class IUserAutoCbs;
class AndroidAuto
{
public:
    AndroidAuto();
    virtual ~AndroidAuto();

    void setConfig(const char* key, const char* value);
    void setConfig(const char* key, int value);
    const char* getConfigString(const char* key, const char* defaultValue);
    int getConfigInt(const char* key, int defaultValue);

    void registerCallbacks(IUserAutoCbs *cbs);
    //void startSession();
    void startSession(bool isWifi = true);

    void getVideoFocus();
    void releaseVideoFocus();
    void getAudioFocus();
    void releaseAudioFocus();

    void setBTParingStatus(bool paired, const char *pinCode, int status);
    void sendTouchEvent(unsigned int x, unsigned int y, int action);
    void sendKeyEvent(unsigned int keycode, int press);
    void sendKnobEvent(unsigned int keycode, int delta);
private:
    AndroidAutoPrivate*     mHandle;
};

#endif // ANDROIDAUTO_H
