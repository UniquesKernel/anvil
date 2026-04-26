set(MODULE_NAME memory)
set(MODULE_SOURCE
    src/memory_allocation.cpp
    src/lazy_stack_allocator.cpp
    src/lazy_scratch_allocator.cpp
    src/scratch_allocator.cpp
    src/stack_allocator.cpp
    src/resizeable_buffer.cpp
)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE})

target_compile_options(memory PRIVATE
    -include ${PROJECT_SOURCE_DIR}/include/anvil/memory/memory_policy.hpp
)

target_link_libraries(${MODULE_NAME} math)

target_include_directories(${MODULE_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

anvil_set_strict_warnings(${MODULE_NAME})

if(BUILD_TESTING)
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

  # Find pybind11
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

  pybind11_add_module(anvil_memory
      bindings/memory_bindings.cpp
      bindings/lazy_stack_bindings.cpp
      bindings/lazy_scratch_bindings.cpp
      bindings/scratch_bindings.cpp
      bindings/stack_bindings.cpp
      bindings/resizeable_buffer_bindings.cpp
      ${MODULE_SOURCE}
    )

  target_include_directories(anvil_memory
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

  target_link_libraries(anvil_memory PRIVATE Python3::Python math)
  target_compile_options(anvil_memory PRIVATE -O0 -g)

  # Copy the module to the test directory for easy importing
  add_custom_command(TARGET anvil_memory POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:anvil_memory>
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/
        COMMENT "Copying anvil_memory module to tests directory"
    )

  add_test(
      NAME scratch_allocator_pytest
      COMMAND ${Python3_EXECUTABLE} -m pytest -s -v "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_scratch_allocator.py"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

  set_tests_properties(scratch_allocator_pytest PROPERTIES
      ENVIRONMENT "HYPOTHESIS_MAX_EXAMPLES=1000;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests:${PROJECT_SOURCE_DIR}/libs"
    )

  add_test(
      NAME stack_allocator_pytest
      COMMAND ${Python3_EXECUTABLE} -m pytest -s -v "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_stack_allocator.py"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

  set_tests_properties(stack_allocator_pytest PROPERTIES
      ENVIRONMENT "HYPOTHESIS_MAX_EXAMPLES=1000;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests:${PROJECT_SOURCE_DIR}/libs"
    )

  add_test(
      NAME lazy_stack_allocator_pytest
      COMMAND ${Python3_EXECUTABLE} -m pytest -s -v "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_lazy_stack_allocator.py"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

  set_tests_properties(lazy_stack_allocator_pytest PROPERTIES
      ENVIRONMENT "HYPOTHESIS_MAX_EXAMPLES=1000;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests:${PROJECT_SOURCE_DIR}/libs"
    )

  add_test(
      NAME lazy_scratch_allocator_pytest
      COMMAND ${Python3_EXECUTABLE} -m pytest -s -v "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_lazy_scratch_allocator.py"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

  set_tests_properties(lazy_scratch_allocator_pytest PROPERTIES
      ENVIRONMENT "HYPOTHESIS_MAX_EXAMPLES=1000;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests:${PROJECT_SOURCE_DIR}/libs"
    )

  add_test(
      NAME resizeable_buffer_pytest
      COMMAND ${Python3_EXECUTABLE} -m pytest -s -v "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_resizeable_buffer.py"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

  set_tests_properties(resizeable_buffer_pytest PROPERTIES
      ENVIRONMENT "HYPOTHESIS_MAX_EXAMPLES=1000;PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/tests:${PROJECT_SOURCE_DIR}/libs"
    )

endif()

