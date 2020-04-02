
template <typename T> ThreadPool<T>::ThreadPool(unsigned int val, std::function<void(T)> f) {
    tot_threads = val;
    ftn = f;
    done = false;
    is_start = false;
    for (unsigned int i = 1; i <= tot_threads; i++) {
        active_threads.push_back(std::thread(&ThreadPool::main_loop, this, i));
    }
}
template <typename T> ThreadPool<T>::~ThreadPool() {
    join_pool();
}
template <typename T> void ThreadPool<T>::main_loop(int i) {
    {
        std::unique_lock<std::mutex> u_lock_start(lock_start);
        while (!is_started()) {
            cv_start.wait(u_lock_start);
        }
    }
    #ifdef PRINT_DEBUG
    safe_print("Start.", i);
    #endif

    /* While queue is not empty, or queue is empty but not signaled done */
    bool queue_is_empty;
    while (!(queue_is_empty = is_queue_empty()) || (queue_is_empty && !is_done())) {
        /* Wait if queue is empty */
        {
            std::unique_lock<std::mutex> u_lock(lock_wait_for_data);
            while (is_queue_empty()) {
                if (is_done()) {
                    return;
                }
                #ifdef PRINT_DEBUG
                safe_print("Wait...", i);
                #endif
                cv_queue_empty.wait(u_lock);
                #ifdef PRINT_DEBUG
                safe_print("Wake", i);
                #endif
            }
        }
        /* Pop queue and feed into function */
        T val;
        if (pop_queue(val)) {
            ftn(val);
        }
        #ifdef PRINT_DEBUG
        else {
            safe_print("Pop fail", i);
        }
        #endif
    }
}
/* Start main_loop i.e. processing queue with thread pool */
template <typename T> void ThreadPool<T>::start() {
    set_start();
    cv_start.notify_all();
}
template <typename T> bool ThreadPool<T>::is_queue_empty() {
    std::lock_guard<std::mutex> lock(lock_queue);
    return queue.empty();
}
/* Push to queue, and notify cv meaning queue has items */
template <typename T> void ThreadPool<T>::push_to_queue(T val) {
    std::lock_guard<std::mutex> lock(lock_queue);
    queue.push(val);
    if (queue.size() >= tot_threads) {
        cv_queue_empty.notify_all();
    }
    else {
        for (unsigned int i = 0; i < queue.size(); i++) {
            cv_queue_empty.notify_one();
        }
    }
}
/* Pop queue, return true for success, false for failure */
template <typename T> bool ThreadPool<T>::pop_queue(T &val) {
    std::lock_guard<std::mutex> lock(lock_queue);
    if (!queue.empty()) {
        val = queue.front();
        queue.pop();
        return true;
    }
    return false;
}
/* Join threads in thread pool */
template <typename T> void ThreadPool<T>::join_pool() {
    set_done();
    for (unsigned int i = 0; i < active_threads.size(); i++) {
        cv_queue_empty.notify_all();
        active_threads[i].join();
        #ifdef PRINT_DEBUG
        safe_print("Exited.", i+1);
        #endif
    }
    active_threads.clear();
}
template <typename T> bool ThreadPool<T>::is_started() {
    std::lock_guard<std::mutex> lock(lock_started);
    return is_start;
}
template <typename T> void ThreadPool<T>::set_start() {
    std::lock_guard<std::mutex> lock(lock_started);
    is_start = true;
}
template <typename T> bool ThreadPool<T>::is_done() {
    std::lock_guard<std::mutex> lock(lock_done);
    return done;
}
template <typename T> void ThreadPool<T>::set_done() {
    std::lock_guard<std::mutex> lock(lock_done);
    done = true;
}
#ifdef PRINT_DEBUG
/* Print to stdout with lock */
template <typename T> void ThreadPool<T>::safe_print(std::string s, int i) {
    std::lock_guard<std::mutex> lock(lock_stdout);
    std::cout << "[THREAD_" << i << "] " << s << std::endl;
}
template <typename T> void ThreadPool<T>::safe_print(std::string s) {
    std::lock_guard<std::mutex> lock(lock_stdout);
    std::cout << "           " << s << std::endl;
}
#endif
