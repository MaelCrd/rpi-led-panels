cmake_minimum_required(VERSION 3.10)
project(rpi-led-panels)

set(CMAKE_CXX_STANDARD 17)

# Add include directory
include_directories(include)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Collect all source files from src directory
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "Build Noise Tool" FORCE) #example if don't need the graph tool
add_subdirectory(deps/FastNoise2)

find_package(oatpp 1.4.0 REQUIRED)


# Build the RGB matrix library using ExternalProject or custom command
# Since the matrix library uses Make, we'll use a custom target
add_custom_target(
    rgbmatrix_lib
    COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib
    COMMENT "Building RGB matrix library"
)

# Create imported library target for the RGB matrix
add_library(rgbmatrix STATIC IMPORTED)
set_target_properties(rgbmatrix PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/lib/librgbmatrix.a
)

add_custom_target(
    qrcodegen_lib
    COMMAND $(MAKE) -C ${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen
    COMMENT "Building QR Code generator library"
)

# Create imported library target for the QR code generator
add_library(qrcodegen STATIC IMPORTED)
set_target_properties(qrcodegen PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen/libqrcodegencpp.a
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
)

target_include_directories(rpi-led-panels PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/matrix/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/qrcodegen"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/nlohmann_proper"
)