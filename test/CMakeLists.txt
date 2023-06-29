cmake_minimum_required(VERSION 3.20)

# Unit tests
if (BUILD_TEST MATCHES "UNIT|ALL|ON")
add_subdirectory(unit)
else()
message("Unit tests disabled.")
endif()

# Integration tests
if (BUILD_TEST MATCHES "INTG|ALL|ON")
add_subdirectory(intg)
else()
message("Integration tests disabled.")
endif()

# Functional tests
if (BUILD_TEST MATCHES "FUNC|ALL|ON")
add_subdirectory(func)
else()
message("Functional tests disabled.")
endif()