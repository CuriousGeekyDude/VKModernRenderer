#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::minizip::minizip" for configuration "Debug"
set_property(TARGET unofficial::minizip::minizip APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::minizip::minizip PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/minizip.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/minizip.dll"
  )

list(APPEND _cmake_import_check_targets unofficial::minizip::minizip )
list(APPEND _cmake_import_check_files_for_unofficial::minizip::minizip "${_IMPORT_PREFIX}/debug/lib/minizip.lib" "${_IMPORT_PREFIX}/debug/bin/minizip.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
