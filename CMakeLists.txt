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
# List any package dependencies
# REQUIRED so a missing OpenSSL fails at configure instead of at link time.
find_package(OpenSSL REQUIRED)

################################################################################
# Create a shared library
add_library(${PROJECT_NAME} SHARED)

# PUBLIC so consumers compile against the same standard as the headers.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

################################################################################
# Target include files
# Globbed on purpose: a new header needs no edit here, and the test-file
# check in test/unit picks it up. CONFIGURE_DEPENDS re-checks the glob at
# build time, so a new file is seen without a manual re-configure.
file(GLOB PUBLIC_HEADER_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/inc/*.hpp
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    # where the library itself will look for its internal headers
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  PUBLIC
    # where top-level project will look for the library's public headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    # where external projects will look for the library's public headers
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

################################################################################
# Target source files
# Globbed like the headers. test/unit builds one test executable per entry,
# so a source without a matching test file fails at configure.
file(GLOB SOURCE_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

target_sources(${PROJECT_NAME}
  PRIVATE
    $<BUILD_INTERFACE:${SOURCE_FILES}>
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
if (BUILD_TEST)
  enable_testing()
  if (TEST_COVERAGE)
    include(cmake/CodeCoverage.cmake)
    append_coverage_compiler_flags_to_target(${PROJECT_NAME})
  endif()
  add_subdirectory(test)
endif()

################################################################################
# INSTALL SECTION
# Set the installation locations for the library
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}_Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Write out the information to the *Target.cmake file
install(
  EXPORT ${PROJECT_NAME}_Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# Copy public headers to destination
install(FILES ${PUBLIC_HEADER_FILES}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

################################################################################
# Write out the config version information
write_basic_package_version_file(
  "${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

# Create the package configuration
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/TemplateConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# Install the package and version config
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# Add in the LICENSE
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")

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