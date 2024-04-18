# CMakeLists.txt
#
# This file is the top-level CMake build script for the lib_fits project.

cmake_minimum_required(VERSION 3.5.0)

# Set the C++ and C compiler paths.
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_C_COMPILER /usr/bin/gcc)

# Define the project name, version, and languages.
project(
    lib_fits 
    LANGUAGES CXX
    VERSION 1.0.0)

# Include the standard GNUInstallDirs module.
include(GNUInstallDirs)

# Set the C++ standard and required status.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

# Define the directory containing the include files.
set(LIB_FITS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include) 

# Include the include directory in the interface include directories.
include_directories(${LIB_FITS_DIR}/lib_fits)

# Create an interface library from the include directory.
add_library(lib_fits INTERFACE ${LIB_FITS_DIR})

# Add the include directory to the interface include directories.
target_include_directories(
    ${PROJECT_NAME}
    INTERFACE
    $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Set Boost to use static libraries.
set(Boost_USE_STATIC_LIBS ON)

# Find the Boost libraries.
find_package(Boost 1.84.0)

# Include the Boost include directories.
include_directories(${Boost_INCLUDE_DIRS})

# Link the Boost libraries to the interface library.
target_link_libraries(lib_fits ${Boost_LIBRARIES})

# Install the library, target exports, and config files.
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Include the CMakePackageConfigHelpers module.
include(CMakePackageConfigHelpers)

# Write the version file for the config file.
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
                                 VERSION ${PROJECT_VERSION}
                                 COMPATIBILITY SameMajorVersion)

# Define the install directory for the include files.
if(NOT INCLUDE_INSTALL_DIR)
  set(INCLUDE_INSTALL_DIR include/lib_fits)
endif()

# Configure the config file.
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
  PATH_VARS INCLUDE_INSTALL_DIR)

# Install the target exports and config files.
install(EXPORT ${PROJECT_NAME}_Targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
              "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake)

# Install the include files.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

