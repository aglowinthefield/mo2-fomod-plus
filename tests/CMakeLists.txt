cmake_minimum_required(VERSION 3.30)
# Also allow isolated validation without configuring any MO2 plugin or deployment path.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(fomod_plus_tests LANGUAGES CXX)
endif ()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
enable_testing()

set(gtest_force_shared_crt ON CACHE BOOL "Use the same CRT as the test executable" FORCE)
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.16.0)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_Declare(pugixml GIT_REPOSITORY https://github.com/zeux/pugixml GIT_TAG v1.14)
FetchContent_MakeAvailable(googletest json pugixml)
find_package(Qt6 COMPONENTS Core Gui REQUIRED)

add_executable(runTests
    test_fomod_info_file.cpp
    test_stringutil.cpp
    fomoddata/test_conditionevaluator.cpp
    fomoddata/test_fomoddb.cpp
    fomoddata/test_fomoddbentry.cpp
    fomoddata/test_pluginreader.cpp
    moduleconf/test_moduleconf_bos.cpp
    moduleconf/test_moduleconf_citytrees.cpp
    moduleconf/test_moduleconf_embers.cpp
    moduleconf/test_moduleconf_lux.cpp
    moduleconf/test_moduleconf_miniembers.cpp
    moduleconf/test_moduleconf_precision.cpp
    moduleconf/test_moduleconf_xavbio.cpp
    patchfinder/test_patchfinder.cpp
    ../patchfinder/lib/PatchFinder.cpp
    ../share/xml/FomodInfoFile.cpp
    ../share/xml/ModuleConfiguration.cpp
)
target_compile_definitions(runTests PRIVATE TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fomoddata")
target_include_directories(runTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../share")
target_link_libraries(runTests PRIVATE gtest_main nlohmann_json::nlohmann_json pugixml Qt6::Core Qt6::Gui)
if (MSVC)
    target_compile_options(runTests PRIVATE /utf-8 /Zc:__cplusplus /permissive- /bigobj)
endif ()
add_test(NAME runTests COMMAND runTests)
