# This CMakeLists.txt file is used to build the unit tests for the lib_fits C++ library.

# The minimum version of CMake required is 3.5.0.
cmake_minimum_required(VERSION 3.5.0)

# Set the project name and version.
project(tests VERSION 1.0.0)

# Set the C++ standard and required status.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

# Add an executable target for the unit tests.
add_executable(tests main.cpp test_ifits.cpp test_ofits.cpp)

# Add the googletest subdirectory.
add_subdirectory(googletest)

# Include the GoogleTest module to make it easier to discover and run tests.
include(GoogleTest)

# Link against the lib_fits library, Google Test, and Google Mock.
target_link_libraries(tests PUBLIC gtest gtest_main)

# Set Boost to use static libraries.
set(Boost_USE_STATIC_LIBS ON)

# Find the Boost libraries.
find_package(Boost 1.84.0)

# Include the Boost include directories.
include_directories(${Boost_INCLUDE_DIRS})

# Find the lib_fits library.
find_package(lib_fits CONFIG REQUIRED)

# Include the lib_fits include directory in the include directories.
include_directories(${lib_fits_INCLUDE_DIR})

# Link the executable against the Boost and lib_fits libraries.
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE uring)
    # Define a compile definition for the target.
    target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IO_URING)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IOCP)
endif()

# Discover and run the tests.
gtest_discover_tests(tests)

# Enable testing.
enable_testing()

