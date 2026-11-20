// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

#include <util/thread_pool.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

auto multiply(int a, int b)
{
    return a * b;
}

TEST(thread_pool, globalMultiply)
{
    util::thread_pool pool;
    auto result = pool.enqueue(multiply, 3, 4);
    EXPECT_EQ(result.get(), 12);
}

TEST(thread_pool, lambdaMultiply)
{
    util::thread_pool pool;
    auto result = pool.enqueue([](int a, int b) { return a * b; }, 3, 4);
    EXPECT_EQ(result.get(), 12);
}

TEST(thread_pool, functorMultiply)
{
    util::thread_pool pool;
    auto result = pool.enqueue(std::multiplies<int>{}, 3, 4);
    EXPECT_EQ(result.get(), 12);
}

TEST(thread_pool, passReference)
{
    int x = 2;
    {
        util::thread_pool pool;
        pool.enqueue_detach([](int& a) { a *= 2; }, std::ref(x));
    }
    EXPECT_EQ(x, 4);
}

TEST(thread_pool, passRawReference)
{
    int x = 2;
    {
        util::thread_pool pool;
        pool.enqueue_detach([](int& a) { a *= 2; }, x);
    }
    EXPECT_EQ(x, 2);
}

TEST(thread_pool, paramPassing)
{
    util::thread_pool pool(4);
    constexpr auto total_tasks = 30;
    std::vector<std::future<int>> futures;

    for (auto i = 0; i < total_tasks; i++) {
        auto task = [index = i]() { return index; };

        futures.push_back(pool.enqueue(task));
    }

    for (std::size_t j = 0; j < futures.size(); j++) {
        EXPECT_EQ(static_cast<int>(j), futures[j].get());
    }
}

TEST(thread_pool, multiTypeParameter)
{
    util::thread_pool pool{};
    struct test_struct
    {
        int value{};
        double d_value{};
    };

    test_struct test;

    auto task = [&test](int x, double y) -> test_struct {
        test.value = x;
        test.d_value = y;

        return test_struct{x, y};
    };

    auto future = pool.enqueue(task, 2, 3.2);
    const auto result = future.get();
    EXPECT_EQ(result.value, test.value);
    EXPECT_EQ(result.d_value, test.d_value);
}

TEST(thread_pool, workCompletedAfterDestruction)
{
    std::atomic<int> counter{0};
    constexpr auto total_tasks = 30;
    {
        util::thread_pool pool(4);
        for (auto i = 0; i < total_tasks; i++) {
            // Long enough that most tasks are still queued when the pool is
            // destroyed, which is what this test is about.
            auto task = [i, &counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1 + (i % 5)));
                ++counter;
            };
            pool.enqueue_detach(task);
        }
    }

    EXPECT_EQ(counter.load(), total_tasks);
}

TEST(thread_pool, taskExeceptionDoesNotKill)
{
    auto throw_task = [](int) -> int { throw std::logic_error("Error occurred."); };
    auto regular_task = [](int input) -> int { return input * 2; };

    std::atomic_uint_fast64_t count(0);

    auto throw_no_return = []() { throw std::logic_error("Error occurred."); };
    auto no_throw_no_return = [&count]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        count += 1;
    };

    {
        util::thread_pool pool;

        auto throw_future = pool.enqueue(throw_task, 1);
        auto no_throw_future = pool.enqueue(regular_task, 2);

        EXPECT_THROW(throw_future.get(), std::logic_error);
        EXPECT_EQ(no_throw_future.get(), 4);

        // do similar check for tasks without return
        pool.enqueue_detach(throw_no_return);
        pool.enqueue_detach(no_throw_no_return);
    }

    EXPECT_EQ(count.load(), 1);
}
