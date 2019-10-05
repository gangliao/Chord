# Gang Liao, gangliao@cs.umd.edu
#
# generic.cmake defines CMakes functions that look like Bazel's
# building rules (https://bazel.build/).
#
#
# -------------------------------------------
#     C++
# -------------------------------------------
# cc_library
# cc_testing
# -------------------------------------------
#
# To build a static library example.a from example.cc using the system
#  compiler (like GCC):
#
#   cc_library(example SRCS example.cc)
#
# To build a static library example.a from multiple source files
# example{1,2,3}.cc:
#
#   cc_library(example SRCS example1.cc example2.cc example3.cc)
#
# To build a shared library example.so from example.cc:
#
#   cc_library(example SHARED SRCS example.cc)
#
# To specify that a library new_example.a depends on other libraies:
#
#   cc_library(new_example SRCS new_example.cc DEPS example)
#
# Static libraries can be composed of other static libraries:
#
#   cc_library(composed DEPS dependent1 dependent2 dependent3)
#
# To build a unit test binary, which is an executable binary with
# GoogleTest linked:
#
#   cc_testing(example_test SRCS example_test.cc DEPS example)
#
# It is pretty often that executable and test binaries depend on
# pre-defined external libaries like glog and gflags defined in
# /cmake/external/*.cmake:
#
#   cc_testing(example_test SRCS example_test.cc DEPS example glog gflags)
#

# including binary directory for generated headers.
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(NOT APPLE AND NOT ANDROID)
    find_package(Threads REQUIRED)
    link_libraries(${CMAKE_THREAD_LIBS_INIT})
    set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl -lrt")
endif(NOT APPLE AND NOT ANDROID)

function(merge_static_libs TARGET_NAME)
  set(libs ${ARGN})
  list(REMOVE_DUPLICATES libs)

  # Get all propagation dependencies from the merged libraries
  foreach(lib ${libs})
    list(APPEND libs_deps ${${lib}_LIB_DEPENDS})
  endforeach()
  list(REMOVE_DUPLICATES libs_deps)

  if(APPLE) # Use OSX's libtool to merge archives
    # To produce a library we need at least one source file.
    # It is created by add_custom_command below and will helps
    # also help to track dependencies.
    set(dummyfile ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_dummy.c)

    # Make the generated dummy source file depended on all static input
    # libs. If input lib changes,the source file is touched
    # which causes the desired effect (relink).
    add_custom_command(OUTPUT ${dummyfile}
      COMMAND ${CMAKE_COMMAND} -E touch ${dummyfile}
      DEPENDS ${libs})

    # Generate dummy staic lib
    file(WRITE ${dummyfile} "const char * dummy = \"${dummyfile}\";")
    add_library(${TARGET_NAME} STATIC ${dummyfile})
    target_link_libraries(${TARGET_NAME} ${libs_deps})

    foreach(lib ${libs})
      # Get the file names of the libraries to be merged
      set(libfiles ${libfiles} $<TARGET_FILE:${lib}>)
    endforeach()
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND rm "${CMAKE_CURRENT_BINARY_DIR}/lib${TARGET_NAME}.a"
      COMMAND /usr/bin/libtool -static -o "${CMAKE_CURRENT_BINARY_DIR}/lib${TARGET_NAME}.a" ${libfiles})
  else() # general UNIX: use "ar" to extract objects and re-add to a common lib
    foreach(lib ${libs})
      set(objlistfile ${lib}.objlist) # list of objects in the input library
      set(objdir ${lib}.objdir)

      add_custom_command(OUTPUT ${objdir}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${objdir}
        DEPENDS ${lib})

      add_custom_command(OUTPUT ${objlistfile}
        COMMAND ${CMAKE_AR} -x "$<TARGET_FILE:${lib}>"
        COMMAND ${CMAKE_AR} -t "$<TARGET_FILE:${lib}>" > ../${objlistfile}
        DEPENDS ${lib} ${objdir}
        WORKING_DIRECTORY ${objdir})

      # Empty dummy source file that goes into merged library		
      set(mergebase ${lib}.mergebase.c)		
      add_custom_command(OUTPUT ${mergebase}		
        COMMAND ${CMAKE_COMMAND} -E touch ${mergebase}		
        DEPENDS ${objlistfile})		

      list(APPEND mergebases "${mergebase}")
    endforeach()

    add_library(${TARGET_NAME} STATIC ${mergebases})
    target_link_libraries(${TARGET_NAME} ${libs_deps})

    # Get the file name of the generated library
    set(outlibfile "$<TARGET_FILE:${TARGET_NAME}>")

    foreach(lib ${libs})
      add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_AR} cr ${outlibfile} *.o
        COMMAND ${CMAKE_RANLIB} ${outlibfile}
        WORKING_DIRECTORY ${lib}.objdir)
    endforeach()
  endif()
endfunction(merge_static_libs)

