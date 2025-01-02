cmake_minimum_required(VERSION 3.30)
add_library(fomod_plus SHARED)
set_property(TARGET fomod_plus PROPERTY CXX_STANDARD 20)

target_include_directories(
        fomod_plus
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
#        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include
)

if(MSVC)
    target_compile_options(
            fomod_plus
            PRIVATE
            /bigobj # increase number of sections in .obj file
            /W4     # level 4 warnings
            /WX     # treat all compiler warnings as errors
            /wd4201 # nonstandard extension used: nameless struct/union
            /wd4458 # declaration of 'identifier' hides class member
    )
endif()

mo2_configure_plugin(
        fomod_plus
        WARNINGS OFF
#        PRIVATE_DEPENDS
#        bsatk lootcli uibase
#        boost fmt zlib
#        Qt::WebEngineWidgets
)
mo2_install_target(fomod_plus)
