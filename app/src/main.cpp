#include "bit.hpp"
#include "bloom.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static auto __console = spdlog::stdout_color_mt("app");

int
main(int argc, char *argv[])
{
    util::Bloom bloomFilter;

    spdlog::cfg::load_env_levels();

    for (auto idx = 1; idx < argc; ++idx) {
        bloomFilter.insert(argv[1]);
        SPDLOG_LOGGER_DEBUG(__console, "Inserting word {} at index {}",
            argv[idx], idx);
    }
}