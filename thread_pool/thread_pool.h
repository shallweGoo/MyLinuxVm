#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <iostream>
#include "../locker/locker.h"
#include "../log/block_queue.h"
#include "../mysql/sql_connection_pool.h"
#include "../basedefine/base_define.h"
#include "../log/log.h"
#include "../timer/webserver_timer.h"
#include <vector>
#include <string>
#include <thread>
#include <memory>
#include <ctime>
#include <cmath>
#include <atomic>
#include <unordered_set>

template <typename T>
class thread_pool {
public:
    //默认线程池数量为8
    thread_pool(int actor_model, connection_pool* conn_pool, int max_thread_num = 16, int max_request = 10000, int timeout = 60);
    ~thread_pool();
    bool append(T* req, int state);
    bool append_p(T* req);
    void init(time_t timeout = 60, float add_factor = 0.8, float recycle_factor = 0.2, float init_thread_factor = 0.5);
    int GetValidThreadNum() {return m_free_thread_num;}

public:
    //设置类相关
    void SetTimeout(time_t ss) { m_thread_time_out = ss;} //秒为单位
    void SetAddFactor(float add_factor) { m_add_factor = add_factor; }
    void SetRecycleFactor(float recycle_factor) { m_recycle_factor = recycle_factor; }
    void SetInitThreadFactor(float init_thread_factor) { m_init_thread_factor = init_thread_factor;} 

private:
    static void* Worker(void* arg); //工作线程，让线程池里面的线程都注册为这个
    void Run();
    static void* Manager(void* arg); // 管理者线程
    void Check();
private:
    int m_max_thread_num;
    int m_free_thread_num;

    int m_max_requests;
    // vector<thread> m_threads;
    unordered_set<std::thread::id> m_threads;               // 线程管理
    unordered_set<std::thread::id> m_recycle_thread_ids;    // 需要回收的id集合
    block_queue<T*> m_request_queue; //请求队列
    sem m_task_sem;
    sem m_thread_sem;
    timespec m_time_spec;         // 信号量超时时间系数
    connection_pool* m_conn_pool;
    int m_actor_model;

    //log
    int m_log_status = LogEnable;

    //一些线程池自动回收机制相关的控制变量
    static std::mutex m_mutex;           // 互斥锁
    time_t m_thread_time_out;     // 设置回收事件为60s;
    float m_add_factor;           // 线程池扩容负载系数
    float m_recycle_factor;       // 线程池回收负载系数
    float m_init_thread_factor;   // 初始化线程池线程数量系数
};

template <typename T>
std::mutex thread_pool<T>::m_mutex; // 初始化互斥锁



template <typename T>
thread_pool<T>::thread_pool(int actor_model, connection_pool* conn_pool,
                            int max_thread_num, int max_request, int timeout):
                            m_max_thread_num(max_thread_num),
                            m_max_requests(max_request),
                            m_conn_pool(conn_pool),
                            m_actor_model(actor_model) {
    if (max_thread_num <= 0 || max_request <= 0) {
        LOG_ERROR("%s", "thread pool construct fail because thread num unenough.");
        exit(-1);
    }

    init(timeout);  //设置超时时间和扩容相关变量
    int create_thread_num = floor(max_thread_num * m_init_thread_factor);  // 初始化线程数

    for (int i = 0; i < create_thread_num; ++i) {
        thread worker_thread(Worker, this);
        std::thread::id thread_id = worker_thread.get_id();
        std::unique_lock<std::mutex> locker(m_mutex);
        m_threads.insert(thread_id);
        --m_free_thread_num;
        locker.unlock();
        if (worker_thread.joinable()) {
            worker_thread.detach();
        } else {
            LOG_ERROR("%s", "thread detach error");
            exit(-1);
        }
        
    }

    // 管理者线程
    thread manager_thread(Manager, this);
    if (manager_thread.joinable()) {
        manager_thread.detach();
    } else {
        LOG_ERROR("%s", "thread detach error");
        exit(-1);
    }
    
    m_request_queue.resize(m_max_requests);
    m_task_sem = 0;                                //  初始化资源信号量
    m_free_thread_num = create_thread_num;         //  空闲线程数现在等于一个最大线程数
}

template <typename T>
void thread_pool<T>::init(time_t timeout, float add_factor, float recycle_factor, float init_thread_factor) {
    SetTimeout(timeout);
    SetAddFactor(add_factor);
    SetRecycleFactor(recycle_factor);
    clock_gettime(CLOCK_REALTIME, &m_time_spec); // 使用的是UTC时间的秒数, 用于信号量的超时等待
    m_time_spec.tv_sec += m_thread_time_out;
    SetInitThreadFactor(init_thread_factor);
}


