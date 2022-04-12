// Copyright 2014 Art. All Rights Reserved.

#ifndef POSIX_THREAD_H
#define POSIX_THREAD_H

#include <pthread.h>
#include <sched.h>
#include <semaphore.h>

//thread
class ArkThread {
public:
    ArkThread() : mRunning(false) { }
    virtual ~ArkThread();
    bool start();
    bool join();
    unsigned long id();
    bool isRunning();
    void yield();
    bool setPriority(int priority);
    bool setName(const char* name);
protected:
    static void* callback(void* arg);
    virtual void run() = 0;
private:
    pthread_t mThread;
    bool mRunning;
};

//lock
class Mutex {
public:
    Mutex();
    ~Mutex();
    void lock();
    void unlock();
private:
    pthread_mutex_t mMutex;
};

class Autolock {
public:
    Autolock(Mutex* mutex);
    ~Autolock();
private:
    Mutex* mMutex;
};

//sem
class Semaphore {
public:
    Semaphore();
    Semaphore(int value);
    ~Semaphore();
    bool down();
    bool up();
private:
    sem_t mSem;
};
#endif
