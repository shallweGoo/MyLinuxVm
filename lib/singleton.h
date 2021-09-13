#ifndef SINGLETON_H
#define SINGLETON_H

#include "./noncopyable.h"

template <typename T>
class Singleton : noncopyable 
{
public:
    static T& GetInstance();

public:
    Singleton() = delete;
    ~Singleton() = delete;
};


template <typename T>
T& Singleton<T>::GetInstance() {
    static T instance;
    return instance;
}


#endif