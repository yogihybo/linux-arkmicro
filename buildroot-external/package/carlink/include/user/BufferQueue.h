#pragma once


#include <pthread.h>

struct QueueMsg {
    int type;
    int len;
    unsigned char * buf;
    struct QueueMsg * next;
};

class BufferQueue{

public:
    BufferQueue();
    virtual ~BufferQueue();

public:
    void ClearBufferQueue();

    void WriteQueue(int type,  unsigned char * msgBuf, int msgLen);

    int ReadQueue(int *pType,  unsigned char ** msgBuf, int * msgLen);

    int GetQuequeSize();

    void SetQueueCond(pthread_cond_t * cond);

    void InitThread();

    bool IsExitedThread(pthread_t thread_id);

    void ExitedThread(pthread_t thread_id);

    void WakeUpThread(pthread_t thread_id);
private:
    void ClearInnerBufferQueue(pthread_mutex_t * p_mutex,
                               QueueMsg ** pQueueHead,
                               QueueMsg ** pQueueTail,
                               int * pQueueSize);

    void WriteInnerQueue(pthread_mutex_t * p_mutex,
                         unsigned char * msgBuf,
                         int msgLen,
                         QueueMsg ** pQueueHead,
                         QueueMsg ** pQueueTail,
                         int * pQueueSize,
                         int type);

    int ReadInnerQueue(pthread_mutex_t * p_mutex,
                       unsigned char ** msgBuf,
                       int * msgLen,
                       QueueMsg ** pQueueHead,
                       QueueMsg ** pQueueTail,
                       int * pQueueSize,
                       int *pType);

private:
    pthread_mutex_t     mQueueMutex;
    QueueMsg *          mPtrQueueHead;
    QueueMsg *          mPtrQueueTail;
    int                 mQueueSize;
    pthread_mutex_t     mThreadMutex;
    pthread_cond_t      mThreadCond;
    bool                mExitedThread;

};

