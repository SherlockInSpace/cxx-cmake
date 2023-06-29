#pragma once

#include <atomic>
#include <concepts>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <semaphore>
#include <thread>
#include <type_traits>
#include <version>

namespace util {
    /**
     * @brief Simple concept for the Lockable and Basic Lockable types as defined by the C++
     * standard.
     * @details See https://en.cppreference.com/w/cpp/named_req/Lockable and
     * https://en.cppreference.com/w/cpp/named_req/BasicLockable for details.
     */
    template <typename Lock>
    concept is_lockable = requires(Lock&& lock) {
                            lock.lock();
                            lock.unlock();
                            { lock.try_lock() } -> std::convertible_to<bool>;
                        };

    namespace details {
#if __cpp_lib_move_only_function
        using default_function_type = std::move_only_function<void()>;
#else
        using default_function_type = std::function<void()>;
#endif
    }  // namespace details

    template <typename FunctionType = details::default_function_type>
        requires std::invocable<FunctionType> &&
                 std::is_same_v<void, std::invoke_result_t<FunctionType>>
    class thread_pool {
      public:
        explicit thread_pool(
            const unsigned int &number_of_threads = std::thread::hardware_concurrency())
            : tasks_(number_of_threads) {
            for (std::size_t i = 0; i < number_of_threads; ++i) {
                try {
                    threads_.emplace_back([&, id = i](const std::stop_token &stop_tok) {
                        do {
                            // wait until signaled
                            tasks_[id].signal.acquire();

                            do {
                                // invoke the task
                                while (auto task = tasks_[id].tasks.pop()) {
                                    try {
                                        pending_tasks_.fetch_sub(1, std::memory_order_release);
                                        std::invoke(std::move(task.value()));
                                    } catch (...) {
                                    }
                                }

                                // try to steal a task
                                for (std::size_t j = 1; j < tasks_.size(); ++j) {
                                    const std::size_t index = (id + j) % tasks_.size();
                                    if (auto task = tasks_[index].tasks.steal()) {
                                        // steal a task
                                        pending_tasks_.fetch_sub(1, std::memory_order_release);
                                        std::invoke(std::move(task.value()));
                                        // stop stealing once we have invoked a stolen task
                                        break;
                                    }
                                }

                            } while (pending_tasks_.load(std::memory_order_acquire) > 0);
                        } while (!stop_tok.stop_requested());
                    });
                } catch (...) {
                    // catch all
                }
            }
        }

        ~thread_pool() {
            // stop all threads
            for (std::size_t i = 0; i < threads_.size(); ++i) {
                threads_[i].request_stop();
                tasks_[i].signal.release();
                threads_[i].join();
            }
        }

        /// thread pool is non-copyable
        thread_pool(const thread_pool &) = delete;
        thread_pool &operator=(const thread_pool &) = delete;

        /**
         * @brief Enqueue a task into the thread pool that returns a result.
         * @details Note that task execution begins once the task is enqueued.
         * @tparam Function An invokable type.
         * @tparam Args Argument parameter pack
         * @tparam ReturnType The return type of the Function
         * @param[in] func The callable function
         * @param[in] args The parameters that will be passed (copied) to the function.
         * @return A std::future<ReturnType> that can be used to retrieve the returned value.
         */
        template <typename Function, typename... Args,
                  typename ReturnType = std::invoke_result_t<Function &&, Args &&...>>
            requires std::invocable<Function, Args...>
        [[nodiscard]] std::future<ReturnType> enqueue(Function func, Args... args) {
#if __cpp_lib_move_only_function
            // we can do this in C++23 because we now have support for move only functions
            std::promise<ReturnType> promise;
            auto future = promise.get_future();
            auto task = [f = std::move(func), ... largs = std::move(args),
                         promise = std::move(promise)]() mutable {
                try {
                    promise.set_value(f(largs...));
                } catch (...) {
                    promise.set_exception(std::current_exception());
                }
            };
            enqueue_task(std::move(task));
            return future;
#else
            /*
             * use shared promise here so that we don't break the promise later (until C++23)
             *
             * with C++23 we can do the following:
             *
             * std::promise<ReturnType> promise;
             * auto future = promise.get_future();
             * auto task = [f = std::move(func), ...largs = std::move(args),
                              promise = std::move(promise)]() mutable {...};
             */
            auto shared_promise = std::make_shared<std::promise<ReturnType>>();
            auto task = [f = std::move(func), ... largs = std::move(args),
                         promise = shared_promise]() {
                try {
                    promise->set_value(f(largs...));
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            };

            // get the future before enqueuing the task
            auto future = shared_promise->get_future();
            // enqueue the task
            enqueue_task(std::move(task));
            return future;
#endif
        }

        /**
         * @brief Enqueue a task to be executed in the thread pool that returns void.
         * @tparam Function An invokable type.
         * @tparam Args Argument parameter pack for Function
         * @param[in] func The callable to be executed
         * @param[in] args Arguments that will be passed to the function.
         */
        template <typename Function, typename... Args>
            requires std::invocable<Function, Args...> &&
                     std::is_same_v<void, std::invoke_result_t<Function &&, Args &&...>>
        void enqueue_detach(Function &&func, Args &&...args) {
            enqueue_task(
                std::move([f = std::forward<Function>(func),
                           ... largs = std::forward<Args>(args)]() mutable -> decltype(auto) {
                    // suppress exceptions
                    try {
                        std::invoke(f, largs...);
                    } catch (...) {
                    }
                }));
        }

      private:
        template <typename T, typename Lock = std::mutex>
            requires is_lockable<Lock>
        class thread_safe_queue {
        public:
            using value_type = T;
            using size_type = typename std::deque<T>::size_type;

            thread_safe_queue() = default;

            void push(T&& value) {
                std::lock_guard lock(mutex_);
                data_.push_back(std::forward<T>(value));
            }

            [[nodiscard]] bool empty() const {
                std::lock_guard lock(mutex_);
                return data_.empty();
            }

            [[nodiscard]] std::optional<T> pop() {
                std::lock_guard lock(mutex_);
                if (data_.empty()) return std::nullopt;

                auto front = std::move(data_.front());
                data_.pop_front();
                return front;
            }

            [[nodiscard]] std::optional<T> steal() {
                std::lock_guard lock(mutex_);
                if (data_.empty()) return std::nullopt;

                auto back = std::move(data_.back());
                data_.pop_back();
                return back;
            }

        private:
            std::deque<T> data_{};
            mutable Lock mutex_{};
        };

        template <typename Function>
        void enqueue_task(Function &&f) {
            const std::size_t i = count_++ % tasks_.size();
            pending_tasks_.fetch_add(1, std::memory_order_relaxed);
            tasks_[i].tasks.push(std::forward<Function>(f));
            tasks_[i].signal.release();
        }

        struct task_item {
            thread_safe_queue<FunctionType> tasks{};
            std::binary_semaphore signal{0};
        };

        std::vector<std::jthread> threads_;
        std::deque<task_item> tasks_;
        std::size_t count_{};
        std::atomic_int_fast64_t pending_tasks_{};
    };

    /**
     * @example mandelbrot/source/main.cpp
     * Example showing how to use thread pool with tasks that return a value. Outputs a PPM image of
     * a mandelbrot.
     */
}  // namespace dp