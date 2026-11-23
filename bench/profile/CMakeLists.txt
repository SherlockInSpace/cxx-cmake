# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

# Tracy from the image or sysroot, the client built with TRACY_ENABLE. If CPM
# is allowed to fetch, it builds the v0.15.0 source tarball, checked by its
# SHA-256.
CPMAddPackage(
  NAME Tracy
  FORCE "${CPM_DOWNLOAD_Tracy}"
  VERSION 0.15.0
  URL https://github.com/wolfpld/tracy/archive/refs/tags/v0.15.0.tar.gz
  URL_HASH SHA256=3d57da54aa67d7875da1e8fecd0bd5957675cef73c27dd52bffb6ab8d290a5e6
  FIND_PACKAGE_ARGUMENTS "CONFIG"
  OPTIONS
    "TRACY_ENABLE ON"
)

# The only target that links Tracy. It puts zones around util's calls from
# here; util itself is the same library that ships.
add_executable(util_profile main.cpp)
target_link_libraries(util_profile
  PRIVATE util::util util_warnings Tracy::TracyClient
)
set_target_properties(util_profile PROPERTIES CXX_SCAN_FOR_MODULES OFF)

# Runs the harness once without a profiler attached, so the profile preset
# proves it builds and runs. docs/profiling.md shows how to capture a trace.
if(BUILD_TESTING)
  add_test(NAME profile.run COMMAND util_profile)
  set_tests_properties(profile.run PROPERTIES LABELS profile)
endif()
