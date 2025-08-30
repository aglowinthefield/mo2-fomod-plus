cmake_minimum_required(VERSION 3.30)
include(FetchContent)
add_library(fomod_plus_installer SHARED)
set_property(TARGET fomod_plus_installer PROPERTY CXX_STANDARD 20)

target_include_directories(
        fomod_plus_installer
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../share
)

FetchContent_Declare(pugixml GIT_REPOSITORY https://github.com/zeux/pugixml GIT_TAG v1.14)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(pugixml json)

set(project_type plugin)
project(fomod_plus_installer)

target_link_libraries(fomod_plus_installer PRIVATE pugixml nlohmann_json::nlohmann_json dbghelp)
mo2_configure_plugin(fomod_plus_installer WARNINGS OFF PRIVATE_DEPENDS)
mo2_install_target(fomod_plus_installer)
