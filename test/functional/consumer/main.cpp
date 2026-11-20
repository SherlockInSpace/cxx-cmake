// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

// Uses every public header through the installed package. Exits non-zero, with
// a message on stderr, on the first thing that does not work.

#include <util/bit.hpp>
#include <util/bloom.hpp>
#include <util/thread_pool.hpp>

#include <cstdio>

namespace {

int fail(const char* what)
{
    std::fputs(what, stderr);
    std::fputs("\n", stderr);
    return 1;
}

} // namespace

int main()
{
    auto bloom = util::Bloom::create(4);
    if (!bloom) {
        return fail("Bloom::create(4) failed");
    }
    bloom->insert("installed");
    if (!bloom->contains("installed") || bloom->contains("missing")) {
        return fail("Bloom lookups are wrong");
    }
    if (util::Bloom::create(0).has_value()) {
        return fail("Bloom::create(0) should fail");
    }

    if (util::bit::min(3, 7) != 3 || util::bit::max(3, 7) != 7) {
        return fail("bit::min/max are wrong");
    }

    util::thread_pool pool(2);
    auto result = pool.enqueue([](int x, int y) { return x * y; }, 6, 7);
    if (result.get() != 42) {
        return fail("thread_pool returned the wrong value");
    }
    return 0;
}
