#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "CppUTest::CppUTest" for configuration "RelWithDebInfo"
set_property(TARGET CppUTest::CppUTest APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(CppUTest::CppUTest PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/CppUTest.lib"
  )

list(APPEND _cmake_import_check_targets CppUTest::CppUTest )
list(APPEND _cmake_import_check_files_for_CppUTest::CppUTest "${_IMPORT_PREFIX}/lib/CppUTest.lib" )

# Import target "CppUTest::CppUTestExt" for configuration "RelWithDebInfo"
set_property(TARGET CppUTest::CppUTestExt APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(CppUTest::CppUTestExt PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/CppUTestExt.lib"
  )

list(APPEND _cmake_import_check_targets CppUTest::CppUTestExt )
list(APPEND _cmake_import_check_files_for_CppUTest::CppUTestExt "${_IMPORT_PREFIX}/lib/CppUTestExt.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
