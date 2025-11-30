cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
# Skip optional Vulkan wrapper lookups to avoid noisy Qt warnings when Vulkan SDK isn't present.
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders ON CACHE BOOL "" FORCE)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/install")

set(MO2_PLUGIN_TARGET_DIR "D:/Modding/MO2/plugins")
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
    include(${DEPENDENCIES_DIR}/cmake_common/mo2.cmake)
else ()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake_common/mo2.cmake)
endif ()

project(fomod_plus)
set(enable_warnings OFF)

add_subdirectory(installer)
add_subdirectory(scanner)
add_subdirectory(tests)

# Package target to create plugin distribution
add_custom_target(package
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_LIST_DIR}/package/plugins"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_LIST_DIR}/package/translations"
    
    # Copy DLLs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "$<TARGET_FILE:fomod_plus_installer>" 
        "${CMAKE_CURRENT_LIST_DIR}/package/plugins/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        "$<TARGET_FILE:fomod_plus_scanner>" 
        "${CMAKE_CURRENT_LIST_DIR}/package/plugins/"
    
    # Copy all .qm files using CMake script
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR="${CMAKE_CURRENT_BINARY_DIR}" 
        -DDEST_DIR="${CMAKE_CURRENT_LIST_DIR}/package/translations"
        -P "${CMAKE_CURRENT_LIST_DIR}/copy_translations.cmake"
    
    DEPENDS fomod_plus_installer fomod_plus_scanner
    COMMENT "Packaging FOMOD Plus plugin for distribution"
)
