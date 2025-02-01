#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "volk::volk" for configuration "Debug"
set_property(TARGET volk::volk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(volk::volk PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/volk.lib"
  )

list(APPEND _cmake_import_check_targets volk::volk )
list(APPEND _cmake_import_check_files_for_volk::volk "${_IMPORT_PREFIX}/debug/lib/volk.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
