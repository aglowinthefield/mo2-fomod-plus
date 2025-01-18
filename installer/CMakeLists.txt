cmake_minimum_required(VERSION 3.30)
include(FetchContent)
add_library(fomod_plus_installer SHARED)
set_property(TARGET fomod_plus_installer PROPERTY CXX_STANDARD 20)

set(CMAKE_GENERATOR_PLATFORM x64)
target_include_directories(fomod_plus_installer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

FetchContent_Declare(pugixml GIT_REPOSITORY https://github.com/zeux/pugixml GIT_TAG v1.14)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(pugixml json)

set(project_type plugin)
project(fomod_plus_installer)

if (MSVC)
    target_compile_options(
            fomod_plus_installer
            PRIVATE
            /bigobj # increase number of sections in .obj file
            /W4     # level 4 warnings
            /WX     # treat all compiler warnings as errors
            /wd4201 # nonstandard extension used: nameless struct/union
            /wd4458 # declaration of 'identifier' hides class member
    )
endif ()

target_link_libraries(fomod_plus_installer PRIVATE pugixml nlohmann_json::nlohmann_json)
mo2_configure_plugin(fomod_plus_installer WARNINGS OFF)
mo2_install_target(fomod_plus_installer)
