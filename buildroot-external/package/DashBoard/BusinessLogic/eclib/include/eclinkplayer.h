#ifndef ECLINKPLAYER_H
#define ECLINKPLAYER_H
#include <pthread.h>

typedef void (*USBCALLBACK)(bool);
class ECLinkplayer
{
public:
    ECLinkplayer();

    void init(int offsetx, int offsety, int screenwitdh, int screenheight);
    void show(bool show);
    void release();


    void register_usbevent(USBCALLBACK callback);

private:
    USBCALLBACK mUsbCallback;
    pthread_t     mThread;
};

#endif // ECLINKPLAYER_H
