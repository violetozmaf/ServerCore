#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "redis++::redis++_static" for configuration "Debug"
set_property(TARGET redis++::redis++_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(redis++::redis++_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/redis++_static.lib"
  )

list(APPEND _cmake_import_check_targets redis++::redis++_static )
list(APPEND _cmake_import_check_files_for_redis++::redis++_static "${_IMPORT_PREFIX}/lib/redis++_static.lib" )

# Import target "redis++::redis++" for configuration "Debug"
set_property(TARGET redis++::redis++ APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(redis++::redis++ PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/redis++.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/redis++.dll"
  )

list(APPEND _cmake_import_check_targets redis++::redis++ )
list(APPEND _cmake_import_check_files_for_redis++::redis++ "${_IMPORT_PREFIX}/lib/redis++.lib" "${_IMPORT_PREFIX}/bin/redis++.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
