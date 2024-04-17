# FITS-cpp-library

## Description

Library for fast reading/writing of FITS files using Boost Asio

## Installation

### On Linux

> Before installing, make sure that you have **Boost installed at least 1.84** and **liburing**.

1. Clone this repository to your local machine:

    ```bash
    git clone https://github.com/cilantrofe/FITS-cpp-library.git
    ```

2. Navigate to the `lib_fits` directory:

    ```bash
    cd FITS-cpp-library/lib_fits
    ```

3. Run CMake to generate build files in a `build` directory:

    ```bash
    cmake -Bbuild
    ```

4. Navigate to the `build` directory:

    ```bash
    cd build
    ```

5. Build the project using Make:

    ```bash
    make
    ```

6. Finally, install the project in the selected configuration:

    ```bash
    sudo cmake --build . --config Release --target install
    ```
After following these steps, FITS-cpp-library should be successfully installed on your system, and you can use it.

### On Windows

> It is better to install WSL and use the instructions above.
> If this is not possible, make sure that you have **Boost installed at least 1.84 and Visual Studio installed at least 17**.
> I advise to use the shell in VSCode. To use **MSBuild** in VSCode, you need to run it in a special way.
1. Open Developer Command Prompt for VS 2022 and enter:
    ```bash
    code
    ```
VSCode should open.
That will setup the path correctly to use VS2022 tools, including MSBuild. And VSCode will inherit that path so will have access to MSBuild.

2. Clone this repository to your local machine:

    ```bash
    git clone https://github.com/cilantrofe/FITS-cpp-library.git
    ```

3. Navigate to the `lib_fits` directory:

    ```bash
    cd FITS-cpp-library/lib_fits
    ```

4. Run CMake to generate build files in a `build` directory and point the way to the Boost:

    ```bash
    cmake -Bbuild -DBOOST_ROOT=/path/to/your/boost
    ```
5. Navigate to the `build` directory:

    ```bash
    cd build
    ```

6. Build the project using MSBuild:

    ```bash
    msbuild lib_fits.sln /p:CL=/std:c++20 /permissive- /D_WIN32_WINNT=0x0601
    ```

7. Finally, install the project(use Administration mode) in the selected configuration:

    ```bash
    cmake --build . --config Release --target install
    ```
After following these steps, FITS-cpp-library should be successfully installed on your system, and you can use it.

## How to use

To use the FITS-cpp-library, include it in your C++ project and utilize its functionality for reading and writing FITS files using Boost Asio.

Here's an example of how to use the library in your code:

### examples/main.cpp:
```cpp
#include <lib_fits.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <memory>


int main()
{
    //
    // Writing
    //

    // Creating example.fits with 2 HDU's of size 20*30 and 10*6*5 in examples/build
    ofits<int16_t, float> example_write{"example.fits", {{{20, 30}, {10, 6, 5}}}};
    std::vector<int16_t> data_1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Write data to first HDU
    // {1, 2} - index --> offset for data = (1 × NAXIS2 + 2) × sizeof(int16_t) relative to current HDU in data block
    example_write.async_write_data<0>({1, 2}, boost::asio::buffer(data_1), [](const boost::system::error_code &error,
        std::size_t bytes_transferred) {
        if (!error) {
            std::cout << "Data written successfully!" << std::endl;
        } else {
            std::cerr << "Error writing data: " << error.message() << std::endl;
        } });

    // Run context
    example_write.run();

    // Add header to second HDU
    example_write.value_as<1>("EXAMPLE", "2024-04-13");


    //
    // Reading
    //

    // Open example.fits
    ifits example_read("example.fits");

    // Get value from header EXAMPLE
    auto hdu_1 = example_read.get_hdu<1>().value_as<std::string>("EXAMPLE");

    std::cout << hdu_1 << std::endl;

    auto hdu_0 = example_read.get_hdu<0>();

    // Apply a function to the current HDU, based on its BITPIX value
    // Async read data from first HDU
    // Pattern Visitor
    hdu_0.apply([](auto x)
                {
        // Using shared_ptr to manage buffer memory
        auto buffer = std::make_shared<std::vector<int16_t>>(10);
        x.async_read_data({1, 2}, boost::asio::buffer(*buffer), [buffer](const boost::system::error_code &error,
            std::size_t bytes_transferred) {
            if (!error) {
                std::cout << "Data read successfully!" << std::endl;
                std::cout << "Bytes read: " << bytes_transferred << std::endl;
                for (std::size_t i = 0; i < 10; ++i) {
                    std::cout << (*buffer)[i] << " ";
                }
                std::cout << std::endl;
            } else {
                std::cerr << "Error reading data: " << error.message() << std::endl;
            }
        }); });

    // Run the IO context for asynchronous operations
    example_read.run();

    return 0;
}
```

### examples/CMakeLists.txt:
```cmake
# This CMake file defines the example project, which is a simple application
# that demonstrates the use of the lib_fits library.

cmake_minimum_required(VERSION 3.5.0)

# The project() function defines the project name and language.
project(
    examples 
    LANGUAGES CXX)

# Set the C++ standard and required status.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)     

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
)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} uring)
    # Define a compile definition for the target.
    target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IO_URING)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE BOOST_ASIO_HAS_IOCP)
endif()

# Link the executable against the lib_fits library.
target_link_libraries(${PROJECT_NAME} lib_fits::lib_fits)
```

### Build
#### On Linux
```bash
cmake -Bbuild
cd build
make
./examples
```

#### On Windows
```bash
cmake -Bbuild -DBOOST_ROOT=/path/to/your/boost
cd build
msbuild examples.sln /p:CL=/std:c++20 /permissive- /D_WIN32_WINNT=0x0601
cd Debug
./examples.exe
```

### Output
```bash
Data written successfully!
2024-04-13
Data read successfully!
Bytes read: 20
1 2 3 4 5 6 7 8 9 10 
```
