set(MODULE_NAME graphic)

include(${CMAKE_SOURCE_DIR}/cmake/Functions.cmake)

# anvil_set_strict_warnings(${MODULE_NAME})

add_executable(example1 example1.cpp)

target_include_directories(graphic
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(example1 PRIVATE graphic)
