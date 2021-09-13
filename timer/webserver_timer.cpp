#include "webserver_timer.h"

using namespace std;

namespace webserver {


// Timer::Timer(uint32_t ms) : m_interval(ms) {
//     m_is_repeat = false;
//     m_p_user = nullptr; 
// }


Timer::Timer(uint32_t ms, const TimerEvent& event, void* p_user, bool repeat)
    : m_event_callback(event)
    , m_interval(ms)
    , m_is_repeat(repeat)
    , m_args(p_user) 
{
    if (m_interval == 0) {
        m_interval = 1;
    }
}

void Timer::Start(int64_t microseconds, bool repeat) {
    m_is_repeat = repeat;
    auto time_begin = std::chrono::high_resolution_clock::now();
    int64_t elapsed = 0;
    do {
        std::this_thread::sleep_for(std::chrono::microseconds(microseconds - elapsed));
        time_begin = std::chrono::high_resolution_clock::now();
        m_event_callback(m_args); //执行完一次函数之后
        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - time_begin).count();
        if (elapsed < 0) {
            elapsed = 0;
        }
    } while (m_is_repeat);

}


int64_t TimerQueue::GetTimeNow() {
    auto time_point = std::chrono::steady_clock::now();                                     // 不会被系统调整的单调时钟
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();      // 转化为毫秒
}


TimerId TimerQueue::AddTimer(const TimerEvent& event, uint32_t ms, bool repeat, void* p_user) {
    std::unique_lock<std::mutex> locker(m_mutex);

    int64_t timeout_point = GetTimeNow();

    TimerId timer_id = {timeout_point + ms, ++m_last_timer_id};
    
    shared_ptr<Timer> this_timer =  make_shared<Timer>(ms, event, p_user, repeat);

    this_timer->SetNextTimeout(timeout_point);

    if (repeat) {
        m_repeat_timers.emplace(timer_id.second, this_timer);
    }

    m_timers.insert({timer_id, this_timer});
    return timer_id;
}

void TimerQueue::RemoveTimer(TimerId timer_id) {
    std::unique_lock<std::mutex> locker(m_mutex);
    auto itr = m_repeat_timers.find(timer_id.second);
    if (itr != m_repeat_timers.end()) {
        TimerId t = {itr->second->GetNextTimeout(), timer_id.second};   //获取下一次到期的TimerId, 可以检索出来
        m_repeat_timers.erase(itr);
        m_timers.erase(t);
    } else {
        m_timers.erase(timer_id);
    }
}

int64_t TimerQueue::GetTimerRemaining() {
    std::unique_lock<std::mutex> locker(m_mutex);
    if (m_timers.empty()) {
        return -1;
    }

    // 最早过期时间的事件在begin这个位置
    int64_t ms = m_timers.begin()->first.first - GetTimeNow();
    
    if (ms <= 0) {  //已经过期
        return 0;
    }

    return ms;
}

void TimerQueue::HandleTImerEvent() {
    if (!m_timers.empty() || !m_timers.empty()) {
        std::unique_lock<std::mutex> locker(m_mutex);
        int64_t now_time_point = GetTimeNow();
        while (!m_timers.empty() && m_timers.begin()->first.first <= now_time_point) {
            m_timers.begin()->second->m_event_callback(m_timers.begin()->second->m_args);
            if (m_timers.begin()->second->IsRepeat()) { // 如果是一个重复任务
                m_timers.begin()->second->SetNextTimeout(now_time_point); //更新下一个任务执行时间
                TimerId t_id = {m_timers.begin()->second->GetNextTimeout(), m_timers.begin()->first.second};
                auto timer_ptr = std::move(m_timers.begin()->second);
                m_timers.erase(m_timers.begin());
                m_timers.insert({t_id, std::move(timer_ptr)});
            } else {
                m_timers.erase(m_timers.begin());
            }
        }
    }

}


} // namespace mywebserver
