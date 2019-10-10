#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <thread>

namespace chord {

template <typename T>
class SyncQueue {
   public:
    SyncQueue(int maxSize) : maxSize_(maxSize), needStop_(false) {}

    void Put(const T& x) { Add(x); }
    void Put(T&& x) { Add(std::forward<T>(x)); }

    void Take(T& t) {
        std::unique_lock<std::mutex> lock(mutex_);
        notEmpty_.wait(lock, [this] { return needStop_ || NotEmpty(); });
        if (needStop_) return;
        t = queue_.front();
        queue_.pop_front();
        notFull_.notify_one();
    }

    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            needStop_ = true;
        }
        notFull_.notify_all();
        notEmpty_.notify_all();
    }
    bool Empty() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    bool Full() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() == maxSize_;
    }
    size_t Size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    int Count() { return queue_.size(); }

   private:
    bool NotFull() const {
        bool full = queue_.size() >= maxSize_;
        return !full;
    }
    bool NotEmpty() const {
        bool empty = queue_.empty();
        return !empty;
    }

    template <typename F>
    void Add(F&& x) {
        std::unique_lock<std::mutex> lock(mutex_);
        notFull_.wait(lock, [this]() { return needStop_ || NotFull(); });
        if (needStop_) return;
        queue_.push_back(std::forward<F>(x));
        notEmpty_.notify_one();
    }

   private:
    std::list<T> queue_;
    std::mutex mutex_;
    std::condition_variable notEmpty_;
    std::condition_variable notFull_;
    int maxSize_;
    bool needStop_;
};

const int g_maxThreadCnt = 100;

class threadpool {
   public:
    using Task = std::function<void()>;
    threadpool(int numThreads = std::thread::hardware_concurrency()) : queue_(g_maxThreadCnt) { Start(numThreads); }

    ~threadpool() { Stop(); }

    void Stop() {
        std::call_once(flag_, [this] { StopThreadGroup(); });
    }

    void AddTask(Task&& task) { queue_.Put(std::forward<Task>(task)); }

    void AddTask(const Task& task) { queue_.Put(task); }

   private:
    void Start(int numThreads) {
        running_ = true;
        // create threads
        for (int i = 0; i < numThreads; ++i) {
            threadgroup_.push_back(std::make_shared<std::thread>(&threadpool::RunInThread, this));
        }
    }

    void RunInThread() {
        while (running_) {
            Task task;
            queue_.Take(task);
            if (!running_) return;
            task();
        }
    }

    void StopThreadGroup() {
        running_ = false;
        queue_.Stop();
        for (auto thread : threadgroup_) {
            if (thread) {
                thread->join();
            }
        }
        threadgroup_.clear();
    }

    std::list<std::shared_ptr<std::thread>> threadgroup_;
    SyncQueue<Task> queue_;
    std::atomic_bool running_;
    std::once_flag flag_;
};

}  // namespace chord