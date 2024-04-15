# FITS-cpp-library

## Description

Library for fast reading/writing of FITS files using Boost Asio

## Installation

1. Clone this repository to your local machine:

    ```
    git clone https://github.com/cilantrofe/FITS-cpp-library.git
    ```

2. Navigate to the `lib_fits` directory:

    ```
    cd FITS-cpp-library/lib_fits
    ```

3. Run CMake to generate build files in a `build` directory (before doing this, make sure that you have Boost installed at least 1.84):

    ```
    cmake -Bbuild
    ```

4. Navigate to the `build` directory:

    ```
    cd build
    ```

5. Build the project using Make:

    ```
    make
    ```

6. Finally, install the project:

    ```
    cmake --build . --config Release --target install
    ```

After following these steps, FITS-cpp-library should be successfully installed on your system.

## How to use

To use the FITS-cpp-library, include it in your C++ project and utilize its functionality for reading and writing FITS files using Boost Asio.

Here's an example of how to use the library in your code:

### examples/main.cpp:
```
#include <lib_fits.hpp>
#include <iostream>
#include <boost/asio.hpp>

int main() {
    /*
    / Writing
    */

    // Creating example.fits with 2 HDU's of size 20*30 and 10*6*5 in examples/build
    ofits<int16_t, float> example_file{"example.fits",  {{{20, 30}, {10, 6, 5}}}};

    // Async-write data to example.fits
    std::vector<float> data = {0.1f, 2.0f, 3.0f, 4.0f, 5.0f};

    example_file.async_write_data<1>({0, 0}, data, /* your WriteToken */ [](const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (!error) {
            std::cout << "Data written successfully!" << std::endl;
        } else {
            std::cerr << "Error writing data: " << error.message() << std::endl;
        } 
    });

    // Run context
    example_file.run();
    
    // Add header
    example_file.value_as<0>("EXAMPLE", "2024-04-13");

    /*
    / Reading
    */


}
```

### examples/CMakeLists.txt:
```
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
```


