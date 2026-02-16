cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
# Skip optional Vulkan wrapper lookups to avoid noisy Qt warnings when Vulkan SDK isn't present.
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders ON CACHE BOOL "" FORCE)

# Help CI/local builds find vcpkg-installed dependencies when VCPKG_ROOT is set.
if (DEFINED ENV{VCPKG_ROOT})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows/share")
    foreach(_7zip_hint
            "$ENV{VCPKG_ROOT}/installed/x64-windows/share/7zip"
            "$ENV{VCPKG_ROOT}/installed/x64-windows-static-md/share/7zip"
            "$ENV{VCPKG_ROOT}/packages/7zip_x64-windows/share/7zip")
        if (NOT DEFINED 7zip_DIR AND EXISTS "${_7zip_hint}")
            set(7zip_DIR "${_7zip_hint}")
        endif ()
    endforeach()
    foreach(_mo2_cmake_hint
            "$ENV{VCPKG_ROOT}/installed/x64-windows/share/mo2-cmake"
            "$ENV{VCPKG_ROOT}/packages/mo2-cmake_x64-windows/share/mo2-cmake")
        if (NOT DEFINED mo2-cmake_DIR AND EXISTS "${_mo2_cmake_hint}")
            set(mo2-cmake_DIR "${_mo2_cmake_hint}")
        endif ()
    endforeach()
endif ()

