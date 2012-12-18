function(get_gcc_flags target_name)
  # CMake does not provide an easy way to get all compiler switches,
  # so this function is a fishing expedition to get them.
  # http://public.kitware.com/Bug/view.php?id=1260
  set(compile_args ${CMAKE_CXX_FLAGS})
  string(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype_name)
  if(CMAKE_CXX_FLAGS_${buildtype_name})
      list(APPEND compile_args ${CMAKE_CXX_FLAGS_${buildtype_name}})
  endif()
  get_directory_property(dir_inc INCLUDE_DIRECTORIES)
  foreach(item ${dir_inc})
    LIST(APPEND compile_args "-I" ${item})
  endforeach()
  get_directory_property(dir_defs COMPILE_DEFINITIONS)
  foreach(item ${dir_defs})
      list(APPEND compile_args -D${item})
  endforeach()
  get_directory_property(buildtype_defs COMPILE_DEFINITIONS_${buildtype_name})
  foreach(item ${buildtype_defs})
     list(APPEND compile_args -D${item})
  endforeach()
  get_target_property(target_type ${target_name} TYPE)
  if(${target_type} STREQUAL SHARED_LIBRARY)
    list(APPEND compile_args ${CMAKE_CXX_COMPILE_OPTIONS_PIC})
  endif()
  set(compile_args ${compile_args} PARENT_SCOPE)
  #message(STATUS ${compile_args})
endfunction()

function(add_pch_linux header_filename target_name pch_suffix)
  set(gch_target_name "${target_name}_pch")
  get_filename_component(header_basename ${header_filename} NAME)
  set(gch_filename "${CMAKE_CURRENT_BINARY_DIR}/${header_basename}.${pch_suffix}")
  get_gcc_flags(${target_name}) # Sets compile_args in this scope. It's even better than Intercal's COME FROM!
  #message(STATUS ${compile_args})
  list(APPEND compile_args -c ${CMAKE_CURRENT_SOURCE_DIR}/${header_filename} -o ${gch_filename})
  separate_arguments(compile_args)
  add_custom_command(OUTPUT ${gch_filename}
    COMMAND ${CMAKE_CXX_COMPILER} ${compile_args}
    DEPENDS ${header_filename})
  add_custom_target(${gch_target_name} DEPENDS ${gch_filename})
  add_dependencies(${target_name} ${gch_target_name})
  
  # Add the PCH to every source file's include list.
  # This is the only way that is supported by both GCC and Clang.
  set_property(TARGET ${target_name} APPEND_STRING PROPERTY COMPILE_FLAGS "-include ${header_basename}")
  
  # Each directory should have only one precompiled header
  # for simplicity. If there are several, the current dir
  # gets added to the search path several times.
  # It should not be an issue, though.
  include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

try_run(IS_CLANG did_build ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/cmake/isclang.cc)

if(UNIX)
  if(NOT APPLE)
    option(use_pch "Use precompiled headers." TRUE)
  endif()
endif()

if(use_pch)
  message(STATUS "Using precompiled headers.")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winvalid-pch")
  if(IS_CLANG)
    set(precompiled_header_extension pch)
  else()
    set(precompiled_header_extension gch)
  endif()
  macro(add_pch _header_filename _target_name)
      add_pch_linux(${_header_filename} ${_target_name} ${precompiled_header_extension})
  endmacro()
else()
  message(STATUS "Not using precompiled headers.")
  macro(add_pch _header_filename _target_name)
  endmacro()
endif()
