// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

// Tracy profiling harness. Each frame runs the Bloom filter and the thread
// pool through a workload, with zones around the calls into util. util has no
// Tracy code, so what happens inside it comes from Tracy's sampling.

#include <util/bloom.hpp>
#include <util/thread_pool.hpp>

#include <tracy/Tracy.hpp>

#include <cstddef>
#include <cstdint>
#include <future>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kKeys = 4096;
constexpr int kTasks = 1000;
constexpr int kFrames = 100;

std::vector<std::string> makeKeys(const std::string& prefix, std::size_t count)
{
    std::vector<std::string> keys;
    keys.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        keys.push_back(prefix + std::to_string(i));
    }
    return keys;
}

// Busy work the optimiser cannot drop, so a task costs time without sleeping.
std::uint64_t spin(std::uint64_t rounds)
{
    std::uint64_t value = rounds;
    for (std::uint64_t i = 0; i < rounds; ++i) {
        value = (value * 6364136223846793005ULL) + 1442695040888963407ULL;
    }
    return value;
}

void bloomWorkload(const std::vector<std::string>& members, const std::vector<std::string>& probes)
{
    ZoneScopedN("bloom");
    util::Bloom filter;
    {
        ZoneScopedN("bloom/insert");
        for (const auto& key : members) {
            filter.insert(key);
        }
    }
    {
        ZoneScopedN("bloom/contains");
        std::int64_t hits = 0;
        for (const auto& key : probes) {
            if (filter.contains(key)) {
                ++hits;
            }
        }
        TracyPlot("bloom false positives", hits);
    }
}

std::uint64_t threadPoolWorkload(util::thread_pool<>& pool)
{
    ZoneScopedN("thread_pool");
    std::vector<std::future<std::uint64_t>> futures;
    futures.reserve(kTasks);
    for (int i = 0; i < kTasks; ++i) {
        const auto rounds = static_cast<std::uint64_t>(i % 7) * 500;
        futures.push_back(pool.enqueue([rounds] { return spin(rounds); }));
    }
    std::uint64_t total = 0;
    for (auto& future : futures) {
        total += future.get();
    }
    return total;
}

} // namespace

int main()
{
    const auto members = makeKeys("member-", kKeys);
    const auto probes = makeKeys("absent-", kKeys);
    util::thread_pool pool(4);
    std::uint64_t total = 0;
    for (int frame = 0; frame < kFrames; ++frame) {
        bloomWorkload(members, probes);
        total += threadPoolWorkload(pool);
        FrameMark;
    }
    // Using the sum keeps the tasks' work from being optimised away.
    return total == 0 ? 1 : 0;
}
