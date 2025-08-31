cmake_minimum_required(VERSION 3.30)
project(fomod_plus_installer LANGUAGES CXX)

file(GLOB_RECURSE SRCS "*.cpp" "*.h")
add_library(fomod_plus_installer ${SRCS})
set_property(TARGET fomod_plus_installer PROPERTY CXX_STANDARD 23)

target_include_directories(
    fomod_plus_installer
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../share
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

target_link_libraries(fomod_plus_installer PUBLIC
        pugixml::shared
        pugixml::pugixml
        nlohmann_json::nlohmann_json
        Qt6::Widgets
        dbghelp
        mo2-uibase
)
install(TARGETS fomod_plus_installer RUNTIME DESTINATION "D:/MO2-test-branch/install/bin/plugins")
