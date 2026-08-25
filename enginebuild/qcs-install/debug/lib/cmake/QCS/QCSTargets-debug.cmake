#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "QCS::QCS" for configuration "Debug"
set_property(TARGET QCS::QCS APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(QCS::QCS PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/quantum_engine.lib"
  )

list(APPEND _cmake_import_check_targets QCS::QCS )
list(APPEND _cmake_import_check_files_for_QCS::QCS "${_IMPORT_PREFIX}/lib/quantum_engine.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
