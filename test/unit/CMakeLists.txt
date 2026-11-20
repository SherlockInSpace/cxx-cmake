# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

# One binary for all the unit tests. We link util once and pick a module
# with ctest -R.
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# Every library source and public header needs a test file of the same stem in
# test/unit/src/, so an untested module fails at configure rather than in
# review. A stem with nothing to test on its own goes in UTIL_TEST_EXEMPT with
# a comment saying why. The check only sees that the file exists; the coverage
# floor is what notices a test file that tests nothing.
set(UTIL_TEST_EXEMPT "")

foreach(file IN LISTS SOURCE_FILES PUBLIC_HEADER_FILES)
  cmake_path(GET file STEM stem)
  if(stem IN_LIST UTIL_TEST_EXEMPT)
    continue()
  endif()
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/${stem}.cpp")
    cmake_path(RELATIVE_PATH file BASE_DIRECTORY "${PROJECT_SOURCE_DIR}")
    message(FATAL_ERROR
      "${file} has no unit test: "
      "add test/unit/src/${stem}.cpp or list ${stem} in UTIL_TEST_EXEMPT")
  endif()
endforeach()

# Our own glob rather than SOURCE_FILES, so a test file needs no matching
# file in src/.
file(GLOB UNIT_TEST_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(util_unit_tests ${UNIT_TEST_FILES})
target_link_libraries(util_unit_tests
  PRIVATE util::util GTest::gtest_main
)

# PRE_TEST lists the tests when ctest runs, not at build time, so a cross
# build never has to execute the binary on the build host.
gtest_discover_tests(util_unit_tests
  DISCOVERY_MODE PRE_TEST
  PROPERTIES LABELS unit
)
