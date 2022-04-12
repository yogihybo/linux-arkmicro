#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

typedef enum
{
    AudioStreamMedia = 0, // 音乐
    AudioStreamCall,		// 电话
    AudioStreamRECOGNITION, // siri
    AudioStreamAlt,      // 辅助音,包括导航音,系统提示音
    AudioStreamRec,		 // 录音
    AudioStreamAlert
} AudioStreamType;

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
};

#endif // THREAD_H
