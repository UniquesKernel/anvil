set(MODULE_NAME math_comparison)
set(MODULE_SOURCE src/comparison.cpp)

include(${PROJECT_SOURCE_DIR}/cmake/Functions.cmake)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE})

target_compile_options(${MODULE_NAME} PRIVATE
  -include ${PROJECT_SOURCE_DIR}/include/anvil/memory/memory_policy.hpp
)

target_include_directories(${MODULE_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

anvil_set_strict_warnings(${MODULE_NAME})

if (BUILD_TESTING)

  anvil_find_pybind11()
  set(TEST_MODULE anvil_math)
  pybind11_add_module(${TEST_MODULE}
    bindings/comparison_bindings.cpp
    bindings/minmax_bindings.cpp
    bindings/classify.cpp
    ${MODULE_SOURCE}
  )

  set_target_properties(${TEST_MODULE} PROPERTIES
     CXX_CLANG_TIDY ""
     CXX_CPPCHECK ""
  )

  target_include_directories(${TEST_MODULE} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

  target_link_libraries(${TEST_MODULE} PRIVATE Python3::Python)
  anvil_set_strict_warnings(${TEST_MODULE})
  target_compile_options(${TEST_MODULE} PRIVATE
    -fexceptions
    -frtti
  )

  add_custom_command(TARGET ${TEST_MODULE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TEST_MODULE}> ${CMAKE_CURRENT_SOURCE_DIR}/tests/
    COMMENT "Copying math_module to linear algebra tests directory"
  )

  add_test(
    NAME comparison_pytest
    COMMAND ${Python3_EXECUTABLE} -m pytest -q "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_comparison.py"
     WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  )
endif()
