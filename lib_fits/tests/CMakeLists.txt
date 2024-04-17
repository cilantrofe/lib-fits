# This CMakeLists.txt file is used to build the unit tests for the lib_fits C++ library.

# The minimum version of CMake required is 3.5.0.
cmake_minimum_required(VERSION 3.5.0)

# Set the project name and version.
project(tests VERSION 1.0.0)

# Add an executable target for the unit tests.
add_executable(tests main.cpp test_ifits.cpp test_ofits.cpp)

# Set the C++ standard and required status.
set_property(TARGET tests PROPERTY CXX_STANDARD 20)
set_property(TARGET tests PROPERTY CXX_STANDARD_REQUIRED ON)

# Include the GoogleTest module to make it easier to discover and run tests.
include(GoogleTest)

# Get the parent directory of the project source directory.
get_filename_component(PARENT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

# Include the include directory in the interface include directories.
target_include_directories(tests PRIVATE ${PARENT_DIR}/include/lib_fits)

# Link against the lib_fits library, Google Test, and Google Mock.
target_link_libraries(tests PUBLIC lib_fits gtest gtest_main)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE uring)
    # Define a compile definition for the target.
    target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IO_URING)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IOCP)
    add_compile_options(/D _WIN32_WINNT=0x0601)
endif()

# Discover and run the tests.
gtest_discover_tests(tests)

# Enable testing.
enable_testing()

