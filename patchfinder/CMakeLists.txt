cmake_minimum_required(VERSION 3.30)
project(fomod_plus_patch_finder)

add_library(fomod_plus_patch_finder SHARED)
include(FetchContent)
set(project_type plugin)
set_property(TARGET fomod_plus_patch_finder PROPERTY CXX_STANDARD 20)

set(CMAKE_GENERATOR_PLATFORM x64)

FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_Declare(pugixml GIT_REPOSITORY https://github.com/zeux/pugixml GIT_TAG v1.14)
FetchContent_MakeAvailable(pugixml json)

target_include_directories(
        fomod_plus_patch_finder
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../share
        ${CMAKE_CURRENT_SOURCE_DIR}/../share/FOMODData
        ${CMAKE_CURRENT_SOURCE_DIR}/../share/xml
        ${MO2_ARCHIVE_INCLUDE_DIRS}
        ${RESOURCE_DIR}
)

# Include shared sources for XML parsing
file(GLOB SHARE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../share/**/*.cpp")
target_sources(fomod_plus_patch_finder PRIVATE ${SHARE_SOURCES})

if (MSVC)
    target_compile_options(
            fomod_plus_patch_finder
            PRIVATE
            /bigobj # increase number of sections in .obj file
            /W4     # level 4 warnings
            /WX     # treat all compiler warnings as errors
            /wd4201 # nonstandard extension used: nameless struct/union
            /wd4458 # declaration of 'identifier' hides class member
    )
endif ()

target_link_libraries(fomod_plus_patch_finder PRIVATE pugixml nlohmann_json::nlohmann_json)
mo2_configure_plugin(fomod_plus_patch_finder WARNINGS OFF PRIVATE_DEPENDS archive)
mo2_install_target(fomod_plus_patch_finder)
