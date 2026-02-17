set(MODULE_NAME math_example)

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)

# anvil_set_strict_warnings(${MODULE_NAME})

add_executable(vector2_example vector2_example.cpp)
target_include_directories(vector2_example
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(vector2_example PRIVATE math)

add_executable(vector3_example vector3_example.cpp)
target_include_directories(vector3_example
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(vector3_example PRIVATE math)
