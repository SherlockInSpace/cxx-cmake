// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

// Why the library reports recoverable failures with std::expected rather than
// exceptions (docs/DECISIONS.md): the same check written both ways, on the
// path that succeeds and the path that fails. On the success path the two are
// close; on the failure path the exception has to be thrown, unwound and
// caught, and costs far more. The last benchmark is the real thing,
// Bloom::create with a count it refuses.
//
// One check takes about a nanosecond, too close to the timer and to a shared
// runner's noise to compare from one release to the next, so each iteration
// makes a batch of kCalls calls (the "/1000" in each name) and reports calls
// per second.

#include <util/bloom.hpp>

#include <benchmark/benchmark.h>

#include <cstdint>
#include <expected>
#include <stdexcept>

namespace {

enum class CheckError : unsigned char
{
    kNotPositive
};

// noinline so each call is a real call, as it would be across a library
// boundary, rather than folded into the loop.
[[gnu::noinline]] std::expected<int, CheckError> checkExpected(int value)
{
    if (value <= 0) {
        return std::unexpected(CheckError::kNotPositive);
    }
    return value;
}

[[gnu::noinline]] int checkThrow(int value)
{
    if (value <= 0) {
        throw std::invalid_argument("value must be positive");
    }
    return value;
}

// Calls per iteration for every benchmark here.
constexpr std::int64_t kCalls = 1000;

void expectedSuccess(benchmark::State& state)
{
    const std::int64_t calls = state.range(0);
    int input = 1;
    for ([[maybe_unused]] auto iteration : state) {
        for (std::int64_t call = 0; call < calls; ++call) {
            benchmark::DoNotOptimize(input);
            benchmark::DoNotOptimize(checkExpected(input));
        }
    }
    state.SetItemsProcessed(state.iterations() * calls);
}
BENCHMARK(expectedSuccess)->Name("expected_vs_throw/expected_success")->Arg(kCalls);

void expectedFailure(benchmark::State& state)
{
    const std::int64_t calls = state.range(0);
    int input = 0;
    for ([[maybe_unused]] auto iteration : state) {
        for (std::int64_t call = 0; call < calls; ++call) {
            benchmark::DoNotOptimize(input);
            benchmark::DoNotOptimize(checkExpected(input));
        }
    }
    state.SetItemsProcessed(state.iterations() * calls);
}
BENCHMARK(expectedFailure)->Name("expected_vs_throw/expected_failure")->Arg(kCalls);

void throwSuccess(benchmark::State& state)
{
    const std::int64_t calls = state.range(0);
    int input = 1;
    for ([[maybe_unused]] auto iteration : state) {
        for (std::int64_t call = 0; call < calls; ++call) {
            benchmark::DoNotOptimize(input);
            try {
                benchmark::DoNotOptimize(checkThrow(input));
            } catch (const std::invalid_argument& error) {
                benchmark::DoNotOptimize(&error);
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * calls);
}
BENCHMARK(throwSuccess)->Name("expected_vs_throw/throw_success")->Arg(kCalls);

void throwFailure(benchmark::State& state)
{
    const std::int64_t calls = state.range(0);
    int input = 0;
    for ([[maybe_unused]] auto iteration : state) {
        for (std::int64_t call = 0; call < calls; ++call) {
            benchmark::DoNotOptimize(input);
            try {
                benchmark::DoNotOptimize(checkThrow(input));
            } catch (const std::invalid_argument& error) {
                benchmark::DoNotOptimize(&error);
            }
        }
    }
    state.SetItemsProcessed(state.iterations() * calls);
}
BENCHMARK(throwFailure)->Name("expected_vs_throw/throw_failure")->Arg(kCalls);

void bloomCreateRefused(benchmark::State& state)
{
    const std::int64_t calls = state.range(0);
    std::size_t count = 0;
    for ([[maybe_unused]] auto iteration : state) {
        for (std::int64_t call = 0; call < calls; ++call) {
            benchmark::DoNotOptimize(count);
            benchmark::DoNotOptimize(util::Bloom::create(count));
        }
    }
    state.SetItemsProcessed(state.iterations() * calls);
}
BENCHMARK(bloomCreateRefused)->Name("expected_vs_throw/bloom_create_refused")->Arg(kCalls);

} // namespace
