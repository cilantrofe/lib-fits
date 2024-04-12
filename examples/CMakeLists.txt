cmake_minimum_required(VERSION 3.5.0)

project(
    examples 
    LANGUAGES CXX)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.84.0)
include_directories(${Boost_INCLUDE_DIRS})

find_package(lib_fits CONFIG REQUIRED)

message(${lib_fits_DIR})
message(${lib_fits_INCLUDE_DIR})

include_directories(${lib_fits_INCLUDE_DIR})

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(${PROJECT_NAME}
    pthread
    uring
)
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IO_URING)

target_link_libraries(${PROJECT_NAME} lib_fits::lib_fits)