# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

# A downstream project in miniature: only the two lines DESIGN.md promises.
cmake_minimum_required(VERSION 4.3)
project(consumer LANGUAGES CXX)

find_package(util ${EXPECTED_UTIL_VERSION} EXACT CONFIG REQUIRED)

add_executable(consumer main.cpp)
target_link_libraries(consumer PRIVATE util::util)
