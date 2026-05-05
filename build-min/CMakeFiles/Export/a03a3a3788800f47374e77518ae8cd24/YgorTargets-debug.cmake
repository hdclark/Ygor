#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Ygor::ygor" for configuration "Debug"
set_property(TARGET Ygor::ygor APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(Ygor::ygor PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libygor.so"
  IMPORTED_SONAME_DEBUG "libygor.so"
  )

list(APPEND _cmake_import_check_targets Ygor::ygor )
list(APPEND _cmake_import_check_files_for_Ygor::ygor "${_IMPORT_PREFIX}/lib/libygor.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
