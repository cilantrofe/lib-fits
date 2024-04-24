# This CMake file defines the example project, which is a simple application
# that demonstrates the use of the lib_fits library.

cmake_minimum_required(VERSION 3.5.0)

# The project() function defines the project name and language.
project(
    experiment 
    LANGUAGES CXX)

# Set the C++ standard and required status.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)     

# Find CFITSIO
find_package(PkgConfig REQUIRED)
pkg_check_modules(CFITSIO REQUIRED IMPORTED_TARGET cfitsio)

# Create an executable target for the example project.
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(experiment  PkgConfig::CFITSIO)