cmake_minimum_required(VERSION 3.16)
project(rpi-led-panels)

# Enable ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Note: LLD is not used because it cannot process GCC's LTO (GIMPLE) objects.

# Unity Build (Jumbo build) batches .cpp files to dramatically speed up clean builds.
# Uncomment the line below to re-enable Unity Build:
# set(CMAKE_UNITY_BUILD ON)

set(CMAKE_CXX_STANDARD 17)

# Add include directory
include_directories(include)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect all source files from src directory automatically (CONFIGURE_DEPENDS tracks added/removed files)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/*.c")

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE) #example if don't need the graph tool
add_subdirectory(deps/FastNoise2)

# Fix for FastNoise2 adding x86 flag on ARM architecture and fix strict vector types
get_target_property(FS_OPTS FastSIMD_FastNoise COMPILE_OPTIONS)
if(NOT FS_OPTS)
    set(FS_OPTS "")
endif()
list(REMOVE_ITEM FS_OPTS "-mno-vzeroupper")
list(REMOVE_ITEM FS_OPTS "-Wno-nan-infinity-disabled")
list(APPEND FS_OPTS "-flax-vector-conversions")
set_target_properties(FastSIMD_FastNoise PROPERTIES COMPILE_OPTIONS "${FS_OPTS}")


# Ensure FastNoise/FastSIMD targets stay separate if Unity Build is re-enabled
set_target_properties(FastNoise PROPERTIES UNITY_BUILD OFF)
set_target_properties(FastSIMD PROPERTIES UNITY_BUILD OFF)
set_target_properties(FastSIMD_FastNoise PROPERTIES UNITY_BUILD OFF)

# Disable Oat++ internal tests, test library, and installation targets
set(OATPP_BUILD_TESTS OFF CACHE BOOL "Build oatpp tests" FORCE)
set(OATPP_LINK_TEST_LIBRARY OFF CACHE BOOL "Link oatpp test library" FORCE)
set(OATPP_INSTALL OFF CACHE BOOL "Install oatpp" FORCE)
# Add the oatpp submodule directory
add_subdirectory(deps/oatpp)
# Create an alias target so existing references to oatpp::oatpp continue to work
if(NOT TARGET oatpp::oatpp)
    add_library(oatpp::oatpp ALIAS oatpp)
endif()

find_package(CURL REQUIRED) # libcurl4-openssl-dev


# Build the RGB matrix library using ExternalProject or custom command
# Since the matrix library uses Make, we'll use a custom target
set(RGBMATRIX_LIB "${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib/librgbmatrix.a")
add_custom_command(
    OUTPUT ${RGBMATRIX_LIB}
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib "CFLAGS=-O3 -g -fPIC -W -Wall -Wextra -Wno-unused-parameter" "CXXFLAGS=-O3 -g -fPIC -fno-exceptions -std=c++17 -W -Wall -Wextra -Wno-unused-parameter"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib
    DEPENDS ${CMAKE_CURRENT_LIST_FILE}
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

# stb_image header-only library
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/deps")

add_executable(rpi-led-panels ${SOURCES})

# Add dependency to ensure library is built first
add_dependencies(rpi-led-panels rgbmatrix_lib)
add_dependencies(rpi-led-panels qrcodegen_lib)

####
target_link_libraries(rpi-led-panels PRIVATE
    FastNoise2
    FastSIMD
    FastSIMD_FastNoise
    rgbmatrix
    qrcodegen
    oatpp::oatpp
    CURL::libcurl
    stb_image
)

target_include_directories(rpi-led-panels PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/nlohmann_proper"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/FastNoise2/src"
)

if(NOT DEFINED ASSETS_DIR)
    set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
endif()
target_compile_definitions(rpi-led-panels PRIVATE 
    ASSETS_DIR="${ASSETS_DIR}"
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