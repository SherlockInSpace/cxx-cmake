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
# Build tests
# include(CTest) above defines BUILD_TESTING, default ON. Pass
# -DBUILD_TESTING=OFF when you only need the library.
if(BUILD_TESTING)
  add_subdirectory(test)
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
# Build Documentation
# first we can indicate the documentation build as an
# option and set it to ON by default
option(BUILD_DOC "Build documentation" ON)

if (BUILD_DOC)
  # check if Doxygen is installed
  find_package(Doxygen)
  if (DOXYGEN_FOUND)
      # set input and output files
      set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
      set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

      # request to configure the file
      configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
      message("Doxygen build started")

      # note the option ALL which allows to build
      # the docs together with the application
      add_custom_target( doc_doxygen ALL
          COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          COMMENT "Generating API documentation with Doxygen"
          VERBATIM )
  else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate the doxygen documentation")
  endif (DOXYGEN_FOUND)
endif(BUILD_DOC)