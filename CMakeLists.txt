cmake_minimum_required(VERSION 3.31)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# For vim nerds. Will export to your output dir from cmake. Copy to root.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_cmake_property(multiconfig GENERATOR_IS_MULTI_CONFIG)
if (multiconfig)
  set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE)
else()
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

project(fomod_plus)

# MSVC or clang-cl (MSVC-like frontend) need /Zc:__cplusplus; MSVC also needs /std:...
if (MSVC
    OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
  add_compile_options(/Zc:__cplusplus /std:c++latest)
endif()


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_subdirectory(shared)
add_subdirectory(scanner)
add_subdirectory(installer)
#add_subdirectory(tests)
