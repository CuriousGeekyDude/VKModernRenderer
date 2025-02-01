#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "poly2tri::poly2tri" for configuration "Release"
set_property(TARGET poly2tri::poly2tri APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(poly2tri::poly2tri PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/poly2tri.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/poly2tri.dll"
  )

list(APPEND _cmake_import_check_targets poly2tri::poly2tri )
list(APPEND _cmake_import_check_files_for_poly2tri::poly2tri "${_IMPORT_PREFIX}/lib/poly2tri.lib" "${_IMPORT_PREFIX}/bin/poly2tri.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
