set(MODULE_NAME graphic)
set(MODULE_SOURCE src/canvas.cpp)

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)

add_library(${MODULE_NAME} STATIC ${MODULE_SOURCE})

target_include_directories(${MODULE_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(graphic PUBLIC memory)

anvil_set_strict_warnings(${MODULE_NAME})

if(BUILD_TESTING)
  anvil_find_pybind11()

  set(TEST_MODULE ${MODULE_NAME}_module)
  pybind11_add_module(${TEST_MODULE}
    bindings/canvas_bindings.cpp
    ${MODULE_SOURCE}
  )

  set_target_properties(${TEST_MODULE} PROPERTIES
    CXX_CLANG_TIDY ""
    CXX_CPPCHECK ""
  )

  target_include_directories(${TEST_MODULE}
    PRIVATE
      ${CMAKE_SOURCE_DIR}/include
  )

  target_link_libraries(${TEST_MODULE} PRIVATE Python3::Python memory)

  anvil_set_strict_warnings(${TEST_MODULE})

  target_compile_options(${TEST_MODULE} PRIVATE
    -fexceptions
    -frtti
  )

  add_custom_command(TARGET ${TEST_MODULE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TEST_MODULE}>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/
    COMMENT "Copying graphic_module to tests directory"
  )

  add_test(
    NAME canvas_pytest
    COMMAND ${Python3_EXECUTABLE} -m pytest -q "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_canvas.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  )

endif()
