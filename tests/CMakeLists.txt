cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)
include(FetchContent)

# Override the minimum required version for googletest
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_MINIMUM_REQUIRED_VERSION 3.30)

# Use dynamic CRT (/MDd, /MD) to match Qt6 — must be set before FetchContent.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Use QT_ROOT_DIR env var (set by jurplel/install-qt-action in CI), fall back to local path.
if (DEFINED ENV{QT_ROOT_DIR})
    set(CMAKE_PREFIX_PATH "$ENV{QT_ROOT_DIR}")
else ()
    set(CMAKE_PREFIX_PATH "C:/Qt/6.7.3/msvc2022_64")
endif ()
add_definitions(-DTEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fomoddata")

FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.16.0)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_Declare(pugixml GIT_REPOSITORY https://github.com/zeux/pugixml GIT_TAG v1.14)
FetchContent_MakeAvailable(googletest json pugixml)

enable_testing()

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

file(GLOB SHARE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../share/**/*.cpp")
file(GLOB INSTALLER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../installer/lib/Logger.cpp")
file(GLOB TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/fomoddata/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/moduleconf/*.cpp"
)

add_executable(runTests ${SHARE_SOURCES} ${TEST_SOURCES} ${INSTALLER_SOURCES})
target_sources(runTests PRIVATE ${TEST_SOURCES} ${SHARE_SOURCES} ${INSTALLER_SOURCES})
target_include_directories(runTests PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../share)

target_link_libraries(runTests gtest gtest_main nlohmann_json::nlohmann_json pugixml Qt6::Core Qt6::Gui)
add_test(NAME runTests COMMAND runTests)
