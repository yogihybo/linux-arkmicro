#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>
//#include <QObject>
//#include "arkcommon.h"
#include "util.h"

#define QUEUE_SIZE 100
#define QUEUE_LOG_DISABLE 1


#define uint32 uint32_t
#define uint8  uint8_t




typedef struct Msg_s
{
    uint32 type;
    uint8  *pdata;
    uint32 len;
} Msg_t;


typedef struct Queue_s
{
    int head;
    int rear;
    sem_t sem;
    Msg_t data[QUEUE_SIZE];
}Queue_t;



class MsgQueue
{
public:
    MsgQueue();
    virtual ~MsgQueue();

    static MsgQueue *instance();
/*
signals:
    void sendSignal(int type, int status);
    void sendDialNumberSignal(int type, QString number);
    void sendMediaInfoSignal(int type, QString song, QString artist, QString album);
*/
public:
    //void sendLinkStatus(const int type, const int status);
    //void sendPhoneNumber(const int type, QString& number);
    //void sendMediaInfo(const int type, QString& song, QString& artist, QString& album);

    int InitMessage(void *paramter);
    int SendMessage(int MsgID, char* pMsgData = NULL, int MsgLen = 0);

    int HandlerMessage(int MsgID, char *pMsgData, int MsgLen);

   void SetCallback(void (*callback)(int, void*, int, void*, void *), void *parameter);

private:
    int MsgQueueInit(Queue_t* Q);
    int MsgDeQueue(Queue_t* Q, Msg_t* msg);
    int MsgEnQueue(Queue_t* Q, Msg_t* msg);

    static void* SendMsgFunc(void *parameter);
    static void* MsgHandlerFunc(void *parameter);
private:

    void (*m_callback)(int,void *,int,void*,void*);
    void*               m_parameter_player;
    void*               m_parameter;
    Queue_t             m_MsgQueue;
    Msg_t               m_Msg;
    sem_t               m_sSem;

    static MsgQueue *mInstance;

};

#endif // MSGQUEUE_H