macro(_build_target func_tag)
  set(_sources ${ARGN})

  # Given a variable containing a file list,
  # it will remove all the files wich basename
  # does not match the specified pattern.
  if(${CMAKE_VERSION} VERSION_LESS 3.6)
    foreach(src_file ${_sources})
      get_filename_component(base_name ${src_file} NAME)
      if(${base_name} MATCHES "\\.proto$")
        list(REMOVE_ITEM _sources "${src_file}")
      endif()
    endforeach()
  else(${CMAKE_VERSION} VERSION_LESS 3.6)
    list(FILTER _sources EXCLUDE REGEX "\\.proto$")
  endif(${CMAKE_VERSION} VERSION_LESS 3.6)

  if (${func_tag} STREQUAL "cc_lib")
    add_library(${_sources})
  elseif(${func_tag} STREQUAL "cc_bin")
    list(REMOVE_ITEM _sources STATIC SHARED)
    add_executable(${_sources})
  elseif(${func_tag} STREQUAL "nv_lib")
    cuda_add_library(${_sources})
  elseif(${func_tag} STREQUAL "nv_bin")
    list(REMOVE_ITEM _sources STATIC SHARED)
    cuda_add_executable(${_sources})
  endif()
endmacro(_build_target)

function(cmake_library TARGET_NAME)
  set(options STATIC SHARED)
  set(oneValueArgs TAG)
  set(multiValueArgs SRCS DEPS)
  cmake_parse_arguments(cmake_library "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if (cmake_library_SRCS)
    if (cmake_library_SHARED) # build *.so
      set(_lib_type SHARED)
    else(cmake_library_SHARED)
      set(_lib_type STATIC)
    endif(cmake_library_SHARED)
    _build_target(${cmake_library_TAG} ${TARGET_NAME} ${_lib_type} ${cmake_library_SRCS}) 
    if (cmake_library_DEPS)
      add_dependencies(${TARGET_NAME} ${cmake_library_DEPS})
      target_link_libraries(${TARGET_NAME} ${cmake_library_DEPS})
    endif(cmake_library_DEPS)
  else(cmake_library_SRCS)
    if (cmake_library_DEPS AND ${cmake_library_TAG} STREQUAL "cc_lib")
      merge_static_libs(${TARGET_NAME} ${cmake_library_DEPS})
    else()
      message(FATAL_ERROR "Contact Gang Liao (gangliao@cs.umd.edu)")
    endif()
  endif(cmake_library_SRCS)
endfunction(cmake_library)

macro(check_gtest)
  set(options STATIC SHARED)
  set(oneValueArgs TAG)
  set(multiValueArgs SRCS DEPS)
  cmake_parse_arguments(check_gtest "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  
  set(gtest_deps DEPS gtest gtest_main)
  foreach(filename ${check_gtest_SRCS})
    file(STRINGS ${filename} output REGEX "(int|void)[ ]+main")
    if(output)
      list(REMOVE_ITEM gtest_deps gtest_main)
      break()
    endif(output)
  endforeach()

  set(${gtest_deps} ${gtest_deps} PARENT_SCOPE)
endmacro(check_gtest)

function(cc_library)
  cmake_library(${ARGV} TAG cc_lib)
endfunction(cc_library)

function(cc_binary)
  cmake_library(${ARGV} TAG cc_bin)
endfunction(cc_binary)

function(cc_testing)
  check_gtest(${ARGV})
  cmake_library(${ARGV} TAG cc_bin ${gtest_deps})
  add_test(${ARGV0} ${ARGV0})
endfunction(cc_testing)

function(_protobuf_generate_cpp SRCS HDRS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: _protobuf_generate_cpp() called without any proto files")
    return()
  endif()

  set(${SRCS})
  set(${HDRS})

  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(PROTO_CURRENT_SOURCE_DIR ${ABS_FIL} DIRECTORY)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    message(${PROTO_CURRENT_SOURCE_DIR}) 
    set(_protobuf_protoc_src "${PROTO_CURRENT_SOURCE_DIR}/${FIL_WE}.pb.cc")
    set(_protobuf_protoc_hdr "${PROTO_CURRENT_SOURCE_DIR}/${FIL_WE}.pb.h")
    message(${_protobuf_protoc_hdr})
    list(APPEND ${SRCS} "${_protobuf_protoc_src}")
    list(APPEND ${HDRS} "${_protobuf_protoc_hdr}")
    
    add_custom_command(
      OUTPUT "${_protobuf_protoc_src}"
             "${_protobuf_protoc_hdr}"

      COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} 
      -I${PROTO_CURRENT_SOURCE_DIR}
      --cpp_out "${PROTO_CURRENT_SOURCE_DIR}" ${ABS_FIL}
      DEPENDS ${ABS_FIL} protoc
      COMMENT "Running C++ protocol buffer compiler on ${FIL}"
      VERBATIM)
  endforeach()
set_source_files_properties(${${SRCS}} ${${HDRS}} PROPERTIES GENERATED TRUE)
set(${SRCS} ${${SRCS}} PARENT_SCOPE)
set(${HDRS} ${${HDRS}} PARENT_SCOPE)
message(STATUS "Generate Protobuf CPP Source Files: ${${SRCS}}")
message(STATUS "Generate Protobuf CPP Header Files: ${${HDRS}}")
endfunction()

function(proto_library)
  set(options STATIC SHARED)
  set(oneValueArgs TAG)
  set(multiValueArgs SRCS DEPS)
  cmake_parse_arguments(proto_library "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # change output to src dir
  # PROTOBUF_GENERATE_CPP(proto_srcs PROTO_HDRS ${proto_library_SRCS})
  _protobuf_generate_cpp(proto_srcs PROTO_HDRS ${proto_library_SRCS})
  # including binary directory for generated headers (protobuf hdrs).
  # include_directories(${CMAKE_CURRENT_BINARY_DIR})
  include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  cmake_library(${ARGV} SRCS ${proto_srcs} ${PROTO_HDRS} DEPS protobuf TAG cc_lib)
endfunction(proto_library)
