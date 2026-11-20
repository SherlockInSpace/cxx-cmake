// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

// Thread-pool throughput, and how well work stealing evens out uneven tasks.

#include <util/thread_pool.hpp>

#include <benchmark/benchmark.h>

#include <cstdint>
#include <future>
#include <vector>

namespace {

// Busy work the optimiser cannot drop, so a task costs time without sleeping.
std::uint64_t spin(std::uint64_t rounds)
{
    std::uint64_t value = rounds;
    for (std::uint64_t i = 0; i < rounds; ++i) {
        value = (value * 6364136223846793005ULL) + 1442695040888963407ULL;
        benchmark::DoNotOptimize(value);
    }
    return value;
}

// Many tiny tasks: the cost of enqueueing, scheduling and handing back a
// result, per task.
void threadPoolThroughput(benchmark::State& state)
{
    constexpr int kTasks = 10000;
    util::thread_pool pool(static_cast<unsigned int>(state.range(0)));
    std::vector<std::future<int>> futures;
    futures.reserve(kTasks);
    for ([[maybe_unused]] auto iteration : state) {
        futures.clear();
        for (int i = 0; i < kTasks; ++i) {
            futures.push_back(pool.enqueue([i] { return i; }));
        }
        for (auto& future : futures) {
            benchmark::DoNotOptimize(future.get());
        }
    }
    state.SetItemsProcessed(state.iterations() * kTasks);
}
BENCHMARK(threadPoolThroughput)
    ->Name("thread_pool/throughput")
    ->ArgName("threads")
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->UseRealTime();

// Eight tasks on four threads where every fourth is six times the unit and
// the rest one to three units: the shape of the old evenTaskLoading unit test.
// Tasks are dealt round-robin, so without stealing one thread runs both long
// tasks back to back (12 units); with stealing the wall time stays close to
// the longest task (6 units plus a little).
void threadPoolUnevenLoad(benchmark::State& state)
{
    const auto unit = static_cast<std::uint64_t>(state.range(0));
    util::thread_pool pool(4);
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(8);
    for ([[maybe_unused]] auto iteration : state) {
        futures.clear();
        for (std::uint64_t i = 1; i <= 8; ++i) {
            const std::uint64_t units = (i % 4 == 0) ? 6 : i % 4;
            futures.push_back(pool.enqueue(spin, units * unit));
        }
        for (auto& future : futures) {
            benchmark::DoNotOptimize(future.get());
        }
    }
}
BENCHMARK(threadPoolUnevenLoad)
    ->Name("thread_pool/uneven_load")
    ->ArgName("unit")
    ->Arg(1 << 20)
    ->UseRealTime()
    ->Unit(benchmark::kMillisecond);

} // namespace
