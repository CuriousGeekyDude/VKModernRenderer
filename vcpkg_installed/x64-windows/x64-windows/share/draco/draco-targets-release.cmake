#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "draco::draco" for configuration "Release"
set_property(TARGET draco::draco APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(draco::draco PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/draco.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/draco.dll"
  )

list(APPEND _cmake_import_check_targets draco::draco )
list(APPEND _cmake_import_check_files_for_draco::draco "${_IMPORT_PREFIX}/lib/draco.lib" "${_IMPORT_PREFIX}/bin/draco.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
