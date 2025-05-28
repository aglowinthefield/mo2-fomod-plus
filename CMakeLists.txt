cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/install")

#set(MO2_PLUGIN_TARGET_DIR "D:/Modding/mo2-dev-build/plugins")
if (NOT DEFINED MO2_PLUGIN_TARGET_DIR AND DEFINED ENV{MO2_PLUGIN_TARGET_DIR})
    set(MO2_PLUGIN_TARGET_DIR $ENV{MO2_PLUGIN_TARGET_DIR})
endif ()

if (DEFINED MO2_PLUGIN_TARGET_DIR)
    message(STATUS "Using MO2_PLUGIN_TARGET_DIR: ${MO2_PLUGIN_TARGET_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${MO2_PLUGIN_TARGET_DIR})
else ()
    message(STATUS "Using default install path")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_LIST_DIR}/../../../install/bin/plugins)
endif ()

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