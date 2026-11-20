// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

// Why the library reports recoverable failures with std::expected rather than
// exceptions (docs/DECISIONS.md): the same check written both ways, on the
// path that succeeds and the path that fails. On the success path the two are
// close; on the failure path the exception has to be thrown, unwound and
// caught, and costs far more. The last benchmark is the real thing,
// Bloom::create with a count it refuses.

#include <util/bloom.hpp>

#include <benchmark/benchmark.h>

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

void expectedSuccess(benchmark::State& state)
{
    int input = 1;
    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(input);
        benchmark::DoNotOptimize(checkExpected(input));
    }
}
BENCHMARK(expectedSuccess)->Name("expected_vs_throw/expected_success");

void expectedFailure(benchmark::State& state)
{
    int input = 0;
    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(input);
        benchmark::DoNotOptimize(checkExpected(input));
    }
}
BENCHMARK(expectedFailure)->Name("expected_vs_throw/expected_failure");

void throwSuccess(benchmark::State& state)
{
    int input = 1;
    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(input);
        try {
            benchmark::DoNotOptimize(checkThrow(input));
        } catch (const std::invalid_argument& error) {
            benchmark::DoNotOptimize(&error);
        }
    }
}
BENCHMARK(throwSuccess)->Name("expected_vs_throw/throw_success");

void throwFailure(benchmark::State& state)
{
    int input = 0;
    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(input);
        try {
            benchmark::DoNotOptimize(checkThrow(input));
        } catch (const std::invalid_argument& error) {
            benchmark::DoNotOptimize(&error);
        }
    }
}
BENCHMARK(throwFailure)->Name("expected_vs_throw/throw_failure");

void bloomCreateRefused(benchmark::State& state)
{
    std::size_t count = 0;
    for ([[maybe_unused]] auto iteration : state) {
        benchmark::DoNotOptimize(count);
        benchmark::DoNotOptimize(util::Bloom::create(count));
    }
}
BENCHMARK(bloomCreateRefused)->Name("expected_vs_throw/bloom_create_refused");

} // namespace
