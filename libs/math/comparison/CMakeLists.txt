set(MODULE_NAME math_comparison)
set(MODULE_SOURCE src/minmax.cpp)

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
