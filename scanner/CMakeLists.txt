cmake_minimum_required(VERSION 3.30)
project(fomod_plus_scanner LANGUAGES CXX)

file(GLOB_RECURSE SRCS "*.cpp" "*.h")
add_library(fomod_plus_scanner ${SRCS})
set_property(TARGET fomod_plus_scanner PROPERTY CXX_STANDARD 23)

target_include_directories(
    fomod_plus_scanner
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../share
)

find_package(mo2-archive CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

target_link_libraries(fomod_plus_scanner PUBLIC
        Qt6::Widgets
        mo2-archive
        mo2-uibase
)
install(TARGETS fomod_plus_scanner RUNTIME DESTINATION "D:/MO2-test-branch/install/bin/plugins")
