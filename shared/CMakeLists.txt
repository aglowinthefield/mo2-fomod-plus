cmake_minimum_required(VERSION 3.30)
project(fomod_plus_shared LANGUAGES CXX)

# Create header-only interface library
add_library(fomod_plus_shared INTERFACE)

# Set include directories
target_include_directories(fomod_plus_shared INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Require C++23
target_compile_features(fomod_plus_shared INTERFACE cxx_std_23)

# Find and link Qt6 since stringutil uses QString
find_package(Qt6 CONFIG REQUIRED COMPONENTS Core)
target_link_libraries(fomod_plus_shared INTERFACE Qt6::Core)