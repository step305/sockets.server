//
// Created by sanch on 05.11.2022.
//

#ifndef ALGOCPP_FIFO_H
#define ALGOCPP_FIFO_H

// based on https://stackoverflow.com/a/16075550
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

namespace utils {

    template <typename T>
    class FIFO {
    public:
        explicit FIFO(bool blocking = false, size_t size = 10);
        ~FIFO();
        bool push(T item);
        bool pop(T &item);
        bool empty();
        bool full();
    private:
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable flag_;
        bool blocking_;
        size_t max_size_;
    };

    template<typename T>
    FIFO<T>::FIFO(bool blocking, size_t size) : mutex_(), flag_() {
        blocking_ = blocking;
        max_size_ = size;
        queue_ = std::queue<T>();
    };

    template<typename T>
    FIFO<T>::~FIFO() = default;

    template<typename T>
    bool FIFO<T>::push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() == max_size_) {
            return false;
        }
        queue_.push(item);
        flag_.notify_one();
        return true;
    }

    template<typename T>
    bool FIFO<T>::pop(T &item) {
        if (!blocking_ && queue_.empty()) {
            return false;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            flag_.wait(lock);
        }
        item = queue_.front();
        queue_.pop();
        return true;
    }

    template<typename T>
    bool FIFO<T>::empty() {
        return queue_.size() == 0;
    }

    template<typename T>
    bool FIFO<T>::full() {
        return queue_.size() == max_size_;
    }

} // utils

#endif //ALGOCPP_FIFO_H
