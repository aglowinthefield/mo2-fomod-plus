cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/install")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "D:/Modding/MO2/plugins/fomod_plus")

# Enable debug symbols for RelWithDebInfo
if (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

if(DEFINED DEPENDENCIES_DIR)
    include(${DEPENDENCIES_DIR}/modorganizer_super/cmake_common/mo2.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/cmake_common/mo2.cmake)
endif()

project(fomod_plus)
set(project_type plugin)
set(enable_warnings OFF)

FetchContent_Declare(
        pugixml
        GIT_REPOSITORY https://github.com/zeux/pugixml
        # Maybe we want master/main. Unsure.
        GIT_TAG v1.14
)

FetchContent_MakeAvailable(pugixml)

add_subdirectory(src)
add_subdirectory(tests)
target_link_libraries(fomod_plus PRIVATE pugixml)
