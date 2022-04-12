#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <condition_variable>


class Timer
{
public:
    Timer();
    Timer(const Timer& timer);

      ~Timer();

public:
    void start(int interval, std::function<void()> task);
    void startOnce(int delay, std::function<void()> task);
    void stop();

    bool IsStop() const {return mStop;}
    bool mStop;

private:
    std::atomic<bool> _expired; // timer stopped status
    std::atomic<bool> _try_to_expire; // timer is in stop process
    std::mutex _mutex;
    std::condition_variable _expired_cond;

};

#endif // TIMER_H
