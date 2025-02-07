cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)


set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/install")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "C:/Users/tomau/OneDrive/Desktop/mo2-2.5.0/plugins")
#if (DEFINED ENV{MO2_PLUGIN_TARGET_DIR})
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO $ENV{MO2_PLUGIN_TARGET_DIR})
#else ()
#    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/../../../install/bin/plugins)
#endif ()



# For some reason MSVC doesn't get picked up by CLion.
set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Zi /O2")

if (DEFINED DEPENDENCIES_DIR)
    include(${DEPENDENCIES_DIR}/modorganizer_super/cmake_common/mo2.cmake)
else ()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake_common/mo2.cmake)
endif ()

project(fomod_plus)
set(enable_warnings OFF)

add_subdirectory(installer)
add_subdirectory(scanner)
add_subdirectory(tests)