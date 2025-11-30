#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "7zip::7zip" for configuration "Release"
set_property(TARGET 7zip::7zip APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(7zip::7zip PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/7zip.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/7zip.dll"
  )

list(APPEND _cmake_import_check_targets 7zip::7zip )
list(APPEND _cmake_import_check_files_for_7zip::7zip "${_IMPORT_PREFIX}/lib/7zip.lib" "${_IMPORT_PREFIX}/bin/7zip.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
