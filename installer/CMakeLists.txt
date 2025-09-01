cmake_minimum_required(VERSION 3.30)
include(FetchContent)
project(fomod_plus_installer LANGUAGES CXX)

file(GLOB_RECURSE SRCS "*.cpp" "*.h")
add_library(fomod_plus_installer SHARED ${SRCS})
set_property(TARGET fomod_plus_installer PROPERTY CXX_STANDARD 23)

FetchContent_Declare(pugixml GIT_REPOSITORY https://github.com/zeux/pugixml GIT_TAG v1.14)
FetchContent_MakeAvailable(pugixml)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

target_link_libraries(fomod_plus_installer
        mo2::uibase
        pugixml
        nlohmann_json::nlohmann_json
        Qt6::Widgets
        dbghelp
        fomod_plus_shared
)
install(TARGETS fomod_plus_installer RUNTIME DESTINATION "D:/MO2-test-branch/install/bin/plugins")
