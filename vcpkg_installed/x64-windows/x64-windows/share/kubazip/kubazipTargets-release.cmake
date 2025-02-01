#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "kubazip::kubazip" for configuration "Release"
set_property(TARGET kubazip::kubazip APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(kubazip::kubazip PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/kubazip.lib"
  )

list(APPEND _cmake_import_check_targets kubazip::kubazip )
list(APPEND _cmake_import_check_files_for_kubazip::kubazip "${_IMPORT_PREFIX}/lib/kubazip.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
