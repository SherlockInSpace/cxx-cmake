// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

// Bloom filter insert and lookup cost, dominated by the SHA-256 digest.

#include <util/bloom.hpp>

#include <benchmark/benchmark.h>

#include <cstddef>
#include <string>
#include <vector>

namespace {

std::vector<std::string> makeKeys(const std::string& prefix, std::size_t count)
{
    std::vector<std::string> keys;
    keys.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        keys.push_back(prefix + std::to_string(i));
    }
    return keys;
}

void bloomInsert(benchmark::State& state)
{
    const auto keys = makeKeys("member-", 1024);
    util::Bloom filter;
    std::size_t next = 0;
    for ([[maybe_unused]] auto iteration : state) {
        filter.insert(keys.at(next));
        next = (next + 1) % keys.size();
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(bloomInsert)->Name("bloom/insert");

void bloomContains(benchmark::State& state)
{
    const auto members = makeKeys("member-", 1024);
    const auto probes = makeKeys(state.range(0) != 0 ? "member-" : "absent-", 1024);
    util::Bloom filter;
    for (const auto& key : members) {
        filter.insert(key);
    }
    std::size_t next = 0;
    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(filter.contains(probes.at(next)));
        next = (next + 1) % probes.size();
    }
    state.SetItemsProcessed(state.iterations());
}
// 1: every probe is a member (all k bits checked); 0: absent keys, which
// usually stop at the first clear bit.
BENCHMARK(bloomContains)->Name("bloom/contains")->ArgName("present")->Arg(1)->Arg(0);

} // namespace
