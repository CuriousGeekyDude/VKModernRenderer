#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "glslang::OSDependent" for configuration "Debug"
set_property(TARGET glslang::OSDependent APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glslang::OSDependent PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/OSDependentd.lib"
  )

list(APPEND _cmake_import_check_targets glslang::OSDependent )
list(APPEND _cmake_import_check_files_for_glslang::OSDependent "${_IMPORT_PREFIX}/debug/lib/OSDependentd.lib" )

# Import target "glslang::glslang" for configuration "Debug"
set_property(TARGET glslang::glslang APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glslang::glslang PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/glslangd.lib"
  )

list(APPEND _cmake_import_check_targets glslang::glslang )
list(APPEND _cmake_import_check_files_for_glslang::glslang "${_IMPORT_PREFIX}/debug/lib/glslangd.lib" )

# Import target "glslang::MachineIndependent" for configuration "Debug"
set_property(TARGET glslang::MachineIndependent APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glslang::MachineIndependent PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/MachineIndependentd.lib"
  )

list(APPEND _cmake_import_check_targets glslang::MachineIndependent )
list(APPEND _cmake_import_check_files_for_glslang::MachineIndependent "${_IMPORT_PREFIX}/debug/lib/MachineIndependentd.lib" )

# Import target "glslang::GenericCodeGen" for configuration "Debug"
set_property(TARGET glslang::GenericCodeGen APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glslang::GenericCodeGen PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/GenericCodeGend.lib"
  )

list(APPEND _cmake_import_check_targets glslang::GenericCodeGen )
list(APPEND _cmake_import_check_files_for_glslang::GenericCodeGen "${_IMPORT_PREFIX}/debug/lib/GenericCodeGend.lib" )

# Import target "glslang::glslang-default-resource-limits" for configuration "Debug"
set_property(TARGET glslang::glslang-default-resource-limits APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glslang::glslang-default-resource-limits PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/glslang-default-resource-limitsd.lib"
  )

list(APPEND _cmake_import_check_targets glslang::glslang-default-resource-limits )
list(APPEND _cmake_import_check_files_for_glslang::glslang-default-resource-limits "${_IMPORT_PREFIX}/debug/lib/glslang-default-resource-limitsd.lib" )

# Import target "glslang::SPVRemapper" for configuration "Debug"
set_property(TARGET glslang::SPVRemapper APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glslang::SPVRemapper PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/SPVRemapperd.lib"
  )

list(APPEND _cmake_import_check_targets glslang::SPVRemapper )
list(APPEND _cmake_import_check_files_for_glslang::SPVRemapper "${_IMPORT_PREFIX}/debug/lib/SPVRemapperd.lib" )

# Import target "glslang::SPIRV" for configuration "Debug"
set_property(TARGET glslang::SPIRV APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glslang::SPIRV PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/SPIRVd.lib"
  )

list(APPEND _cmake_import_check_targets glslang::SPIRV )
list(APPEND _cmake_import_check_files_for_glslang::SPIRV "${_IMPORT_PREFIX}/debug/lib/SPIRVd.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
