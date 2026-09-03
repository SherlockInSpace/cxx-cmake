
#include "thread_pool.hpp"

#include <gtest/gtest.h>

#include <iostream>
#include <string>

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

    for (auto j = 0; j < total_tasks; j++) {
        EXPECT_EQ(j, futures[j].get());
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
    std::atomic<int> counter;
    constexpr auto total_tasks = 30;
    {
        util::thread_pool pool(4);
        for (auto i = 0; i < total_tasks; i++) {
            auto task = [i, &counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds((i + 1) * 100));
                ++counter;
            };
            pool.enqueue_detach(task);
        }
    }

    EXPECT_EQ(counter.load(), total_tasks);
}

TEST(thread_pool, evenTaskLoading)
{
    auto delay_task = [](const std::chrono::seconds& seconds) {
        std::cout << std::this_thread::get_id() << " start : " << std::to_string(seconds.count())
                  << "\n";
        std::this_thread::sleep_for(seconds);
        std::cout << std::this_thread::get_id() << " end: " << std::to_string(seconds.count())
                  << "\n";
    };
    constexpr auto long_task_time = 6;
    const auto start_time = std::chrono::steady_clock::now();
    {
        util::thread_pool pool(4);
        for (auto i = 1; i <= 8; ++i) {
            auto delay_amount = std::chrono::seconds(i % 4);
            if (i % 4 == 0) {
                delay_amount = std::chrono::seconds(long_task_time);
            }
            pool.enqueue_detach(delay_task, delay_amount);
        }
        // wait for tasks to complete
    }
    const auto end_time = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    /**
     * Potential execution graph
     * '-' and '*' represent task time.
     * '-' is the first round of tasks and '*' is the second round of tasks
     *
     * - * **
     * -- ***
     * --- ******
     * ------
     */
    std::cout << "total duration: " << duration.count() << "\n";
    // worst case is the same thread doing the long task back to back. Tasks are assigned
    // sequentially in the thread pool so this would be the default execution if there was no work
    // stealing.
    EXPECT_LT(duration.count(), long_task_time * 2);
}

TEST(thread_pool, taskExeceptionDoesNotKill)
{
    auto throw_task = [](int) -> int { throw std::logic_error("Error occurred."); };
    auto regular_task = [](int input) -> int { return input * 2; };

    std::atomic_uint_fast64_t count(0);

    auto throw_no_return = []() { throw std::logic_error("Error occurred."); };
    auto no_throw_no_return = [&count]() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
