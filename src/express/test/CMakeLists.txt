include_directories(..)

add_executable(check-hash "hash_test.c;../hash.c;../../auxlib/bstrlib.c")
set_property(TARGET check-hash PROPERTY COMPILE_DEFINITIONS "HASH_TESTING;HASH_DEBUG")
target_link_libraries(check-hash "m")

add_test(NAME test_hash_impl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND $<TARGET_FILE:check-hash> "${CMAKE_CURRENT_SOURCE_DIR}/hash_names.txt"
  )

add_executable(check-scanner "scanner_test.c;${RE2C_ExpScanner_OUTPUTS};../lexsupport.c;../hash.c;../../auxlib/bstrlib.c")
set_property(TARGET check-scanner APPEND PROPERTY INCLUDE_DIRECTORIES "${RE2C_ExpScanner_INCLUDE_DIR}")
set_property(TARGET check-scanner APPEND PROPERTY INCLUDE_DIRECTORIES "${LEMON_ExpParser_INCLUDE_DIR}")
set_property(TARGET check-scanner PROPERTY COMPILE_DEFINITIONS "HASH_TESTING")

add_test(NAME test_scanner
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND $<TARGET_FILE:check-scanner> "${CMAKE_CURRENT_SOURCE_DIR}/../../../test/unitary_schemas/ur_redecl.exp"
  )
  
add_test(NAME test_plib_parse_err
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMAND $<TARGET_FILE:check-express> "${CMAKE_CURRENT_SOURCE_DIR}/plib_parse_err.exp"
  )

set_tests_properties( test_plib_parse_err PROPERTIES DEPENDS "test_hash_impl" )
set_tests_properties( test_plib_parse_err test_plib_parse_err PROPERTIES LABELS parser )

sc_addexec(print_schemas "../fedex.c;print_schemas.c" "express;base")
sc_addexec(print_attrs "../fedex.c;print_attrs.c" "express;base")

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

