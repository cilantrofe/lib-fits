# This CMake file defines the example project, which is a simple application
# that demonstrates the use of the lib_fits library.

cmake_minimum_required(VERSION 3.5.0)

# The project() function defines the project name and language.
project(
    examples 
    LANGUAGES CXX)

# Set Boost to use static libraries.
set(Boost_USE_STATIC_LIBS ON)

# Find the Boost libraries.
find_package(Boost 1.84.0)

# Include the Boost include directories.
include_directories(${Boost_INCLUDE_DIRS})

# Find the lib_fits library.
find_package(lib_fits CONFIG REQUIRED)

# Print the directory containing the lib_fits config file.
message(STATUS "lib_fits config file directory: ${lib_fits_DIR}")

# Print the directory containing the lib_fits include files.
message(STATUS "lib_fits include directory: ${lib_fits_INCLUDE_DIR}")

# Include the lib_fits include directory in the include directories.
include_directories(${lib_fits_INCLUDE_DIR})

# Create an executable target for the example project.
add_executable(${PROJECT_NAME} main.cpp)

# Link the executable against the Boost and lib_fits libraries.
target_link_libraries(${PROJECT_NAME}
    ${Boost_LIBRARIES}
    uring
)

# Define a compile definition for the target.
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IO_URING)

# Link the executable against the lib_fits library.
target_link_libraries(${PROJECT_NAME} lib_fits::lib_fits)
