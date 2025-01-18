cmake_minimum_required(VERSION 3.30)
project(fomod_plus_scanner)

add_library(fomod_plus_scanner SHARED)
include(FetchContent)
set(project_type plugin)
set_property(TARGET fomod_plus_scanner PROPERTY CXX_STANDARD 20)

set(CMAKE_GENERATOR_PLATFORM x64)
target_include_directories(fomod_plus_scanner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
    target_compile_options(
            fomod_plus_scanner
            PRIVATE
            /bigobj # increase number of sections in .obj file
            /W4     # level 4 warnings
            /WX     # treat all compiler warnings as errors
            /wd4201 # nonstandard extension used: nameless struct/union
            /wd4458 # declaration of 'identifier' hides class member
    )
endif ()
mo2_configure_plugin(fomod_plus_scanner WARNINGS OFF PRIVATE_DEPENDS archive)
mo2_install_target(fomod_plus_scanner)
