set(MODULE_NAME sample)
set(MODULE_SOURCE src/example.cpp)

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE})

target_include_directories(${MODULE_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

anvil_set_strict_warnings(${MODULE_NAME})

if(BUILD_TESTING)
  anvil_find_pybind11()

  pybind11_add_module(example_module
    bindings/sample_bindings.cpp
    ${MODULE_SOURCE}
  )

  set_target_properties(example_module PROPERTIES
    CXX_CLANG_TIDY ""
    CXX_CPPCHECK ""
  )

  target_include_directories(example_module
    PRIVATE
      ${CMAKE_SOURCE_DIR}/include
  )

  target_link_libraries(example_module PRIVATE Python3::Python)

  anvil_set_strict_warnings(example_module)

  target_compile_options(example_module PRIVATE
    -fexceptions
    -frtti
  )

  add_custom_command(TARGET example_module POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:example_module>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/
    COMMENT "Copying example_module to tests directory"
  )

  add_test(
    NAME example_pytest
    COMMAND ${Python3_EXECUTABLE} -m pytest -q "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  )
endif()