template <typename T>
void thread_pool<T>::Check() {
    while (true) { 
        std::unique_lock<std::mutex> locker(m_mutex);
        float factor1 = static_cast<float>(m_free_thread_num) / m_threads.size();   // 空闲线程比例
        float factor2 = 1.0 - factor1;                                              // 工作线程比例
        if (factor1 <= m_recycle_factor) {                                          // 回收逻辑
            for (auto itr = m_recycle_thread_ids.begin(); itr != m_recycle_thread_ids.end(); ++ itr) {
                auto m_threads_itr = m_threads.find(*itr);
                if (m_threads_itr != m_threads.end()) {
                    m_threads.erase(m_threads_itr);
                }
            }
            m_recycle_thread_ids.clear();
        }
        if (factor2 >= m_add_factor) {                                              // 扩容逻辑
            int now_therad_nums = m_threads.size();
            int new_thread_nums = min(m_max_thread_num, int(1.5 * now_therad_nums));
            int need_add_thread_num = new_thread_nums - now_therad_nums;
            for (int i = 0; i < need_add_thread_num; ++ i) {
                std::thread worker_thread(Worker, this);
                std::thread::id id = worker_thread.get_id();
                m_threads.insert(id);
                std::unique_lock<std::mutex> locker(m_mutex);
                --m_free_thread_num;
                locker.unlock();
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));      //10s进行一次管理
    }
}


/**
 * @brief 管理者线程函数
 * @param arg 传入的是this指针
 * @return 
 * */
template <typename T>
void* thread_pool<T>::Manager(void* arg) {
    thread_pool* tp = static_cast<thread_pool*>(arg);
    tp->Check();
    return tp;
}


template <typename T>
thread_pool<T>::~thread_pool() {
    // LOG_DEBUG("%s", "thread pool destruct");
    m_threads.clear();
}

template <typename T>
bool thread_pool<T>::append(T* req, int state) {
    if ((int)m_request_queue.size() >= m_max_requests) {
        // LOG_ERROR("%s", "Reactor append fail!");
        return false;
    }
    req->m_state = state;       //一个http请求
    m_request_queue.push(req);  //已经加锁了
    m_task_sem.post();          //信号量post
    return true;
}

template <typename T>
bool thread_pool<T>::append_p(T* req) {
    if ((int)m_request_queue.size() >= m_max_requests) {
        return false;
    }
    m_request_queue.push(req);  //已经加锁了
    m_task_sem.post();          //信号量post
    return true;
}

template <typename T>
void* thread_pool<T>::Worker(void* arg) {
    thread_pool* tp = static_cast<thread_pool*>(arg);
    tp->Run();
    return tp;
}

template <typename T>
void thread_pool<T>::Run() {
    // time_t thread_start_time = time(NULL);     //初始化一个时间为秒数
    while (true) {
        // m_task_sem.wait();                  //all block here
        int sem_ret = m_task_sem.timewait(&m_time_spec);
        if (sem_ret == -1) {                   //说明信号量超时
            if (errno == ETIMEDOUT) {
                std::unique_lock<std::mutex> locker(m_mutex);
                ++m_free_thread_num;
                m_recycle_thread_ids.insert(std::this_thread::get_id());
                break;
            } else if (errno == EAGAIN) {
                continue;
            }
        }
        if (m_request_queue.empty()) { continue; }
        T* req = nullptr;
        if (m_request_queue.pop(req) == false || req == nullptr) { continue; }
        //1 = reactor, 0 = proactor
        if (ReactorMode == m_actor_model) {
            if (ReadState == req->m_state) {  //写状态
                if (req->ReadOnce() == true) {
                    // std::cout << "read_once == true " << std::endl;
                    req->m_isdeal = ConnIsDeal;
                    connectionRAII mysqlcon(&(req->m_mysql), m_conn_pool); //从sql连接池里面获取一个连接
                    req->process();
                } else {
                    req->m_isdeal = ConnIsDeal;
                    req->m_timer_flag = TimerDisable; //means timer should be closed
                }
            } else { //写状态               
                if (req->Write() == false) { //如果写失败了，那么timer需要被删除
                    // LOG_ERROR("%s", "reactor write fail");
                    req->m_isdeal = ConnIsDeal; 
                    req->m_timer_flag = TimerDisable;
                } else {
                    req->m_isdeal = ConnIsDeal;
                }
            }
        } else {
            connectionRAII mysqlcon(&(req->m_mysql), m_conn_pool); //从sql连接池里面获取一个连接
            req->process();
        }
    }
}


#endif