include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${SC_SOURCE_DIR}/src/base
  )

# Set up the information we need to feed the generated source management
# scripts
set(BASELINE_INFORMATION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/generated/verification_info.cmake")
set(PROJECT_CMAKE_DIR "${SC_SOURCE_DIR}/cmake")
set(MD5_FILELIST
  "${CMAKE_CURRENT_SOURCE_DIR}/expscan.l"
  "${CMAKE_CURRENT_SOURCE_DIR}/expparse.y"
  "${CMAKE_CURRENT_SOURCE_DIR}/generated/expscan.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/generated/expscan.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/generated/expparse.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/generated/expparse.h"
  )
configure_file(${SC_SOURCE_DIR}/cmake/md5_gen.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/express_md5gen.cmake @ONLY)
configure_file(${SC_SOURCE_DIR}/cmake/md5_verify.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/express_verify.cmake @ONLY)

# Convenience target to generate an updated verification_info.cmake file
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/express_md5gen.sentinel
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/express_md5gen.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/express_md5gen.sentinel
  )
add_custom_target(express_md5gen DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/express_md5gen.sentinel)

# Target for actually checking cached MD5 sums against files
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/express_verify.sentinel
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/express_verify.cmake
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/express_verify.sentinel
  DEPENDS ${MD5_FILELIST}
  )
add_custom_target(express_verify DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/express_verify.sentinel)


# Depending on whether we're using pre-generated sources or building them on
# the fly, set up targets and source lists.
if(SC_GENERATE_LP_SOURCES)
  LEMON_TARGET(ExpParser expparse.y expparse.c
               COMPILE_FLAGS "-p -c"
               DEFINES_FILE "expparse.h")
  RE2C_TARGET(ExpScanner expscan.l expscan.c
              COMPILE_FLAGS "-W -Wno-match-empty-string -r -c --tags"
              DEFINES_FILE "expscan.h")
  ADD_RE2C_LEMON_DEPENDENCY(ExpScanner ExpParser)
  set(SCL_SO_SRCS ${LEMON_ExpParser_OUTPUTS} ${RE2C_ExpScanner_OUTPUTS})
  set(SCL_STATIC_SRCS ${LEMON_ExpParser_OUTPUTS} ${RE2C_ExpScanner_OUTPUTS})
else(SC_GENERATE_LP_SOURCES)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/generated)
  set(EXPRESS_GENERATED_SRCS
    generated/expparse.c
    generated/expscan.c
    )
  set(SCL_SO_SRCS ${EXPRESS_GENERATED_SRCS})
  set(SCL_STATIC_SRCS ${EXPRESS_GENERATED_SRCS})
endif(SC_GENERATE_LP_SOURCES)

set(EXPRESS_SOURCES
  symbol.c
  type.c
  variable.c
  expr.c
  entity.c
  caseitem.c
  stmt.c
  alg.c
  scope.c
  schema.c
  resolve.c
  lexact.c
  linklist.c
  error.c
  dict.c
  hash.c
  memory.c
  object.c
  express.c
  ordered_attrs.cc
  info.c
  exp_kw.c
  lexsupport.c
 )

set(EXPRESS_OBJS)
foreach(_src ${EXPRESS_SOURCES})
    string(REPLACE "." "_" _suffix ${_src})
    set(_objlib "objlib_${_suffix}")
    add_library(${_objlib} OBJECT ${_src})
    list(APPEND ${EXPRESS_OBJS} $<TARGET_OBJECTS:${_objlib}>)
endforeach()

foreach(_src ${SCL_SO_SRCS})
    string(REPLACE "." "_" _suffix ${_src})
    set(_objlib "objlib_${_suffix}")
    add_library(${_objlib} OBJECT ${_src})
    list(APPEND ${EXPRESS_OBJS} $<TARGET_OBJECTS:${_objlib}>)
endforeach()

# TODO
# Currently, fedex.c provides the main() for multiple programs. These programs
# provide custom behavior by defining EXPRESSinit_init (called by fedex.c's
# main) which sets other callbacks.
#
# What would be better is to remove the call to EXPRESSinit_init and turn
# fedex.c's main() into an express library function. That way, programs only
# need to link against express (not fedex.o too) and they can do initialization
# in their own main (instead of in a local EXPRESSinit_init definition).
set(CHECK_EXPRESS_SOURCES
  fedex.c
  inithook.c
 )

SET(EXPRESS_PRIVATE_HDRS
  exptoks.h
  stack.h
  lexsupport.h
  )
  
add_library(express SHARED error.c ${EXPRESS_OBJS})
target_link_libraries(express base auxlib)

if(SC_GENERATE_LP_SOURCES)
  set_property(TARGET express
               APPEND PROPERTY INCLUDE_DIRECTORIES "${RE2C_ExpScanner_INCLUDE_DIR}")
  set_property(TARGET express
               APPEND PROPERTY INCLUDE_DIRECTORIES "${LEMON_ExpParser_INCLUDE_DIR}")
endif(SC_GENERATE_LP_SOURCES)

if(SC_GENERATE_LP_SOURCES)
  add_custom_command(TARGET express POST_BUILD
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/express_verify.cmake
    )
else(SC_GENERATE_LP_SOURCES)
  add_dependencies(express express_verify)
endif(SC_GENERATE_LP_SOURCES)

if(NOT SC_IS_SUBBUILD AND SC_GIT_VERSION)
  add_dependencies(express version_string)
endif(NOT SC_IS_SUBBUILD AND SC_GIT_VERSION)
SC_ADDEXEC("check-express" "${CHECK_EXPRESS_SOURCES}" "express;base"  ${SC_EXEC_NOINSTALL})

if(SC_ENABLE_TESTING)
  add_subdirectory(test)
endif(SC_ENABLE_TESTING)

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8
