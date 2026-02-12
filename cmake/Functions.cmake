function(anvil_find_pybind11)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import pybind11; print(pybind11.get_cmake_dir())"
    OUTPUT_VARIABLE pybind11_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE pybind11_FIND_RESULT
  )

  if(pybind11_FIND_RESULT EQUAL 0)
    message(STATUS "Found pybind11 at: ${pybind11_DIR}")
    list(APPEND CMAKE_PREFIX_PATH ${pybind11_DIR})
    find_package(pybind11 CONFIG REQUIRED)
  else()
    message(FATAL_ERROR "pybind11 not found. Install with: pip install pybind11")
  endif()
endfunction()

function(anvil_set_strict_warnings target_name)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        # Common flags for both GCC and Clang
        target_compile_options(${target_name} PRIVATE
            -Wall -Wextra -Wpedantic -Werror
            -pedantic-errors
            -Wconversion -Wsign-conversion
            -Wshadow
            -Wformat=2 -Wformat-security
            -Wundef
            -Wnull-dereference
            -Wdouble-promotion
            -Wold-style-cast
            -Wcast-align -Wcast-qual
            -Woverloaded-virtual
            -Wnon-virtual-dtor
            -Wimplicit-fallthrough
            -Wswitch-enum -Wswitch-default
            -Wmissing-declarations
            -Wmissing-noreturn
            -Wredundant-decls
            -Wvla
            -Wstrict-overflow=5
            -Wfloat-equal
            -Wpointer-arith
            -Wwrite-strings
            -Winit-self
            -fno-exceptions -fno-rtti
            -fstack-protector-strong
        )

        # Runtime fortification (both compilers)
        target_compile_definitions(${target_name} PRIVATE
            _FORTIFY_SOURCE=2
            _GLIBCXX_ASSERTIONS
        )
    endif()

    # GCC-specific flags
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${target_name} PRIVATE
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wformat-signedness
            -Warray-bounds=2  # GCC supports level 2
        )
    endif()

    # Clang-specific flags
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target_name} PRIVATE
            -Wunreachable-code
            -Wunreachable-code-break
            -Wunreachable-code-return
            -Warray-bounds  # Clang only supports level 1 (implicit)
        )
    endif()
endfunction()

function(anvil_enable_static_analysis target_name)
  # clang-tidy
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set_target_properties(${target_name} PROPERTIES
      CXX_CLANG_TIDY "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy;--warnings-as-errors=*;--extra-arg=-Wno-unknown-warning-option;--extra-arg=-Wno-error=unknown-warning-option"
    )
    message(STATUS "clang-tidy enabled for ${target_name}")
  else()
    message(WARNING "clang-tidy not found")
  endif()

  # cppcheck
  find_program(CPPCHECK_EXE NAMES cppcheck)
  if(CPPCHECK_EXE)
    set_target_properties(${target_name} PROPERTIES
      CXX_CPPCHECK "${CPPCHECK_EXE};--enable=all;--inline-suppr;--error-exitcode=1;--suppress=missingIncludeSystem;--suppress=unmatchedSuppression"
    )
    message(STATUS "cppcheck enabled for ${target_name}")
  else()
    message(WARNING "cppcheck not found")
  endif()
endfunction()

function(anvil_enable_static_analysis_root)
  # clang-tidy
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(CMAKE_C_CLANG_TIDY
      "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy;--warnings-as-errors=*;--extra-arg=-Wno-unknown-warning-option;--extra-arg=-Wno-error=unknown-warning-option"
        PARENT_SCOPE
    )
    set(CMAKE_CXX_CLANG_TIDY
      "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy;--warnings-as-errors=*;--extra-arg=-Wno-unknown-warning-option;--extra-arg=-Wno-error=unknown-warning-option"
        PARENT_SCOPE
    )
    message(STATUS "clang-tidy enabled for all targets")
  else()
    message(WARNING "clang-tidy not found")
  endif()

  # cppcheck
  find_program(CPPCHECK_EXE NAMES cppcheck)
  if(CPPCHECK_EXE)
    add_custom_target(cppcheck ALL
      COMMAND ${CPPCHECK_EXE}
        --enable=all
        --inline-suppr
        --error-exitcode=1
        --suppress=missingIncludeSystem
        --suppress=unmatchedSuppression
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Running cppcheck (compile_commands.json)"
      VERBATIM
    )
    message(STATUS "cppcheck enabled (project-wide, compile_commands.json)")
  else()
    message(WARNING "cppcheck not found")
  endif()
endfunction()
