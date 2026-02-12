cmake_minimum_required(VERSION 3.22)

project(TemplateProject VERSION 0.1.0 LANGUAGES C CXX)

option(BUILD_TESTING "Build Tests" ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerStandards)
include(Functions)
anvil_enable_static_analysis_root()

if(BUILD_TESTING)
  enable_testing()
  if(DEFINED Python3_EXECUTABLE)
    set(Python3_ROOT_DIR "${Python3_EXECUTABLE}")
  endif()
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
  message(STATUS "Using Python 3 Interpreter: ${Python3_EXECUTABLE}")
endif()

add_subdirectory(libs/sample)

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  file(CREATE_LINK
    "${CMAKE_BINARY_DIR}/compile_commands.json"
    "${CMAKE_SOURCE_DIR}/compile_commands.json"
    SYMBOLIC
  )
endif()