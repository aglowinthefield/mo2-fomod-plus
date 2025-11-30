#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "mo2::archive" for configuration "Debug"
set_property(TARGET mo2::archive APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(mo2::archive PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/archive.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG "7zip::7zip"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/archive.dll"
  )

list(APPEND _cmake_import_check_targets mo2::archive )
list(APPEND _cmake_import_check_files_for_mo2::archive "${_IMPORT_PREFIX}/debug/lib/archive.lib" "${_IMPORT_PREFIX}/debug/bin/archive.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
