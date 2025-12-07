cmake_minimum_required(VERSION 3.16)
project(rpi-led-panels)

# Enable ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Use LLD linker if available
find_program(LLD_PROGRAM lld)
if(LLD_PROGRAM)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -fuse-ld=lld")
endif()

# Enable Unity Build for faster compilation
# set(CMAKE_UNITY_BUILD ON)

set(CMAKE_CXX_STANDARD 17)

# Add include directory
include_directories(include)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE) #example if don't need the graph tool
add_subdirectory(deps/FastNoise2)

# Disable Unity Build for FastNoise as it causes build errors
# set_target_properties(FastNoise PROPERTIES UNITY_BUILD OFF)

find_package(oatpp 1.4.0 REQUIRED)
find_package(CURL REQUIRED) # libcurl4-openssl-dev


# Build the RGB matrix library using ExternalProject or custom command
# Since the matrix library uses Make, we'll use a custom target
set(RGBMATRIX_LIB "${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib/librgbmatrix.a")
add_custom_command(
    OUTPUT ${RGBMATRIX_LIB}
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib
    COMMENT "Building RGB matrix library"
)
add_custom_target(rgbmatrix_lib DEPENDS ${RGBMATRIX_LIB})

# Create imported library target for the RGB matrix
add_library(rgbmatrix STATIC IMPORTED)
set_target_properties(rgbmatrix PROPERTIES
    IMPORTED_LOCATION ${RGBMATRIX_LIB}
)

set(QRCODEGEN_LIB "${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen/libqrcodegencpp.a")
add_custom_command(
    OUTPUT ${QRCODEGEN_LIB}
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen
    COMMENT "Building QR Code generator library"
)
add_custom_target(qrcodegen_lib DEPENDS ${QRCODEGEN_LIB})

# Create imported library target for the QR code generator
add_library(qrcodegen STATIC IMPORTED)
set_target_properties(qrcodegen PROPERTIES
    IMPORTED_LOCATION ${QRCODEGEN_LIB}
)

# Make sure the library is built before trying to link
add_dependencies(rgbmatrix rgbmatrix_lib)
add_dependencies(qrcodegen qrcodegen_lib)

add_executable(rpi-led-panels ${SOURCES})

# Add dependency to ensure library is built first
add_dependencies(rpi-led-panels rgbmatrix_lib)
add_dependencies(rpi-led-panels qrcodegen_lib)

####
target_link_libraries(rpi-led-panels PRIVATE
    FastNoise2
    rgbmatrix
    qrcodegen
    oatpp::oatpp
    CURL::libcurl
)

target_include_directories(rpi-led-panels PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/nlohmann_proper"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/FastNoise2/src"
)

# Precompile common headers to speed up build
target_precompile_headers(rpi-led-panels PRIVATE
    <algorithm>
    <cmath>
    <cstdint>
    <cstdlib>
    <iostream>
    <string>
    <vector>
    <unistd.h>
)