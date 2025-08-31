cmake_minimum_required(VERSION 3.30)
project(fomod_plus_scanner LANGUAGES CXX)

set(SRCS
    FomodPlusScanner.cpp
    FomodPlusScanner.h
)

add_library(fomod_plus_scanner SHARED ${SRCS})
set_property(TARGET fomod_plus_scanner PROPERTY CXX_STANDARD 23)

find_package(mo2-archive CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

target_link_libraries(fomod_plus_scanner PRIVATE
        Qt6::Widgets
        mo2::uibase
        mo2::archive
        fomod_plus_shared
)
# Copy DLL to MO2 plugins directory after build
add_custom_command(TARGET fomod_plus_scanner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:fomod_plus_scanner>
        "D:/MO2-test-branch/install/bin/plugins/"
)

install(TARGETS fomod_plus_scanner RUNTIME DESTINATION "D:/MO2-test-branch/install/bin/plugins")
