# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

# The functional tier uses the installed package the way a downstream project
# does: install util to a scratch prefix, configure a separate project against
# it with find_package(util <version> EXACT CONFIG REQUIRED), build that
# project and run it. The steps are CTest fixtures, so ctest -L functional or
# -R functional.run always runs them in order.
#
# A cross build cannot run the consumer on the build host, so the tier is
# only added for native builds.
if(CMAKE_CROSSCOMPILING)
  return()
endif()

set(prefix "${CMAKE_CURRENT_BINARY_DIR}/prefix")
set(consumer_build "${CMAKE_CURRENT_BINARY_DIR}/consumer")
set(consumer_cache "${CMAKE_CURRENT_BINARY_DIR}/consumer-cache.cmake")

# What the consumer needs to see: the scratch prefix first, then wherever this
# build found its own dependencies (a static util puts OpenSSL on the
# consumer's link line), and the same compiler, build type and flags (a
# sanitizer build of util needs a sanitizer build of its consumer).
set(consumer_prefix_path "${prefix}" ${CMAKE_PREFIX_PATH})
set(cache_lines
  "set(CMAKE_PREFIX_PATH \"${consumer_prefix_path}\" CACHE STRING \"\")"
  "set(CMAKE_CXX_COMPILER \"${CMAKE_CXX_COMPILER}\" CACHE FILEPATH \"\")"
  "set(CMAKE_BUILD_TYPE \"${CMAKE_BUILD_TYPE}\" CACHE STRING \"\")"
  "set(CMAKE_CXX_FLAGS \"${CMAKE_CXX_FLAGS}\" CACHE STRING \"\")"
  "set(CMAKE_EXE_LINKER_FLAGS \"${CMAKE_EXE_LINKER_FLAGS}\" CACHE STRING \"\")"
  "set(EXPECTED_UTIL_VERSION \"${PROJECT_VERSION}\" CACHE STRING \"\")")
foreach(var OPENSSL_ROOT_DIR OPENSSL_INCLUDE_DIR OPENSSL_CRYPTO_LIBRARY)
  if(${var})
    list(APPEND cache_lines "set(${var} \"${${var}}\" CACHE PATH \"\")")
  endif()
endforeach()
list(JOIN cache_lines "\n" cache_text)
file(GENERATE OUTPUT "${consumer_cache}" CONTENT "${cache_text}\n")

add_test(NAME functional.clean
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${prefix}" "${consumer_build}")
# A multi-config generator has to be told which configuration to install; a
# single-config build has only one, and may not even have CMAKE_BUILD_TYPE.
get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
set(install_config_args "")
if(multi_config)
  set(install_config_args --config $<CONFIG>)
endif()

add_test(NAME functional.install
  COMMAND ${CMAKE_COMMAND} --install "${PROJECT_BINARY_DIR}" --prefix "${prefix}"
    ${install_config_args})
add_test(NAME functional.configure
  COMMAND ${CMAKE_COMMAND}
    -S "${CMAKE_CURRENT_SOURCE_DIR}/consumer" -B "${consumer_build}"
    -G "${CMAKE_GENERATOR}" -C "${consumer_cache}")
add_test(NAME functional.build
  COMMAND ${CMAKE_COMMAND} --build "${consumer_build}")
add_test(NAME functional.run
  COMMAND "${consumer_build}/consumer")

set_tests_properties(functional.clean PROPERTIES FIXTURES_SETUP functional_clean)
set_tests_properties(functional.install PROPERTIES
  FIXTURES_REQUIRED functional_clean FIXTURES_SETUP functional_installed)
set_tests_properties(functional.configure PROPERTIES
  FIXTURES_REQUIRED functional_installed FIXTURES_SETUP functional_configured)
set_tests_properties(functional.build PROPERTIES
  FIXTURES_REQUIRED functional_configured FIXTURES_SETUP functional_built)
set_tests_properties(functional.run PROPERTIES FIXTURES_REQUIRED functional_built)
set_tests_properties(
  functional.clean functional.install functional.configure functional.build functional.run
  PROPERTIES LABELS functional RESOURCE_LOCK functional_prefix)
