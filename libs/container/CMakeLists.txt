set(MODULE_NAME container)
set(MODULE_SOURCE
    src/array.cpp
)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE})

target_compile_options(container PRIVATE
    -include ${PROJECT_SOURCE_DIR}/include/anvil/memory/memory_policy.hpp
)

target_link_libraries(container STATIC math)
target_link_libraries(container STATIC memory)
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

  pybind11_add_module(anvil_container
      bindings/array_bindings.cpp
      ${MODULE_SOURCE}
    )

    target_include_directories(anvil_container
      PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

  target_link_libraries(anvil_container PRIVATE Python3::Python math)
  target_compile_options(anvil_container PRIVATE -O0 -g)

  # Copy the module to the test directory for easy importing
  add_custom_command(TARGET anvil_container POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:anvil_container>
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/
        COMMENT "Copying anvil_container module to tests directory"
    )

  add_test(
      NAME array_pytest
      COMMAND ${Python3_EXECUTABLE} -m pytest -s -v "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_array.py"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    )

endif()