# Pick up manifest-mode installed packages if present.
if (DEFINED VCPKG_INSTALLED_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/x64-windows/share")
    list(APPEND CMAKE_PREFIX_PATH "${VCPKG_INSTALLED_DIR}/x64-windows-static-md/share")
    if (NOT DEFINED 7zip_DIR AND EXISTS "${VCPKG_INSTALLED_DIR}/x64-windows/share/7zip")
        set(7zip_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/share/7zip")
    endif ()
    if (NOT DEFINED 7zip_DIR AND EXISTS "${VCPKG_INSTALLED_DIR}/x64-windows-static-md/share/7zip")
        set(7zip_DIR "${VCPKG_INSTALLED_DIR}/x64-windows-static-md/share/7zip")
    endif ()
    if (NOT DEFINED mo2-cmake_DIR AND EXISTS "${VCPKG_INSTALLED_DIR}/x64-windows/share/mo2-cmake")
        set(mo2-cmake_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/share/mo2-cmake")
    endif ()
endif ()

if (DEFINED DEPENDENCIES_DIR)
    list(APPEND CMAKE_PREFIX_PATH
        ${DEPENDENCIES_DIR}/install/share
        ${DEPENDENCIES_DIR}/install/lib/cmake)
    if (NOT DEFINED 7zip_DIR)
        if (EXISTS "${DEPENDENCIES_DIR}/install/share/7zip")
            set(7zip_DIR "${DEPENDENCIES_DIR}/install/share/7zip")
        elseif (EXISTS "${DEPENDENCIES_DIR}/install/lib/cmake/7zip")
            set(7zip_DIR "${DEPENDENCIES_DIR}/install/lib/cmake/7zip")
        endif ()
    endif ()
    if (NOT DEFINED mo2-cmake_DIR)
        if (EXISTS "${DEPENDENCIES_DIR}/install/share/mo2-cmake")
            set(mo2-cmake_DIR "${DEPENDENCIES_DIR}/install/share/mo2-cmake")
        elseif (EXISTS "${DEPENDENCIES_DIR}/install/lib/cmake/mo2-cmake")
            set(mo2-cmake_DIR "${DEPENDENCIES_DIR}/install/lib/cmake/mo2-cmake")
        endif ()
    endif ()
endif ()

if (EXISTS "${CMAKE_INSTALL_PREFIX}/share")
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX}/share)
endif ()
if (NOT DEFINED 7zip_DIR AND EXISTS "${CMAKE_INSTALL_PREFIX}/share/7zip")
    set(7zip_DIR "${CMAKE_INSTALL_PREFIX}/share/7zip")
endif ()
if (NOT DEFINED mo2-cmake_DIR AND EXISTS "${CMAKE_INSTALL_PREFIX}/share/mo2-cmake")
    set(mo2-cmake_DIR "${CMAKE_INSTALL_PREFIX}/share/mo2-cmake")
endif ()

# Fallback to local vcpkg_installed cache (e.g., manifest builds without a toolchain).
set(_local_vcpkg_root "${CMAKE_CURRENT_LIST_DIR}/vcpkg_installed")
if (EXISTS "${_local_vcpkg_root}/x64-windows/share")
    list(APPEND CMAKE_PREFIX_PATH "${_local_vcpkg_root}/x64-windows/share")
    if (NOT DEFINED 7zip_DIR AND EXISTS "${_local_vcpkg_root}/x64-windows/share/7zip")
        set(7zip_DIR "${_local_vcpkg_root}/x64-windows/share/7zip")
    endif ()
    if (NOT DEFINED mo2-cmake_DIR AND EXISTS "${_local_vcpkg_root}/x64-windows/share/mo2-cmake")
        set(mo2-cmake_DIR "${_local_vcpkg_root}/x64-windows/share/mo2-cmake")
    endif ()
endif ()
unset(_local_vcpkg_root)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/install")

if (NOT DEFINED MO2_PLUGIN_TARGET_DIR)
    set(MO2_PLUGIN_TARGET_DIR "D:/Modding/MO2/plugins")
endif ()
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

set(_fomod_plus_uses_local_mo2_cmake OFF)
set(_fomod_plus_mo2_helper_dir "")
if (DEFINED DEPENDENCIES_DIR AND EXISTS "${DEPENDENCIES_DIR}/cmake_common/mo2.cmake")
    set(_fomod_plus_mo2_helper_dir "${DEPENDENCIES_DIR}/cmake_common")
    include(${_fomod_plus_mo2_helper_dir}/mo2.cmake)
    list(APPEND CMAKE_PREFIX_PATH ${DEPENDENCIES_DIR}/install)
    set(_fomod_plus_uses_local_mo2_cmake ON)
elseif (EXISTS "${CMAKE_CURRENT_LIST_DIR}/cmake_common/mo2.cmake")
    set(_fomod_plus_mo2_helper_dir "${CMAKE_CURRENT_LIST_DIR}/cmake_common")
    include(${_fomod_plus_mo2_helper_dir}/mo2.cmake)
    set(_fomod_plus_uses_local_mo2_cmake ON)
endif ()
if (_fomod_plus_uses_local_mo2_cmake AND NOT COMMAND mo2_find_libraries
        AND EXISTS "${_fomod_plus_mo2_helper_dir}/mo2_targets.cmake")
    include(${_fomod_plus_mo2_helper_dir}/mo2_targets.cmake)
endif ()

project(fomod_plus)
set(enable_warnings OFF)

# If vcpkg package directories are explicitly provided, use find_package regardless
# of whether local helpers were found (they don't have mo2_find_libraries).
if (NOT _fomod_plus_uses_local_mo2_cmake OR DEFINED mo2-uibase_DIR OR DEFINED mo2-archive_DIR)
    find_package(mo2-cmake CONFIG QUIET)
    find_package(mo2-uibase CONFIG REQUIRED)
    find_package(mo2-archive CONFIG REQUIRED)
elseif (COMMAND mo2_find_libraries)
    mo2_find_libraries(uibase archive)
else ()
    message(FATAL_ERROR "Local Mod Organizer helpers were found but mo2_find_libraries() is unavailable. "
        "Ensure ${_fomod_plus_mo2_helper_dir} contains an up-to-date mo2_targets.cmake.")
endif ()

# Provide compatibility with newer MO2 cmake helpers that renamed install macro.
if (NOT COMMAND mo2_install_target AND COMMAND mo2_install_plugin)
    function(mo2_install_target target)
        mo2_install_plugin(${target})
    endfunction()
endif ()

get_target_property(MO2_UIBASE_INCLUDE_DIRS mo2::uibase INTERFACE_INCLUDE_DIRECTORIES)
foreach(dir ${MO2_UIBASE_INCLUDE_DIRS})
    list(APPEND MO2_UIBASE_INCLUDE_DIRS "${dir}/uibase" "${dir}/uibase/game_features")
endforeach()
get_target_property(MO2_ARCHIVE_INCLUDE_DIRS mo2::archive INTERFACE_INCLUDE_DIRECTORIES)
foreach(dir ${MO2_ARCHIVE_INCLUDE_DIRS})
    list(APPEND MO2_ARCHIVE_INCLUDE_DIRS "${dir}/archive")
endforeach()

option(FOMOD_PLUS_BUILD_PATCHFINDER "Build the Patch Finder plugin (beta)" ON)

add_subdirectory(installer)
if (FOMOD_PLUS_BUILD_PATCHFINDER)
    add_subdirectory(patchfinder)
endif ()
add_subdirectory(scanner)
add_subdirectory(tests EXCLUDE_FROM_ALL)

# Package target to create plugin distribution
set(_package_depends fomod_plus_installer fomod_plus_scanner)
set(_package_copy_commands
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:fomod_plus_installer>"
        "${CMAKE_CURRENT_LIST_DIR}/package/plugins/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:fomod_plus_scanner>"
        "${CMAKE_CURRENT_LIST_DIR}/package/plugins/"
)
if (FOMOD_PLUS_BUILD_PATCHFINDER)
    list(APPEND _package_depends fomod_plus_patch_finder)
    list(APPEND _package_copy_commands
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:fomod_plus_patch_finder>"
            "${CMAKE_CURRENT_LIST_DIR}/package/plugins/"
    )
endif ()

add_custom_target(package
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_LIST_DIR}/package/plugins"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_LIST_DIR}/package/translations"

    # Copy DLLs
    ${_package_copy_commands}

    # Copy all .qm files using CMake script
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR="${CMAKE_CURRENT_BINARY_DIR}"
        -DDEST_DIR="${CMAKE_CURRENT_LIST_DIR}/package/translations"
        -P "${CMAKE_CURRENT_LIST_DIR}/copy_translations.cmake"

    DEPENDS ${_package_depends}
    COMMENT "Packaging FOMOD Plus plugin for distribution"
)
