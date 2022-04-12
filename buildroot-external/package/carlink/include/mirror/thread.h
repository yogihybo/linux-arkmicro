#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>



typedef void (*CALLBACK)(void);

class Thread
{
    public:
        Thread();
        virtual ~Thread();

        bool start(void *object = NULL);
        virtual void exit();
        virtual void exit2();

        bool isRunning(){
            return (m_thread_id != 0);
        }
        bool setName(const char* name);

    protected:
        virtual void run() = 0;
        bool is_stop() const;

    private:
        static void *thread_func(void *param);

    protected:
        void (*m_callback)();
        void *m_Object;
        bool m__thread_stop;
        pthread_t m_thread_id;
        struct sched_param m_param;
        pthread_mutex_t mArkMutex;
};

#endif // THREAD_H
