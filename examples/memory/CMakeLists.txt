set(MODULE_NAME memory_example)

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)

# anvil_set_strict_warnings(${MODULE_NAME})

add_executable(memory_example memory_example.cpp)
target_include_directories(memory_example
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(memory_example PRIVATE memory)

