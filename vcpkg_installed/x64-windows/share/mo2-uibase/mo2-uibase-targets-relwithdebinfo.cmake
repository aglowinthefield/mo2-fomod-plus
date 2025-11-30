#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mo2::uibase" for configuration "RelWithDebInfo"
set_property(TARGET mo2::uibase APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(mo2::uibase PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/uibase.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELWITHDEBINFO "Qt6::Qml;Qt6::Quick"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/uibase.dll"
  )

list(APPEND _cmake_import_check_targets mo2::uibase )
list(APPEND _cmake_import_check_files_for_mo2::uibase "${_IMPORT_PREFIX}/lib/uibase.lib" "${_IMPORT_PREFIX}/bin/uibase.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
