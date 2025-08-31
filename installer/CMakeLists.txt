cmake_minimum_required(VERSION 3.30)
project(fomod_plus_installer LANGUAGES CXX)

file(GLOB_RECURSE SRCS "*.cpp" "*.h")
add_library(fomod_plus_installer SHARED ${SRCS})
set_property(TARGET fomod_plus_installer PROPERTY CXX_STANDARD 23)

# 2) Global mapping BEFORE find_package so it applies to imported targets
#set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL RelWithDebInfo)
#set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE   RelWithDebInfo)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)
find_package(Qt6 CONFIG REQUIRED COMPONENTS Widgets)

target_link_libraries(fomod_plus_installer
        mo2::uibase
        pugixml::pugixml
        nlohmann_json::nlohmann_json
        Qt6::Widgets
        dbghelp
        fomod_plus_shared
)
install(TARGETS fomod_plus_installer RUNTIME DESTINATION "D:/MO2-test-branch/install/bin/plugins")
