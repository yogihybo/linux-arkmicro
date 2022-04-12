#ifndef EAPCHANNEL_H
#define EAPCHANNEL_H

#include "thread.h"
#include "CCarLifeLibWrapper.h"


class EapChannel : public Thread
{

public:
    explicit EapChannel();
    virtual ~EapChannel();


    static EapChannel *instance();

    void SetShannel(int channel);
protected:
    virtual void run();             //线程执行函数
private:
    static EapChannel *mInstance;
    int m_nChannel;

};

#endif // EAPCHANNEL_H
