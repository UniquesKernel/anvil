set(MODULE_NAME memory_example)

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)

add_executable(memory_example memory_example.cpp)

anvil_set_strict_warnings(${MODULE_NAME})

target_include_directories(memory_example
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(memory_example PRIVATE memory)

target_compile_options(memory_example PRIVATE -mavx2 -w)

add_executable(memory_example_malloc malloc_example.cpp)

target_compile_options(memory_example_malloc PRIVATE
    -O3
    -w
    -DNDEBUG
    -mavx2
)

target_include_directories(memory_example_malloc
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(memory_example_malloc PRIVATE memory)

set_target_properties(memory_example_malloc PROPERTIES
    CXX_CLANG_TIDY ""
)

add_executable(stack_example stack_memory.cpp)

anvil_set_strict_warnings(${MODULE_NAME})

target_include_directories(stack_example
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(stack_example PRIVATE memory)

add_executable(lazy_stack_example lazy_stack_memory.cpp)

anvil_set_strict_warnings(${MODULE_NAME})

target_include_directories(lazy_stack_example
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(lazy_stack_example PRIVATE memory)
