#pragma once

#include "ThreadPoolError.h"

#include <condition_variable>
#include <expected>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace nubilo {

/**
 * @class ThreadPool
 * @brief A fixed set of worker threads that run submitted tasks from a shared queue.
 *
 * Tasks run in the order they were submitted, as soon as a worker is free. The pool is created
 * through create() and is neither copyable nor movable, so the owner (main) keeps it in a
 * unique_ptr and hands out references.
 */
class ThreadPool {
public:
    /**
     * @brief Creates a pool and starts its worker threads.
     * @param threadCount The number of worker threads. 0 uses the number of hardware threads
     * (or 1 if that cannot be determined).
     * @return The new pool on success, or ThreadPoolError on failure.
     */
    static std::expected<std::unique_ptr<ThreadPool>, ThreadPoolError> create(size_t threadCount);

    ~ThreadPool();  // waits for queued tasks, then joins the threads

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Queues a task to run on one of the workers.
     *
     * An exception thrown by the task is stored in the returned future and rethrown by get().
     * @tparam F A callable that takes no arguments.
     * @param task The task to run.
     * @return A future with the task's result on success, or ThreadPoolError on failure.
     */
    template<typename F>
    auto submit(F&& task) -> std::expected<std::future<std::invoke_result_t<F>>, ThreadPoolError>;

private:
    /**
     * @brief Starts the worker threads. Use create() instead.
     * @param threadCount The number of worker threads, already resolved by create().
     */
    explicit ThreadPool(size_t threadCount);

    /** The loop each worker runs: take a task from the queue and run it, until the pool stops. */
    void workerLoop();

    std::vector<std::jthread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stopping_ = false;
};

}