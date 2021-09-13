#pragma once
/*
    本函数为webserver专用定时器函数
*/
#include <ctime>
#include <vector>
#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <memory>
#include <mutex>
#include <map>
#include <unordered_map>


typedef std::function<void(void*)> TimerEvent;
typedef std::pair<int, int> TimerId;

namespace webserver {

class Timer {
public:
    // Timer() = default;
    // Timer(uint32_t ms); 
    Timer(uint32_t ms, const TimerEvent& event, void* p_user = nullptr, bool repeat = false);
    ~Timer() {};

    static void Sleep(uint64_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void Start(int64_t ms, bool repeat = false);

    void Stop() {
        m_is_repeat = false;
    }

    void SetEventCallback(const TimerEvent& event) {
        m_event_callback = event;
    }

    bool IsRepeat() const { return m_is_repeat; }


    void SetNextTimeout(int64_t current_time_point) {       //把当前时间点作为输入
        m_next_timeout = current_time_point + m_interval;
    }
    
    int64_t GetNextTimeout() const { return m_next_timeout; }

private:
    friend class TimerQueue;

    TimerEvent m_event_callback = [](void*){};

    int64_t m_next_timeout;

    int32_t m_interval;

    bool m_is_repeat = false;

    void* m_args;
}; 


class TimerQueue {
public:
    TimerId AddTimer(const TimerEvent& event, uint32_t ms, bool repeat, void* p_user);

    void RemoveTimer(TimerId);

    int64_t GetTimerRemaining();

    void HandleTImerEvent();

    int64_t GetTimeNow();

private:

    std::mutex m_mutex;

    std::map<TimerId, std::shared_ptr<Timer>> m_timers;

    std::unordered_map<uint32_t, std::shared_ptr<Timer>> m_repeat_timers;

    uint32_t m_last_timer_id = 0;

    uint32_t m_timer_remaining = 0;

};




} // namespace mywebserver