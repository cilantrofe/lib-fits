cmake_minimum_required(VERSION 3.5.0)

project(
    examples 
    LANGUAGES CXX)

find_package(lib_fits CONFIG REQUIRED)

message(${lib_fits_DIR})
message(${lib_fits_INCLUDE_DIR})

include_directories(${lib_fits_INCLUDE_DIR})

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} lib_fits::lib_fits)