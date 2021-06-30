// 线程池

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <pthread.h>
#include <list>
#include "locker.h"

// 模板制定任务，方便实现代码复用
template<typename T>
class threadpool {
private:
    // 线程数
    int m_thread_number;

    // 线程池数组，大小为 m_thread_number
    pthread_t * m_threads;

    // 请求队列中，最多允许的等待处理的请求数量
    int m_max_requests;

    // 请求队列
    std::list <T*> m_workqueue;

    // 互斥锁
    locker m_queuelocker;

    // 信号量，判断是否有任务需要处理
    sem m_queuestat;

    // 线程是否结束
    bool m_stop;

public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/

    threadpool(int thread_number = 8, int max_requests = 100000);
    ~threadpool();
    bool append(T* request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void* worker(void* arg);
    void run();

};


template< typename T >
threadpool< T >::threadpool(int thread_number, int max_requests) : 
        m_thread_number(thread_number), m_max_requests(max_requests), 
        m_stop(false), m_threads(NULL) {

    if((thread_number <= 0) || (max_requests <= 0) ) {
        throw std::exception();
    }

    // pthread_t：描述线程池的数组，大小为m_thread_number
    m_threads = new pthread_t[m_thread_number];     
    if(!m_threads) {
        throw std::exception();
    }

    // 创建thread_number 个线程，并将他们设置为脱离线程。
    for ( int i = 0; i < thread_number; ++i ) {
        printf( "create the %dth thread\n", i);
        if(pthread_create(m_threads + i, NULL, worker, this ) != 0) {
            // 进程创建失败
            delete [] m_threads;
            throw std::exception();
        }
        
        if( pthread_detach( m_threads[i] ) ) {
            // pthread_detach： 主线程与子线程分离，子线程结束后，资源自动回收
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template< typename T >
threadpool< T >::~threadpool() {
    delete [] m_threads;
    m_stop = true;
}

template< typename T >
bool threadpool< T >::append( T* request )
// 添加任务
{
    // 操作工作队列时一定要加锁，因为它被所有线程共享。
    m_queuelocker.lock();       // locker类型
    if ( m_workqueue.size() > m_max_requests ) {    // 请求队列的size > 请求队列中最多允许的、等待处理的请求的数量  
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();     // 是否有任务需要处理
    return true;
}

template< typename T >
void* threadpool< T >::worker( void* arg )
{
    threadpool* pool = ( threadpool* )arg;
    pool->run();
    return pool;
}

template< typename T >
void threadpool< T >::run() {

    while (!m_stop) {   // m_stop：是否结束线程
        m_queuestat.wait();     // 是否有任务需要处理
        m_queuelocker.lock();
        if ( m_workqueue.empty() ) {      // 请求队列
            m_queuelocker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if ( !request ) {
            continue;
        }
        request->process();
    }

}


#endif