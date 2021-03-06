#ifndef _BLOCK_QUEUE_H
#define _BLOCK_QUEUE_H

#include <iostream>
#include <pthread.h>
#include "../locker/locker.h"
#include <queue>

template <typename T>
class block_queue {
public:
    block_queue() = default;
    block_queue(size_t cap) {
        m_capacity = cap;
    }

    void resize(size_t cap) {
        m_capacity = cap;
    }

    ~block_queue() = default;

    bool empty() {
        LockerRAII<locker> lock(m_mutex);
        if (m_queue.empty()) {
            return true;
        }
        return false;
    }

    bool full() {
        LockerRAII<locker> lock(m_mutex);
        if(m_queue.size() >= m_capacity) {
            return true;
        }
        return false;
    }


    size_t size() {
        LockerRAII<locker> lock(m_mutex);
        size_t size = m_queue.size();
        return size;
    }

    bool push(T& item) {
        m_mutex.lock();
        if (m_queue.size() >= m_capacity) {
            m_mutex.unlock();
            m_cond.broadcast(); //满的话就广播通知别人取走
            return false;
        }
        m_queue.push(item);
        m_mutex.unlock();
        m_cond.broadcast(); //唤醒阻塞线程
        return true;
    }

    bool pop(T& get_item) {
        m_mutex.lock();
        while(m_queue.empty()) {
            if (m_cond.wait(m_mutex.get()) == false) {
                return false;
            }
        }
        get_item = m_queue.front();
        m_queue.pop();
        m_mutex.unlock();
        return true;
    }


    T& front() {
        //双检锁
        if (!m_queue.empty()) {
            LockerRAII<locker> lock(m_mutex);
            if (!m_queue.empty()) {
                T& ret = m_queue.front();
                return ret;
            }
            return NULL;
        }
    }


    bool clear(bool isreset) {
        LockerRAII<locker> lock(m_mutex);
        m_queue.clear();
        if (isreset) {
            m_capacity = 0;
        }
        return true;
    }


private:
    queue<T> m_queue;
    locker m_mutex;
    cond m_cond;
    size_t m_capacity;
};


#endif