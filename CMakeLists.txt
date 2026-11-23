# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 4.3)

################################################################################
# Project name and configuration
project(
  "util"
  VERSION 0.1.0
  DESCRIPTION
    "Example C++ CMake library."
  HOMEPAGE_URL ""
  LANGUAGES CXX)

################################################################################
# Do NOT allow in-source builds
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. "
      "Please make a build directory and run CMake from there."
  )
endif()

################################################################################
# Includes
include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

################################################################################
# Dependencies
# Every dependency is a CPMAddPackage. By default each one resolves through
# find_package, from the container image or the Yocto sysroot, and configure
# fails if it is missing: nothing is downloaded. -DCPM_LOCAL_PACKAGES_ONLY=OFF
# lets CPM fetch what find_package cannot find, and -DCPM_<Name>_SOURCE=/path
# builds one dependency from a local checkout. VERSION is the minimum the code
# needs, not a pin; the container tag is the pin. docs/DECISIONS.md has the
# caveats, and the configure log says what resolved.
option(CPM_USE_LOCAL_PACKAGES "Try find_package before downloading" ON)
option(CPM_LOCAL_PACKAGES_ONLY "Only use find_package, never download" ON)
include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

# CPM reports a package missing under CPM_LOCAL_PACKAGES_ONLY with SEND_ERROR,
# which does not stop configure, and then downloads it anyway. With FetchContent
# fully disconnected that download cannot happen, so local-only means no network.
if(CPM_LOCAL_PACKAGES_ONLY)
  set(FETCHCONTENT_FULLY_DISCONNECTED ON)
endif()

# libcrypto only. OpenSSL is not a CMake project, so it always comes from
# find_package (point OPENSSL_ROOT_DIR at a build if it is somewhere unusual).
CPMAddPackage(
  NAME OpenSSL
  VERSION 3.0.0
  FIND_PACKAGE_ARGUMENTS "COMPONENTS Crypto"
)

################################################################################
# Create the library
# Shared unless -DBUILD_SHARED_LIBS=OFF. Only declared when util is the
# top-level project, so pulling util into another build does not change
# that build's default for its own libraries.
if(PROJECT_IS_TOP_LEVEL)
  option(BUILD_SHARED_LIBS "Build shared libraries" ON)
endif()

add_library(${PROJECT_NAME})
# The same name consumers get from find_package, so in-tree code and
# add_subdirectory users link util::util too.
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# libutil.so -> libutil.so.<major> -> libutil.so.<version>. The SONAME is the
# major version, which is what changes when the ABI breaks.
set_target_properties(${PROJECT_NAME} PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

# PUBLIC so consumers compile against the same standard as the headers.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# No C++20 modules here, so skip CMake's module dependency scan. With it on,
# every translation unit gets an extra scanning step and compile_commands.json
# carries GCC-only flags (-fmodules-ts, -fdeps-format=...) that clang-tidy and
# clangd reject. A fork that adds modules turns it back on for those targets.
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_SCAN_FOR_MODULES OFF)

