#ifndef STREAM_H
#define STREAM_H

#include "thread.h"
//#include "videodecode.h"
//#include "BufferQueue.h"
#include "server.h"
#include "controller.h"
#include <functional>
typedef std::function<void (bool, int, int)> FUNCVIDEOSTART;
typedef std::function<void (int, int, unsigned char*, int)> FUNCVIDEOINFO;

//class Decoder;
class Stream : public Thread
{

public:
    typedef struct FrameMeta {
        unsigned long long pts;
        struct FrameMeta* next;
    } FrameMeta;

    typedef struct ReceiverState {
        // meta (in order) for frames not consumed yet
        FrameMeta* frameMetaQueue;
        int remaining; // remaining bytes to receive for the current frame
    } ReceiverState;

    Stream();
    virtual ~Stream();

public:

//    void setDecoder(VideoDecode* decoder);
    void setVideoSocket(socket_t deviceSocket);
    void setController(Controller *pControl);    
    bool startDecode();
    void stopDecode();
//    VideoDecode *getDecoder() const {return m_decoder;}
    void SetVideoStartCallback(FUNCVIDEOSTART funcVideoStart);
    void SetVideoInfoCallback(FUNCVIDEOINFO funcVideoInfo);
    ReceiverState* getReceiverState();


protected:
    void run();

public:
    socket_t            m_videoSocket;
    Controller*         mController;
    bool m_quit;
    FUNCVIDEOSTART      mFuncVideoStart;
    FUNCVIDEOINFO       mFuncVideoInfo;

    // for recorder
    ReceiverState m_receiverState;
    //VideoDecode* m_decoder = NULL;
private:

};

#endif // STREAM_H
