#ifndef _LOCKER_H
#define _LOCKER_H

#include <pthread.h>
#include <semaphore.h>
#include <exception>

class Sem{
public:
    Sem(){
        if(sem_init(&m_sem, 0, 0) != 0){
            throw std::exception();
        }
    }
    ~Sem(){
        sem_destroy(&m_sem);
    }
    bool wait(){
        return sem_wait(&m_sem) == 0;
    }
    bool post(){
        return sem_post(&m_sem) == 0;
    }
private:
    sem_t m_sem;
};

class Locker{
public:
    Locker(){
        pthread_mutex_init(&m_mutex,NULL); 
    }
    ~Locker(){
        pthread_mutex_destroy(&m_mutex);
    }

    bool Lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool UnLock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
private:
    pthread_mutex_t m_mutex;
};

#endif