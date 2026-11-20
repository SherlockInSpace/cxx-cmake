# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

# One binary for all the unit tests. We link util once and pick a module
# with ctest -R.
# GTestConfig.cmake from the image or sysroot. If CPM is allowed to fetch,
# it builds the 1.17.0 release tarball, checked by its SHA-256.
# -DCPM_DOWNLOAD_GTest=ON (with CPM_LOCAL_PACKAGES_ONLY=OFF) builds the tarball
# even when a local GTest exists. CPM itself only reads that switch in
# CPMFindPackage, so it is passed through as FORCE.
CPMAddPackage(
  NAME GTest
  FORCE "${CPM_DOWNLOAD_GTest}"
  VERSION 1.14.0
  URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
  URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
  FIND_PACKAGE_ARGUMENTS "CONFIG"
  OPTIONS "BUILD_GMOCK OFF" "INSTALL_GTEST OFF"
)
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
  PRIVATE util::util util_warnings GTest::gtest_main
)
set_target_properties(util_unit_tests PROPERTIES CXX_SCAN_FOR_MODULES OFF)

if(UTIL_COVERAGE)
  target_compile_options(util_unit_tests PRIVATE --coverage)
  target_link_options(util_unit_tests PRIVATE --coverage)
endif()

# PRE_TEST lists the tests when ctest runs, not at build time, so a cross
# build never has to execute the binary on the build host.
gtest_discover_tests(util_unit_tests
  DISCOVERY_MODE PRE_TEST
  PROPERTIES LABELS unit
)
