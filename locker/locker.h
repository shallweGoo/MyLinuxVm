#ifndef LOCKER_H
#define LOCKER_H

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <exception>
#include "../lib/noncopyable.h"
using namespace std;

class sem {
public: 
    sem(int num = 0) {
        if (sem_init(&m_sem, 0, num) != 0) {
            throw std::exception();
        }
    };

    ~sem() {
        if (sem_destroy(&m_sem) != 0) {
            throw std::exception(); 
        }
    };

    /*允许赋值符号*/

    // sem(sem& ) = delete; 
    // sem& operator=(const sem&)=delete;

    bool wait() {
        return sem_wait(&m_sem) == 0;
    }

    bool post() {
        return sem_post(&m_sem) == 0;
    }

    bool timewait(timespec* t_spc) {
        return sem_timedwait(&m_sem, t_spc) == 0;
    }

private:
    sem_t m_sem;
    time_t m_now_time;
};


class locker {
public:
    locker() {
        if (pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }
    ~locker() {
        if (pthread_mutex_destroy(&m_mutex) != 0) {
            throw std::exception();
        }
    }

    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    
    // bool trylock()
    // {
    //     return pthread_mutex_trylock(&m_mutex) == 0;
    // }

    // bool timelock(const timespec* time)
    // {
    //     return pthread_mutex_timedlock(&m_mutex, time);
    // }

    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t* get() {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;

};




class cond
{
public:
    cond() {
        if (pthread_cond_init(&m_cond,NULL) != 0) {
            throw std::exception();
        }
    }
    ~cond() {
        if(pthread_cond_destroy(&m_cond) != 0) {
            throw std::exception();
        }
    }

    bool wait(pthread_mutex_t* mutex) {
        return pthread_cond_wait(&m_cond,mutex) == 0;
    }


    bool signal(pthread_mutex_t* mutex) {
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;

};


// 锁的RAII类
template <typename T>
class LockerRAII : noncopyable { //不写public为默认private继承,noncopyable类的构造函数为protect属性
public:
    explicit LockerRAII(T& mutex) :m_mutex(mutex) {
        m_mutex.lock();
    }
    ~LockerRAII() {
        m_mutex.unlock();
    };
private:
    T& m_mutex;
};


#endif