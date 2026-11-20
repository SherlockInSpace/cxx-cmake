# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

# Google Benchmark from the image or sysroot. If CPM is allowed to fetch, it
# builds the v1.9.1 source tarball, checked by its SHA-256.
CPMAddPackage(
  NAME benchmark
  FORCE "${CPM_DOWNLOAD_benchmark}"
  VERSION 1.8.0
  URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.tar.gz
  URL_HASH SHA256=32131c08ee31eeff2c8968d7e874f3cb648034377dfc32a4c377fa8796d84981
  FIND_PACKAGE_ARGUMENTS "CONFIG"
  OPTIONS
    "BENCHMARK_ENABLE_TESTING OFF"
    "BENCHMARK_ENABLE_INSTALL OFF"
    "BENCHMARK_ENABLE_WERROR OFF"
)

# One binary for every benchmark, like the unit tests.
file(GLOB BENCH_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(util_bench ${BENCH_FILES})
target_link_libraries(util_bench
  PRIVATE util::util util_warnings benchmark::benchmark_main
)
set_target_properties(util_bench PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# One CTest test per bench/src file, labelled performance. The benchmarks in
# bench/src/<stem>.cpp are named "<stem>/...", so --benchmark_filter picks out
# that file's set. Each writes Google Benchmark JSON to bench/results/, which
# the trend dashboard and the release comparison read. RUN_SERIAL keeps one
# benchmark from timing another.
set(results "${CMAKE_CURRENT_BINARY_DIR}/results")
file(MAKE_DIRECTORY "${results}")
foreach(file IN LISTS BENCH_FILES)
  cmake_path(GET file STEM stem)
  add_test(NAME performance.${stem}
    COMMAND util_bench
      --benchmark_filter=^${stem}/
      --benchmark_out=${results}/${stem}.json
      --benchmark_out_format=json)
  set_tests_properties(performance.${stem} PROPERTIES
    LABELS performance
    RUN_SERIAL TRUE)
endforeach()