################################################################################
# Warnings
# The flags live on util_warnings, an INTERFACE target that util and its tests
# link PRIVATE through $<BUILD_INTERFACE:...>, so they never reach a consumer's
# compile line and the target is never exported. -Werror is a separate switch:
# on in the CI presets, off in dev, so a newer compiler's new warning does not
# stop a developer's build but does stop a pull request.
option(UTIL_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(util_warnings INTERFACE)
target_compile_options(util_warnings INTERFACE
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
    -Wall -Wextra -Wpedantic
    -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast -Wcast-align
    -Wnon-virtual-dtor -Woverloaded-virtual -Wnull-dereference
    -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough>
  $<$<CXX_COMPILER_ID:GNU>:
    -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast>
  $<$<AND:$<BOOL:${UTIL_WARNINGS_AS_ERRORS}>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-Werror>
)
target_link_libraries(${PROJECT_NAME} PRIVATE $<BUILD_INTERFACE:util_warnings>)

################################################################################
# Target sources and public headers
# Globbed on purpose: a new file needs no edit here, and the test-file check
# in test/unit picks it up. CONFIGURE_DEPENDS re-checks the globs at build
# time, so a new file is seen without a manual re-configure.
file(GLOB PUBLIC_HEADER_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/util/*.hpp
)
file(GLOB SOURCE_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# The HEADERS file set puts inc/ on the include path in-tree and
# include/ once installed, so everyone writes #include <util/x.hpp>.
target_sources(${PROJECT_NAME}
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/inc
    FILES ${PUBLIC_HEADER_FILES}
  PRIVATE
    ${SOURCE_FILES}
)

################################################################################
# Link in depedent libraries
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    # The Bloom filter only uses libcrypto, so we leave libssl out.
    OpenSSL::Crypto
)

################################################################################
# Coverage
# gcov instrumentation on util and its unit tests only, never PUBLIC, so
# nothing that links util is instrumented by accident. The coverage preset
# turns it on; the coverage target runs the tests and then gcovr once.
option(UTIL_COVERAGE "Instrument util and its unit tests for gcov coverage" OFF)
set(UTIL_COVERAGE_FAIL_UNDER_LINE 90 CACHE STRING
  "The coverage target fails if total line coverage is under this percentage")
set(UTIL_COVERAGE_FILE_FLOOR 80 CACHE STRING
  "The coverage target fails if any one file's line coverage is under this percentage")

if(UTIL_COVERAGE)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "UTIL_COVERAGE uses gcov and gcovr, so it needs GCC. "
      "With Clang, use -fprofile-instr-generate -fcoverage-mapping and llvm-cov instead.")
  endif()
  find_program(GCOVR_EXECUTABLE gcovr REQUIRED)
  target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
  target_link_options(${PROJECT_NAME} PRIVATE --coverage)
endif()

################################################################################
# Build tests
# include(CTest) above defines BUILD_TESTING, default ON. Pass
# -DBUILD_TESTING=OFF when you only need the library.
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

################################################################################
# Benchmarks
# Google Benchmark binaries in bench/, run weekly and at release rather than on
# every commit (docs/DECISIONS.md). Release builds only, and never with
# coverage: timing a Debug or instrumented build measures the wrong thing.
option(BUILD_BENCH "Build the Google Benchmark binaries in bench/" OFF)

if(BUILD_BENCH)
  if(UTIL_COVERAGE)
    message(FATAL_ERROR "BUILD_BENCH and UTIL_COVERAGE do not mix: "
      "coverage instrumentation distorts the timings")
  endif()
  get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(NOT multi_config AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "BUILD_BENCH needs CMAKE_BUILD_TYPE=Release; "
      "the bench preset sets it")
  endif()
  if(NOT BUILD_TESTING)
    message(FATAL_ERROR "BUILD_BENCH registers the benchmarks with CTest, "
      "so it needs BUILD_TESTING=ON")
  endif()
  add_subdirectory(bench)
endif()

if(UTIL_COVERAGE)
  if(NOT BUILD_TESTING)
    message(FATAL_ERROR "UTIL_COVERAGE needs BUILD_TESTING=ON")
  endif()
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND}
      -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
      -DBINARY_DIR=${PROJECT_BINARY_DIR}
      -DGCOVR=${GCOVR_EXECUTABLE}
      -DCTEST=${CMAKE_CTEST_COMMAND}
      -DFAIL_UNDER_LINE=${UTIL_COVERAGE_FAIL_UNDER_LINE}
      -DFILE_FLOOR=${UTIL_COVERAGE_FILE_FLOOR}
      -P ${PROJECT_SOURCE_DIR}/cmake/coverage-report.cmake
    DEPENDS util_unit_tests
    COMMENT "Running the unit tests and gcovr"
    USES_TERMINAL
    VERBATIM)
endif()

################################################################################
# INSTALL SECTION
# Every CMake package file goes to <libdir>/cmake/util, next to the library.
set(UTIL_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET HEADERS
)

# Write out the information to the *Target.cmake file
install(
  EXPORT ${PROJECT_NAME}_Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${UTIL_INSTALL_CMAKEDIR})

################################################################################
# Write out the config version information
# Before 1.0 a minor bump may break the API, so only the same minor version
# is compatible. Switch to SameMajorVersion at 1.0.
write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion)

# Create the package configuration
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${UTIL_INSTALL_CMAKEDIR})

# Install the package and version config
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${UTIL_INSTALL_CMAKEDIR})

################################################################################
# Documentation
# BUILD_DOCS adds a docs target, which is not part of the default build: run
# cmake --build --preset <preset> --target docs. Doxygen is a build tool, not a
# library dependency, so it is a plain find_package rather than a CPM package.
# The doxygen-awesome-css theme is used when it is installed where the
# container image puts it; nothing is downloaded.
option(BUILD_DOCS "Add the docs target (Doxygen API documentation)" OFF)

if(BUILD_DOCS)
  find_package(Doxygen 1.9 REQUIRED)

  find_path(DOXYGEN_AWESOME_CSS_DIR doxygen-awesome.css
    PATHS /usr/share/doxygen-awesome-css
    DOC "Directory holding doxygen-awesome.css")

  set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
  set(DOXYGEN_PROJECT_NUMBER "${PROJECT_VERSION}")
  set(DOXYGEN_PROJECT_BRIEF "${PROJECT_DESCRIPTION}")
  set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs")
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${PROJECT_SOURCE_DIR}/README.md")
  set(DOXYGEN_STRIP_FROM_PATH "${PROJECT_SOURCE_DIR}")
  set(DOXYGEN_STRIP_FROM_INC_PATH "${PROJECT_SOURCE_DIR}/inc")
  set(DOXYGEN_GENERATE_LATEX NO)
  set(DOXYGEN_GENERATE_TREEVIEW YES)
  set(DOXYGEN_EXTRACT_PRIVATE NO)
  set(DOXYGEN_QUIET YES)
  # Every warning fails the docs target once Doxygen has listed them all, so an
  # undocumented public symbol or a stale @param fails the docs job.
  set(DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS)

  if(DOXYGEN_AWESOME_CSS_DIR)
    set(DOXYGEN_DISABLE_INDEX NO)
    set(DOXYGEN_FULL_SIDEBAR NO)
    set(DOXYGEN_HTML_COLORSTYLE LIGHT)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET
      "${DOXYGEN_AWESOME_CSS_DIR}/doxygen-awesome.css"
      "${DOXYGEN_AWESOME_CSS_DIR}/doxygen-awesome-sidebar-only.css")
  else()
    message(STATUS "doxygen-awesome-css not found: docs use Doxygen's own theme")
  endif()

  doxygen_add_docs(docs
    "${PROJECT_SOURCE_DIR}/inc/util"
    "${PROJECT_SOURCE_DIR}/README.md"
    "${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    "${PROJECT_SOURCE_DIR}/docs"
    COMMENT "Generating the API documentation in ${DOXYGEN_OUTPUT_DIRECTORY}")
endif()
