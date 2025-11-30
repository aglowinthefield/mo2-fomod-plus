
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

####################################################################################

set(_UIBASE_PREFIX_DIR ${PACKAGE_PREFIX_DIR})

find_package(Qt6 CONFIG REQUIRED COMPONENTS Network QuickWidgets Widgets)

include ( "${CMAKE_CURRENT_LIST_DIR}/mo2-uibase-targets.cmake" )


if (MO2_CMAKE_DEPRECATED_UIBASE_INCLUDE)
    target_include_directories(mo2::uibase INTERFACE
        ${_UIBASE_PREFIX_DIR}/include/uibase ${_UIBASE_PREFIX_DIR}/include/uibase/game_features)
endif()
