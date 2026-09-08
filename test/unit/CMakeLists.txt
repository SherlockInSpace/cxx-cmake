# One binary for all the unit tests. We link util once and pick a module
# with ctest -R.
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

# Our own glob rather than SOURCE_FILES, so a test file needs no matching
# file in src/.
file(GLOB UNIT_TEST_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(util_unit_tests ${UNIT_TEST_FILES})
target_link_libraries(util_unit_tests
  PRIVATE util GTest::gtest_main
)

# PRE_TEST lists the tests when ctest runs, not at build time, so a cross
# build never has to execute the binary on the build host.
gtest_discover_tests(util_unit_tests
  DISCOVERY_MODE PRE_TEST
  PROPERTIES LABELS unit
)
