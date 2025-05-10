function(set_compiler_options project_name)
  set(COMPILER_FLAGS PRIVATE 
    -Wformat
    -Wformat-security
    -Werror=format-security
    -Wall
    -Wextra
    -Werror
    -Wpedantic
    -Wconversion
    -Wshadow
    -O3
  )

  if (UNIX)
    list(APPEND COMPILER_FLAGS "-fPIE" "-pie")
  endif()

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    list(APPEND COMPILER_FLAGS -march=native)
  endif()

  target_compile_options(${project_name} PRIVATE
    ${COMPILER_FLAGS}
  )
endfunction()

# Function to recursively find all source files
function(get_all_sources output_var root_dir)
    file(GLOB_RECURSE sources 
        "${root_dir}/*.c"
        "${root_dir}/*.s"
        "${root_dir}/*.asm"
    )
    set(${output_var} ${sources} PARENT_SCOPE)
endfunction()
