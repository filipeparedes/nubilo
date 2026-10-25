#include "concurrency/ThreadPool.h"

#include <atomic>
#include <chrono>
#include <future>
#include <gtest/gtest.h>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

TEST(ThreadPoolTest, CreateWithExplicitThreadCountSucceeds) {
    auto pool = nubilo::ThreadPool::create(3);
    EXPECT_TRUE(pool.has_value());
}

TEST(ThreadPoolTest, CreateWithZeroThreadsUsesHardwareThreadsAndRunsTasks) {
    auto pool = nubilo::ThreadPool::create(0);
    ASSERT_TRUE(pool.has_value());

    auto fut = (*pool)->submit([] { return 7; });
    ASSERT_TRUE(fut.has_value());
    EXPECT_EQ(fut->get(), 7);
}

TEST(ThreadPoolTest, SubmitReturnsTheTaskResult) {
    auto pool = nubilo::ThreadPool::create(2);
    ASSERT_TRUE(pool.has_value());

    auto text = (*pool)->submit([] { return std::string("done"); });
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(text->get(), "done");
}

TEST(ThreadPoolTest, SubmitAcceptsTasksThatReturnNothing) {
    auto pool = nubilo::ThreadPool::create(2);
    ASSERT_TRUE(pool.has_value());

    std::atomic<int> value{0};
    auto fut = (*pool)->submit([&value] { value = 42; });
    ASSERT_TRUE(fut.has_value());
    fut->get();
    EXPECT_EQ(value.load(), 42);
}

TEST(ThreadPoolTest, EverySubmittedTaskRuns) {
    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());

    constexpr int taskCount = 200;
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    for (int i = 0; i < taskCount; ++i) {
        auto fut = (*pool)->submit([&counter] { ++counter; });
        ASSERT_TRUE(fut.has_value());
        futures.push_back(std::move(fut.value()));
    }
    for (auto& f : futures)
        f.get();

    EXPECT_EQ(counter.load(), taskCount);
}

TEST(ThreadPoolTest, TasksRunInSubmissionOrderOnASingleThread) {
    auto pool = nubilo::ThreadPool::create(1);
    ASSERT_TRUE(pool.has_value());

    std::vector<int> order;
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 20; ++i) {
        auto fut = (*pool)->submit([&order, i] { order.push_back(i); });
        ASSERT_TRUE(fut.has_value());
        futures.push_back(std::move(fut.value()));
    }
    for (auto& f : futures)
        f.get();

    ASSERT_EQ(order.size(), 20);
    for (int i = 0; i < 20; ++i)
        EXPECT_EQ(order[i], i);
}

TEST(ThreadPoolTest, TasksRunOnSeveralThreadsAtTheSameTime) {
    constexpr int threadCount = 4;
    auto pool = nubilo::ThreadPool::create(threadCount);
    ASSERT_TRUE(pool.has_value());

    //every task waits until all of them have started, which can only happen if they run in parallel
    std::atomic<int> arrived{0};
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < threadCount; ++i) {
        auto fut = (*pool)->submit([&arrived] {
            ++arrived;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (arrived.load() < threadCount) {
                if (std::chrono::steady_clock::now() > deadline)
                    return false;
                std::this_thread::yield();
            }
            return true;
        });
        ASSERT_TRUE(fut.has_value());
        futures.push_back(std::move(fut.value()));
    }

    for (auto& f : futures)
        EXPECT_TRUE(f.get());
}

TEST(ThreadPoolTest, NeverUsesMoreThreadsThanRequested) {
    auto pool = nubilo::ThreadPool::create(2);
    ASSERT_TRUE(pool.has_value());

    std::mutex mutex;
    std::set<std::thread::id> ids;
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 50; ++i) {
        auto fut = (*pool)->submit([&mutex, &ids] {
            {
                std::lock_guard lock(mutex);
                ids.insert(std::this_thread::get_id());
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
        ASSERT_TRUE(fut.has_value());
        futures.push_back(std::move(fut.value()));
    }
    for (auto& f : futures)
        f.get();

    EXPECT_GE(ids.size(), 1);
    EXPECT_LE(ids.size(), 2);
}

TEST(ThreadPoolTest, ExceptionInATaskIsRethrownByGetAndThePoolKeepsWorking) {
    auto pool = nubilo::ThreadPool::create(1);
    ASSERT_TRUE(pool.has_value());

    auto failing = (*pool)->submit([]() -> int { throw std::runtime_error("boom"); });
    ASSERT_TRUE(failing.has_value());
    EXPECT_THROW((void)failing->get(), std::runtime_error);

    //the only worker must have survived the exception
    auto next = (*pool)->submit([] { return 5; });
    ASSERT_TRUE(next.has_value());
    EXPECT_EQ(next->get(), 5);
}

TEST(ThreadPoolTest, DestructorRunsTasksThatWereAlreadyQueued) {
    auto pool = nubilo::ThreadPool::create(1);
    ASSERT_TRUE(pool.has_value());

    //the single worker is held by the first task, so the others are still queued
    std::promise<void> gate;
    std::shared_future<void> open = gate.get_future().share();
    std::atomic<int> counter{0};

    ASSERT_TRUE((*pool)->submit([open] { open.wait(); }).has_value());
    for (int i = 0; i < 10; ++i)
        ASSERT_TRUE((*pool)->submit([&counter] { ++counter; }).has_value());

    gate.set_value();
    pool->reset();  //destroys the pool

    EXPECT_EQ(counter.load(), 10);
}

TEST(ThreadPoolTest, SubmitIsSafeFromSeveralThreads) {
    auto pool = nubilo::ThreadPool::create(4);
    ASSERT_TRUE(pool.has_value());
    nubilo::ThreadPool* poolPtr = pool->get();

    constexpr int submitters = 4;
    constexpr int tasksEach = 100;
    std::atomic<int> counter{0};
    std::atomic<int> rejected{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < submitters; ++t) {
        threads.emplace_back([poolPtr, &counter, &rejected] {
            std::vector<std::future<void>> futures;
            for (int i = 0; i < tasksEach; ++i) {
                auto fut = poolPtr->submit([&counter] { ++counter; });
                if (!fut) {
                    ++rejected;
                    continue;
                }
                futures.push_back(std::move(fut.value()));
            }
            for (auto& f : futures)
                f.get();
        });
    }
    for (auto& thread : threads)
        thread.join();

    EXPECT_EQ(rejected.load(), 0);
    EXPECT_EQ(counter.load(), submitters * tasksEach);
}