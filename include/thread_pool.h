
#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#define PRINT_DEBUG

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <string>
#include <sstream>
#include <functional>

template<typename T> class ThreadPool {
    private:
        unsigned int tot_threads;
        std::vector<std::thread> active_threads;
        std::queue<T> queue;
        std::function<void(T)> ftn; /* Function to call queue data on */
        std::mutex lock_queue,
                   lock_start,
                   lock_wait_for_data,
                   lock_started,
                   #ifdef PRINT_DEBUG
                   lock_stdout,
                   #endif
                   lock_done;
        std::condition_variable cv_queue_empty,
                                cv_start;
        bool done, /* Ready to exit */
             is_start;
        bool pop_queue(T &);
        void main_loop(int); /* Where threads execute */
        
        /* Methods */
        bool is_queue_empty();
        bool is_done();
        void set_done();
        bool is_started();
        void set_start();
    public:
        ThreadPool(unsigned int, std::function<void(T)> ftn);
        ~ThreadPool();
        void start();
        void push_to_queue(T);
        void join_pool();
        #ifdef PRINT_DEBUG
        void safe_print(std::string, int);
        void safe_print(std::string);
        #endif
};

/* Include .cc rather than compile to support any type for template */
#include "thread_pool.cc"

#endif