cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

# Override the minimum required version for googletest
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_MINIMUM_REQUIRED_VERSION 3.30)


FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.11.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(runTests
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_fomod_info_file.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/xml/FomodInfoFile.cpp
)

target_link_libraries(runTests gtest gtest_main pugixml)
add_test(NAME runTests COMMAND runTests)