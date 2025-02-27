cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
include(FetchContent)

# Override the minimum required version for googletest
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_MINIMUM_REQUIRED_VERSION 3.30)

set(CMAKE_PREFIX_PATH "C:/Qt/6.7.3/msvc2019_64")

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

add_executable(runTests
        ${CMAKE_CURRENT_SOURCE_DIR}/../installer/xml/FomodInfoFile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_fomod_info_file.cpp
        ../installer/lib/Logger.h
        ../installer/xml/ModuleConfiguration.cpp
        ../share/stringutil.h
        moduleconf/test_moduleconf_bos.cpp
        moduleconf/test_moduleconf_citytrees.cpp
        moduleconf/test_moduleconf_embers.cpp
        moduleconf/test_moduleconf_lux.cpp
        moduleconf/test_moduleconf_miniembers.cpp
        moduleconf/test_moduleconf_precision.cpp
        moduleconf/test_moduleconf_xavbio.cpp
        test_stringutil.cpp
)

target_include_directories(runTests PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../share)


target_link_libraries(runTests gtest gtest_main pugixml Qt6::Core Qt6::Gui)
add_test(NAME runTests COMMAND runTests)
