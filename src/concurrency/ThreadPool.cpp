#include "concurrency/ThreadPool.h"

#include <algorithm>
#include <exception>

namespace nubilo {

std::expected<std::unique_ptr<ThreadPool>, ThreadPoolError> ThreadPool::create(size_t threadCount) {
    if (threadCount == 0)
        threadCount = std::max<size_t>(1, std::thread::hardware_concurrency());

    try {
        return std::unique_ptr<ThreadPool>(new ThreadPool(threadCount));
    } catch (const std::exception& e) {
        return std::unexpected(ThreadPoolError{e.what()});
    }
}

ThreadPool::ThreadPool(size_t threadCount) {
    try {
        workers_.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i)
            workers_.emplace_back([this] { workerLoop(); });
    } catch (...) {
        // the workers already started are waiting: tell them to stop so they can be joined
        {
            std::lock_guard lock(mutex_);
            stopping_ = true;
        }
        condition_.notify_all();
        throw;
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    condition_.notify_all();
    // workers_ is destroyed next and its jthreads join
}

void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });

            // stopping and nothing left to run
            if (tasks_.empty())
                return;

            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

}