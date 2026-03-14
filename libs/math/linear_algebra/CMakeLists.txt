set(MODULE_NAME math_linear_algebra)
set(MODULE_SOURCE src/vector.cpp)

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE})

target_compile_options(${MODULE_NAME} PRIVATE
  -include ${CMAKE_SOURCE_DIR}/include/memory/memory_policy.hpp
)

target_include_directories(${MODULE_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

anvil_set_strict_warnings(${MODULE_NAME})

if(BUILD_TESTING)
  anvil_find_pybind11()

  pybind11_add_module(math_module
    bindings/vector_bindings.cpp
    ${MODULE_SOURCE}
  )

  set_target_properties(math_module PROPERTIES
    CXX_CLANG_TIDY ""
    CXX_CPPCHECK ""
  )

  target_include_directories(math_module
    PRIVATE
      ${CMAKE_SOURCE_DIR}/include
  )

  target_link_libraries(math_module PRIVATE Python3::Python)

  anvil_set_strict_warnings(math_module)

  target_compile_options(math_module PRIVATE
    -fexceptions
    -frtti
  )

  add_custom_command(TARGET math_module POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:math_module>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/
    COMMENT "Copying math_module to linear algebra tests directory"
  )

  add_test(
    NAME vector2_pytest
    COMMAND ${Python3_EXECUTABLE} -m pytest -q "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_vector2.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  )

  add_test(
    NAME vector3_pytest
    COMMAND ${Python3_EXECUTABLE} -m pytest -q "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_vector3.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  )
endif()