cmake_minimum_required(VERSION 3.22)

project(TemplateProject VERSION 0.1.0 LANGUAGES C CXX)

option(BUILD_TESTING "Build Tests" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerStandards)
include(Functions)
anvil_enable_static_analysis_root()

option(ENABLE_COVERAGE "Enable coverage reporting" OFF)

if(ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
    message(STATUS "Coverage enabled")
  else()
    message(WARNING "Coverage requested but compiler is not GCC or Clang")
  endif()
endif()

if(BUILD_TESTING)
  enable_testing()
  if(DEFINED Python3_EXECUTABLE)
    set(Python3_ROOT_DIR "${Python3_EXECUTABLE}")
  endif()
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
  message(STATUS "Using Python 3 Interpreter: ${Python3_EXECUTABLE}")
endif()

add_subdirectory(libs/math)
add_subdirectory(libs/graphic)
add_subdirectory(libs/memory)
add_subdirectory(examples)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  file(CREATE_LINK
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
    SYMBOLIC
  )
endif()
