#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include "locker.h"
#include <queue>
#include <assert.h>
#include <stdio.h>
#include <iostream>

const int MaxTaskNum = 1024;
const int MaxThreadNum = 8;

template<class T> 
class ThreadPool{
public:
    ThreadPool(int threadNum=MaxThreadNum, int taskNum=MaxTaskNum);
    ~ThreadPool();
    void run();
    static void* work(void* arg);
    bool append(T* task);
private:
    int m_threadNum;
    int m_taskNum;
    bool m_stop;
    pthread_t* m_pthread;

    std::queue<T*> m_queue;
    Locker m_mutex;
    Sem m_sem; 
};

template<class T>
void* ThreadPool<T>::work(void* arg){
    ThreadPool<T>* pool = (ThreadPool<T>*)arg;
    pool->run();
}

template<class T>
void ThreadPool<T>::run(){
    while(!m_stop){
        m_sem.wait();
        m_mutex.Lock();
        T* task = m_queue.front();
        m_queue.pop();
        m_mutex.UnLock();

        if(task != NULL){
            printf("Thread %lu run\n", pthread_self());
            task->process();
        }     
    }
}

template<class T>
ThreadPool<T>::ThreadPool(int threadNum, int taskNum):m_taskNum(taskNum), m_threadNum(threadNum), m_stop(false) {
    m_pthread = new pthread_t[m_threadNum];
    assert(m_pthread != NULL);

    for(int i = 0; i < m_threadNum; ++i){
        if (pthread_create(m_pthread+i, NULL, work, this) != 0){
            delete []m_pthread;
            throw std::exception();
        }

        if (pthread_detach(m_pthread[i]) != 0){
            delete []m_pthread;
            throw std::exception();
        }
    }
}

template<class T>
ThreadPool<T>::~ThreadPool(){
    if(m_pthread != NULL){
        delete []m_pthread;
    }

    m_stop = true;
}

template<class T>
bool ThreadPool<T>::append(T* task){
    if(m_queue.size() == m_taskNum){
        return false;
    }

    m_mutex.Lock();
    m_queue.push(task);
    m_mutex.UnLock();
    m_sem.post();

    return true;
}

#endif