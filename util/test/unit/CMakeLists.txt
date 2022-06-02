cmake_minimum_required(VERSION 3.20)

# We use Google Test for our testing
find_package(GTest REQUIRED)
include(GoogleTest)

# For every source file in the project there should be a corresponding test file
# with the same name. This enforces unit test creation. If BUILD_TEST is enabled
# but the corresponding test file does not exist, then this will fail.
foreach(file ${SOURCE_FILES})
    cmake_path(GET file FILENAME test_file)
    cmake_path(GET test_file STEM test_name)

    # Each source file will generate it's own separate executable.
    # Conversely, we could change this to a single executable if we wanted
    # to reduce the number of steps for creating files.
    set(TEST_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/${test_file})
    set(TEST_EXECUTABLE ${test_name}_tests)

    add_executable(${TEST_EXECUTABLE} ${TEST_SOURCE})
    target_link_libraries(${TEST_EXECUTABLE}
        PRIVATE ${PROJECT_NAME} GTest::GTest GTest::Main)

    # This will discover all tests for each executable, so running "ctest" in
    # the build directory will run all the executables and tests.
    # Alternatively they can all be run on the command line.
    gtest_discover_tests(${TEST_EXECUTABLE})

endforeach()